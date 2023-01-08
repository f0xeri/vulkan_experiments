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
#include "VulkanShader.hpp"

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
    VulkanBuffer indexBuffer;
};

struct VulkanMaterial {
    VulkanShader shader;
    VkPipeline pipeline = {};
    VkPipelineLayout pipelineLayout = {};
    VkDescriptorSet textureSet = VK_NULL_HANDLE;
};

struct VulkanRenderObject : public RenderObject {
    VulkanMesh *mesh = nullptr;
    VulkanMaterial *material = nullptr;
    glm::mat4 model = glm::mat4(1.0f);
};

#endif //VULKAN_EXPERIMENTS_VULKANMESH_HPP
