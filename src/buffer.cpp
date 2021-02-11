#include <xofo.h>
#include <memory>

using namespace xofo;

Box<Buffer> Buffer::mk(size_t size, VkBufferUsageFlags usage, Mapping map) {
  VkBufferCreateInfo buffer_info = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = usage};

  VmaAllocationCreateInfo allocation_info;
  switch (map) {
    case Mapped:
      allocation_info = {
          .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
          .usage = VMA_MEMORY_USAGE_CPU_TO_GPU,
      };
      break;
    case Unmapped:
      allocation_info = {
          .usage = VMA_MEMORY_USAGE_GPU_ONLY,
      };
      break;
  }
  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo alloc_info;
  CHECKRE(vmaCreateBuffer(vk, &buffer_info, &allocation_info, &buffer,
                          &allocation, &alloc_info));

                          
  return Box<Buffer>(new Buffer {
      .buffer = buffer,
      .mapping = (char*)alloc_info.pMappedData,
      .allocation = allocation,
  });
}

