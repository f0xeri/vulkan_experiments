//
// Created by f0xeri on 31.12.2022.
//

#ifndef VULKAN_EXPERIMENTS_VULKANBACKEND_HPP
#define VULKAN_EXPERIMENTS_VULKANBACKEND_HPP

#include <vulkan/vulkan.h>
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#include <ranges>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <memory>
#include <deque>
#include <functional>
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include <glm/ext.hpp>
#include <map>
#include "vk_mem_alloc.h"
#include "VulkanMesh.hpp"
#include "VulkanShader.hpp"
#include "core/Texture.hpp"

#define FRAME_OVERLAP 1

#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void push_function(std::function<void()>&& function) {
        deletors.push_back(function);
    }

    void flush() {
        // reverse iterate the deletion queue to execute all the functions
        for (auto & deletor : std::ranges::reverse_view(deletors)) {
            deletor(); //call the function
        }

        deletors.clear();
    }
};

struct AllocatedImage {
    VkImage image;
    VmaAllocation allocation;
};

struct FrameData {
    VkSemaphore renderSemaphore, presentSemaphore;
    VkFence renderFence;

    VkCommandPool commandPool;
    VkCommandBuffer mainCommandBuffer;

    std::map<std::string, VulkanBuffer> uniformBuffers;
    //VulkanBuffer uniformBuffer;
    VkDescriptorSet globalDescriptorSet;
};

struct UploadContext {
    VkFence uploadFence;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
};

struct VulkanTexture {
    AllocatedImage image;
    VkImageView imageView;
    Texture texture;
    uint32_t binding;
};

class VulkanBackend {
private:
    VkInstance instance;
    VkSurfaceKHR surface;

    VkExtent2D windowExtent;

    VkPhysicalDevice physicalDevice;
    VkDevice device;
    VkSwapchainKHR swapchain;

    VkFormat swapchainImageFormat;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;

    VkQueue graphicsQueue;
    uint32_t graphicsQueueFamilyIndex;

    VkRenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;

    FrameData frames[FRAME_OVERLAP];
    FrameData &getCurrentFrame() { return frames[frameNumber % FRAME_OVERLAP]; }

    VmaAllocator allocator;

    VulkanMesh triangleMesh;

    VkImageView depthImageView;
    AllocatedImage depthImage;
    VkFormat depthFormat;

    VkDescriptorSetLayout globalSetLayout;
    VkDescriptorPool descriptorPool;

    VkDescriptorSetLayout singleTextureSetLayout;

    std::unique_ptr<ShaderLoader> shaderLoader = nullptr;

    DeletionQueue deletionQueue;

    uint32_t currentImageIndex = 0;

    VkPhysicalDeviceProperties gpuProperties;

    UploadContext uploadContext;

    static VkDescriptorType getDescriptorTypeFromUniformType(UniformType type) {
        switch (type) {
            case UniformType::UNIFORM_BUFFER:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case UniformType::UNIFORM_BUFFER_DYNAMIC:
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            case UniformType::COMBINED_IMAGE_SAMPLER:
                return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case UniformType::STORAGE_BUFFER:
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            case UniformType::STORAGE_BUFFER_DYNAMIC:
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            case UniformType::STORAGE_IMAGE:
                return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case UniformType::INPUT_ATTACHMENT:
                return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            case UniformType::ACCELERATION_STRUCTURE_KHR:
                return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
            default:
                throw std::runtime_error("Unknown uniform type");
        }
    }

public:
    bool isInitialized = false;
    VkDeviceSize getBufferAlignedSize(VkDeviceSize size) const {
        VkDeviceSize minAlignment = gpuProperties.limits.minUniformBufferOffsetAlignment;
        size_t alignedSize = size;
        if (minAlignment > 0) {
            alignedSize = (alignedSize + minAlignment - 1) & ~(minAlignment - 1);
        }
        return alignedSize;
    };

    uint64_t frameNumber = 0;
    std::map<std::string, VulkanMesh> meshes;
    std::map<std::string, VulkanMaterial> materials;
    std::map<std::string, Shader> shaders;
    std::unordered_map<std::string, VulkanTexture> loadedTextures;
    VulkanMaterial currentPipeline;
    void addMesh(const std::string &name, const Mesh &mesh);
    ShaderLoader* getShaderLoader();
    void createShader(const Shader& info);
    void createDescriptors(const Shader &pipelineShader);
    void createGraphicsPipeline(const std::string &name, const Shader &pipelineShader);
    void pushConstants(const void *data, size_t size, ShaderStage stageFlags);
    void bindPipeline(const std::string &name);
    // bind descriptor sets, pass dynamic offsets
    void bindDescriptorSets(const std::vector<uint32_t> &dynamicOffsets);
    void bindDescriptorSets();
    VulkanBuffer createBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
    void setUniformBuffer(const std::string &name, const void *data, size_t size);
    void immediateSubmit(const std::function<void(VkCommandBuffer)>& function);
    void addTexture(const Texture &texture, uint32_t binding);

    VulkanBackend(const std::shared_ptr<GLFWwindow>& window, std::string_view appName, uint32_t width, uint32_t height);
    void init(uint32_t width, uint32_t height);
    void createSwapchain(uint32_t width, uint32_t height);
    void createPhysicalDevice();
    void createLogicalDevice();
    void createQueue();
    void createCommandPoolAndBuffer();
    void createDefaultRenderPass();
    void createFramebuffers();
    void createSemaphoresAndFences();
    void beginFrame();
    void drawMeshes();
    void drawMesh(const VulkanMesh &mesh);
    void drawMeshIndexed(const VulkanMesh &mesh);
    void endFrame();
    void drawFrame();

    void loadShaderModule(std::string_view path, VkShaderModule* outShaderModule);
    void createPipelines();
    void setWindowExtent(uint32_t width, uint32_t height) {
        windowExtent = { width, height };
    }
    void recreateSwapchain(uint32_t width, uint32_t height);
    ~VulkanBackend();

    void loadMeshes();
    void uploadMesh(VulkanMesh& mesh);

    VkImageCreateInfo createImageInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
    VkImageViewCreateInfo createImageViewInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
    VkCommandBufferBeginInfo createCommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0);
    VkSubmitInfo createSubmitInfo(VkCommandBuffer* cmd);
    VkSamplerCreateInfo createSamplerCreateInfo(VkFilter filters, VkSamplerAddressMode samplerAddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);
    VkWriteDescriptorSet createWriteDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo* imageInfo, uint32_t binding);
};

#endif //VULKAN_EXPERIMENTS_VULKANBACKEND_HPP
