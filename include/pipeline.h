#ifndef B6EB7CB5_9227_4FBF_9721_005CAC0511AA
#define B6EB7CB5_9227_4FBF_9721_005CAC0511AA

#include <vulkan/vulkan_core.h>
#include <functional>
#include <type_traits>
#include <unordered_map>
#include "core.h"

namespace xofo {

struct Shader {
  std::vector<u32> bytecode;
  VkShaderModule mod;
  operator VkShaderModule() const { return mod; }
};

struct StageInputs {
  struct {
    std::vector<VkVertexInputAttributeDescription> attr;
    u32 stride;
  } per_vertex, per_instance;
};

struct SetLayout {
  VkDescriptorSetLayout layout;

  operator VkDescriptorSetLayout() const { return layout; }

  SetLayout(std::vector<VkDescriptorSetLayoutBinding> const& bindings) {
    VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = (u32)bindings.size(),
        .pBindings = bindings.data(),
    };

    CHECKRE(vkCreateDescriptorSetLayout(vk, &info, 0, &layout));

    for (auto& binding : bindings) {
      this->bindings[binding.binding] = binding.descriptorType;
    }
  }

  bool has_binding(u32 idx, VkDescriptorType* out = 0) {
    if (bindings.find(idx) != bindings.end()) {
      if (out) {
        *out = bindings[idx];
      }
      return true;
    }
    return false;
  }

 private:
  std::unordered_map<u32, VkDescriptorType> bindings;
};

struct PipelineState {
  VkCullModeFlags culling = VK_CULL_MODE_NONE;
  VkPolygonMode polygon = VK_POLYGON_MODE_FILL;
  VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  bool depth_test = true;
  bool depth_write = true;
  f32 line_width = 1;
};

struct vector_hasher {
  u64 operator()(std::vector<ShaderResource*> const& v) const {
    u64 hash = v.size();
    for (auto& i : v)
      hash ^= (u64)i + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
  }
};

struct Pipeline {
  static std::vector<Pipeline*> pipelines;
  std::string shader;
  VkPipelineLayout layout;
  VkPipeline pipeline;

  std::vector<SetLayout> set_layouts;
  std::vector<VkDescriptorPool> pools;
  std::unordered_map<std::vector<ShaderResource*>,
                     VkDescriptorSet,
                     vector_hasher>
      sets;

  StageInputs inputs;

  Shader shaders[2];

  PipelineState state;

  operator VkPipeline() { return pipeline; }
  operator VkPipelineLayout() { return layout; }
  void create();
  void recompile();

  static Box<Pipeline> mk(std::string const& shader,
                          PipelineState const& state = {});

  void reset();

  void bind(VkCommandBuffer cmd = vk) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  }

  template <class T>
  void push(T&& data, u32 offset = 0, VkCommandBuffer cmd = vk) {
    vkCmdPushConstants(cmd, layout, 17, offset, sizeof(T), &data);
  }

  void set_state(PipelineState const& state) {
    this->state = state;
    reset();
  }

  bool has_binding(u32 set, u32 idx, VkDescriptorType* out = 0) {
    if (set_layouts.size() <= set) {
      return false;
    }
    return set_layouts[set].has_binding(idx, out);
  }

  VkDescriptorSet allocate_set(u32 idx) {
    VkDescriptorSet set;
    VkDescriptorSetAllocateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pools.at(idx),
        .descriptorSetCount = 1,
        .pSetLayouts = &set_layouts.at(idx).layout,
    };
    CHECKRE(vkAllocateDescriptorSets(vk, &info, &set));
    return set;
  }

  void bind_set0(VkDescriptorSet set, u32 idx = 0, VkCommandBuffer cmd = vk) {
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, idx,
                            1, &set, 0, 0);
  }
  
  void bind_set(std::function<void(VkDescriptorSet)> const& write_set,
                std::vector<ShaderResource*> const& res,
                u32 idx = 0,
                VkCommandBuffer cmd = vk) {
    auto& set = sets[res];
    if (!set) {
      set = allocate_set(idx);
      write_set(set);
    }
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, idx,
                            1, &set, 0, 0);
  }

  ~Pipeline();

 private:
  Pipeline(std::string const& shader, PipelineState const& state);
};

}  // namespace xofo
#endif /* B6EB7CB5_9227_4FBF_9721_005CAC0511AA */
