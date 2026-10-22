//
// Created by divmone on 7/23/2026.
//

#pragma once

// #include <stb_image.h>

#include <types/ImageData.hpp>
#include <vector>

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