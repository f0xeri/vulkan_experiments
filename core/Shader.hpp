//
// Created by f0xeri on 02.01.2023.
//

#ifndef VULKAN_EXPERIMENTS_SHADER_HPP
#define VULKAN_EXPERIMENTS_SHADER_HPP

#include <vector>
#include <string>
#include "DescriptorBinding.hpp"

enum class ShaderStage
{
    VERTEX,
    TESSELLATION_CONTROL,
    TESSELLATION_EVALUATION,
    GEOMETRY,
    FRAGMENT,
    COMPUTE,
};

struct ShaderStageInfo
{
    ShaderStage stage = ShaderStage::VERTEX;
    std::vector<uint32_t>bytecode;
};

struct Shader {
    std::string name;
    std::vector<ShaderStageInfo> stagesInfo;
    DescriptorBinding descriptorBinding;
};

class ShaderLoader
{
public:
    virtual ShaderStageInfo loadFromBinaryFile(const std::string& filepath, ShaderStage stage) = 0;
    virtual ~ShaderLoader() = default;
};

#endif //VULKAN_EXPERIMENTS_SHADER_HPP
