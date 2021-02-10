#include <pipeline.h>
#include <vk.h>
#include <vulkan/vulkan_core.h>
#include <fstream>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <unordered_map>

using namespace std;

string read_to_string(const char* file) {
  ifstream t(file);
  return string(istreambuf_iterator<char>(t), istreambuf_iterator<char>());
}

vector<u32> compile_glsl(const char* file, shaderc_shader_kind stage) {
  auto src = read_to_string(file);
  shaderc::Compiler comp;
  auto re = comp.CompileGlslToSpv(src.c_str(), src.size(), stage, file);
  return vector<u32>(re.begin(), re.end());
}

VkFormat type_format(int basetype, int vecsize) {
  switch (basetype) {
    case spirv_cross::SPIRType::Int:
      switch (vecsize) {
        case 1:
          return VK_FORMAT_R32_SINT;
        case 2:
          return VK_FORMAT_R32G32_SINT;
        case 3:
          return VK_FORMAT_R32G32B32_SINT;
        case 4:
          return VK_FORMAT_R32G32B32A32_SINT;
      }
    case spirv_cross::SPIRType::UInt:
      switch (vecsize) {
        case 1:
          return VK_FORMAT_R32_UINT;
        case 2:
          return VK_FORMAT_R32G32_UINT;
        case 3:
          return VK_FORMAT_R32G32B32_UINT;
        case 4:
          return VK_FORMAT_R32G32B32A32_UINT;
      }
    case spirv_cross::SPIRType::Float:
      switch (vecsize) {
        case 1:
          return VK_FORMAT_R32_SFLOAT;
        case 2:
          return VK_FORMAT_R32G32_SFLOAT;
        case 3:
          return VK_FORMAT_R32G32B32_SFLOAT;
        case 4:
          return VK_FORMAT_R32G32B32A32_SFLOAT;
      }
      break;
    default:
      throw;
  }
  throw;
}

vector<u32> compile_shader(string const& file,
                           shaderc_shader_kind stage,
                           VkShaderModule& module) {
  auto bin = compile_glsl(file.c_str(), stage);

  VkShaderModuleCreateInfo module_info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = bin.size() * 4,
      .pCode = bin.data()};

  CHECKRE(vkCreateShaderModule(vk, &module_info, 0, &module));

  return move(bin);
}

namespace sc = spirv_cross;

void build_set_layouts(sc::Compiler& cc,
                       sc::SmallVector<sc::Resource> const& res,
                       VkDescriptorType ty,
                       VkShaderStageFlags stage,
                       vector<vector<VkDescriptorSetLayoutBinding>>& bindings) {
  for (auto& res : res) {
    auto set =
        cc.get_decoration(res.id, spv::Decoration::DecorationDescriptorSet);
    auto bind = cc.get_decoration(res.id, spv::Decoration::DecorationBinding);
    bindings.resize(set + 1);
    bindings[set].resize(bind + 1);

    auto& desc = bindings[set][bind];
    if (desc.descriptorCount != 0) {
      if (desc.descriptorType != ty) {
        throw;
      }
    }
    desc.binding = bind;
    desc.descriptorType = ty;
    desc.descriptorCount = 1;
    desc.stageFlags |= stage;
  }
}

auto extract_input_state_and_uniforms(
    vector<u32> const& bytecode,
    VkShaderStageFlags stage,
    vector<vector<VkDescriptorSetLayoutBinding>>& bindings) {
  sc::Compiler cc(bytecode);
  auto res = cc.get_shader_resources();

  vector<VkVertexInputAttributeDescription> attr(res.stage_inputs.size());

  for (auto& input : res.stage_inputs) {
    auto loc = cc.get_decoration(input.id, spv::Decoration::DecorationLocation);
    auto ty = cc.get_type(input.type_id);

    VkFormat format = type_format(ty.basetype, ty.vecsize);
    attr[loc].format = format;
    attr[loc].location = loc;
    attr[loc].offset = ty.vecsize * 4;
  }
  u32 stride = 0;

  for (auto& input : attr) {
    auto tmp = stride;
    stride += input.offset;
    input.offset = tmp;
  }

  build_set_layouts(cc, res.uniform_buffers, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    stage, bindings);
  build_set_layouts(cc, res.storage_buffers, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    stage, bindings);
  build_set_layouts(cc, res.subpass_inputs, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
                    stage, bindings);
  build_set_layouts(cc, res.storage_images, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                    stage, bindings);
  build_set_layouts(cc, res.sampled_images,
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
                    stage, bindings);

  struct {
    vector<VkVertexInputAttributeDescription> attr;
    u32 stride;
  } re = {attr, stride};

  return re;
}

