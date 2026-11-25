//
// Created by divmone on 11/8/2026.
//

#pragma once

#include <glm/glm.hpp>
#include <renderSystem/types/common/VulkanTexture.hpp>
#include <vulkan/vulkan_raii.hpp>

struct VkSprite {
    uint32_t                indicesCount = 0;
    vk::raii::Buffer        vertexBuffer{nullptr};
    vk::raii::DeviceMemory  vertexBufferMemory{nullptr};
    vk::raii::Buffer        indexBuffer{nullptr};
    vk::raii::DeviceMemory  indexBufferMemory{nullptr};
    bool                    hasTexture;
    VkTexture               texture;
    glm::vec4               color{0.0, 1.0, 0.0, 1.0};
    vk::raii::DescriptorSet descriptorSet{nullptr};
    // std::vector<vk::raii::DescriptorSet> descriptorSets_;
};