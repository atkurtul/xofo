#include "texture.h"
#include <image.h>
#include <math.h>
#include <stb_image.h>
#include <vk.h>
#include <cstdio>
#include <cstdlib>

void copy_texture(VkCommandBuffer cmd,
                  VkBuffer src,
                  VkImage image,
                  VkExtent2D extent) {
  VkImageSubresourceRange subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .levelCount = 1,
      .layerCount = 1,
  };

  VkImageMemoryBarrier barr = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      //.srcAccessMask = 0,
      .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange = subresourceRange,
  };
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1, &barr);

  // Copy the first mip of the chain, remaining mips will be generated
  VkBufferImageCopy region = {.imageSubresource =
                                  {
                                      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                      .mipLevel = 0,
                                      .baseArrayLayer = 0,
                                      .layerCount = 1,
                                  },
                              .imageExtent = {
                                  .width = extent.width,
                                  .height = extent.height,
                                  .depth = 1,
                              }};

  vkCmdCopyBufferToImage(cmd, src, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1, &region);

  VkImageMemoryBarrier barr1 = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange = subresourceRange,
  };
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1,
                       &barr1);
}

void generate_mips(VkCommandBuffer cmd,
                   uint mip,
                   VkImage image,
                   VkExtent2D extent) {
  for (uint i = 1; i < mip; ++i) {
    VkImageBlit image_blit = {
        .srcSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i - 1,
                .layerCount = 1,
            },
        .dstSubresource =
            {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i,
                .layerCount = 1,
            },

    };
    image_blit.srcOffsets[1] = {
        .x = (i32)std::max((extent.width >> (i - 1)), 1u),
        .y = (i32)std::max((extent.height >> (i - 1)), 1u),
        .z = 1};
    image_blit.dstOffsets[1] = {
        .x = (i32)std::max((extent.width >> i), 1u),
        .y = (i32)std::max((extent.height >> i), 1u),
        .z = 1,
    };
    VkImageSubresourceRange subrange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = i,
        .levelCount = 1,
        .layerCount = 1,
    };

    VkImageMemoryBarrier barr1 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subrange,
    };

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1,
                         &barr1);

    vkCmdBlitImage(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_blit,
                   VK_FILTER_LINEAR);

    VkImageMemoryBarrier barr2 = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subrange,
    };
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, 0, 0, 0, 1,
                         &barr2);
  }

  VkImageMemoryBarrier barr = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
      .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
      .oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .image = image,
      .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                           .levelCount = mip,
                           .layerCount = 1},
  };
  vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, 0, 0, 0, 1,
                       &barr);
}

#include <mango/image/surface.hpp>

Box<Texture> Texture::mk(std::string file, VkFormat format) {

  std::replace(file.begin(), file.end(), '\\', '/');
  int width, height, n;
  u8* data = stbi_load(file.c_str(), &width, &height, &n, 4);

  u32 size = width * height * 4;
  if (!data) {
    printf("Failed to load texture %s\n", file.c_str());
    abort();
    mango::Bitmap bitmap(file, mango::Format(32, mango::Format::UNORM,
                                             mango::Format::RGBA, 8, 8, 8, 8));
    width = bitmap.width;
    height = bitmap.height;

    size = width * height * 4;
    data = (u8*)malloc(size);
    memcpy(data, bitmap.address(0,0), size);
  }

  VkExtent2D extent = {(u32)width, (u32)height};

  u32 mip = (u32)floor(log2(width > height ? width : height)) + 1;
  auto image =
      Box<Texture>((Texture*)Image::mk(
                       format, Image::Src | Image::Dst | Image::Sampled, extent,
                       mip, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT)
                       .release());

  auto src = Buffer::mk(size, Buffer::Src, Buffer::Mapped);
  memcpy(src->mapping, data, size);
  free(data);
  vk.execute([&](auto cmd) {
    copy_texture(cmd, *src, *image, extent);
    generate_mips(cmd, mip, *image, extent);
  });
  return image;
}
