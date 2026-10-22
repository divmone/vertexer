//
// Created by divmone on 10/22/2026.
//

#pragma once

#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color{1.0, 0.0, 0.0};
    glm::vec2 texCoord{0.0, 0.0};
};