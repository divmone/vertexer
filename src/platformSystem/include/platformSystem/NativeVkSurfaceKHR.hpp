#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "NativeWindowHandler.hpp"

VkSurfaceKHR GetNativeVkSurfaceKHR(const vk::raii::Instance& instance,
                                   const NativeWindowHandler& h);