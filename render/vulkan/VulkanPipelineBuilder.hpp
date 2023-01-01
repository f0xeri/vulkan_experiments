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
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    VkPipelineLayout pipelineLayout;

    VkPipeline buildPipeline(VkDevice device, VkRenderPass pass);
    static VkPipelineShaderStageCreateInfo createShaderStageInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
    static VkPipelineVertexInputStateCreateInfo createVertexInputInfo(const std::shared_ptr<std::vector<VkVertexInputBindingDescription>>& bindingDescriptions,
                                                                      const std::shared_ptr<std::vector<VkVertexInputAttributeDescription>>& attributeDescriptions);
    static VkPipelineVertexInputStateCreateInfo createVertexInputInfo();
    static VkPipelineInputAssemblyStateCreateInfo createInputAssemblyInfo(VkPrimitiveTopology topology);
    static VkPipelineRasterizationStateCreateInfo createRasterizerInfo(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace);
    static VkPipelineMultisampleStateCreateInfo createMultisamplingInfo(VkSampleCountFlagBits sampleCount);
    static VkPipelineColorBlendAttachmentState createColorBlendAttachmentState();
    static VkPipelineLayoutCreateInfo createPipelineLayoutInfo();
    static VkPipelineDepthStencilStateCreateInfo createDepthStencilInfo(bool bDepthTest, bool bDepthWrite, VkCompareOp compareOp);
    ~VulkanPipelineBuilder();
};


#endif //VULKAN_EXPERIMENTS_VULKANPIPELINEBUILDER_HPP
