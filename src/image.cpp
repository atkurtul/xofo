#include "image.h"
#include <vk.h>
#include <vulkan/vulkan_core.h>

#include <unordered_map>
unordered_map<u64, VkSampler> samplers;

void destroy_samplers() {
  for(auto [k,v]:samplers) {
    (vkDestroySampler(vk, v, 0));
  }
}

static VkSampler create_sampler(VkSamplerAddressMode mode, uint mip) {
  u64 key = ((u64)mode << 32 | mip);
  auto existing = samplers.find(key);
  if (existing != samplers.end())
    return existing->second;

  VkSamplerCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
      .magFilter = VK_FILTER_LINEAR,
      .minFilter = VK_FILTER_LINEAR,
      .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
      .addressModeU = mode,
      .addressModeV = mode,
      .addressModeW = mode,
      //.anisotropyEnable = 1,
      //.maxAnisotropy = 1,
      .compareOp = VK_COMPARE_OP_NEVER,
      .maxLod = (float)mip,
      .borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
  };
  
  VkSampler sampler;
  CHECKRE(vkCreateSampler(vk, &info, 0, &sampler));
  return samplers[key] = sampler;
}


void Image::type_layout(VkImageUsageFlags usage,
                        VkDescriptorType& type,
                        VkImageLayout& layout) {
  switch (usage & (Sampled | Storage | Color | DepthStencil | Input)) {
    case Sampled:
      type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
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

Image::Image(VkFormat format,
             VkImageUsageFlags usage,
             VkExtent2D extent,
             u32 mip,
             VkSampleCountFlagBits ms,
             VkImageAspectFlags aspect) {
  // VkImage handle = MkVkImage(format, usage, extent, mip, ms);

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

  VmaAllocationCreateInfo allocation_info = {.usage =
                                                 VMA_MEMORY_USAGE_GPU_ONLY};
  VmaAllocationInfo out_info;
  CHECKRE(vmaCreateImage(vk, &info, &allocation_info, &image,
                                &allocation, &out_info));

  VkImageViewCreateInfo view_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .image = image,
      .viewType = VK_IMAGE_VIEW_TYPE_2D,
      .format = format,
      .subresourceRange = {
          .aspectMask = aspect,
          .levelCount = 1,
          .layerCount = 1,
      }};
  CHECKRE(vkCreateImageView(vk, &view_info, 0, &view));

  type_layout(usage, type, layout);

  sampler = create_sampler(VK_SAMPLER_ADDRESS_MODE_REPEAT, mip);
}

Image::~Image() {
  vkDestroyImageView(vk, view, 0);
  vkDestroyImage(vk, image, 0);
  vmaFreeMemory(vk, allocation);
}

void Image::bind_to_set(VkDescriptorSet set) {
  VkDescriptorImageInfo info = {
      .sampler = sampler,
      .imageView = view,
      .imageLayout = layout,
  };

  VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                .dstSet = set,
                                .dstBinding = 0,
                                .descriptorCount = 1,
                                .descriptorType = type,
                                .pImageInfo = &info};

  vkUpdateDescriptorSets(vk, 1, &write, 0, 0);
}