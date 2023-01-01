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

    VkPipeline buildPipeline(VkDevice device, VkRenderPass pass);
    static VkPipelineShaderStageCreateInfo createShaderStageInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
    static VkPipelineVertexInputStateCreateInfo createVertexInputInfo(VkVertexInputBindingDescription bindingDescription, std::vector<VkVertexInputAttributeDescription> attributeDescriptions);
    static VkPipelineVertexInputStateCreateInfo createVertexInputInfo();
    static VkPipelineInputAssemblyStateCreateInfo createInputAssemblyInfo(VkPrimitiveTopology topology);
    static VkPipelineRasterizationStateCreateInfo createRasterizerInfo(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace);
    static VkPipelineMultisampleStateCreateInfo createMultisamplingInfo(VkSampleCountFlagBits sampleCount);
    static VkPipelineColorBlendAttachmentState createColorBlendAttachmentState();
    static VkPipelineLayoutCreateInfo createPipelineLayoutInfo();

    ~VulkanPipelineBuilder();
};


#endif //VULKAN_EXPERIMENTS_VULKANPIPELINEBUILDER_HPP
