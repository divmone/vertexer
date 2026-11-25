//
// Created by dmitry on 24.11.2026.
//

#pragma once

#include <glm/glm.hpp>

struct PushConstantObjectBlend {
    alignas(16) glm::mat4 model;
    alignas(16) glm::vec4 color;
};
