//
// Created by dmitry on 24.11.2026.
//

#pragma once

#include <glm/glm.hpp>

struct UBObject {
    alignas(16) glm::vec3 lightPos;
    alignas(16) glm::vec3 lightColor;
    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 cameraPos;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};
