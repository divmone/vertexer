//
// Created by divmone on 10/22/2026.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "typesSystem/MeshData.hpp"

struct VulkanVertex : Vertex {
    static vk::VertexInputBindingDescription getBindingDescription() {
        return {.binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 4>
    getAttributeDescriptions() {
        return {{{.location = 0,
                  .binding = 0,
                  .format = vk::Format::eR32G32B32Sfloat,
                  .offset = offsetof(Vertex, pos)},
                 {.location = 1,
                  .binding = 0,
                  .format = vk::Format::eR32G32B32Sfloat,
                  .offset = offsetof(Vertex, normal)},
                 {.location = 2,
                  .binding = 0,
                  .format = vk::Format::eR32G32B32Sfloat,
                  .offset = offsetof(Vertex, color)},
                 {.location = 3,
                  .binding = 0,
                  .format = vk::Format::eR32G32Sfloat,
                  .offset = offsetof(Vertex, texCoord)}}};
    }

    VulkanVertex& operator=(const glm::vec3& position) {
        if (&this->pos == &position) {
            return *this;
        }

        pos = position;
        return *this;
    }
};

;