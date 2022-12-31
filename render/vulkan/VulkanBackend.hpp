//
// Created by f0xeri on 31.12.2022.
//

#ifndef VULKAN_EXPERIMENTS_VULKANBACKEND_HPP
#define VULKAN_EXPERIMENTS_VULKANBACKEND_HPP

#include <vulkan/vulkan.h>
#include <windows.h>
#include <vulkan/vulkan_win32.h>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <memory>
#include "GLFW/glfw3.h"

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

class VulkanBackend {
private:
    uint64_t frameNumber = 0;

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

    VkCommandPool commandPool;
    VkCommandBuffer mainCommandBuffer;

    VkRenderPass renderPass;
    std::vector<VkFramebuffer> framebuffers;

    VkSemaphore renderSemaphore, presentSemaphore;
    VkFence renderFence;

    VkPipeline trianglePipeline;
    VkPipelineLayout trianglePipelineLayout;
public:
    VulkanBackend(const std::shared_ptr<GLFWwindow>& window, std::string_view appName, uint32_t width, uint32_t height);
    void createSwapchain(const std::shared_ptr<GLFWwindow>& window, uint32_t width, uint32_t height);
    void createPhysicalDevice();
    void createLogicalDevice();
    void createQueue();
    void createCommandPoolAndBuffer();
    void createDefaultRenderPass();
    void createFramebuffers();
    void createSemaphoresAndFences();
    void drawFrame();

    void loadShaderModule(std::string_view path, VkShaderModule* outShaderModule);
    void createPipelines();
    ~VulkanBackend();
};


#endif //VULKAN_EXPERIMENTS_VULKANBACKEND_HPP
