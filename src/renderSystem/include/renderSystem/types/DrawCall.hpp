//
// Created by divmone on 10/22/2026.
//

#pragma once

#include <glm/glm.hpp>

struct SubDrawCall {
    bool isBlend{false};
    bool hasTexture{false};
    glm::vec4 color{0.0};
    uint32_t textureIndex{0};
    int32_t distanceToCam{0};
    glm::mat4 mat{1.0};
    uint32_t indexCount{0};
    uint32_t firstIndex{0};
    vk::DescriptorSet descriptorSet{nullptr};
};

struct DrawCall3d {
    vk::Buffer vertexBuffer{nullptr};
    vk::Buffer indexBuffer{nullptr};
    std::vector<SubDrawCall> materialData;
};
