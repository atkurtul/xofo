#ifndef E7CD5CCF_D975_419F_81AE_1FAAB38BA0F3
#define E7CD5CCF_D975_419F_81AE_1FAAB38BA0F3


#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "typedefs.h"
#include "vk_mem_alloc.h"


template <class T>
using Box = std::unique_ptr<T>;
template <class T>
using Rc = std::shared_ptr<T>;
template <class T>
using Opt = std::optional<T>;

const char* vk_result_string(VkResult re);
const char* desc_type_string(VkDescriptorType ty);
#define CHECKRE(expr)                                                          \
  {                                                                            \
    while (VkResult re = (expr)) {                                             \
      printf("Error: %s\n %s:%d\n", vk_result_string(re), __FILE__, __LINE__); \
      abort();                                                                 \
    }                                                                          \
  }

namespace xofo {

void init();

void at_exit(std::function<void()> const& f);

VkExtent2D extent();
void recreate();
void execute(std::function<void(VkCommandBuffer cmd)> const& f);

void register_recreation_callback(std::function<void(VkExtent2D)> const& f);
void draw(std::function<void(VkCommandBuffer)> const&,
          std::function<void()> const& imgui);

VkRenderPass renderpass();
u32 buffer_count();

void resize(VkExtent2D x);
bool get_key(char);
bool mbutton(int lr);
void hide_mouse(bool state) ;

int poll();
double dt();
vec2 mdelta();

VkSampler create_sampler(VkSamplerAddressMode mode, uint mip);

static struct VulkanProxy {
  operator VkInstance();
  operator VkDevice();
  operator VkPhysicalDevice();
  operator VmaAllocator();
} vk;
}  // namespace xofo

#endif /* E7CD5CCF_D975_419F_81AE_1FAAB38BA0F3 */
