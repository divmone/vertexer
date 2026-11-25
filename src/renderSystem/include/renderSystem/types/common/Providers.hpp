//
// Created by divmone on 10/18/2026.
//

#pragma once

#include <functional>
#include <vulkan/vulkan_raii.hpp>

struct SurfaceProvider {
    std::function<vk::SurfaceKHR(vk::Instance)> createSurface;
};

struct ExtensionProvider {
    std::vector<const char*> extensions;
};

//
struct ExtentProvider1 {
    std::function<std::pair<uint32_t, uint32_t>()> getExtentSizes;
    int                                            width;
    int                                            height;
};