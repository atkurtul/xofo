#ifndef E7CD5CCF_D975_419F_81AE_1FAAB38BA0F3
#define E7CD5CCF_D975_419F_81AE_1FAAB38BA0F3
#include <buffer.h>
#include <image.h>
#include <texture.h>

#include <window.h>
#include <functional>
#include <iostream>
#include <memory>
#include <vector>

using namespace std;


template <class T>
using Box = unique_ptr<T>;

struct Resources {
  VkCommandPool pool;
  VkSwapchainKHR swapchain;
  VkRenderPass renderpass;
  VkSurfaceFormatKHR fmt;
  VkExtent2D extent;

  Box<Image> depth_buffer;
  u32 curr;
  struct PerFrame {
    VkImage img;
    VkImageView view;
    VkFramebuffer framebuffer;
    VkCommandBuffer cmd;
    VkFence fence;
    VkSemaphore acquire;
    VkSemaphore present;
  };

  vector<PerFrame> frames;

  void init();
  void free_frames();
  void free();
};

extern struct Vk {
  inline static const char* layer[] = {"VK_LAYER_KHRONOS_validation"};

  VkInstance instance;
  VkDevice dev;
  VkPhysicalDevice pdev;
  VkQueue queue;
  VmaAllocator allocator;
  Window win;
  Resources res;

  Vk();
  ~Vk();

  operator VkInstance() const { return instance; }
  operator VkDevice() const { return dev; }
  operator VkPhysicalDevice() const { return pdev; }
  operator VmaAllocator() const { return allocator; }

  template <class F>
  void execute(F func) {
    auto cmd = get_cmd();
    func(cmd);
    submit_cmd(cmd);
  }

  VkCommandBuffer get_cmd();

  void submit_cmd(VkCommandBuffer cmd);
  static void init();

  void draw(std::function<void(VkCommandBuffer)>);

 private:
  void init_device();

} vk;

#endif /* E7CD5CCF_D975_419F_81AE_1FAAB38BA0F3 */
