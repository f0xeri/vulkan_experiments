//
// Created by f0xeri on 01.01.2023.
//

#ifndef VULKAN_EXPERIMENTS_VULKANMESH_HPP
#define VULKAN_EXPERIMENTS_VULKANMESH_HPP

#include <vulkan/vulkan.h>
#include <vector>
#include "vk_mem_alloc.h"
#include "VulkanBuffer.hpp"
#include "glm/glm.hpp"
#include "core/Mesh.hpp"

struct VertexInputDescription {
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;

    VkPipelineVertexInputStateCreateFlags flags = 0;
};

struct VulkanVertex : public Vertex {
    static VertexInputDescription getVertexDescription();
};

struct VulkanMesh : public Mesh {
    VulkanBuffer vertexBuffer;
};


#endif //VULKAN_EXPERIMENTS_VULKANMESH_HPP
