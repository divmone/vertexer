//
// Created by divmone on 10/22/2026.
//

#pragma once

#include <typesSystem/MeshData.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <renderSystem/types/VulkanTexture.hpp>

struct VkModel {
    uint32_t indicesCount = 0;
    vk::raii::Buffer vertexBuffer{nullptr};
    vk::raii::DeviceMemory vertexBufferMemory{nullptr};
    vk::raii::Buffer indexBuffer{nullptr};
    vk::raii::DeviceMemory indexBufferMemory{nullptr};
    std::vector<Mesh> meshes;
    std::vector<Node> nodes;
    std::vector<uint32_t> rootNodes;
    std::vector<Texture> textures;
    uint32_t textureOffset{0};
};