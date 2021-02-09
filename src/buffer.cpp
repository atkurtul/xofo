#include <vk.h>

Buffer::Buffer(size_t size, VkBufferUsageFlags usage, Mapping map) {
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
  CHECKRE(vmaCreateBuffer(vk, &buffer_info, &allocation_info, &buffer, &allocation,
                  &alloc_info));
  mapping = (char*)alloc_info.pMappedData;
}

Buffer::~Buffer() {
  vmaDestroyBuffer(vk, buffer, allocation);
}

void Buffer::bind_to_set(VkDescriptorSet set){
    VkDescriptorBufferInfo info = {
        .buffer = buffer,
        .offset = 0,
        .range = VK_WHOLE_SIZE,
    };

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = set,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &info};

    (vkUpdateDescriptorSets(vk, 1, &write, 0, 0));
  }