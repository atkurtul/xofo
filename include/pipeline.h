#ifndef B6EB7CB5_9227_4FBF_9721_005CAC0511AA
#define B6EB7CB5_9227_4FBF_9721_005CAC0511AA
#include "core.h"

namespace xofo {

struct Shader {
  std::vector<u32> bytecode;
  VkShaderModule module;
};

struct Pipeline {
  std::string shader;
  VkPipelineLayout layout;
  VkPipeline pipeline;
  std::vector<VkDescriptorSetLayout> set_layouts;
  std::vector<VkDescriptorPool> pools;

  std::vector<VkVertexInputAttributeDescription> attr;
  u32 stride;
  VkShaderModule modules[2];
  operator VkPipeline() { return pipeline; }
  operator VkPipelineLayout() { return layout; }
  void create();
  void reload_shaders();
  Pipeline(std::string const& shader);

  VkPolygonMode mode = VK_POLYGON_MODE_FILL;
  VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  bool depth_test = true;
  bool depth_write = true;

  void reset();
  void recompile() {}

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
};

}  // namespace xofo
#endif /* B6EB7CB5_9227_4FBF_9721_005CAC0511AA */
