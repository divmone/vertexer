//
// Created by divmone on 10/22/2026.
//

#pragma once

#include <typesSystem/Vertex.hpp>

#include "../../typesSystem/include/typesSystem/MeshData.hpp"

struct Model {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Mesh> meshes;
    std::vector<Node> nodes;
    std::vector<uint32_t> rootNodes;
    std::vector<ImageData> images;
};
