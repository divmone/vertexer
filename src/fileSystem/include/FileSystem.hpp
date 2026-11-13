//
// Created by divmone on 11/8/2026.
//

#pragma once

#include <string>

class FS {
public:
    enum class FileType {
        eNone = 0,
        eShader,
        ePipelineCache,
        eAsset,
        eScene
    };

    static std::string GetFullPath(FileType type, std::string_view path);
    constexpr static const char* GetProjectBinDir();
    constexpr static const char* GetProjectRootDir();
};