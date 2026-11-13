//
// Created by divmone on 11/13/2026.
//

#pragma once

#include <cstdint>
#include <resourceSystem/types/ImageData.hpp>
#include <typesSystem/MeshData.hpp>
#include <typesSystem/Vertex3d.hpp>
#include <vector>

struct SpriteData {
    std::vector<Vertex2d> vertices_;
    std::vector<uint32_t> indices_;
    ImageData image_;
};