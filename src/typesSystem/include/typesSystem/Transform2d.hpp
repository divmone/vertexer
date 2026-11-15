//
// Created by divmone on 11/15/2026.
//

#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

struct Transform2d {
    glm::vec2 pos_{0.0f};
    glm::vec2 scale_{1.0f};
    float rotateAngle_{0.0f};
    glm::mat4 model_{1.0f};

    glm::mat4 GetModelMat() const {
        return glm::translate(model_, glm::vec3(pos_, 0.0f)) *
               glm::rotate(model_, rotateAngle_, {0.0f, 0.0f, 1.0f}) *
               glm::scale(model_, glm::vec3(scale_, 1.0f));
    }
};