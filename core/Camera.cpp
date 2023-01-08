//
// Created by f0xeri on 08.01.2023.
//

#include "Camera.hpp"
#include <glm/ext.hpp>

Camera::Camera(glm::vec3 pos, float FOV) {
    updateVectors();
}

void Camera::updateVectors() {
    front = glm::vec3(rotation * glm::vec4(0, 0, -1, 1));
    up = glm::vec3(rotation * glm::vec4(0, 1, 0, 1));
    right = glm::vec3(rotation * glm::vec4(1, 0, 0, 1));
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) {
    auto projection = glm::perspective(FOV, aspectRatio, 0.1f, 5000.0f);
    projection[1][1] *= -1;
    return projection;
}

glm::mat4 Camera::getViewMatrix() {
    return glm::lookAt(pos, pos + front, up);
}

void Camera::rotate(float x, float y, float z) {
    rotation = glm::rotate(rotation, x, glm::vec3(1, 0, 0));
    rotation = glm::rotate(rotation, y, glm::vec3(0, 1, 0));
    rotation = glm::rotate(rotation, z, glm::vec3(0, 0, 1));
    updateVectors();
}
