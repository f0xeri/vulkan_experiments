//
// Created by f0xeri on 31.12.2022.
//

#ifndef VULKAN_EXPERIMENTS_VULKANPIPELINEBUILDER_HPP
#define VULKAN_EXPERIMENTS_VULKANPIPELINEBUILDER_HPP


#include <vulkan/vulkan.h>
#include <vector>

class VulkanPipelineBuilder {
public:
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    VkPipelineVertexInputStateCreateInfo vertexInputInfo;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkViewport viewport;
    VkRect2D scissor;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineLayout pipelineLayout;

public:
    VkPipeline buildPipeline(VkDevice device, VkRenderPass pass);
    ~VulkanPipelineBuilder();
};


#endif //VULKAN_EXPERIMENTS_VULKANPIPELINEBUILDER_HPP
