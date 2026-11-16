//
// Created by divmone on 7/23/2026.
//

#pragma once

#include <fastgltf/core.hpp>
#include <resourceSystem/types/ImageData.hpp>
#include <resourceSystem/types/Model.hpp>
#include <string>
#include <vector>

class AssetLoader {
public:
    static ModelData loadModelFromGltf(const std::string& pathToFile);
    static ImageData loadImageDataFromFile(const std::string& pathToFile);

    // static std::vector<VkMaterial> LoadMaterialsFromGltf(const std::string&
    // pathToFile);
};
