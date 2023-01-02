//
// Created by f0xeri on 01.01.2023.
//

#include <cstring>
#include "VulkanBuffer.hpp"

void VulkanBuffer::uploadBuffer(VmaAllocator allocator, void *data, size_t size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = bufferUsage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;

    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer, &allocation, nullptr);

    void* mappedData;
    vmaMapMemory(allocator, allocation, &mappedData);
    memcpy(mappedData, data, size);
    vmaUnmapMemory(allocator, allocation);
}

VulkanBuffer::~VulkanBuffer() {

}
