//
// Created by divmone on 11/14/2026.
//

#pragma once

#include <vector>

#include "resourceSystem/types/ImageData.hpp"
#include "typesSystem/Vertex3d.hpp"

struct SpriteData {
    std::vector<Vertex2d> vertices;
    std::vector<uint32_t> indices;
    ImageData             image;
};
