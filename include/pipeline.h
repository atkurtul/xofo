#include <util.h>
#include <vulkan/vulkan_core.h>
#include <string>
#include <vector>
using namespace std;

struct Shader {
  vector<u32> bytecode;
  VkShaderModule module;
};

struct Pipeline {
  string shader;
  VkPipelineLayout layout;
  VkPipeline pipeline;
  vector<VkDescriptorSetLayout> set_layouts;
  vector<VkDescriptorPool> pools;

  vector<VkVertexInputAttributeDescription> attr;
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