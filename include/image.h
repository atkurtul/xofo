#ifndef B4289538_0D44_47DC_927C_CB5FB0CAE07A
#define B4289538_0D44_47DC_927C_CB5FB0CAE07A
#include <vk_mem_alloc.h>
#include <util.h>

struct Image {
  VmaAllocation allocation;
  VkImage image;
  VkImageView view;
  VkSampler sampler;
  VkDescriptorType type;
  VkImageLayout layout;
  enum {
    Src = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    Dst = VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    Sampled = VK_IMAGE_USAGE_SAMPLED_BIT,
    Storage = VK_IMAGE_USAGE_STORAGE_BIT,
    Color = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
    DepthStencil = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    Transient = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
    Input = VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
  };

  operator VkImageView() const { return view; }
  operator VkImage() const { return image; }

  static void type_layout(VkImageUsageFlags usage,
                          VkDescriptorType& type,
                          VkImageLayout& layout);

  static Box<Image> mk(VkFormat format,
                       VkImageUsageFlags usage,
                       VkExtent2D extent,
                       u32 mip,
                       VkSampleCountFlagBits ms,
                       VkImageAspectFlags aspect);

  ~Image();

  VkDescriptorSet bind_to_set(VkDescriptorSet set, u32 bind);
};

#endif /* B4289538_0D44_47DC_927C_CB5FB0CAE07A */
