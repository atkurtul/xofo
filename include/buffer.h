#ifndef D8F23374_2F16_4692_A5AE_7D3369B8E620
#define D8F23374_2F16_4692_A5AE_7D3369B8E620
#include <vk_mem_alloc.h>
#include <util.h>
#include <cstring>


struct Buffer {
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
  char* mapping;
  VmaAllocation allocation;

  operator VkBuffer() const { return buffer; }

  void copy(size_t len, void* pp, size_t offset = 0) { memcpy(mapping + offset, pp, len); }

  template <class T>
  void copy(T const& obj, size_t offset = 0) {
    memcpy(mapping + offset, &obj, sizeof(T));
  }

  ~Buffer();

  static Box<Buffer> mk(size_t size, VkBufferUsageFlags usage, Mapping);


  VkDescriptorSet bind_to_set(VkDescriptorSet set, u32 bind);

};

#endif /* D8F23374_2F16_4692_A5AE_7D3369B8E620 */
