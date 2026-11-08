//
// Created by divmone on 11/8/2026.
//

#pragma once

#include <renderSystem/types/VulkanTexture.hpp>
#include <vulkan/vulkan_raii.hpp>

struct VkSprite {
    uint32_t indicesCount = 0;
    vk::raii::Buffer vertexBuffer{nullptr};
    vk::raii::DeviceMemory vertexBufferMemory{nullptr};
    vk::raii::Buffer indexBuffer{nullptr};
    vk::raii::DeviceMemory indexBufferMemory{nullptr};
    Texture texture;
};