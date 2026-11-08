//
// Created by divmone on 10/22/2026.
//

#pragma once

#include <glm/glm.hpp>

struct Vertex3d {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec3 color{1.0, 0.0, 0.0};
    glm::vec2 texCoord{0.0, 0.0};
};

struct Vertex2d {
    glm::vec2 pos;
    glm::vec4 color;
    glm::vec2 texCoord;
};