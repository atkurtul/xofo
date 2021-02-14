#include "pipeline.h"
#include <vulkan/vulkan_core.h>
#include <xofo.h>

#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>

using namespace std;
using namespace xofo;

static string read_to_string(const char* file) {
  ifstream t(file);
  return string(istreambuf_iterator<char>(t), istreambuf_iterator<char>());
}

static vector<u32> compile_glsl(const char* file, shaderc_shader_kind stage) {
  auto src = read_to_string(file);
  shaderc::Compiler comp;
  shaderc::CompileOptions opt;
  // opt.SetOptimizationLevel(shaderc_optimization_level_performance);

  auto re = comp.CompileGlslToSpv(src.c_str(), src.size(), stage, file, opt);
  if (re.GetCompilationStatus()) {
    cerr << re.GetErrorMessage() << "\n";
    abort();
  }
  cout << "Size is: " << re.end() - re.begin() << "\n";
  return vector<u32>(re.begin(), re.end());
}

static VkFormat type_format(int basetype, int vecsize) {
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

static Shader compile_shader(string const& file, shaderc_shader_kind stage) {
  Shader shader = {.bytecode = compile_glsl(file.c_str(), stage)};

  VkShaderModuleCreateInfo module_info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = shader.bytecode.size() * 4,
      .pCode = shader.bytecode.data()};

  CHECKRE(vkCreateShaderModule(vk, &module_info, 0, &shader.mod));

  return shader;
}

namespace sc = spirv_cross;

