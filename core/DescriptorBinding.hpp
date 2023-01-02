//
// Created by f0xeri on 03.01.2023.
//

#ifndef VULKAN_EXPERIMENTS_DESCRIPTORBINDING_HPP
#define VULKAN_EXPERIMENTS_DESCRIPTORBINDING_HPP

#include <cstdint>
#include <string>
#include <vector>

enum class UniformType : uint32_t
{
    SAMPLER = 0,
    COMBINED_IMAGE_SAMPLER,
    SAMPLED_IMAGE,
    STORAGE_IMAGE,
    UNIFORM_TEXEL_BUFFER,
    STORAGE_TEXEL_BUFFER,
    UNIFORM_BUFFER,
    STORAGE_BUFFER,
    UNIFORM_BUFFER_DYNAMIC,
    STORAGE_BUFFER_DYNAMIC,
    INPUT_ATTACHMENT,
    INLINE_UNIFORM_BLOCK_EXT,
    ACCELERATION_STRUCTURE_KHR,
};

class DescriptorBinding {
public:
    struct UniformElement {
        uint32_t binding;
        std::string name;
        UniformType type;
        uint32_t allocSize;
    };
    std::vector<UniformElement> uniforms;
    DescriptorBinding() = default;
    void addUniform(uint32_t binding, const std::string& name, UniformType type, uint32_t allocSize) {
        uniforms.push_back({binding, name, type, allocSize});
    }
};

#endif //VULKAN_EXPERIMENTS_DESCRIPTORBINDING_HPP
