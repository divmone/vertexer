//
// Created by dmitry on 24.11.2026.
//

#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <memory>
#include <renderSystem/types/2d/DrawCall2d.hpp>
#include <renderSystem/types/2d/PushConstantObject2d.hpp>
#include <renderSystem/types/2d/VulkanSprite.hpp>
#include <renderSystem/types/common/VulkanTexture.hpp>
#include <resourceSystem/types/ImageData.hpp>
#include <resourceSystem/types/Sprite.hpp>
#include <resourceSystem/types/SpriteData.hpp>
#include <span>
#include <unordered_map>
#include <utility>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

struct EngineStats;
class VulkanContext;

// Owns everything that is rendered with the 2D pipeline: sprites and the
// RmlUi draw calls share a single pipeline, layout and matrices.
class Vulkan2dContext {
public:
    using UiTextureHandle = std::size_t;

    explicit Vulkan2dContext(VulkanContext& base) : base_(&base) {}

    void Init();

    void SetProj2dMatrix(const glm::mat4& matrix);

#pragma region Sprites

    SpriteHandle AddSprite(const SpriteData& sprite);
    void         AddSpriteToPipeline(const Sprite& sprite);

#pragma endregion Sprites

#pragma region UiRendering

    uint32_t AddUiGeometry(std::span<const Vertex2d> vertices,
                           std::span<const uint32_t> indices);
    void     ReleaseUiGeometry(uint32_t geometryHandle);

    UiTextureHandle AddUiTexture(const ImageData& image);
    void            ReleaseUiTexture(UiTextureHandle textureHandle);

    void AddUiDrawCall(uint32_t         geometryHandle,
                       const glm::mat4& model,
                       UiTextureHandle  textureHandle,
                       bool             hasTexture,
                       bool             scissorEnabled,
                       vk::Rect2D       scissor);

#pragma endregion UiRendering

#pragma region DrawCalls

    void RenderDrawCalls2d(vk::raii::CommandBuffer& commandBuffer) const;

#pragma endregion DrawCalls

    // Records the 2D part of the frame: clears the swapchain image, draws the
    // queued draw calls and leaves the image ready for presentation.
    void RenderFrame(vk::raii::CommandBuffer& commandBuffer,
                     uint32_t                 imageIndex,
                     EngineStats&             stats);

private:
    void CreateDescriptorSetLayout2d();
    void CreateGraphicsPipeline2d();
    void CreateDescriptorSetForSprite(VkSprite& sprite) const;

    vk::raii::DescriptorSet CreateUiTextureDescriptorSet(
        const VkTexture& texture) const;

    struct UiTextureEntry {
        VkTexture               texture;
        vk::raii::DescriptorSet descriptorSet{nullptr};
        uint32_t                width{0};
        uint32_t                height{0};
    };

    struct UiGeometryEntry {
        vk::raii::Buffer       vertexBuffer{nullptr};
        vk::raii::DeviceMemory vertexBufferMemory{nullptr};
        vk::raii::Buffer       indexBuffer{nullptr};
        vk::raii::DeviceMemory indexBufferMemory{nullptr};
        uint32_t               indexCount{0};
    };

    VulkanContext* base_{nullptr};

    vk::raii::DescriptorSetLayout descriptorSetLayout2d_{nullptr};
    vk::raii::PipelineLayout      pipelineLayout2d_{nullptr};
    vk::raii::Pipeline            graphicsPipeline2d_{nullptr};

    glm::mat4 view2d_{1.0f};
    glm::mat4 proj2d_;

    std::vector<DrawCall2d>                    drawCalls2d_;
    std::unordered_map<SpriteHandle, VkSprite> sprites_;
    SpriteHandle                               spriteHandle_{0};

    std::vector<std::unique_ptr<UiTextureEntry>>  uiTextures_;
    std::vector<std::unique_ptr<UiGeometryEntry>> uiGeometries_;
};
