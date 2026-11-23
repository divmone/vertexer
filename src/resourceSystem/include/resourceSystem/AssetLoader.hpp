//
// Created by divmone on 7/23/2026.
//

#pragma once

#include <fastgltf/core.hpp>
#include <string>
#include <vector>

#include <resourceSystem/types/ImageData.hpp>
#include <resourceSystem/types/Model.hpp>

class AssetLoader {
public:
    static Model LoadModelFromGltf(const std::string& pathToFile);
    static ImageData LoadImageDataFromFile(const std::string& pathToFile);

    // static std::vector<VkMaterial> LoadMaterialsFromGltf(const std::string&
    // pathToFile);
};
