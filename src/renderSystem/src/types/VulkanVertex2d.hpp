//
// Created by divmone on 11/8/2026.
//

#pragma once

struct VulkanVertex2d {
    static vk::VertexInputBindingDescription getBindingDescription() {
        return {.binding = 0,
                .stride = sizeof(Vertex2d),
                .inputRate = vk::VertexInputRate::eVertex};
    }

    static std::array<vk::VertexInputAttributeDescription, 3>
    getAttributeDescriptions() {
        return {{{.location = 0,
                  .binding = 0,
                  .format = vk::Format::eR32G32Sfloat,
                  .offset = offsetof(Vertex2d, pos)},
                 {.location = 1,
                  .binding = 0,
                  .format = vk::Format::eR32G32B32A32Sfloat,
                  .offset = offsetof(Vertex2d, color)},
                 {.location = 2,
                  .binding = 0,
                  .format = vk::Format::eR32G32Sfloat,
                  .offset = offsetof(Vertex2d, texCoord)}}};
    }
};
