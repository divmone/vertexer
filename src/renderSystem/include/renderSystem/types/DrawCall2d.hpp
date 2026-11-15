//
// Created by divmone on 11/16/2026.
//

#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan_raii.hpp>

struct DrawCall2d {
    glm::mat4 mat{1.0};
    bool hasTexture{false};
    glm::vec4 color{0.0};
    uint32_t indexCount{0};
    vk::Buffer vertexBuffer_{nullptr};
    vk::Buffer indexBuffer_{nullptr};
    vk::DescriptorSet descriptorSet_{nullptr};
};