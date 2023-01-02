//
// Created by f0xeri on 31.12.2022.
//

#include "VulkanBackend.hpp"
#include "VulkanPipelineBuilder.hpp"

#include <cmath>
#include <fstream>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

VulkanBackend::VulkanBackend(const std::shared_ptr<GLFWwindow>& window, std::string_view appName, uint32_t width, uint32_t height) {
    windowExtent = {width, height};
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = appName.data();
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 2;
    const char* extensions[] = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    createInfo.ppEnabledExtensionNames = extensions;

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }

    // Create surface from window
    if (glfwCreateWindowSurface(instance, window.get(), nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }

    createPhysicalDevice();
    createLogicalDevice();

    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = device;
    allocatorInfo.instance = instance;
    vmaCreateAllocator(&allocatorInfo, &allocator);

    shaderLoader = std::unique_ptr<ShaderLoader>(new VulkanShaderLoader(device));
}

void VulkanBackend::init(uint32_t width, uint32_t height) {
    createSwapchain(width, height);
    createQueue();
    createCommandPoolAndBuffer();
    createDefaultRenderPass();
    createFramebuffers();
    createSemaphoresAndFences();
    createPipelines();
    loadMeshes();
}

VulkanBackend::~VulkanBackend() {
    vkDeviceWaitIdle(device);
    deletionQueue.flush();
    vmaDestroyAllocator(allocator);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
}


void VulkanBackend::createPhysicalDevice() {
    // Get physical device
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    physicalDevice = devices[0];

    // print device properties
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    std::cout << "Device name: " << deviceProperties.deviceName << std::endl;
}

void VulkanBackend::createLogicalDevice() {
    // Create logical device and swapchain
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = 0;
    queueCreateInfo.queueCount = 1;
    float queuePriority = 1.0f;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = 1;
    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;

    if (vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device");
    }
}

void VulkanBackend::createSwapchain(uint32_t width, uint32_t height) {
    swapchainImageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    windowExtent = {width, height};

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = 2;
    swapchainCreateInfo.imageFormat = swapchainImageFormat;
    swapchainCreateInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchainCreateInfo.imageExtent.width = width;
    swapchainCreateInfo.imageExtent.height = height;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swapchain");
    }

    // Get swapchain images
    uint32_t imageCount = 0;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

    // Create image views
    swapchainImageViews.resize(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &createInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views");
        }
    }
    deletionQueue.push_function([=, this]() {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
    });

    VkExtent3D depthImageExtent = {windowExtent.width, windowExtent.height, 1};
    depthFormat = VK_FORMAT_D32_SFLOAT;
    VkImageCreateInfo depthImageCreateInfo = createImageInfo(depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);
    VmaAllocationCreateInfo depthImageAllocCreateInfo = {};
    depthImageAllocCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    depthImageAllocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (vmaCreateImage(allocator, &depthImageCreateInfo, &depthImageAllocCreateInfo, &depthImage.image, &depthImage.allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create depth image");
    }
    VkImageViewCreateInfo depthImageViewCreateInfo = createImageViewInfo(depthFormat, depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);
    if (vkCreateImageView(device, &depthImageViewCreateInfo, nullptr, &depthImageView) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create depth image view");
    }

    deletionQueue.push_function([=, this]() {
        vkDestroyImageView(device, depthImageView, nullptr);
        vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
    });
}

void VulkanBackend::createQueue() {
    // create graphics queue and save family index
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
    for (uint32_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamilyIndex = i;
            break;
        }
    }
    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
}

void VulkanBackend::createCommandPoolAndBuffer() {
    // create command pool
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.pNext = nullptr;
    poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }

    // create command buffer
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    poolInfo.pNext = nullptr;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &mainCommandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer");
    }
    deletionQueue.push_function([=, this]() {
        vkDestroyCommandPool(device, commandPool, nullptr);
    });
}

void VulkanBackend::createDefaultRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment = {};
    depthAttachment.flags = 0;
    depthAttachment.format = depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef = {};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = &attachments[0];
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency depthDependency = {};
    depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    depthDependency.dstSubpass = 0;
    depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.srcAccessMask = 0;
    depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkSubpassDependency dependencies[2] = { dependency, depthDependency };
    renderPassInfo.dependencyCount = 2;
    renderPassInfo.pDependencies = &dependencies[0];

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }

    deletionQueue.push_function([=, this]() {
        vkDestroyRenderPass(device, renderPass, nullptr);
    });
}

