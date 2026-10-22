//
// Created by divmone on 8/2/2026.
//

#include "Scene.hpp"

Object& Scene::AddObject(uint32_t idx, const std::string& name) {
    objects_.push_back({counter_++, name, idx});
    return objects_.back();
}

std::vector<Object>& Scene::GetObjects() {
    return objects_;
}
