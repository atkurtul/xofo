#include <xofo.h>
#include <image.h>

#include <mango/image/surface.hpp>

using namespace std;
using namespace xofo;

struct ImageLayout {
  static constexpr auto Undefined = VK_IMAGE_LAYOUT_UNDEFINED;
  static constexpr auto General = VK_IMAGE_LAYOUT_GENERAL;
  static constexpr auto ColorAttachment =
      VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  static constexpr auto DepthStencilAttachment =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  static constexpr auto DepthStencilReadOnly =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  static constexpr auto ShaderReadOnly =
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  static constexpr auto Src = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  static constexpr auto Dst = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  static constexpr auto Preinitialized = VK_IMAGE_LAYOUT_PREINITIALIZED;
  static constexpr auto DepthReadOnlyStencilAttachment =
      VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
  static constexpr auto DepthAttachmentStencilReadOnly =
      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
  static constexpr auto DepthAttachment =
      VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
  static constexpr auto DepthReadOnly = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
  static constexpr auto StencilAttachment =
      VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
  static constexpr auto StencilReadOnly =
      VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
  static constexpr auto PresentSrc = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  static constexpr auto SharedPresent = VK_IMAGE_LAYOUT_SHARED_PRESENT_KHR;
  static constexpr auto ShadingRate = VK_IMAGE_LAYOUT_SHADING_RATE_OPTIMAL_NV;
  static constexpr auto FragmentDensityMap =
      VK_IMAGE_LAYOUT_FRAGMENT_DENSITY_MAP_OPTIMAL_EXT;
  static constexpr auto FragmentShadingRateAttachment =
      VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
};

struct PipelineStage {
  static constexpr auto TopOfPipe = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  static constexpr auto DrawIndirect = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
  static constexpr auto VertexInput = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
  static constexpr auto VertexShader = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
  static constexpr auto TessellationControlShader =
      VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT;
  static constexpr auto TessellationEvaluationShader =
      VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT;
  static constexpr auto GeometryShader = VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;
  static constexpr auto FragmentShader = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  static constexpr auto EarlyFragmentTests =
      VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  static constexpr auto LateFragmentTests =
      VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  static constexpr auto ColorAttachmentOutput =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  static constexpr auto ComputeShader = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  static constexpr auto Transfer = VK_PIPELINE_STAGE_TRANSFER_BIT;
  static constexpr auto BottomOfPipe = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  static constexpr auto Host = VK_PIPELINE_STAGE_HOST_BIT;
  static constexpr auto AllGraphics = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
  static constexpr auto AllCommands = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  static constexpr auto TransformFeedback =
      VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT;
  static constexpr auto ConditionalRendering =
      VK_PIPELINE_STAGE_CONDITIONAL_RENDERING_BIT_EXT;
  static constexpr auto AccelerationStructureBuild =
      VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
  static constexpr auto RayTracingShader =
      VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
  static constexpr auto ShadingRateImage =
      VK_PIPELINE_STAGE_SHADING_RATE_IMAGE_BIT_NV;
  static constexpr auto TaskShader = VK_PIPELINE_STAGE_TASK_SHADER_BIT_NV;
  static constexpr auto MeshShader = VK_PIPELINE_STAGE_MESH_SHADER_BIT_NV;
  static constexpr auto FragmentDensityProcess =
      VK_PIPELINE_STAGE_FRAGMENT_DENSITY_PROCESS_BIT_EXT;
  static constexpr auto CommandPreprocess =
      VK_PIPELINE_STAGE_COMMAND_PREPROCESS_BIT_NV;
  static constexpr auto FragmentShadingRateAttachment =
      VK_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
};