void VulkanBackend::createFramebuffers() {
    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext = nullptr;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.width = windowExtent.width;
    framebufferInfo.height = windowExtent.height;
    framebufferInfo.layers = 1;

    framebuffers.resize(swapchainImageViews.size());

    for (size_t i = 0; i < swapchainImageViews.size(); i++) {
        VkImageView attachments[2];
        attachments[0] = swapchainImageViews[i];
        attachments[1] = depthImageView;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.attachmentCount = 2;
        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer");
        }
        deletionQueue.push_function([=, this]() {
            vkDestroyFramebuffer(device, framebuffers[i], nullptr);
            vkDestroyImageView(device, swapchainImageViews[i], nullptr);
        });
    }
}

void VulkanBackend::createSemaphoresAndFences() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.pNext = nullptr;
    semaphoreInfo.flags = 0;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &presentSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create semaphores");
    }

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = nullptr;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if (vkCreateFence(device, &fenceInfo, nullptr, &renderFence) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create fence");
    }

    deletionQueue.push_function([=, this]() {
        vkDestroyFence(device, renderFence, nullptr);
        vkDestroySemaphore(device, presentSemaphore, nullptr);
        vkDestroySemaphore(device, renderSemaphore, nullptr);
    });
}

void VulkanBackend::drawFrame() {
    glm::vec3 camPos = { 0.f,0.f,-2.f };
    glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
    glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
    glm::mat4 model = glm::rotate(glm::mat4{ 1.0f }, glm::radians(frameNumber * 0.4f), glm::vec3(0, 1, 0));
    glm::mat4 meshMatrix = projection * view * model;

    vkWaitForFences(device, 1, &renderFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &renderFence);

    uint32_t imageIndex;
    auto result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, presentSemaphore, VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain(windowExtent.width, windowExtent.height);
        return;
    }
    vkResetCommandBuffer(mainCommandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer(mainCommandBuffer, &beginInfo);
    VkClearValue clearValue;
    float flash = std::abs(std::sin(frameNumber / 120.f));
    clearValue.color = { { 0.0f, flash, 0.0f, 1.0f } };

    VkClearValue depthClear;
    depthClear.depthStencil.depth = 1.f;

    VkClearValue clearValues[] = { clearValue, depthClear };

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.pNext = nullptr;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = windowExtent;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = &clearValues[0];

    vkCmdBeginRenderPass(mainCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);
    vkCmdPushConstants(mainCommandBuffer, trianglePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &meshMatrix);
    VkDeviceSize offset = 0;
    for (auto& mesh : meshes) {
        vkCmdPushConstants(mainCommandBuffer, trianglePipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &meshMatrix);
        vkCmdBindVertexBuffers(mainCommandBuffer, 0, 1, &mesh.vertexBuffer.buffer, &offset);
        vkCmdDraw(mainCommandBuffer, mesh.vertices.size(), 1, 0, 0);
    }
    //vkCmdDraw(mainCommandBuffer, triangleMesh.vertices.size(), 1, 0, 0);
    vkCmdEndRenderPass(mainCommandBuffer);
    VK_CHECK(vkEndCommandBuffer(mainCommandBuffer));

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &presentSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &mainCommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderSemaphore;

    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, renderFence));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

    frameNumber++;
}

void VulkanBackend::loadShaderModule(std::string_view path, VkShaderModule* outShaderModule) {
    std::ifstream file(path.data(), std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
    file.close();

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();
    VkShaderModule shaderModule;

    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        std::cout << "Failed to create shader module " << path << std::endl;
    }
    else {
        std::cout << "Created shader module " << path << std::endl;
    }
    *outShaderModule = shaderModule;
}

