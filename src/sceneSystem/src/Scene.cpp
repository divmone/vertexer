//
// Created by divmone on 8/2/2026.
//

#include <sceneSystem/Scene.hpp>

Object& Scene::AddObject(uint32_t idx, const std::string& name) {
    objects_.push_back({.id = counter_++, .name = name, .modelIndex = idx});
    return objects_.back();
}

std::vector<Object>& Scene::GetObjects() {
    return objects_;
}
