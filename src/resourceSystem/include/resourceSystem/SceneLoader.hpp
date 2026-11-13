//
// Created by divmone on 9/21/2026.
//

#pragma once

#include <sceneSystem/Scene.hpp>
#include <string>

#include "types/SceneMeshData.hpp"

class VulkanContext;

// TODO FIX IT!!!!

class SceneLoader {
public:
    static Scene LoadSceneFromJSON(std::string_view path);
    static SceneMeshData LoadSceneMeshData(const std::string& path);
};
