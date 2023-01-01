//
// Created by f0xeri on 01.01.2023.
//

#ifndef VULKAN_EXPERIMENTS_MESH_HPP
#define VULKAN_EXPERIMENTS_MESH_HPP

#include <vector>
#include "glm/vec3.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
};

struct Mesh {
    std::vector<Vertex> vertices;
};

#endif //VULKAN_EXPERIMENTS_MESH_HPP
