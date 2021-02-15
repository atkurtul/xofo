#ifndef D8F23374_2F16_4692_A5AE_7D3369B8E620
#define D8F23374_2F16_4692_A5AE_7D3369B8E620
#include <vulkan/vulkan_core.h>
#include "core.h"

namespace xofo {
struct Buffer : ShaderResource {
  enum {
    Src = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    Dst = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
    Vertex = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    Index = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    Uniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
  };

  enum Mapping {
    Mapped,
    Unmapped,
  };

  VkBuffer buffer;
  u8* mapping;
  VmaAllocation allocation;

  operator VkBuffer() const { return buffer; }

  void bind_vertex(u64 offset = 0, u32 binding = 0, VkCommandBuffer cmd = vk) {
    vkCmdBindVertexBuffers(cmd, binding, 1, &buffer, &offset);
  }

  void bind_index(u64 offset = 0, VkCommandBuffer cmd = vk) {
    vkCmdBindIndexBuffer(cmd, buffer, offset, VK_INDEX_TYPE_UINT32);
  }

  void copy(size_t len, void* pp, size_t offset = 0) {
    memcpy(mapping + offset, pp, len);
  }

  template <class T>
  void copy(T const& obj, size_t offset = 0) {
    memcpy(mapping + offset, &obj, sizeof(T));
  }

  ~Buffer() { vmaDestroyBuffer(vk, buffer, allocation); }

  VkDescriptorSet bind_to_set(VkDescriptorSet set, u32 bind) {
    VkDescriptorBufferInfo info = {
        .buffer = buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = set,
        .dstBinding = bind,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &info};

    vkUpdateDescriptorSets(vk, 1, &write, 0, 0);
    return set;
  }

  static Box<Buffer> mk(u64 size, VkBufferUsageFlags usage, Mapping);

 protected:
  Buffer(u64 size, VkBufferUsageFlags usage, Mapping);
};

}  // namespace xofo

#endif /* D8F23374_2F16_4692_A5AE_7D3369B8E620 */
