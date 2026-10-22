//
// Created by divmone on 10/22/2026.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

struct Texture {
    vk::raii::Image textureImage{nullptr};
    vk::raii::DeviceMemory textureImageMemory{nullptr};
    vk::raii::ImageView textureImageView{nullptr};
    vk::raii::Sampler textureSampler{nullptr};
};