void VulkanBackend::createPipelines() {
    auto vulkanShader = VulkanShader(device, shader);
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = VulkanPipelineBuilder::createPipelineLayoutInfo();
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &trianglePipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout");
    }

    VertexInputDescription vertexDescription = VulkanVertex::getVertexDescription();

    VulkanPipelineBuilder pipelineBuilder;

    for (auto &stage : vulkanShader.stages) {
        pipelineBuilder.shaderStages.push_back(VulkanPipelineBuilder::createShaderStageInfo(stage.stage, stage.module));
    }

    pipelineBuilder.vertexInputInfo = VulkanPipelineBuilder::createVertexInputInfo(std::shared_ptr<std::vector<VkVertexInputBindingDescription>>(&vertexDescription.bindings, [](std::vector<VkVertexInputBindingDescription>*) {}),
                                                                                   std::shared_ptr<std::vector<VkVertexInputAttributeDescription>>(&vertexDescription.attributes, [](std::vector<VkVertexInputAttributeDescription>*) {}));
    pipelineBuilder.inputAssembly = VulkanPipelineBuilder::createInputAssemblyInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineBuilder.rasterizer = VulkanPipelineBuilder::createRasterizerInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
    pipelineBuilder.multisampling = VulkanPipelineBuilder::createMultisamplingInfo(VK_SAMPLE_COUNT_1_BIT);
    pipelineBuilder.colorBlendAttachment = VulkanPipelineBuilder::createColorBlendAttachmentState();
    pipelineBuilder.depthStencil = VulkanPipelineBuilder::createDepthStencilInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

    pipelineBuilder.viewport = {};
    pipelineBuilder.viewport.x = 0.0f;
    pipelineBuilder.viewport.y = 0.0f;
    pipelineBuilder.viewport.width = (float)windowExtent.width;
    pipelineBuilder.viewport.height = (float)windowExtent.height;
    pipelineBuilder.viewport.minDepth = 0.0f;
    pipelineBuilder.viewport.maxDepth = 1.0f;

    pipelineBuilder.scissor = {};
    pipelineBuilder.scissor.offset = { 0, 0 };
    pipelineBuilder.scissor.extent = windowExtent;
    pipelineBuilder.pipelineLayout = trianglePipelineLayout;

    trianglePipeline = pipelineBuilder.buildPipeline(device, renderPass);

    deletionQueue.push_function([=, this]() {
        //vkDestroyShaderModule(device, triangleVertShader, nullptr);
        //vkDestroyShaderModule(device, triangleFragShader, nullptr);
        for (auto &stage : vulkanShader.stages) {
            vkDestroyShaderModule(device, stage.module, nullptr);
        }
        vkDestroyPipeline(device, trianglePipeline, nullptr);
        vkDestroyPipelineLayout(device, trianglePipelineLayout, nullptr);
    });
}

void VulkanBackend::recreateSwapchain(uint32_t width, uint32_t height) {
    vkDeviceWaitIdle(device);
    deletionQueue.flush();

    createSwapchain(width, height);
    createCommandPoolAndBuffer();
    createDefaultRenderPass();
    createFramebuffers();
    createSemaphoresAndFences();
    createPipelines();
    loadMeshes();
}

void VulkanBackend::loadMeshes() {
    for (auto &mesh : meshes) {
        uploadMesh(mesh);
    }
}

void VulkanBackend::uploadMesh(VulkanMesh& mesh) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = sizeof(mesh.vertices[0]) * mesh.vertices.size();
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

    vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &mesh.vertexBuffer.buffer, &mesh.vertexBuffer.allocation, nullptr);

    deletionQueue.push_function([=, this]() {
        vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
    });

    void *data;
    vmaMapMemory(allocator, mesh.vertexBuffer.allocation, &data);
    memcpy(data, mesh.vertices.data(), (size_t)bufferInfo.size);
    vmaUnmapMemory(allocator, mesh.vertexBuffer.allocation);
}

void VulkanBackend::addMesh(const Mesh &mesh) {
    VulkanMesh vulkanMesh = {};
    vulkanMesh.vertices = mesh.vertices;
    vulkanMesh.vertexBuffer = {};
    meshes.push_back(vulkanMesh);
    //uploadMesh(*vulkanMesh);
}

VkImageCreateInfo VulkanBackend::createImageInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent = extent;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usageFlags;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    return imageInfo;
}

VkImageViewCreateInfo VulkanBackend::createImageViewInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    return viewInfo;
}

ShaderLoader *VulkanBackend::getShaderLoader() {
    return shaderLoader.get();
}

void VulkanBackend::createShader(const Shader &info) {
    shader = info;
    //auto result = std::unique_ptr<Shader>(&vulkanShader);
}
