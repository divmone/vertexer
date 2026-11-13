//
// Created by divmone on 10/22/2026.
//

#pragma once

#include <typesSystem/Vertex3d.hpp>
#include <vulkan/vulkan_raii.hpp>

struct VulkanVertex3d : Vertex3d {
    static vk::VertexInputBindingDescription getBindingDescription() {
        return {.binding = 0,
                .stride = sizeof(Vertex3d),
                .inputRate = vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 4>
    getAttributeDescriptions() {
        return {{{.location = 0,
                  .binding = 0,
                  .format = vk::Format::eR32G32B32Sfloat,
                  .offset = offsetof(Vertex3d, pos)},
                 {.location = 1,
                  .binding = 0,
                  .format = vk::Format::eR32G32B32Sfloat,
                  .offset = offsetof(Vertex3d, normal)},
                 {.location = 2,
                  .binding = 0,
                  .format = vk::Format::eR32G32B32Sfloat,
                  .offset = offsetof(Vertex3d, color)},
                 {.location = 3,
                  .binding = 0,
                  .format = vk::Format::eR32G32Sfloat,
                  .offset = offsetof(Vertex3d, texCoord)}}};
    }

    VulkanVertex3d& operator=(const glm::vec3& position) {
        if (&this->pos == &position) {
            return *this;
        }

        pos = position;
        return *this;
    }
};
