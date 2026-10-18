//
// Created by divmone on 8/2/2026.
//

#include "Scene.hpp"
Object& Scene::addObject(uint32_t idx, const std::string &name) {
    objects_.push_back({counter++, name, idx});
    return objects_.back();
}

std::vector<Object> & Scene::getObjects() {
    return objects_;
}
