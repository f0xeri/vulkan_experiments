//
// Created by f0xeri on 30.12.2022.
//
#define TINYOBJLOADER_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "Application.hpp"
#include "Shader.hpp"

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
    glfwSetWindowUserPointer(mainWindow.get(), this);
    glfwSetFramebufferSizeCallback(mainWindow.get(), [](GLFWwindow* window, int width, int height) {
        auto app = static_cast<Application *>(glfwGetWindowUserPointer(window));
        app->vulkanBackend->setWindowExtent(width, height);
        app->vulkanBackend->recreateSwapchain(width, height);
    });
    initScene();
    vulkanBackend->init(width, height);
    initPipelines();
}

Application::~Application() {
    glfwTerminate();
    vulkanBackend.reset();
    mainWindow.reset();
}

void Application::initScene() {
    Shader shader = {};
    shader.name = "default";
    shader.stagesInfo.push_back(vulkanBackend->getShaderLoader()->loadFromBinaryFile("assets/shaders/triangle.vert.spv", ShaderStage::VERTEX));
    shader.stagesInfo.push_back(vulkanBackend->getShaderLoader()->loadFromBinaryFile("assets/shaders/triangle.frag.spv", ShaderStage::FRAGMENT));
    shader.descriptorBinding = DescriptorBinding();
    shader.descriptorBinding.addUniform(0, "cameraBuffer", UniformType::UNIFORM_BUFFER, sizeof(CameraData));
    shader.constants.push_back({"modelBuffer", sizeof(glm::mat4), 0, {ShaderStage::VERTEX}});
    vulkanBackend->createShader(shader);

    Mesh cvpiMesh;
    cvpiMesh.loadFromObj("assets/cvpi.obj");
    vulkanBackend->addMesh("cvpi", cvpiMesh);
    vulkanBackend->addMesh("cvpi2", cvpiMesh);

    vulkanBackend->meshes["cvpi2"].model = glm::translate(glm::mat4(1.0f), glm::vec3(-60.0f, 0.0f, -20.0f));
    vulkanBackend->meshes["cvpi"].model = glm::translate(glm::mat4(1.0f), glm::vec3(60.0f, 0.0f, -20.0f));
}

void Application::run() {
    glm::vec3 camPos = { 0.f,0.f,-100.f };
    while (!glfwWindowShouldClose(mainWindow.get())) {
        glfwPollEvents();
        vulkanBackend->beginFrame();

        glm::mat4 view = glm::translate(glm::mat4(1.f), camPos);
        glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.0f);
        projection[1][1] *= -1;

        vulkanBackend->bindPipeline("default");
        vulkanBackend->bindDescriptorSets();
        CameraData cameraData = {view, projection, projection * view};
        vulkanBackend->setUniformBuffer("cameraBuffer", &cameraData, sizeof(CameraData));
        for (auto &mesh : vulkanBackend->meshes) {
            glm::mat4 model = glm::rotate(mesh.second.model, glm::radians(vulkanBackend->frameNumber * 0.4f), glm::vec3(0, 1, 0));
            vulkanBackend->pushConstants(&model, sizeof(glm::mat4), ShaderStage::VERTEX);
            vulkanBackend->drawMesh(mesh.second);
        }
        vulkanBackend->endFrame();
    }
}

void Application::initPipelines() {
    Texture cvpiTexture("cvpiTexture");
    cvpiTexture.loadTextureFromFile("assets/cvpi.jpg");

    Texture cvpiTexture2("cvpiTexture2");
    cvpiTexture2 = cvpiTexture;
    cvpiTexture2.name = "cvpiTexture2";


    vulkanBackend->addTexture(cvpiTexture, 0);
    vulkanBackend->addTexture(cvpiTexture2, 1);

    vulkanBackend->createDescriptors(vulkanBackend->shaders["default"]);
    vulkanBackend->createGraphicsPipeline("default", vulkanBackend->shaders["default"]);
}
