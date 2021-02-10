#include <util.h>
#include <string>
#include <vector>

struct Pipeline {
  VkPipelineLayout layout;
  VkPipeline pipeline;
  std::vector<VkDescriptorSetLayout> set_layouts;
  std::vector<VkDescriptorPool> pools;

  
  operator VkPipeline() { return pipeline; }
  operator VkPipelineLayout() { return layout; }

  
  Pipeline(std::string const& shader);

  VkDescriptorSet alloc_set(u32 set);
  ~Pipeline();
};