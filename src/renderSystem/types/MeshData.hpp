//
// Created by divmone on 7/23/2026.
//

#pragma once

// #include <stb_image.h>

#include <glm/detail/type_quat.hpp>
#include <glm/gtc/quaternion.hpp>
#include <types/ImageData.hpp>
#include <vector>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color{1.0, 0.0, 0.0};
    glm::vec2 texCoord{0.0, 0.0};
};

struct DrawCall {
    uint32_t textureIndex{0};
    glm::mat4 mat{1.0};
    bool hasTexture{false};
    glm::vec4 color{0.0};
    uint32_t indexCount{0};
    uint32_t firstIndex{0};
    int32_t distanceToCam{0};
    uint32_t modelIndex{0};
};

struct Primitive {
    uint32_t firstIndex;
    uint32_t indexCount;
    int32_t materialIndex{0};
    int32_t textureIndex{0};
    glm::vec4 color;
    bool isBlend{false};
    bool hasTexture{false};
    int32_t distanceToCam{0};
};

struct Mesh {
    std::vector<Primitive> primitives;
    std::vector<Primitive> blendPrimitives;
};

struct Node {
    std::vector<uint32_t> children;
    int32_t meshIndex;
    glm::mat4 transformation{1.0};
};

struct Model {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Mesh> meshes;
    std::vector<Node> nodes;
    std::vector<uint32_t> rootNodes;
    std::vector<ImageData> images;
};

struct Texture {
    vk::raii::Image textureImage{nullptr};
    vk::raii::DeviceMemory textureImageMemory{nullptr};
    vk::raii::ImageView textureImageView{nullptr};
    vk::raii::Sampler textureSampler{nullptr};
};

struct VkMaterial {};

struct VkModel {
    uint32_t indicesCount = 0;
    vk::raii::Buffer vertexBuffer{nullptr};
    vk::raii::DeviceMemory vertexBufferMemory{nullptr};
    vk::raii::Buffer indexBuffer{nullptr};
    vk::raii::DeviceMemory indexBufferMemory{nullptr};
    std::vector<Mesh> meshes;
    std::vector<Node> nodes;
    std::vector<uint32_t> rootNodes;
    std::vector<Texture> textures;
    uint32_t textureOffset{0};
};
