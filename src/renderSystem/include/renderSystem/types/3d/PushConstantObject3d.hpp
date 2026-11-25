//
// Created by dmitry on 24.11.2026.
//

#pragma once

#include <cstdint>
#include <glm/glm.hpp>

struct PushConstantObject3d {
    alignas(16) glm::mat4  model;
    alignas(16) glm::vec4  color;
    alignas(16) uint32_t   hasTexture;
};
