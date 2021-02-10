#include <util.h>
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
  void reset();
  VkDescriptorSet alloc_set(u32 set);
  ~Pipeline();
};