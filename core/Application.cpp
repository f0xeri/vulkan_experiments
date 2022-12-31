//
// Created by f0xeri on 30.12.2022.
//

#include "Application.hpp"

Application::Application(int width, int height, const char* title) {
    this->width = width;
    this->height = height;
    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }
    glfwVulkanSupported();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    mainWindow = std::shared_ptr<GLFWwindow>(glfwCreateWindow(width, height, title, nullptr, nullptr), [](GLFWwindow* window) {
        glfwDestroyWindow(window);
        glfwTerminate();
    });
    if (!mainWindow) {
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwMakeContextCurrent(mainWindow.get());

    vulkanBackend = std::make_unique<VulkanBackend>(mainWindow, "Vulkan Experiments", width, height);
}

Application::~Application() {
    glfwTerminate();
    vulkanBackend.reset();
    mainWindow.reset();
}

void Application::run() {
    while (!glfwWindowShouldClose(mainWindow.get())) {
        glfwPollEvents();
        vulkanBackend->drawFrame();
    }
}
