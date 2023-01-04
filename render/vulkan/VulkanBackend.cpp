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
    //createPipelines();
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
    gpuProperties = deviceProperties;
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

    for (auto & frame : frames) {
        if (vkCreateCommandPool(device, &poolInfo, nullptr, &frame.commandPool) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool");
        }
        // create command buffer
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        poolInfo.pNext = nullptr;
        allocInfo.commandPool = frame.commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(device, &allocInfo, &frame.mainCommandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffer");
        }
        deletionQueue.push_function([=, this]() {
            vkDestroyCommandPool(device, frame.commandPool, nullptr);
        });
    }

    VkCommandPoolCreateInfo uploadCommandPoolInfo = poolInfo;
    vkCreateCommandPool(device, &uploadCommandPoolInfo, nullptr, &uploadContext.commandPool);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    poolInfo.pNext = nullptr;
    allocInfo.commandPool = uploadContext.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &uploadContext.commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer");
    }
    deletionQueue.push_function([=, this]() {
        vkDestroyCommandPool(device, uploadContext.commandPool, nullptr);
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

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = nullptr;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateFence(device, &fenceInfo, nullptr, &uploadContext.uploadFence);
    deletionQueue.push_function([=, this]() {
        vkDestroyFence(device, uploadContext.uploadFence, nullptr);
    });

    for (auto & frame : frames) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.presentSemaphore) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &frame.renderSemaphore) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semaphores");
        }

        if (vkCreateFence(device, &fenceInfo, nullptr, &frame.renderFence) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create fence");
        }

        deletionQueue.push_function([=, this]() {
            vkDestroyFence(device, frame.renderFence, nullptr);
            vkDestroySemaphore(device, frame.presentSemaphore, nullptr);
            vkDestroySemaphore(device, frame.renderSemaphore, nullptr);
        });
    }
}

void VulkanBackend::recreateSwapchain(uint32_t width, uint32_t height) {
    vkDeviceWaitIdle(device);
    deletionQueue.flush();

    createSwapchain(width, height);
    createCommandPoolAndBuffer();
    createDefaultRenderPass();
    createFramebuffers();
    createSemaphoresAndFences();
    //createPipelines();
    for (auto &texture : loadedTextures) {
        addTexture(texture.second.texture, texture.second.binding);
    }
    for (auto &pipeline : materials) {
        // TODO: fix it
        createDescriptors(pipeline.second.shader);
        createGraphicsPipeline(pipeline.first, pipeline.second.shader);
    }
    loadMeshes();
}

void VulkanBackend::loadMeshes() {
    for (auto &mesh : meshes) {
        uploadMesh(mesh.second);
    }
}

void VulkanBackend::uploadMesh(VulkanMesh& mesh) {
    /*mesh.vertexBuffer.uploadBuffer(allocator, mesh.vertices.data(), sizeof(mesh.vertices[0]) * mesh.vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
    deletionQueue.push_function([=, this]() {
        vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
    });*/
    const size_t bufferSize = mesh.vertices.size() * sizeof(Vertex);
    VkBufferCreateInfo stagingBufferInfo = {};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.pNext = nullptr;
    stagingBufferInfo.size = bufferSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;
    VulkanBuffer stagingBuffer;
    VK_CHECK(vmaCreateBuffer(allocator, &stagingBufferInfo, &vmaallocInfo, &stagingBuffer.buffer, &stagingBuffer.allocation, nullptr));

    void* data;
    vmaMapMemory(allocator, stagingBuffer.allocation, &data);
    memcpy(data, mesh.vertices.data(), bufferSize);
    vmaUnmapMemory(allocator, stagingBuffer.allocation);

    VkBufferCreateInfo vertexBufferInfo = {};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.pNext = nullptr;

    vertexBufferInfo.size = bufferSize;
    vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vmaallocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    VK_CHECK(vmaCreateBuffer(allocator, &vertexBufferInfo, &vmaallocInfo, &mesh.vertexBuffer.buffer, &mesh.vertexBuffer.allocation, nullptr));

    immediateSubmit([&](VkCommandBuffer commandBuffer) {
        VkBufferCopy copyRegion = {};
        copyRegion.size = bufferSize;
        copyRegion.dstOffset = 0;
        copyRegion.srcOffset = 0;
        vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer, mesh.vertexBuffer.buffer, 1, &copyRegion);
    });

    deletionQueue.push_function([=, this]() {
        vmaDestroyBuffer(allocator, mesh.vertexBuffer.buffer, mesh.vertexBuffer.allocation);
    });
    vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
}

