//
// Created by divmone on 8/2/2026.
//

#pragma once
#include <vector>

#include "types/Object.hpp"

class Scene {
public:
    Object&              AddObject(uint32_t idx, const std::string& name);
    std::vector<Object>& GetObjects();
private:
    std::vector<Object> objects_;
    size_t              counter_{0};
};
