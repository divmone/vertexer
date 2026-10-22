//
// Created by divmone on 9/21/2026.
//

#pragma once

#include <string>
#include "../Scene.hpp"

class VulkanContext;

class SceneLoader {
public:
    static Scene loadSceneFromJSON(VulkanContext& vkContext, const std::string& path);
};
