//
// Created by divmone on 10/22/2026.
//

#pragma once

#include <glm/glm.hpp>

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