static void build_set_layouts(
    sc::Compiler& cc,
    sc::SmallVector<sc::Resource> const& res,
    VkDescriptorType ty,
    VkShaderStageFlags stage,
    vector<vector<VkDescriptorSetLayoutBinding>>& bindings) {
  for (auto& res : res) {
    auto set =
        cc.get_decoration(res.id, spv::Decoration::DecorationDescriptorSet);
    auto bind = cc.get_decoration(res.id, spv::Decoration::DecorationBinding);
    if (bindings.size() < set + 1) {
      bindings.resize(set + 1);
    }
    if (bindings[set].size() < bind + 1) {
      bindings[set].resize(bind + 1);
    }
    cout << set << ":" << bind << "\n";
    cout << "\t" << desc_type_string(ty) << "\n";
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

static StageInputs extract_input_state_and_uniforms(
    vector<u32> const& bytecode,
    VkShaderStageFlags stage,
    vector<vector<VkDescriptorSetLayoutBinding>>& bindings) {
  sc::Compiler cc(bytecode);
  auto res = cc.get_shader_resources();

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

  if (stage != VK_SHADER_STAGE_VERTEX_BIT)
    return {};

  map<u32, VkVertexInputAttributeDescription> inputs;

  for (auto& input : res.stage_inputs) {
    auto loc = cc.get_decoration(input.id, spv::Decoration::DecorationLocation);
    auto ty = cc.get_type(input.type_id);
    VkFormat format = type_format(ty.basetype, ty.vecsize);
    inputs[loc] = VkVertexInputAttributeDescription{
        .location = loc,
        .binding = loc >= 10,
        .format = format,
        .offset =
            ty.vecsize * 4,  // this isnt offset. it will be calculated later
    };
  }

  StageInputs input = {};
  for (auto& [k, v] : inputs) {
    auto* into = &input.per_vertex;
    if (k >= 10) {
      into = &input.per_instance;
    }
    into->attr.push_back(v);
    into->attr.back().offset = into->stride;
    into->stride += v.offset;
  }

  return input;
}

Box<Pipeline> Pipeline::mk(string const& shader) {
  auto re = Box<Pipeline>(new Pipeline(shader));
  xofo::register_recreation_callback([&](auto extent) { re->reset(); });
  return re;
}

Pipeline::Pipeline(string const& shader)
    : shader(shader),
      shaders{
          compile_shader(shader + ".vert", shaderc_vertex_shader),
          compile_shader(shader + ".frag", shaderc_fragment_shader),
      } {
  vector<vector<VkDescriptorSetLayoutBinding>> bindings;

  inputs = extract_input_state_and_uniforms(
      shaders[0].bytecode, VK_SHADER_STAGE_VERTEX_BIT, bindings);

  extract_input_state_and_uniforms(shaders[1].bytecode,
                                   VK_SHADER_STAGE_FRAGMENT_BIT, bindings);

  // decriptor pool
  for (auto& bindings : bindings) {
    if (bindings.empty())
      continue;
    unordered_map<VkDescriptorType, u32> sizes;
    vector<VkDescriptorPoolSize> pools;
    VkDescriptorPool pool;
    for (auto& bind : bindings) {
      sizes[bind.descriptorType] += bind.descriptorCount * 1024;
    }

    for (auto kv : sizes) {
      pools.push_back({kv.first, kv.second});
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

  // set layouts
  vector<VkDescriptorSetLayout> layouts;
  layouts.reserve(bindings.size());
  set_layouts.reserve(bindings.size());

  for (auto& bindings : bindings) {
    set_layouts.emplace_back(bindings);
    layouts.push_back(set_layouts.back().layout);
  }

  // pipeline layout

  VkPushConstantRange range = {
      .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      .offset = 0,
      .size = 256,
  };

  VkPipelineLayoutCreateInfo layout_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = (u32)layouts.size(),
      .pSetLayouts = layouts.data(),
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &range,
  };

  CHECKRE(vkCreatePipelineLayout(vk, &layout_info, 0, &layout));

  create();
}

void Pipeline::recompile() {
  vkDestroyShaderModule(vk, shaders[0], 0);
  vkDestroyShaderModule(vk, shaders[1], 0);
  shaders[0] = compile_shader(shader + ".vert", shaderc_vertex_shader);
  shaders[1] = compile_shader(shader + ".frag", shaderc_fragment_shader);
  reset();
}

void Pipeline::create() {
  VkPipelineShaderStageCreateInfo stages[2] = {
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = shaders[0],
          .pName = "main",
      },
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = shaders[1],
          .pName = "main",
      }};

  vector<VkVertexInputBindingDescription> bindings;

  VkPipelineVertexInputStateCreateInfo VertexInputState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
  };

  if (inputs.per_vertex.attr.size()) {
    bindings.push_back(VkVertexInputBindingDescription{
        .binding = 0,
        .stride = inputs.per_vertex.stride,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    });
  }

  if (inputs.per_instance.attr.size()) {
    bindings.push_back(VkVertexInputBindingDescription{
        .binding = 1,
        .stride = inputs.per_instance.stride,
        .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
    });
  }

  vector<VkVertexInputAttributeDescription> inputs;

  if (bindings.size()) {
    inputs.reserve(this->inputs.per_vertex.attr.size() +
                   this->inputs.per_instance.attr.size());

    inputs.insert(inputs.end(), this->inputs.per_vertex.attr.begin(),
                  this->inputs.per_vertex.attr.end());

    inputs.insert(inputs.end(), this->inputs.per_instance.attr.begin(),
                  this->inputs.per_instance.attr.end());

    VertexInputState.vertexBindingDescriptionCount = (u32)bindings.size();
    VertexInputState.pVertexBindingDescriptions = bindings.data();
    VertexInputState.vertexAttributeDescriptionCount = (u32)inputs.size();
    VertexInputState.pVertexAttributeDescriptions = inputs.data();

    cout << "Input size: " << inputs.size() << "\n";
    for (auto& input : inputs) {
      cout << "\tlocation: " << input.location << "\n";
      cout << "\tbinding: " << input.binding << "\n";
      cout << "\tformat: " << input.format << "\n";
      cout << "\toffset: " << input.offset << "\n--------------------\n";
    }
  }

  VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = topology,
  };

  auto extent = xofo::extent();
  VkViewport Viewports = {
      .width = (f32)extent.width,
      .height = (f32)extent.height,
      .maxDepth = 1.f,
  };

  VkRect2D Scissors = {.extent = extent};
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
      .depthTestEnable = depth_test,
      .depthWriteEnable = depth_write,
      .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
  };

  VkPipelineColorBlendAttachmentState Attachments = {
      .blendEnable = 0,
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
      .polygonMode = mode,
      .cullMode = culling,
      .lineWidth = line_width};

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
      .renderPass = xofo::renderpass(),
  };

  CHECKRE(vkCreateGraphicsPipelines(vk, 0, 1, &info, 0, &pipeline));
}

VkDescriptorSet Pipeline::alloc_set(u32 n) {
  VkDescriptorSetAllocateInfo info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .descriptorPool = pools.at(n),
      .descriptorSetCount = 1,
      .pSetLayouts = &set_layouts.at(n).layout,
  };
  VkDescriptorSet set;
  CHECKRE(vkAllocateDescriptorSets(vk, &info, &set));
  return set;
}

Pipeline::~Pipeline() {
  vkDestroyShaderModule(vk, shaders[0], 0);
  vkDestroyShaderModule(vk, shaders[1], 0);
  vkDestroyPipeline(vk, pipeline, 0);
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