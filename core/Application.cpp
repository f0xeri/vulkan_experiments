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
    vulkanBackend = std::make_unique<VulkanBackend>();
    glfwSetWindowUserPointer(mainWindow.get(), this);
    glfwSetFramebufferSizeCallback(mainWindow.get(), [](GLFWwindow* window, int width, int height) {
        auto app = static_cast<Application *>(glfwGetWindowUserPointer(window));
        app->vulkanBackend->setWindowExtent(width, height);
        app->vulkanBackend->recreateSwapchain(width, height);
    });
    initScene();
    vulkanBackend->init(mainWindow, "Vulkan Experiments", width, height);
}

Application::~Application() {
    glfwTerminate();
    vulkanBackend.reset();
    mainWindow.reset();
}

void Application::initScene() {
    std::shared_ptr<Mesh> triangleMesh = std::make_shared<Mesh>();
    triangleMesh->vertices.resize(3);
    triangleMesh->vertices[0].position = { 1.0f, 1.0f, 0.0f };
    triangleMesh->vertices[1].position = { -1.0f, 1.0f, 0.0f };
    triangleMesh->vertices[2].position = { 0.0f, -1.0f, 0.0f };

    triangleMesh->vertices[0].color = { 1.0f, 0.0f, 0.0f };
    triangleMesh->vertices[1].color = { 0.0f, 1.0f, 0.0f };
    triangleMesh->vertices[2].color = { 0.0f, 0.0f, 1.0f };

    vulkanBackend->addMesh(triangleMesh);

    std::shared_ptr<Mesh> triangleMesh2 = std::make_shared<Mesh>();
    triangleMesh2->vertices.resize(3);
    triangleMesh2->vertices[0].position = { 1.0f, 1.0f, 10.0f };
    triangleMesh2->vertices[1].position = { -1.0f, 1.0f, 10.0f };
    triangleMesh2->vertices[2].position = { 0.0f, -1.0f, 10.0f };

    triangleMesh2->vertices[0].color = { 1.0f, 0.0f, 0.0f };
    triangleMesh2->vertices[1].color = { 1.0f, 0.0f, 0.0f };
    triangleMesh2->vertices[2].color = { 1.0f, 0.0f, 0.0f };

    vulkanBackend->addMesh(triangleMesh2);
}

void Application::run() {
    while (!glfwWindowShouldClose(mainWindow.get())) {
        glfwPollEvents();
        vulkanBackend->drawFrame();
    }
}