void VulkanBackend::addMesh(const std::string &name, const Mesh &mesh) {
    VulkanMesh vulkanMesh = {};
    vulkanMesh.vertices = mesh.vertices;
    vulkanMesh.vertexBuffer = {};
    meshes[name] = vulkanMesh;
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
    //shader = info;
    shaders[info.name] = info;
    //auto result = std::unique_ptr<Shader>(&vulkanShader);
}

void VulkanBackend::createGraphicsPipeline(const std::string &name, const Shader &pipelineShader) {
    auto vulkanShader = VulkanShader(device, pipelineShader);
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = VulkanPipelineBuilder::createPipelineLayoutInfo();
    VkPushConstantRange pushConstantRange = {};
    std::vector<VkPushConstantRange> pushConstantRanges;
    for (auto &pushConstant : pipelineShader.constants) {
        pushConstantRange.stageFlags = convertShaderStagesArrayVulkan(pushConstant.stages);
        pushConstantRange.offset = pushConstant.offset;
        pushConstantRange.size = pushConstant.size;
        pushConstantRanges.push_back(pushConstantRange);
    }
    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges.size());
    pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();

    VkDescriptorSetLayout setLayouts[] = { globalSetLayout, singleTextureSetLayout };

    pipelineLayoutInfo.setLayoutCount = 2;
    pipelineLayoutInfo.pSetLayouts = setLayouts;

    VkPipelineLayout pipelineLayout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout " + name);
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
    pipelineBuilder.pipelineLayout = pipelineLayout;

    auto pipeline = pipelineBuilder.buildPipeline(device, renderPass);

    deletionQueue.push_function([=, this]() {
        for (auto &texture : loadedTextures) {
            vmaDestroyImage(allocator, texture.second.image.image, texture.second.image.allocation);
            vkDestroyImageView(device, texture.second.imageView, nullptr);
            std::cout << "Destroyed texture " << texture.first << std::endl;
        }
        for (auto &stage : vulkanShader.stages) {
            vkDestroyShaderModule(device, stage.module, nullptr);
        }
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    });
    materials[name] = VulkanMaterial{vulkanShader, pipeline, pipelineLayout};
    /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.pNext = nullptr;
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &singleTextureSetLayout;
    vkAllocateDescriptorSets(device, &allocInfo, &materials[name].textureSet);

    std::vector<VkWriteDescriptorSet> descriptorWrites;
    for (auto &texture : loadedTextures) {
        VkSamplerCreateInfo samplerInfo;
        samplerInfo = createSamplerCreateInfo(VK_FILTER_NEAREST);
        VkSampler blockySampler;
        vkCreateSampler(device, &samplerInfo, nullptr, &blockySampler);
        //write to the descriptor set so that it points to our texture
        VkDescriptorImageInfo *imageBufferInfo = new VkDescriptorImageInfo();
        imageBufferInfo->sampler = blockySampler;
        imageBufferInfo->imageView = texture.second.imageView;
        imageBufferInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        descriptorWrites.push_back(createWriteDescriptorImage(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, materials[name].textureSet, imageBufferInfo, texture.second.binding));
        deletionQueue.push_function([=, this]() {
            vkDestroySampler(device, blockySampler, nullptr);
        });
    }
    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    // delete imageBufferInfo pointers
    for (auto &write : descriptorWrites) {
        delete write.pImageInfo;
    }
}

void VulkanBackend::bindPipeline(const std::string &name) {
    currentPipeline = materials[name];
    vkCmdBindPipeline(getCurrentFrame().mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline.pipeline);
    //vkCmdBindDescriptorSets(getCurrentFrame().mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline.pipelineLayout, 0, 1, &getCurrentFrame().globalDescriptorSet, 0, nullptr);
}

