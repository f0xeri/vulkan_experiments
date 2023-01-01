//
// Created by f0xeri on 01.01.2023.
//

#ifndef VULKAN_EXPERIMENTS_VULKANBUFFER_HPP
#define VULKAN_EXPERIMENTS_VULKANBUFFER_HPP


#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"

struct VulkanBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = {};
};


#endif //VULKAN_EXPERIMENTS_VULKANBUFFER_HPP
