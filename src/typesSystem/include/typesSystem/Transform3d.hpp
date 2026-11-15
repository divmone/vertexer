//
// Created by divmone on 11/15/2026.
//

#pragma once

#include <glm/glm.hpp>

struct Transform3d {
    glm::vec3 pos{0.0};
    glm::quat rotation{1.0, 0.0, 0.0, 0.0};
    glm::vec3 scale{1.0, 1.0, 1.0};
    glm::vec3 eulerAnglesDeg{0.0f};

    glm::mat4 getModelMat() const {
        return glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rotation) *
               glm::scale(glm::mat4(1.0), scale);
    }

    void rotate(glm::vec3 axis, float angle) {
        rotation = glm::rotate(rotation, glm::radians(angle), axis);
    }
};