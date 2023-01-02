//
// Created by f0xeri on 30.12.2022.
//

#ifndef VULKAN_EXPERIMENTS_APPLICATION_HPP
#define VULKAN_EXPERIMENTS_APPLICATION_HPP

#include <memory>
#include <GLFW/glfw3.h>
#include "render/vulkan/VulkanBackend.hpp"

class Application {
private:
    std::shared_ptr<GLFWwindow> mainWindow;
    std::unique_ptr<VulkanBackend> vulkanBackend;
public:
    int width;
    int height;
    Application(int width, int height, const char* title);
    ~Application();
    void initScene();
    void initPipelines();
    void run();
};


#endif //VULKAN_EXPERIMENTS_APPLICATION_HPP
