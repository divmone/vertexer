//
// Created by divmone on 7/23/2026.
//

#pragma once

#include <fastgltf/core.hpp>
#include <string>
#include <vector>

#include "types/ImageData.hpp"
#include "types/Model.hpp"

class AssetLoader {
public:
    static Model loadModelFromGltf(const std::string& pathToFile);
    static ImageData loadImageDataFromFile(const std::string& pathToFile);

    // static std::vector<VkMaterial> LoadMaterialsFromGltf(const std::string&
    // pathToFile);
};
