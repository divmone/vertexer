//
// Created by divmone on 10/18/2026.
//

#pragma once

#include <functional>
#include <vulkan/vulkan_raii.hpp>

struct SurfaceProvider {
    std::function<vk::SurfaceKHR(vk::Instance)> CreateSurface;
};

struct ExtensionProvider {
    std::vector<const char*> extensions_;
};

//
struct ExtentProvider {
    std::function<std::pair<uint32_t, uint32_t>()> GetExtentSizes;
    int                                            width_;
    int                                            height_;
};