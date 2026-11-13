//
// Created by divmone on 11/8/2026.
//

#include <FileSystem.hpp>
#include <filesystem>

#include "fileConfig.hpp"

std::string FS::GetFullPath(FileType type, std::string_view path) {
    std::filesystem::path fullPath{GetProjectRootDir()};

    switch (type) {
        case FileType::eShader:
            fullPath /= "shaders";
            fullPath /= path;
            fullPath.replace_extension(".spv");
            break;
        case FileType::eAsset:
            fullPath /= "assets";
            fullPath /= path;
            break;
        case FileType::ePipelineCache:
            fullPath /= "cache";
            fullPath /= path;
            fullPath.replace_extension(".cache");
            break;
        case FileType::eScene:
            fullPath /= "scene";
            fullPath /= path;
            fullPath.replace_extension(".json");
            break;
        default:
            throw std::runtime_error("Unknown file type");
    }

    return fullPath.string();
}

constexpr const char* FS::GetProjectBinDir() {
    return PROJECT_BIN_DIR;
}

constexpr const char* FS::GetProjectRootDir() {
    return PROJECT_ROOT_DIR;
}