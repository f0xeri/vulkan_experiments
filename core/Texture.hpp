//
// Created by f0xeri on 04.01.2023.
//

#ifndef VULKAN_EXPERIMENTS_TEXTURE_HPP
#define VULKAN_EXPERIMENTS_TEXTURE_HPP

#include <iostream>
#include "stb_image.h"

class Texture {

public:
    Texture() = default;
    explicit Texture(const char *name) : name(name) {};
    void loadTextureFromFile(const char *path) {
        stbi_set_flip_vertically_on_load(true);
        data = stbi_load(path, &width, &height, &nrChannels, STBI_rgb_alpha);
        if (data) {

        }
        else {
            std::cout << "Failed to load texture" << std::endl;
        }
    }
    ~Texture(){
        //delete data;
    }

    const char *name;
    int width{}, height{}, nrChannels{};
    unsigned char *data = nullptr;

    // get pixel color
    glm::vec4 getPixelColor(int x, int y) const {
        int index = (y * width + x) * 4;
        return {data[index], data[index + 1], data[index + 2], data[index + 3]};
    }
};


#endif //VULKAN_EXPERIMENTS_TEXTURE_HPP