void VulkanBackend::beginFrame() {
    vkWaitForFences(device, 1, &getCurrentFrame().renderFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &getCurrentFrame().renderFence);

    auto result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, getCurrentFrame().presentSemaphore, VK_NULL_HANDLE, &currentImageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain(windowExtent.width, windowExtent.height);
        return;
    }
    vkResetCommandBuffer(getCurrentFrame().mainCommandBuffer, 0);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer(getCurrentFrame().mainCommandBuffer, &beginInfo);
    VkClearValue clearValue;
    clearValue.color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    VkClearValue depthClear;
    depthClear.depthStencil.depth = 1.f;

    VkClearValue clearValues[] = { clearValue, depthClear };
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.pNext = nullptr;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[currentImageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = windowExtent;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = &clearValues[0];

    vkCmdBeginRenderPass(getCurrentFrame().mainCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanBackend::drawMeshes() {
    //vkCmdBindPipeline(mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline.pipeline);
    //vkCmdPushConstants(mainCommandBuffer, currentPipeline.pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &meshMatrix);
    VkDeviceSize offset = 0;
    for (auto& mesh : meshes) {
        vkCmdBindVertexBuffers(getCurrentFrame().mainCommandBuffer, 0, 1, &mesh.second.vertexBuffer.buffer, &offset);
        vkCmdDraw(getCurrentFrame().mainCommandBuffer, mesh.second.vertices.size(), 1, 0, 0);
    }
}

void VulkanBackend::drawMesh(const VulkanMesh &mesh) {
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(getCurrentFrame().mainCommandBuffer, 0, 1, &mesh.vertexBuffer.buffer, &offset);
    vkCmdDraw(getCurrentFrame().mainCommandBuffer, mesh.vertices.size(), 1, 0, 0);
}

void VulkanBackend::endFrame() {
    vkCmdEndRenderPass(getCurrentFrame().mainCommandBuffer);
    VK_CHECK(vkEndCommandBuffer(getCurrentFrame().mainCommandBuffer));

    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &getCurrentFrame().presentSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &getCurrentFrame().mainCommandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &getCurrentFrame().renderSemaphore;

    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submitInfo, getCurrentFrame().renderFence));

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &getCurrentFrame().renderSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &currentImageIndex;
    presentInfo.pResults = nullptr;

    VK_CHECK(vkQueuePresentKHR(graphicsQueue, &presentInfo));

    frameNumber++;
}

void VulkanBackend::pushConstants(const void *data, size_t size, ShaderStage stageFlags) {
    vkCmdPushConstants(getCurrentFrame().mainCommandBuffer, currentPipeline.pipelineLayout, convertShaderStageVulkan(stageFlags), 0, size, data);
}

VulkanBuffer VulkanBackend::createBuffer(size_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.size = size;
    bufferInfo.usage = usage;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;

    VulkanBuffer buffer;
    VK_CHECK(vmaCreateBuffer(allocator, &bufferInfo, &allocInfo, &buffer.buffer, &buffer.allocation, nullptr));
    return buffer;
}

