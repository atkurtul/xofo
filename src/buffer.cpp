#include "core.h"
#include <xofo.h>

using namespace xofo;

Buffer::Buffer(u64 size, VkBufferUsageFlags usage, Mapping map) {
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
  VmaAllocationInfo alloc_info;
  CHECKRE(vmaCreateBuffer(vk, &buffer_info, &allocation_info, &buffer,
                          &allocation, &alloc_info));

  mapping = (u8*)alloc_info.pMappedData;
}

Box<Buffer> Buffer::mk(u64 size, VkBufferUsageFlags usage, Mapping map) {
  return Box<Buffer>(new Buffer(size, usage, map));
}


Box<Buffer> Buffer::unmapped(std::vector<std::pair<u64, u8*>> raws, VkBufferUsageFlags usage) {

  u64 total = 0;
  for(auto [s, _] : raws) total += s;
  
  auto buffer = Buffer::mk(total, usage | Buffer::Dst, Buffer::Unmapped);

  auto staging = Buffer::mk(total, Buffer::Src, Buffer::Mapped);

  u64 offset = 0;

  for(auto [size, data] : raws) {
    memcpy(staging->mapping + offset, data, size);
    offset += size;
  }

  xofo::execute([&](auto cmd) {
      VkBufferCopy reg = {0, 0, total};
      vkCmdCopyBuffer(cmd, *staging, *buffer, 1, &reg);
  });

  return buffer;
}