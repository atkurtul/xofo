
#include <fstream>
#include <xofo.h>

#include <spirv_reflect.h>

using namespace std;
using namespace xofo;

vector<Pipeline*> Pipeline::pipelines;

static string read_to_string(const char* file) {
  ifstream t(file);
  return string(istreambuf_iterator(t), istreambuf_iterator<char>());
}

static vector<u32> read_binary(const char* file) {
  ifstream t(file, ios::binary);
  auto tmp =
      vector<u8>(istreambuf_iterator<char>(t), istreambuf_iterator<char>());
  return vector<u32>((u32*)tmp.data(), (u32*)(tmp.data() + tmp.size()));
}

uint32_t type_stride(SpvReflectTypeDescription* ty) {
  auto num = ty->traits.numeric;
  auto width = num.scalar.width >> 3;
  if (ty->type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {
    return width * num.matrix.row_count * num.matrix.column_count;
  }
  if (ty->type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) {
    return width * num.vector.component_count;
  }
  return width;
}

static Shader compile_shader(string const& file, VkShaderStageFlags stage) {
  auto out = file + ".spv";
  
  auto cmd = "glslc "+ file + " -o " + out;

  if (system(cmd.data()) || !std::ifstream(out, ios::binary).good()) {
    abort();
  }
  
  Shader shader = {
      .bytecode = read_binary(out.data()),
  };

  remove(out.data());

  VkShaderModuleCreateInfo module_info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = shader.bytecode.size() * 4,
      .pCode = shader.bytecode.data()};

  CHECKRE(vkCreateShaderModule(vk, &module_info, 0, &shader.mod));

  return shader;
}

void build_set_layouts(vector<SpvReflectDescriptorBinding*>& bindings,
                       vector<vector<VkDescriptorSetLayoutBinding>>& layouts,
                       VkShaderStageFlags stage) {
  std::sort(bindings.begin(), bindings.end(), [](auto a, auto b) {
    return (a->set != b->set) ? (a->set < b->set) : (a->binding < b->binding);
  });

  for (auto bind : bindings) {
    if (layouts.size() <= bind->set) {
      layouts.resize(bind->set + 1);
    }
    if (layouts[bind->set].size() <= bind->binding) {
      layouts[bind->set].resize(bind->binding + 1);
    }
    auto& desc = layouts[bind->set][bind->binding];
    auto desc_ty = (VkDescriptorType)bind->descriptor_type;
    if (desc.descriptorCount != 0) {
      if (desc.descriptorType != desc_ty ||
          desc.descriptorCount != bind->count) {
        throw;
      }
    }
    desc.binding = bind->binding;
    desc.descriptorType = desc_ty;
    desc.descriptorCount = bind->count;
    desc.stageFlags |= stage;
  }
}

StageInputs get_stage_inputs(vector<SpvReflectInterfaceVariable*>& vars) {
  StageInputs inputs = {};

  std::sort(vars.begin(), vars.end(),
            [](auto a, auto b) { return a->location < b->location; });

  for (auto var : vars) {
    auto input = &inputs.per_vertex;
    uint32_t binding = 0;

    if (var->location >= 10) {
      input = &inputs.per_instance;
      binding = 1;
    }

    input->attr.push_back({.location = var->location,
                           .binding = binding,
                           .format = (VkFormat)var->format,
                           .offset = input->stride});
    input->stride += type_stride(var->type_description);
  }

  return inputs;
}

static StageInputs extract_input_state_and_uniforms(
    vector<uint32_t> const& bytecode,
    vector<vector<VkDescriptorSetLayoutBinding>>& layouts) {
  spv_reflect::ShaderModule obj(bytecode);

  CHECKRE((VkResult)obj.GetResult());
  uint32_t count;

  CHECKRE((VkResult)obj.EnumerateInputVariables(&count, 0));
  vector<SpvReflectInterfaceVariable*> vars(count);
  CHECKRE((VkResult)obj.EnumerateInputVariables(&count, vars.data()));

  CHECKRE((VkResult)obj.EnumerateDescriptorBindings(&count, 0));
  vector<SpvReflectDescriptorBinding*> bindings(count);
  CHECKRE((VkResult)obj.EnumerateDescriptorBindings(&count, bindings.data()));

  build_set_layouts(bindings, layouts, obj.GetShaderStage());
  return get_stage_inputs(vars);
}

Box<Pipeline> Pipeline::mk(string const& shader, PipelineState const& state) {
  auto re = Box<Pipeline>(new Pipeline(shader, state));
  xofo::register_recreation_callback([&](auto extent) { re->reset(); });
  pipelines.push_back(re.get());
  return re;
}

Pipeline::Pipeline(string const& shader, PipelineState const& state)
    : shader(shader),
      shaders{
          compile_shader(shader + ".vert", VK_SHADER_STAGE_VERTEX_BIT),
          compile_shader(shader + ".frag", VK_SHADER_STAGE_FRAGMENT_BIT),
      },
      state(state) {
  vector<vector<VkDescriptorSetLayoutBinding>> bindings;

  inputs = extract_input_state_and_uniforms(shaders[0].bytecode, bindings);

  extract_input_state_and_uniforms(shaders[1].bytecode, bindings);

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
  shaders[0] = compile_shader(shader + ".vert", VK_SHADER_STAGE_VERTEX_BIT);
  shaders[1] = compile_shader(shader + ".frag", VK_SHADER_STAGE_FRAGMENT_BIT);
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

    // cout << "Input size: " << inputs.size() << "\n";
    // for (auto& input : inputs) {
    //   cout << "\tlocation: " << input.location << "\n";
    //   cout << "\tbinding: " << input.binding << "\n";
    //   cout << "\tformat: " << input.format << "\n";
    //   cout << "\toffset: " << input.offset << "\n--------------------\n";
    // }
  }

  VkPipelineInputAssemblyStateCreateInfo InputAssemblyState = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .topology = state.topology,
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
      .depthTestEnable = state.depth_test,
      .depthWriteEnable = state.depth_write,
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
      .polygonMode = state.polygon,
      .cullMode = state.culling,
      .lineWidth = state.line_width};

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

Pipeline::~Pipeline() {
  pipelines.erase(find(pipelines.begin(), pipelines.end(), this));

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