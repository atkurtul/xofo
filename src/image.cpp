#include <xofo.h>


using namespace std;
using namespace xofo;

void Image::type_layout(VkImageUsageFlags usage,
                        VkDescriptorType& type,
                        VkImageLayout& layout) {
  switch (usage & (Sampled | Storage | Color | DepthStencil | Input)) {
    case Sampled:
      type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      // type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
      layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      break;
    case Storage:
      type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      break;
    case Color:
      type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      break;
    case DepthStencil:
      type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
      break;
    case Input:
      type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
      layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      break;
  }
}

Box<Image> Image::mk(VkImageCreateInfo const& info, Type type) {
  return Box<Image>(new Image(info, type));
}

Image::Image(VkImageCreateInfo const& info, Type type) {
  width = info.extent.width;
  height = info.extent.height;
  VmaAllocationCreateInfo allocation_info = {.usage =
                                                 VMA_MEMORY_USAGE_GPU_ONLY};
  VmaAllocationInfo out_info;

  CHECKRE(vmaCreateImage(vk, &info, &allocation_info, &image, &allocation,
                         &out_info));

  VkImageSubresourceRange subrange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .levelCount = info.mipLevels,
      .layerCount = 1,
  };

  auto view_type = VK_IMAGE_VIEW_TYPE_2D;
  switch (type) {
    case Type::DepthBuffer:
      subrange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      break;
    case Type::CubeMap:
      subrange.levelCount = info.mipLevels;
      subrange.layerCount = 6;
      view_type = VK_IMAGE_VIEW_TYPE_CUBE;
      break;
    case Type::Standard:
      break;
  }

  VkImageViewCreateInfo view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = view_type,
      .format = info.format,
      .components = {VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G,
                     VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A},
      .subresourceRange = subrange,
  };

  CHECKRE(vkCreateImageView(vk, &view_info, 0, &view));

  type_layout(info.usage, this->type, layout);

  sampler = create_sampler(VK_SAMPLER_ADDRESS_MODE_REPEAT, info.mipLevels);
}


Image::Image(VkFormat format,
             VkImageUsageFlags usage,
             VkExtent2D extent,
             u32 mip,
             Type type,
             VkSampleCountFlagBits ms)
    : Image(
          VkImageCreateInfo{
              .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
              .imageType = VK_IMAGE_TYPE_2D,
              .format = format,
              .extent = {.width = extent.width,
                         .height = extent.height,
                         .depth = 1},
              .mipLevels = mip,
              .arrayLayers = 1,
              .samples = ms,
              .tiling = VK_IMAGE_TILING_OPTIMAL,
              .usage = usage,
              .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
          },
          type) {}

Box<Image> Image::mk(VkFormat format,
                     VkImageUsageFlags usage,
                     VkExtent2D extent,
                     u32 mip,
                     Type type,
                     VkSampleCountFlagBits ms) {
  VkImageCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format,
      .extent = {.width = extent.width, .height = extent.height, .depth = 1},
      .mipLevels = mip,
      .arrayLayers = 1,
      .samples = ms,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  return Image::mk(info, type);
}

Image::~Image() {
  vkDestroyImageView(vk, view, 0);
  vkDestroyImage(vk, image, 0);
  vmaFreeMemory(vk, allocation);
}
