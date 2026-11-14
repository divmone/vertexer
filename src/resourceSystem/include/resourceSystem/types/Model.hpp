//
// Created by divmone on 10/22/2026.
//

#pragma once

#include <cstdint>
#include <resourceSystem/types/ImageData.hpp>
#include <typesSystem/MeshData.hpp>
#include <typesSystem/Vertex3d.hpp>

struct Model {
    std::vector<Vertex3d> vertices_;
    std::vector<uint32_t> indices;
    std::vector<Mesh> meshes_;
    std::vector<Node> nodes_;
    std::vector<uint32_t> rootNodes_;
    std::vector<ImageData> images_;
};
