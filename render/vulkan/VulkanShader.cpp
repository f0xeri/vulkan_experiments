//
// Created by f0xeri on 02.01.2023.
//

#include <stdexcept>
#include <iostream>
#include <fstream>
#include "VulkanShader.hpp"

VkShaderStageFlagBits convertShaderStageVulkan(ShaderStage stage)
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

VulkanShader::VulkanShader(VkDevice device, const Shader &info) {
    this->device = device;
    name = info.name;
    stages.reserve(info.stagesInfo.size());
    for (const auto& stageInfo : info.stagesInfo) {
        ShaderStage stage;
        stage.stage = convertShaderStageVulkan(stageInfo.stage);
        VkShaderModuleCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.codeSize = stageInfo.bytecode.size() * sizeof(uint32_t);
        createInfo.pCode = reinterpret_cast<const uint32_t*>(stageInfo.bytecode.data());
        if (vkCreateShaderModule(device, &createInfo, nullptr, &stage.module) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module " + name);
        }
        else {
            std::cout << "Created shader module " << name << std::endl;
        }
        stages.push_back(stage);
    }
}

VulkanShader::~VulkanShader() {

}

ShaderStageInfo VulkanShaderLoader::loadFromBinaryFile(const std::string &filepath, ShaderStage stage) {
    std::ifstream file(filepath.data(), std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
    file.close();

    ShaderStageInfo result;
    result.stage = stage;
    result.bytecode = buffer;
    return result;
}
