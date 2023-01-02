//
// Created by f0xeri on 01.01.2023.
//

#ifndef VULKAN_EXPERIMENTS_VULKANBUFFER_HPP
#define VULKAN_EXPERIMENTS_VULKANBUFFER_HPP


#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

class VulkanBuffer {
    VmaAllocator allocator = {};
public:
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = {};

    VulkanBuffer() = default;
    ~VulkanBuffer();
    void uploadBuffer(VmaAllocator allocator, void* data, size_t size, VkBufferUsageFlags bufferUsage, VmaMemoryUsage memoryUsage);
};


#endif //VULKAN_EXPERIMENTS_VULKANBUFFER_HPP