void VulkanBackend::createDescriptors(const Shader &pipelineShader) {
    std::vector<VkDescriptorPoolSize> sizes ={{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
                                              { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10 },
                                              { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 }};

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = 0;
    pool_info.maxSets = 10;
    pool_info.poolSizeCount = (uint32_t)sizes.size();
    pool_info.pPoolSizes = sizes.data();

    vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);

    std::vector<VkDescriptorSetLayoutBinding> bindings;

    for (auto& uniform : pipelineShader.descriptorBinding.uniforms) {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = uniform.binding;
        binding.descriptorType = getDescriptorTypeFromUniformType(uniform.type);
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        binding.pImmutableSamplers = nullptr;
        bindings.push_back(binding);
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();
    layoutInfo.flags = 0;
    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &globalSetLayout);

    for (auto & frame : frames) {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &globalSetLayout;

        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &frame.globalDescriptorSet));

        std::vector<VkWriteDescriptorSet> writes;
        for (auto& uniform : pipelineShader.descriptorBinding.uniforms) {
            uint32_t sz = uniform.allocSize;
            if (getDescriptorTypeFromUniformType(uniform.type) == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
                sz = getBufferAlignedSize(sz) * meshes.size();
            }
            frame.uniformBuffers[uniform.name] = createBuffer(sz, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = frame.uniformBuffers[uniform.name].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = uniform.allocSize;

            VkWriteDescriptorSet descriptorWrite{};
            descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrite.dstSet = frame.globalDescriptorSet;
            descriptorWrite.dstBinding = uniform.binding;
            descriptorWrite.descriptorType = getDescriptorTypeFromUniformType(uniform.type);
            descriptorWrite.descriptorCount = 1;
            // TODO: mem leak
            auto buffInf = new VkDescriptorBufferInfo{frame.uniformBuffers[uniform.name].buffer, 0, uniform.allocSize};
            descriptorWrite.pBufferInfo = buffInf;
            descriptorWrite.pImageInfo = nullptr;
            descriptorWrite.pTexelBufferView = nullptr;
            writes.push_back(descriptorWrite);
        }
        vkUpdateDescriptorSets(device, writes.size(), writes.data(), 0, nullptr);

    }
    deletionQueue.push_function([&]() {
        vkDestroyDescriptorSetLayout(device, globalSetLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, singleTextureSetLayout, nullptr);
        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        for (auto & frame : frames) {
            for (auto& uniform : pipelineShader.descriptorBinding.uniforms) {
                vmaDestroyBuffer(allocator, frame.uniformBuffers[uniform.name].buffer, frame.uniformBuffers[uniform.name].allocation);
            }
        }
    });

    std::vector<VkDescriptorSetLayoutBinding> textureBindings;
    for (uint32_t i = 0; i < loadedTextures.size(); i++) {
        auto textureBind = new VkDescriptorSetLayoutBinding();
        textureBind->binding = i;
        textureBind->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textureBind->descriptorCount = 1;
        textureBind->stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        textureBind->pImmutableSamplers = nullptr;
        textureBindings.push_back(*textureBind);
    }

    VkDescriptorSetLayoutCreateInfo textureLayoutInfo{};
    textureLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutInfo.pNext = nullptr;
    textureLayoutInfo.bindingCount = textureBindings.size();
    textureLayoutInfo.pBindings = textureBindings.data();
    textureLayoutInfo.flags = 0;
    vkCreateDescriptorSetLayout(device, &textureLayoutInfo, nullptr, &singleTextureSetLayout);
}

void VulkanBackend::setUniformBuffer(const std::string &name, const void *data, size_t size) {
    // TODO: it will not work for dynamic uniform buffers
    auto &frame = getCurrentFrame();
    void *mapped;
    vmaMapMemory(allocator, frame.uniformBuffers[name].allocation, &mapped);
    memcpy(mapped, data, size);
    vmaUnmapMemory(allocator, frame.uniformBuffers[name].allocation);
}

void VulkanBackend::bindDescriptorSets(const std::vector<uint32_t> &dynamicOffsets) {
    auto &frame = getCurrentFrame();
    vkCmdBindDescriptorSets(frame.mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline.pipelineLayout, 0, 1, &frame.globalDescriptorSet, dynamicOffsets.size(), dynamicOffsets.data());
}

void VulkanBackend::bindDescriptorSets() {
    auto &frame = getCurrentFrame();
    vkCmdBindDescriptorSets(frame.mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline.pipelineLayout, 0, 1, &frame.globalDescriptorSet, 0, nullptr);
    // TODO: fix hardcoded value
    if (materials["default"].textureSet != VK_NULL_HANDLE)
        vkCmdBindDescriptorSets(frame.mainCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, currentPipeline.pipelineLayout, 1, 1, &materials["default"].textureSet, 0, nullptr);
}

void VulkanBackend::immediateSubmit(const std::function<void(VkCommandBuffer)> &function) {
    VkCommandBuffer cmd = uploadContext.commandBuffer;
    VkCommandBufferBeginInfo cmdBeginInfo = createCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    vkResetFences(device, 1, &uploadContext.uploadFence);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));
    function(cmd);
    VK_CHECK(vkEndCommandBuffer(cmd));
    VkSubmitInfo submit = createSubmitInfo(&cmd);
    VK_CHECK(vkQueueSubmit(graphicsQueue, 1, &submit, uploadContext.uploadFence));
    vkWaitForFences(device, 1, &uploadContext.uploadFence, true, UINT64_MAX);
    vkResetFences(device, 1, &uploadContext.uploadFence);
    vkResetCommandPool(device, uploadContext.commandPool, 0);
}

