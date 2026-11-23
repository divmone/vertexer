//
// Created by divmone on 11/13/2026.
//

#pragma once

#include <cstdint>
#include <resourceSystem/types/ImageData.hpp>
#include <typesSystem/MeshData.hpp>
#include <typesSystem/Vertex3d.hpp>
#include <vector>

#include "typesSystem/Transform2d.hpp"

using SpriteHandle = uint32_t;

struct Sprite {
    Transform2d transform;
    SpriteHandle handle;
};