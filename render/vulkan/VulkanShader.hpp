//
// Created by f0xeri on 02.01.2023.
//

#ifndef VULKAN_EXPERIMENTS_VULKANSHADER_HPP
#define VULKAN_EXPERIMENTS_VULKANSHADER_HPP


#include <vulkan/vulkan.h>
#include "core/Shader.hpp"

static VkShaderStageFlagBits convertShaderStageVulkan(ShaderStage stage)
{
    switch (stage)
    {
        case ShaderStage::VERTEX:
            return VK_SHADER_STAGE_VERTEX_BIT;
        case ShaderStage::TESSELLATION_CONTROL:
            return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        case ShaderStage::TESSELLATION_EVALUATION:
            return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        case ShaderStage::GEOMETRY:
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        case ShaderStage::FRAGMENT:
            return VK_SHADER_STAGE_FRAGMENT_BIT;
        case ShaderStage::COMPUTE:
            return VK_SHADER_STAGE_COMPUTE_BIT;
        default:
            return VK_SHADER_STAGE_VERTEX_BIT;
    }
}

class VulkanShader : public Shader {
    VkDevice device;
public:
    struct ShaderStage {
        VkShaderStageFlagBits stage = {};
        VkShaderModule module = VK_NULL_HANDLE;
    };

    std::vector<ShaderStage> stages;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

public:
    VulkanShader() = default;
    VulkanShader(VkDevice device, const Shader& info);
    ~VulkanShader();
};

class VulkanShaderLoader : public ShaderLoader
{
    VkDevice device;
public:
    explicit VulkanShaderLoader(VkDevice device) : device(device) {}
    ShaderStageInfo loadFromBinaryFile(const std::string& filepath, ShaderStage stage) override;
};

#endif //VULKAN_EXPERIMENTS_VULKANSHADER_HPP