void set_image_layout(
    VkCommandBuffer cmd,
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    VkImageSubresourceRange subrange,
    VkPipelineStageFlags src_stage_mask = PipelineStage::Transfer,
    VkPipelineStageFlags dst_stage_mask = PipelineStage::Transfer) {
  // Create an image barrier object
  VkImageMemoryBarrier imageMemoryBarrier = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
      .oldLayout = old_layout,
      .newLayout = new_layout,
      .image = image,
      .subresourceRange = subrange,
  };

  // Source layouts (old)
  // Source access mask controls actions that have to be finished on the old
  // layout before it will be transitioned to the new layout
  switch (old_layout) {
    case ImageLayout::Undefined:
      // Image layout is undefined (or does not matter)
      // Only valid as initial layout
      // No flags required, listed only for completeness
      imageMemoryBarrier.srcAccessMask = 0;
      break;

    case ImageLayout::Preinitialized:
      // Image is preinitialized
      // Only valid as initial layout for linear images, preserves memory
      // contents Make sure host writes have been finished
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
      break;

    case ImageLayout::ColorAttachment:
      // Image is a color attachment
      // Make sure any writes to the color buffer have been finished
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      break;

    case ImageLayout::DepthStencilAttachment:
      // Image is a depth/stencil attachment
      // Make sure any writes to the depth/stencil buffer have been finished
      imageMemoryBarrier.srcAccessMask =
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;

    case ImageLayout::Src:
      // Image is a transfer source
      // Make sure any reads from the image have been finished
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      break;

    case ImageLayout::Dst:
      // Image is a transfer destination
      // Make sure any writes to the image have been finished
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;

    case ImageLayout::ShaderReadOnly:
      // Image is read by a shader
      // Make sure any shader reads from the image have been finished
      imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    default:
      // Other source layouts aren't handled (yet)
      break;
  }

  // Target layouts (new)
  // Destination access mask controls the dependency for the new image layout
  switch (new_layout) {
    case ImageLayout::Dst:
      // Image will be used as a transfer destination
      // Make sure any writes to the image have been finished
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      break;

    case ImageLayout::Src:
      // Image will be used as a transfer source
      // Make sure any reads from the image have been finished
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
      break;

    case ImageLayout::ColorAttachment:
      // Image will be used as a color attachment
      // Make sure any writes to the color buffer have been finished
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      break;

    case ImageLayout::DepthStencilAttachment:
      // Image layout will be used as a depth/stencil attachment
      // Make sure any writes to depth/stencil buffer have been finished
      imageMemoryBarrier.dstAccessMask |=
          VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
      break;

    case ImageLayout::ShaderReadOnly:
      // Image will be read in a shader (sampler, input attachment)
      // Make sure any writes to the image have been finished
      if (imageMemoryBarrier.srcAccessMask == 0) {
        imageMemoryBarrier.srcAccessMask =
            VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
      }
      imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      break;
    default:
      // Other source layouts aren't handled (yet)
      break;
  }

  // Put barrier inside setup command buffer
  vkCmdPipelineBarrier(cmd, src_stage_mask, dst_stage_mask, 0, 0, 0, 0, 0, 1,
                       &imageMemoryBarrier);
}

void copy_texture(VkCommandBuffer cmd,
                  VkBuffer src,
                  VkImage image,
                  VkExtent2D extent) {
  VkImageSubresourceRange subrange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .levelCount = 1,
      .layerCount = 1,
  };

  set_image_layout(cmd, image, ImageLayout::Undefined, ImageLayout::Dst,
                   subrange);

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

  vkCmdCopyBufferToImage(cmd, src, image, ImageLayout::Dst, 1, &region);

  set_image_layout(cmd, image, ImageLayout::Dst, ImageLayout::Src, subrange);
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

    image_blit.srcOffsets[1] = {.x = (i32)max((extent.width >> (i - 1)), 1u),
                                .y = (i32)max((extent.height >> (i - 1)), 1u),
                                .z = 1};
    image_blit.dstOffsets[1] = {
        .x = (i32)max((extent.width >> i), 1u),
        .y = (i32)max((extent.height >> i), 1u),
        .z = 1,
    };

    VkImageSubresourceRange subrange = {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = i,
        .levelCount = 1,
        .layerCount = 1,
    };

    set_image_layout(cmd, image, ImageLayout::Undefined, ImageLayout::Dst,
                     subrange);
    vkCmdBlitImage(cmd, image, ImageLayout::Src, image, ImageLayout::Dst, 1,
                   &image_blit, VK_FILTER_LINEAR);

    set_image_layout(cmd, image, ImageLayout::Dst, ImageLayout::Src, subrange);
  }

  set_image_layout(cmd, image, ImageLayout::Src, ImageLayout::ShaderReadOnly,
                   {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = mip,
                    .layerCount = 1},
                   PipelineStage::Transfer, PipelineStage::FragmentShader);
}

