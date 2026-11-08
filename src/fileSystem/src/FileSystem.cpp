//
// Created by divmone on 11/8/2026.
//

#include <FileSystem.hpp>
#include <filesystem>

#include "fileConfig.hpp"

std::string FS::GetFullPath(FileType type, const std::string& path) {
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
        default:
            throw std::runtime_error("Unknown file type");
    }

    return fullPath.string();
}

const char* FS::GetProjectBinDir() {
    return PROJECT_BIN_DIR;
}

const char* FS::GetProjectRootDir() {
    return PROJECT_ROOT_DIR;
}