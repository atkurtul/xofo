#ifndef B6EB7CB5_9227_4FBF_9721_005CAC0511AA
#define B6EB7CB5_9227_4FBF_9721_005CAC0511AA

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
  std::vector<VkDescriptorType> bindings;

  operator VkDescriptorSetLayout() const { return layout; }

  SetLayout(std::vector<VkDescriptorSetLayoutBinding> const& bindings) {
    VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = (u32)bindings.size(),
        .pBindings = bindings.data(),
    };
    CHECKRE(vkCreateDescriptorSetLayout(vk, &info, 0, &layout));

    for (auto& binding : bindings) {
      this->bindings.push_back(binding.descriptorType);
    }
  }
};

struct Pipeline {
  std::string shader;
  VkPipelineLayout layout;
  VkPipeline pipeline;

  std::vector<SetLayout> set_layouts;
  std::vector<VkDescriptorPool> pools;
  StageInputs inputs;

  Shader shaders[2];

  operator VkPipeline() { return pipeline; }
  operator VkPipelineLayout() { return layout; }
  void create();
  void recompile();

  static Box<Pipeline> mk(std::string const& shader);

  VkCullModeFlags culling = VK_CULL_MODE_NONE;
  VkPolygonMode mode = VK_POLYGON_MODE_FILL;
  VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  bool depth_test = true;
  bool depth_write = true;
  f32 line_width = 1;

  void reset();

  void bind(VkCommandBuffer cmd = vk) {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
  }

  void bind_set(auto set, u32 idx = 0, VkCommandBuffer cmd = vk) {
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, idx, 1,
                            &set, 0, 0);
  }

  template <class T>
  void push(T&& data, u32 offset = 0, VkCommandBuffer cmd = vk) {
    vkCmdPushConstants(cmd, layout, 17, offset, sizeof(T), &data);
  }

  void set_mode(VkPolygonMode mode) {
    this->mode = mode;
    reset();
  }

  void set_line_width(f32 width) {
    line_width = width;
    reset();
  }

  void set_topology(VkPrimitiveTopology topology) {
    this->topology = topology;
    reset();
  }

  bool toggle_depth_test(VkPrimitiveTopology topology) {
    depth_test = !depth_test;
    reset();
    return depth_test;
  }

  bool toggle_depth_write(VkPrimitiveTopology topology) {
    depth_write = !depth_write;
    reset();
    return depth_write;
  }

  VkDescriptorSet alloc_set(u32 set);
  ~Pipeline();

 private:
  Pipeline(std::string const& shader);
};

}  // namespace xofo
#endif /* B6EB7CB5_9227_4FBF_9721_005CAC0511AA */
