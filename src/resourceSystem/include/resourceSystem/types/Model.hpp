//
// Created by divmone on 10/22/2026.
//

#pragma once

#include <cstdint>
#include <resourceSystem/types/ImageData.hpp>
#include <typesSystem/MeshData.hpp>
#include <typesSystem/Vertex3d.hpp>

struct Model {
    std::vector<Vertex3d> vertices;
    std::vector<uint32_t> indices;
    std::vector<Mesh> meshes;
    std::vector<Node> nodes;
    std::vector<uint32_t> rootNodes;
    std::vector<ImageData> images;
};
