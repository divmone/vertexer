//
// Created by divmone on 9/23/2026.
//

#pragma once

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

#include "typesSystem/Transform3d.hpp"

using ObjectHandle = uint32_t;

struct Object {
    size_t id;
    std::string name;
    ObjectHandle handle;
    Transform3d transform;
    glm::vec3 color;
    std::string modelName;
};
