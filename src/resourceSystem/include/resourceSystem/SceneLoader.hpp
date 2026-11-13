//
// Created by divmone on 9/21/2026.
//

#pragma once

#include <sceneSystem/Scene.hpp>
#include <string>

#include "types/SceneMeshData.hpp"

// TODO !!!!!!!!!!!!!!!!!OPTIMIZE IT!!!!!!!!!!!!

class SceneLoader {
public:
    // TODO !!!!!!!!!!!!!!!!!OPTIMIZE IT!!!!!!!!!!!!
    static Scene LoadSceneFromJSON(std::string_view path);
    // TODO !!!!!!!!!!!!!!!!!OPTIMIZE IT!!!!!!!!!!!!
    static SceneMeshData LoadSceneMeshData(const std::string& path);
};
