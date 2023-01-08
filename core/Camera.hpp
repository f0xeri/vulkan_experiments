//
// Created by f0xeri on 08.01.2023.
//

#ifndef VULKAN_EXPERIMENTS_CAMERA_HPP
#define VULKAN_EXPERIMENTS_CAMERA_HPP

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Camera
{
private:
    void updateVectors();
public:
    float FOV;
    glm::vec3 pos;
    glm::mat4 rotation;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;

    Camera(glm::vec3 pos, float FOV);
    ~Camera() = default;

    glm::mat4 getProjectionMatrix(float aspectRatio);
    glm::mat4 getViewMatrix();
    void rotate(float x, float y, float z);
};


#endif //VULKAN_EXPERIMENTS_CAMERA_HPP