Pipeline::Pipeline(string const& shader) : shader(shader) {
  auto bin0 =
      compile_shader(shader + ".vert", shaderc_vertex_shader, modules[0]);
  auto bin1 =
      compile_shader(shader + ".frag", shaderc_fragment_shader, modules[1]);

  vector<vector<VkDescriptorSetLayoutBinding>> bindings;

  auto [attr, stride] = extract_input_state_and_uniforms(
      bin0, VK_SHADER_STAGE_VERTEX_BIT, bindings);
  this->attr = move(attr);
  this->stride = stride;
  extract_input_state_and_uniforms(bin1, VK_SHADER_STAGE_FRAGMENT_BIT,
                                   bindings);
  // decriptor pool
  for (auto& bindings : bindings) {
    unordered_map<VkDescriptorType, u32> sizes;
    vector<VkDescriptorPoolSize> pools;
    VkDescriptorPool pool;
    for (auto& bind : bindings) {
      sizes[bind.descriptorType] += bind.descriptorCount * 4096;
    }
    for (auto [k, v] : sizes) {
      pools.push_back({k, v});
    }
    VkDescriptorPoolCreateInfo pinfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 4096,
        .poolSizeCount = (u32)pools.size(),
        .pPoolSizes = pools.data(),
    };
    CHECKRE(vkCreateDescriptorPool(vk, &pinfo, 0, &pool));
    this->pools.push_back(pool);
  }

  //set layouts

  for (auto& bindings : bindings) {
    VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = (u32)bindings.size(),
        .pBindings = bindings.data(),
    };
    VkDescriptorSetLayout layout;
    CHECKRE(vkCreateDescriptorSetLayout(vk, &info, 0, &layout));
    set_layouts.push_back(layout);
  }

  //pipeline layout
  VkPipelineLayoutCreateInfo layout_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = (u32)set_layouts.size(),
      .pSetLayouts = set_layouts.data(),
  };

  CHECKRE(vkCreatePipelineLayout(vk, &layout_info, 0, &layout));

  create();
}

void Pipeline::reload_shaders() {
  vkDestroyShaderModule(vk, modules[0], 0);
  vkDestroyShaderModule(vk, modules[1], 0);
  auto bin0 =
      compile_shader(shader + ".vert", shaderc_vertex_shader, modules[0]);
  auto bin1 =
      compile_shader(shader + ".frag", shaderc_fragment_shader, modules[1]);
  reset();
}

void Pipeline::create() {
  VkPipelineShaderStageCreateInfo stages[2] = {
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = modules[0],
          .pName = "main",
      },
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = modules[1],
          .pName = "main",
      }};

  VkVertexInputBindingDescription input_binding = {
      .stride = stride,
  };

  VkPipelineVertexInputStateCreateInfo VertexInputState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = 1,
      .pVertexBindingDescriptions = &input_binding,
      .vertexAttributeDescriptionCount = (u32)attr.size(),
      .pVertexAttributeDescriptions = attr.data(),
  };

  VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
  };

  VkViewport Viewports = {
      .width = (f32)vk.res.extent.width,
      .height = (f32)vk.res.extent.height,
      .maxDepth = 1.f,
  };
  VkRect2D Scissors = {.extent = vk.res.extent};
  VkPipelineViewportStateCreateInfo ViewportState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .viewportCount = 1,
      .pViewports = &Viewports,
      .scissorCount = 1,
      .pScissors = &Scissors,
  };

  VkPipelineMultisampleStateCreateInfo MultisampleState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
  };
  VkPipelineDepthStencilStateCreateInfo DepthStencilState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
      .depthTestEnable = 1,
      .depthWriteEnable = 1,
      .depthCompareOp = VK_COMPARE_OP_LESS,
  };
  VkPipelineColorBlendAttachmentState Attachments = {
      .blendEnable = 1,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };
  VkPipelineColorBlendStateCreateInfo ColorBlendState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .attachmentCount = 1,
      .pAttachments = &Attachments,
  };
  VkPipelineDynamicStateCreateInfo DynamicState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
  };

  VkPipelineRasterizationStateCreateInfo RasterizationState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .lineWidth = 1.f};

  VkGraphicsPipelineCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .stageCount = 2,
      .pStages = stages,
      .pVertexInputState = &VertexInputState,
      .pInputAssemblyState = &InputAssemblyState,
      .pViewportState = &ViewportState,
      .pRasterizationState = &RasterizationState,
      .pMultisampleState = &MultisampleState,
      .pDepthStencilState = &DepthStencilState,
      .pColorBlendState = &ColorBlendState,
      .pDynamicState = &DynamicState,
      .layout = layout,
      .renderPass = vk.res.renderpass,
  };
  CHECKRE(vkCreateGraphicsPipelines(vk, 0, 1, &info, 0, &pipeline));
}

VkDescriptorSet Pipeline::alloc_set(u32 n) {
  VkDescriptorSetAllocateInfo info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pools[n],
      .descriptorSetCount = 1,
      .pSetLayouts = &set_layouts[n],
  };
  VkDescriptorSet set;
  CHECKRE(vkAllocateDescriptorSets(vk, &info, &set));
  return set;
}

Pipeline::~Pipeline() {
  vkDestroyPipeline(vk, pipeline, 0);
  vkDestroyShaderModule(vk, modules[0], 0);
  vkDestroyShaderModule(vk, modules[1], 0);
  vkDestroyPipelineLayout(vk, layout, 0);
  for (auto pool : pools) {
    vkDestroyDescriptorPool(vk, pool, 0);
  }
  for (auto layout : set_layouts) {
    vkDestroyDescriptorSetLayout(vk, layout, 0);
  }
}

void Pipeline::reset() {
  vkDeviceWaitIdle(vk);
  vkDestroyPipeline(vk, pipeline, 0);
  create();
}