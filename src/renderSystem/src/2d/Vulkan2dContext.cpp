#include <array>
#include <logSystem/Logger.hpp>
#include <renderSystem/2d/Vulkan2dContext.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <renderSystem/detail/DescriptorSetLayoutBuilder.hpp>
#include <renderSystem/detail/PipelineBuilder.hpp>

#include "FileSystem.hpp"
#include <renderSystem/types/2d/VulkanVertex2d.hpp>

void Vulkan2dContext::Init() {
    CreateDescriptorSetLayout2d();
    CreateGraphicsPipeline2d();
}

void Vulkan2dContext::SetProj2dMatrix(const glm::mat4& matrix) {
    proj2d_ = matrix;
}

void Vulkan2dContext::CreateGraphicsPipeline2d() {
    auto shaderModule = base_->CreateShaderModule(
        base_->LoadShader(FS::GetFullPath(FS::FileType::eShader, "shader_2d")));

    pipelineLayout2d_ = PipelineBuilder::CreatePipelineLayout(
        base_->device_,
        {{
            .stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
            .offset = 0,
            .size = sizeof(PushConstantObject2d),
        }},
        {*descriptorSetLayout2d_});

    PipelineBuilder builder;

    graphicsPipeline2d_ =
        builder.Shaders(shaderModule)
            .ColorFormat(base_->swapChainSurfaceFormat_.format)
            .VertexType<VulkanVertex2d>()
            .DepthTestOff()
            .DepthFormat(base_->depthFormat_)
            .BlendingOn()
            .Layout(pipelineLayout2d_)
            .Build(base_->device_);
}

void Vulkan2dContext::CreateDescriptorSetLayout2d() {
    DescriptorSetLayoutBuilder builder;
    descriptorSetLayout2d_ =
        builder
            .Add({.binding = 0,
                  .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                  .descriptorCount = 1,
                  .stageFlags = vk::ShaderStageFlagBits::eFragment})
            .Build(base_->device_);
}

void Vulkan2dContext::CreateDescriptorSetForSprite(VkSprite& sprite) const {
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = base_->generalDescriptorPool_,
        .descriptorSetCount = 1,
        .pSetLayouts = &*descriptorSetLayout2d_};

    sprite.descriptorSet =
        std::move(base_->device_.allocateDescriptorSets(allocInfo)[0]);

    const auto& texture = sprite.texture;

    vk::DescriptorImageInfo imageInfo{
        .sampler = texture.textureSampler,
        .imageView = texture.textureImageView,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    std::array writes{vk::WriteDescriptorSet{
        .dstSet = sprite.descriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &imageInfo}};
    base_->device_.updateDescriptorSets(writes, {});
}

void Vulkan2dContext::AddSpriteToPipeline(const Sprite& sprite) {
    auto& vkSprite = sprites_.at(sprite.handle);

    drawCalls2d_.push_back({.mat = sprite.transform.GetModelMat(),
                            .color = vkSprite.color,
                            .hasTexture = vkSprite.hasTexture,
                            .indexCount = vkSprite.indicesCount,
                            .vertexBuffer_ = *vkSprite.vertexBuffer,
                            .indexBuffer_ = *vkSprite.indexBuffer,
                            .descriptorSet_ = *vkSprite.descriptorSet});
}

void Vulkan2dContext::RenderDrawCalls2d(
    vk::raii::CommandBuffer& commandBuffer) const {
    if (drawCalls2d_.empty()) {
        return;
    }

    vk::DescriptorSet fallbackDescriptorSet{nullptr};
    for (const auto& texture : uiTextures_) {
        if (texture) {
            fallbackDescriptorSet = *texture->descriptorSet;
            break;
        }
    }

    const vk::Rect2D fullScissor{.offset = {.x = 0, .y = 0},
                                 .extent = base_->swapChainExtent_};

    for (const auto& drawCall : drawCalls2d_) {
        const auto descriptorSet = drawCall.descriptorSet_ != nullptr
                                       ? drawCall.descriptorSet_
                                       : fallbackDescriptorSet;
        if (descriptorSet == nullptr) {
            continue;
        }

        commandBuffer.setScissor(
            0, drawCall.scissorEnabled ? drawCall.scissor_ : fullScissor);

        commandBuffer.bindVertexBuffers(0, drawCall.vertexBuffer_, {0});
        commandBuffer.bindIndexBuffer(
            drawCall.indexBuffer_, 0, vk::IndexType::eUint32);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                         pipelineLayout2d_,
                                         0,
                                         descriptorSet,
                                         nullptr);

        commandBuffer.pushConstants<PushConstantObject2d>(
            pipelineLayout2d_,
            vk::ShaderStageFlagBits::eAllGraphics,
            0,
            PushConstantObject2d{.model = drawCall.mat,
                                 .view = view2d_,
                                 .proj = proj2d_,
                                 .color = drawCall.color,
                                 .hasTexture = drawCall.hasTexture ? 1u : 0u});

        commandBuffer.drawIndexed(drawCall.indexCount, 1, 0, 0, 0);
    }

    commandBuffer.setScissor(0, fullScissor);
}

SpriteHandle Vulkan2dContext::AddSprite(const SpriteData& sprite) {
    VkSprite                vkSprite;
    vk::raii::CommandBuffer commandBuffer = base_->BeginSingleTimeCommand();

    std::vector<vk::raii::Buffer>       stagingBuffers;
    std::vector<vk::raii::DeviceMemory> stagingMemories;

    auto [vertexBuffer, vertexBufferMemory] =
        base_->CreateUniversalBuffer<Vertex2d>(
            commandBuffer,
            stagingBuffers,
            stagingMemories,
            sprite.vertices,
            vk::BufferUsageFlagBits::eVertexBuffer);

    vkSprite.vertexBuffer = std::move(vertexBuffer);
    vkSprite.vertexBufferMemory = std::move(vertexBufferMemory);

    auto [indexBuffer, indexBufferMemory] =
        base_->CreateUniversalBuffer<uint32_t>(
            commandBuffer,
            stagingBuffers,
            stagingMemories,
            sprite.indices,
            vk::BufferUsageFlagBits::eIndexBuffer);

    vkSprite.indexBuffer = std::move(indexBuffer);
    vkSprite.indexBufferMemory = std::move(indexBufferMemory);
    vkSprite.indicesCount = sprite.indices.size();

    VkTexture texture = base_->CreateVkTextureFromImageData(
        commandBuffer, stagingBuffers, stagingMemories, sprite.image);

    vkSprite.texture = std::move(texture);

    CreateDescriptorSetForSprite(vkSprite);

    base_->EndSingleTimeCommand(std::move(commandBuffer));

    sprites_[spriteHandle_] = std::move(vkSprite);

    return spriteHandle_++;
}
