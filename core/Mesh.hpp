//
// Created by f0xeri on 01.01.2023.
//

#ifndef VULKAN_EXPERIMENTS_MESH_HPP
#define VULKAN_EXPERIMENTS_MESH_HPP

#include <vector>
#include "glm/glm.hpp"
#include "tiny_obj_loader.h"
#include "Shader.hpp"

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;
    glm::vec2 uv;

    friend bool operator==(const Vertex& lhs, const Vertex& rhs) {
        return lhs.position == rhs.position && lhs.normal == rhs.normal && lhs.color == rhs.color && lhs.uv == rhs.uv;
    }

    friend bool operator<(const Vertex& lhs, const Vertex& rhs) {
        return lhs.position != rhs.position;
    }
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    glm::mat4 model = glm::mat4(1.0f);

    // load obj file
    bool loadFromObj(const char *path) {

        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path)) {
            return false;
        }

        //vertices.resize(attrib.vertices.size() / 3);
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};

                vertex.position = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]
                };

                vertex.uv = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1] };

                vertex.color = {1.0f, 1.0f, 1.0f};

                //vertices[index.vertex_index] = vertex;
                vertices.push_back(vertex);
                //indices.push_back(index.vertex_index);
            }
        }
        return true;
    }

    // Generate a terrain quad patch for feeding to the tessellation control shader with given size
    static Mesh generateTerrainPatch(int size) {
        Mesh mesh{};
        const uint32_t vertexCount = size * size;
        mesh.vertices.resize(vertexCount);

        const float wx = 2.0f;
        const float wy = 2.0f;

        for (auto x = 0; x < size; x++) {
            for (auto y = 0; y < size; y++) {
                uint32_t index = (x + y * size);
                mesh.vertices[index].position = {x * wx + wx / 2.0f - (float)size * wx / 2.0f,
                                            0.0f,
                                            y * wy + wy / 2.0f - (float)size * wy / 2.0f
                };
                mesh.vertices[index].normal = {0.0f, 1.0f, 0.0f};
                mesh.vertices[index].color = {1.0f, 0.0f, 0.0f};
                mesh.vertices[index].uv = glm::vec2((float)x / size, (float)y / size) * 1.0f;
            }
        }

        const uint32_t w = (size - 1);
        const uint32_t indexCount = w * w * 4;
        mesh.indices.resize(indexCount);

        for (auto x = 0; x < w; x++) {
            for (auto y = 0; y < w; y++) {
                uint32_t index = (x + y * w) * 4;
                mesh.indices[index] = x + y * size;
                mesh.indices[index + 1] = mesh.indices[index] + size;
                mesh.indices[index + 2] = mesh.indices[index + 1] + 1;
                mesh.indices[index + 3] = mesh.indices[index] + 1;
            }
        }
        return mesh;
    }
};

struct RenderObject {
    Mesh mesh;
    Shader shader;
};


#endif //VULKAN_EXPERIMENTS_MESH_HPP
