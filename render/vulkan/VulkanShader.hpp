//
// Created by f0xeri on 02.01.2023.
//

#ifndef VULKAN_EXPERIMENTS_VULKANSHADER_HPP
#define VULKAN_EXPERIMENTS_VULKANSHADER_HPP


#include <vulkan/vulkan.h>
#include "core/Shader.hpp"

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
