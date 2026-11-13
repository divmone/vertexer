//
// Created by divmone on 11/8/2026.
//

#pragma once

#include <renderSystem/types/VulkanTexture.hpp>
#include <vulkan/vulkan_raii.hpp>

struct VkSprite {
    uint32_t indicesCount_ = 0;
    vk::raii::Buffer vertexBuffer_{nullptr};
    vk::raii::DeviceMemory vertexBufferMemory_{nullptr};
    vk::raii::Buffer indexBuffer_{nullptr};
    vk::raii::DeviceMemory indexBufferMemory_{nullptr};
    bool hasTexture_;
    VkTexture texture_;
    glm::vec4 color_;
};