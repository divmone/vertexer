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
    };

    static std::string GetFullPath(FileType type, const std::string& path);
    static const char* GetProjectBinDir();
    static const char* GetProjectRootDir();
};