Box<Texture> xofo::Texture::mk(void* data,
                               VkFormat format,
                               u32 width,
                               u32 height) {
  u64 size = width * height * 4;

  VkExtent2D extent = {width, height};

  u32 mip = (u32)floor(log2(width > height ? width : height)) + 1;

  auto image = Box<Texture>(
      (Texture*)Image::mk(format, Image::Src | Image::Dst | Image::Sampled,
                          extent, mip)
          .release());

  auto src = Buffer::mk(size, Buffer::Src, Buffer::Mapped);
  memcpy(src->mapping, data, size);

  xofo::execute([&](auto cmd) {
    copy_texture(cmd, *src, *image, extent);
    generate_mips(cmd, mip, *image, extent);
  });

  return image;
}

Box<Texture> Texture::mk(string file, VkFormat format) {
  replace(file.begin(), file.end(), '\\', '/');

  mango::Bitmap bitmap(file, mango::Format(32, mango::Format::UNORM,
                                           mango::Format::RGBA, 8, 8, 8, 8));

  return Texture::mk(bitmap.image, format, bitmap.width, bitmap.height);
}

using namespace xofo;

#include <ktx.h>
#include <ktxvulkan.h>

Box<Texture> xofo::load_cubemap(string file, VkFormat format) {
  ktxTexture* tex;
  ktxResult result = ktxTexture_CreateFromNamedFile(
      file.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &tex);
  if (result) {
    cerr << "Err loading " << file << "\n";
    abort();
  }

  u32 width = tex->baseWidth;
  u32 height = tex->baseHeight;

  u32 mip = tex->numLevels;
  u64 size = ktxTexture_GetSize(tex);
  u8* data = ktxTexture_GetData(tex);

  cout << "Width: " << width << "\n";
  cout << "Height: " << height << "\n";
  cout << "Mip: " << mip << "\n";
  cout << "Size: " << size << "\n";

  auto staging = Buffer::mk(size, Buffer::Src, Buffer::Mapped);
  memcpy(staging->mapping, data, size);

  // Create optimal tiled target image
  VkImageCreateInfo image_info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
      // This flag is required for cube map images
      .flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
      .imageType = VK_IMAGE_TYPE_2D,
      .format = format,
      .extent = {width, height, 1},
      .mipLevels = mip,
      // Cube faces count as array layers in Vulkan
      .arrayLayers = 6,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .tiling = VK_IMAGE_TILING_OPTIMAL,
      .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  auto image = Image::mk(image_info, Image::Type::CubeMap);

  vector<VkBufferImageCopy> regions;
  regions.reserve(6 * mip);

  for (uint32_t face = 0; face < 6; face++) {
    for (uint32_t level = 0; level < mip; level++) {
      // Calculate offset into staging buffer for the current mip level and face
      ktx_size_t offset;
      KTX_error_code ret =
          ktxTexture_GetImageOffset(tex, level, 0, face, &offset);
      assert(ret == KTX_SUCCESS);
      VkBufferImageCopy region = {
          .bufferOffset = offset,
          .imageExtent = {.width = width >> level,
                          .height = height >> level,
                          .depth = 1},
      };
      region.imageSubresource = {
          .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
          .mipLevel = level,
          .baseArrayLayer = face,
          .layerCount = 1,
      };
      regions.push_back(region);
    }
  }

  VkImageSubresourceRange subrange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = mip,
      .layerCount = 6,
  };

  xofo::execute([&](auto cmd) {
    set_image_layout(cmd, *image, ImageLayout::Undefined, ImageLayout::Dst,
                     subrange);

    vkCmdCopyBufferToImage(cmd, *staging, *image, ImageLayout::Dst,
                           regions.size(), regions.data());

    set_image_layout(cmd, *image, ImageLayout::Dst, ImageLayout::ShaderReadOnly,
                     subrange, PipelineStage::Transfer,
                     PipelineStage::FragmentShader);
  });

  return Box<Texture>((Texture*)image.release());
}