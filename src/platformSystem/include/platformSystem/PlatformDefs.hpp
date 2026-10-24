#pragma once

#include <vulkan/vulkan_raii.hpp>

#if defined(_WIN64)
#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#endif