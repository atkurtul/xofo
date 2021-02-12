#ifndef B6EB7CB5_9227_4FBF_9721_005CAC0511AA
#define B6EB7CB5_9227_4FBF_9721_005CAC0511AA
#include "core.h"
#include <vulkan/vulkan_core.h>

namespace xofo {

struct Shader {
  std::vector<u32> bytecode;
  VkShaderModule mod;

  operator VkShaderModule() const { return mod; }
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
  std::vector<VkVertexInputAttributeDescription> attr;

  u32 stride;
  Shader shaders[2];

  operator VkPipeline() { return pipeline; }
  operator VkPipelineLayout() { return layout; }
  void create();
  void recompile();
  
  static Box<Pipeline> mk(std::string const& shader);

  VkPolygonMode mode = VK_POLYGON_MODE_FILL;
  VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  bool depth_test = true;
  bool depth_write = true;

  void reset();


  void set_mode(VkPolygonMode mode) {
    this->mode = mode;
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
