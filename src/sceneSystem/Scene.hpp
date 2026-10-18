//
// Created by divmone on 8/2/2026.
//

#pragma once
#include <vector>
#include "MeshData.hpp"
#include "types/Object.hpp"

class Scene {
public:
    Object& addObject(uint32_t idx, const std::string &name);
    std::vector<Object>& getObjects();
private:
    std::vector<Object> objects_;
    size_t counter{0};
};
