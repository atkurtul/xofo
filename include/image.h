#ifndef B4289538_0D44_47DC_927C_CB5FB0CAE07A
#define B4289538_0D44_47DC_927C_CB5FB0CAE07A
#include "core.h"

namespace xofo {
struct Image : ShaderResource {
  VmaAllocation allocation;
  VkImage image;
  VkImageView view;
  VkSampler sampler;
  VkDescriptorType type;
  VkImageLayout layout;
  u32 width, height;
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

  enum class Type {
    Standard,
    DepthBuffer,
    CubeMap,
  };

  operator VkImageView() const { return view; }
  operator VkImage() const { return image; }

  static void type_layout(VkImageUsageFlags usage,
                          VkDescriptorType& type,
                          VkImageLayout& layout);

  static Box<Image> mk(VkImageCreateInfo const& info, Type ty = Type::Standard);

  static Box<Image> mk(VkFormat format,
                       VkImageUsageFlags usage,
                       VkExtent2D extent,
                       u32 mip,
                       Type ty = Type::Standard,
                       VkSampleCountFlagBits ms = VK_SAMPLE_COUNT_1_BIT);

  ~Image();

  VkDescriptorSet bind_to_set(VkDescriptorSet set, u32 bind);

  protected:
  Image(VkImageCreateInfo const& info, Type ty = Type::Standard);
  Image(VkFormat format,
                       VkImageUsageFlags usage,
                       VkExtent2D extent,
                       u32 mip,
                       Type ty = Type::Standard,
                       VkSampleCountFlagBits ms = VK_SAMPLE_COUNT_1_BIT);
};

struct Texture : Image  {
  static std::vector<Texture*> textures;
  std::string origin;
  static Box<Texture> mk(std::string file, VkFormat format);
  static Box<Texture> mk(std::string tag, void* data, VkFormat format, u32 width, u32 height);


  static Box<Texture> load_cubemap_ktx(std::string file, VkFormat format);
  static Box<Texture> load_cubemap_6_files_from_folder(std::string folder, VkFormat format);
  static Box<Texture> load_cubemap_single_file(std::string folder, VkFormat format);

  private:
  using Image::Image;
};


}  // namespace xofo



#endif /* B4289538_0D44_47DC_927C_CB5FB0CAE07A */