VkCommandBufferBeginInfo VulkanBackend::createCommandBufferBeginInfo(VkCommandBufferUsageFlags flags) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = flags;
    return beginInfo;
}

VkSubmitInfo VulkanBackend::createSubmitInfo(VkCommandBuffer *cmd) {
    VkSubmitInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    info.pNext = nullptr;
    info.waitSemaphoreCount = 0;
    info.pWaitSemaphores = nullptr;
    info.pWaitDstStageMask = nullptr;
    info.commandBufferCount = 1;
    info.pCommandBuffers = cmd;
    info.signalSemaphoreCount = 0;
    info.pSignalSemaphores = nullptr;
    return info;
}

// TODO: mem leak somewhere here
void VulkanBackend::addTexture(const Texture &texture, uint32_t binding) {
    void *pixels = texture.data;
    VkDeviceSize imageSize = texture.width * texture.height * 4;
    auto stagingBuffer = createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
    void *data;
    vmaMapMemory(allocator, stagingBuffer.allocation, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(allocator, stagingBuffer.allocation);

    VkExtent3D imageExtent;
    imageExtent.width = static_cast<uint32_t>(texture.width);
    imageExtent.height = static_cast<uint32_t>(texture.height);
    imageExtent.depth = 1;

    VkImageCreateInfo dimageInfo = createImageInfo(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, imageExtent);

    AllocatedImage *newImage = new AllocatedImage();

    VmaAllocationCreateInfo dimgAllocInfo = {};
    dimgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    vmaCreateImage(allocator, &dimageInfo, &dimgAllocInfo, &newImage->image, &newImage->allocation, nullptr);
    //delete texture.data;
    immediateSubmit([&](VkCommandBuffer cmd) {
        VkImageSubresourceRange range;
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.baseMipLevel = 0;
        range.levelCount = 1;
        range.baseArrayLayer = 0;
        range.layerCount = 1;

        VkImageMemoryBarrier imageBarrier_toTransfer = {};
        imageBarrier_toTransfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;

        imageBarrier_toTransfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageBarrier_toTransfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toTransfer.image = newImage->image;
        imageBarrier_toTransfer.subresourceRange = range;

        imageBarrier_toTransfer.srcAccessMask = 0;
        imageBarrier_toTransfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toTransfer);

        VkBufferImageCopy copyRegion = {};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = 1;
        copyRegion.imageExtent = imageExtent;

        //copy the buffer into the image
        vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, newImage->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        VkImageMemoryBarrier imageBarrier_toReadable = imageBarrier_toTransfer;

        imageBarrier_toReadable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageBarrier_toReadable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        imageBarrier_toReadable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageBarrier_toReadable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        //barrier the image into the shader readable layout
        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageBarrier_toReadable);
    });

    vmaDestroyBuffer(allocator, stagingBuffer.buffer, stagingBuffer.allocation);
    VulkanTexture resTexture{};
    resTexture.texture = texture;
    resTexture.image = *newImage;
    // TODO: do we need it?
    delete newImage;
    resTexture.binding = binding;
    VkImageViewCreateInfo imageInfo = createImageViewInfo(VK_FORMAT_R8G8B8A8_SRGB, resTexture.image.image, VK_IMAGE_ASPECT_COLOR_BIT);
    vkCreateImageView(device, &imageInfo, nullptr, &resTexture.imageView);
    loadedTextures[texture.name] = resTexture;
}

VkSamplerCreateInfo VulkanBackend::createSamplerCreateInfo(VkFilter filters, VkSamplerAddressMode samplerAddressMode) {
    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.pNext = nullptr;
    info.magFilter = filters;
    info.minFilter = filters;
    info.addressModeU = samplerAddressMode;
    info.addressModeV = samplerAddressMode;
    info.addressModeW = samplerAddressMode;
    return info;
}

VkWriteDescriptorSet VulkanBackend::createWriteDescriptorImage(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo *imageInfo, uint32_t binding) {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext = nullptr;
    write.dstBinding = binding;
    write.dstSet = dstSet;
    write.descriptorCount = 1;
    write.descriptorType = type;
    write.pImageInfo = imageInfo;
    return write;
}
