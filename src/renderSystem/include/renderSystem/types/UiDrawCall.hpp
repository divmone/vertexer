//
// Created by divmone on 11/19/2026.
//

#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

struct UiDrawCall {
    glm::mat4 mat{1.0f};
    bool hasTexture{false};
    uint32_t indexCount{0};
    vk::Buffer vertexBuffer_{nullptr};
    vk::Buffer indexBuffer_{nullptr};
    vk::DescriptorSet descriptorSet_{nullptr};
    bool scissorEnabled{false};
    vk::Rect2D scissor_{};
};
