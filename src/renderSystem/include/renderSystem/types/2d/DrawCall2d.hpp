//
// Created by divmone on 11/16/2026.
//

#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

// A single 2D draw call. Shared by world sprites and by the RmlUi renderer:
// sprites fill color from their SpriteData, the UI always uses white and may
// request a scissor rectangle.
struct DrawCall2d {
    glm::mat4         mat{1.0f};
    glm::vec4         color{1.0f, 1.0f, 1.0f, 1.0f};
    bool              hasTexture{false};
    uint32_t          indexCount{0};
    vk::Buffer        vertexBuffer_{nullptr};
    vk::Buffer        indexBuffer_{nullptr};
    vk::DescriptorSet descriptorSet_{nullptr};
    bool              scissorEnabled{false};
    vk::Rect2D        scissor_{};
};
