//
// Created by divmone on 11/8/2026.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

class DescriptorPool {
public:
    DescriptorPool() = default;
private:
    vk::raii::DescriptorPool pool_;
};
