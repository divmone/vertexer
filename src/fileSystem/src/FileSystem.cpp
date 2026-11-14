//
// Created by divmone on 11/8/2026.
//

#include <FileSystem.hpp>
#include <filesystem>

#include "fileConfig.hpp"

#if defined(_WIN32)
    #include <windows.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <limits.h>
#elif defined(__APPLE__)
    #include <mach-o/dyld.h>
    #include <climits>
    #include <cstdlib>
#endif

std::string FS::GetFullPath(FileType type, std::string_view path) {
    std::filesystem::path fullPath{GetExePath()};
    fullPath = fullPath.parent_path();
    fullPath /= "../../..";

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

const char* FS::GetExePath() {
#if defined(_WIN32)
        wchar_t path[MAX_PATH];
        DWORD length = GetModuleFileNameW(NULL, path, MAX_PATH);
        if (length == 0) return "";

        std::wstring wstr(path);
        return std::string(wstr.begin(), wstr.end());

#elif defined(__linux__)
        char path[PATH_MAX];
        ssize_t length = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (length == -1) return "";

        path[length] = '\0';
        return std::string(path);

#elif defined(__APPLE__)
        char path[PATH_MAX];
        uint32_t size = sizeof(path);
        if (_NSGetExecutablePath(path, &size) == 0) {
            char real_path[PATH_MAX];
            if (realpath(path, real_path) != nullptr) {
                return real_path;
            }
        }
        return "";
#else
        return ""; // Unsupported platform
#endif
}