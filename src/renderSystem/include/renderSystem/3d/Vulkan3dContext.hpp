//
// Created by dmitry on 24.11.2026.
//

#pragma once

#include <glm/glm.hpp>
#include <renderSystem/types/3d/VulkanModel.hpp>
#include <renderSystem/types/common/VulkanTexture.hpp>
#include <resourceSystem/types/Model.hpp>
#include <resourceSystem/types/Sprite.hpp>
#include <string>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

class VulkanContext;

class Vulkan3dContext {
public:
    explicit Vulkan3dContext(VulkanContext& base) : base_(&base) {}

    void Init();
    void FinishLoading();

    void SetViewMatrix(const glm::mat4& matrix);
    void SetProjMatrix(const glm::mat4& matrix);

    SpriteHandle AddModel(const Model& model);
    uint32_t     LoadAsset(const std::string& path);

private:
    void CreateDescriptorSetLayout();
    void CreateBlendDescriptorSetLayout();
    void CreateBindlessDescriptorSetLayout();

    void CreateUniformBuffers();
    void CreateDescriptorSets();
    void CreateBindlessDescriptorSet();
    void CreateTextureDescriptorSet();

    void CreateGraphicsPipeline();
    void CreateBlendingPipeline();

    void UpdateUniformBuffer(uint32_t index) const;

    void ProcessNodes(unsigned         root,
                      const glm::mat4& matrix,
                      const VkModel&   model,
                      uint32_t         modelIndex);
    void DrawBlendNodes(unsigned         root,
                        const glm::mat4& matrix,
                        const VkModel&   model);

    std::vector<VkTexture> CreateTexturesFromImages(
        vk::raii::CommandBuffer&             commandBuffer,
        std::vector<vk::raii::Buffer>&       stagingBuffers,
        std::vector<vk::raii::DeviceMemory>& stagingMemories,
        const Model&                         model);

    VkModel LoadModel(const std::string& path);

    VulkanContext* base_{nullptr};
};
