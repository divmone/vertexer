#include <chrono>
#include <fstream>
#include <logSystem/Logger.hpp>
#include <platformSystem/NativeVkSurfaceKHR.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <resourceSystem/AssetLoader.hpp>
#include <sceneSystem/Scene.hpp>
#include "resourceSystem/types/Sprite.hpp"


uint32_t VulkanContext::AddUiGeometry(std::span<const Vertex2d> vertices,
                                      std::span<const uint32_t> indices) {
    auto geometry = std::make_unique<UiGeometryEntry>();

    vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommand();
    std::vector<vk::raii::Buffer> stagingBuffers;
    std::vector<vk::raii::DeviceMemory> stagingMemories;

    auto [vertexBuffer, vertexBufferMemory] = CreateUniversalBuffer<Vertex2d>(
        commandBuffer, stagingBuffers, stagingMemories, vertices,
        vk::BufferUsageFlagBits::eVertexBuffer);
    geometry->vertexBuffer = std::move(vertexBuffer);
    geometry->vertexBufferMemory = std::move(vertexBufferMemory);

    auto [indexBuffer, indexBufferMemory] = CreateUniversalBuffer<uint32_t>(
        commandBuffer, stagingBuffers, stagingMemories, indices,
        vk::BufferUsageFlagBits::eIndexBuffer);
    geometry->indexBuffer = std::move(indexBuffer);
    geometry->indexBufferMemory = std::move(indexBufferMemory);
    geometry->indexCount = static_cast<uint32_t>(indices.size());

    EndSingleTimeCommand(std::move(commandBuffer));

    uiGeometries_.push_back(std::move(geometry));

    // Handle 0 is reserved as "invalid".
    return static_cast<uint32_t>(uiGeometries_.size());
}


void VulkanContext::ReleaseUiGeometry(uint32_t geometryHandle) {
    if (geometryHandle == 0) {
        return;
    }

    const auto index = static_cast<size_t>(geometryHandle - 1);
    if (index >= uiGeometries_.size()) {
        return;
    }

    device_.waitIdle();
    uiGeometries_[index].reset();
}


vk::raii::DescriptorSet VulkanContext::CreateUiTextureDescriptorSet(
    const VkTexture& texture) const {
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = generalDescriptorPool_,
        .descriptorSetCount = 1,
        .pSetLayouts = &*descriptorSetLayout2d_};

    auto descriptorSet = std::move(device_.allocateDescriptorSets(allocInfo)[0]);

    vk::DescriptorImageInfo imageInfo{
        .sampler = texture.textureSampler,
        .imageView = texture.textureImageView,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    std::array writes{vk::WriteDescriptorSet{
        .dstSet = descriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &imageInfo}};
    device_.updateDescriptorSets(writes, {});

    return descriptorSet;
}


uintptr_t VulkanContext::AddUiTexture(const ImageData& image) {
    auto texture = std::make_unique<UiTextureEntry>();

    vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommand();
    std::vector<vk::raii::Buffer> stagingBuffers;
    std::vector<vk::raii::DeviceMemory> stagingMemories;

    texture->texture = CreateVkTextureFromImageData(
        commandBuffer, stagingBuffers, stagingMemories, image);
    texture->width = image.width;
    texture->height = image.height;
    texture->descriptorSet =
        CreateUiTextureDescriptorSet(texture->texture);

    EndSingleTimeCommand(std::move(commandBuffer));

    uiTextures_.push_back(std::move(texture));

    return static_cast<uintptr_t>(uiTextures_.size());
}


void VulkanContext::ReleaseUiTexture(uintptr_t textureHandle) {
    if (textureHandle == 0) {
        return;
    }

    const auto index = static_cast<size_t>(textureHandle - 1);
    if (index >= uiTextures_.size()) {
        return;
    }

    device_.waitIdle();
    uiTextures_[index].reset();
}


uintptr_t VulkanContext::CreateOffscreenTarget(uint32_t width,
                                               uint32_t height) {
    auto texture = std::make_unique<UiTextureEntry>();
    texture->width = width;
    texture->height = height;

    const auto format = swapChainSurfaceFormat_.format;

    std::tie(texture->texture.textureImage,
             texture->texture.textureImageMemory) =
        CreateImage(width, height, format, vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eColorAttachment |
                        vk::ImageUsageFlagBits::eSampled,
                    vk::MemoryPropertyFlagBits::eDeviceLocal);

    texture->texture.textureImageView =
        CreateImageView(texture->texture.textureImage, format,
                        vk::ImageAspectFlagBits::eColor);

    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eClampToEdge;
    samplerInfo.maxLod = 0.0f;
    texture->texture.textureSampler =
        vk::raii::Sampler(device_, samplerInfo);

    texture->descriptorSet =
        CreateUiTextureDescriptorSet(texture->texture);

    uiTextures_.push_back(std::move(texture));

    return static_cast<uintptr_t>(uiTextures_.size());
}


void VulkanContext::RenderSpriteToOffscreen(SpriteHandle sprite,
                                            uintptr_t targetHandle) {
    if (targetHandle == 0) {
        return;
    }

    const auto targetIndex = static_cast<size_t>(targetHandle - 1);
    if (targetIndex >= uiTextures_.size() || !uiTextures_[targetIndex]) {
        return;
    }

    const auto spriteIt = sprites_.find(sprite);
    if (spriteIt == sprites_.end()) {
        return;
    }

    auto& target = *uiTextures_[targetIndex];
    const auto& vkSprite = spriteIt->second;

    vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommand();

    TransitionImageLayout(commandBuffer, target.texture.textureImage,
                          vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eColorAttachmentOptimal);

    vk::ClearValue clearValue =
        vk::ClearColorValue(0.02f, 0.04f, 0.08f, 1.0f);

    vk::RenderingAttachmentInfo colorAttachment{};
    colorAttachment.imageView = target.texture.textureImageView;
    colorAttachment.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.clearValue = clearValue;

    vk::RenderingInfo renderingInfo{};
    renderingInfo.renderArea =
        vk::Rect2D{.offset = vk::Offset2D{.x = 0, .y = 0},
                   .extent = vk::Extent2D{.width = target.width,
                                          .height = target.height}};
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachment;
    renderingInfo.pDepthAttachment = nullptr;

    commandBuffer.beginRendering(renderingInfo);

    commandBuffer.setViewport(
        0, vk::Viewport(0.0f, 0.0f, static_cast<float>(target.width),
                        static_cast<float>(target.height), 0.0f, 1.0f));
    commandBuffer.setScissor(
        0, vk::Rect2D(vk::Offset2D(0, 0),
                      vk::Extent2D(target.width, target.height)));

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                               graphicsPipeline2d_);

    commandBuffer.bindVertexBuffers(0, *vkSprite.vertexBuffer, {0});
    commandBuffer.bindIndexBuffer(*vkSprite.indexBuffer, 0,
                                  vk::IndexType::eUint32);
    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                     pipelineLayout2d_, 0,
                                     *vkSprite.descriptorSet, nullptr);

    glm::mat4 model = glm::translate(
        glm::mat4(1.0f),
        glm::vec3(static_cast<float>(target.width) * 0.5f,
                  static_cast<float>(target.height) * 0.5f, 0.0f));
    model = glm::scale(
        model, glm::vec3(static_cast<float>(target.width) * 0.5f,
                         static_cast<float>(target.height) * 0.5f, 1.0f));

    const glm::mat4 projection = glm::orthoZO(
        0.0f, static_cast<float>(target.width), 0.0f,
        static_cast<float>(target.height), 0.0f, 10.0f);

    commandBuffer.pushConstants<PushConstantObject2d>(
        pipelineLayout2d_, vk::ShaderStageFlagBits::eAllGraphics, 0,
        PushConstantObject2d{model, glm::mat4(1.0f), projection,
                             glm::vec4(1.0f), 1});

    commandBuffer.drawIndexed(vkSprite.indicesCount, 1, 0, 0, 0);

    commandBuffer.endRendering();

    TransitionImageLayout(commandBuffer, target.texture.textureImage,
                          vk::ImageLayout::eColorAttachmentOptimal,
                          vk::ImageLayout::eShaderReadOnlyOptimal);

    EndSingleTimeCommand(std::move(commandBuffer));
}


std::pair<uint32_t, uint32_t> VulkanContext::GetUiTextureSize(
    uintptr_t textureHandle) const {
    if (textureHandle == 0) {
        return {0, 0};
    }

    const auto index = static_cast<size_t>(textureHandle - 1);
    if (index >= uiTextures_.size() || !uiTextures_[index]) {
        return {0, 0};
    }

    return {uiTextures_[index]->width, uiTextures_[index]->height};
}


void VulkanContext::AddUiDrawCall(uint32_t geometryHandle,
                                  const glm::mat4& model,
                                  uintptr_t textureHandle,
                                  bool hasTexture,
                                  bool scissorEnabled,
                                  vk::Rect2D scissor) {
    if (geometryHandle == 0) {
        return;
    }

    const auto geometryIndex = static_cast<size_t>(geometryHandle - 1);
    if (geometryIndex >= uiGeometries_.size() ||
        !uiGeometries_[geometryIndex]) {
        return;
    }

    const auto& geometry = *uiGeometries_[geometryIndex];

    UiDrawCall drawCall;
    drawCall.mat = model;
    drawCall.hasTexture = hasTexture;
    drawCall.indexCount = geometry.indexCount;
    drawCall.vertexBuffer_ = *geometry.vertexBuffer;
    drawCall.indexBuffer_ = *geometry.indexBuffer;
    drawCall.scissorEnabled = scissorEnabled;
    drawCall.scissor_ = scissor;

    if (textureHandle != 0) {
        const auto textureIndex = textureHandle - 1;
        if (textureIndex < uiTextures_.size() && uiTextures_[textureIndex]) {
            drawCall.descriptorSet_ =
                *uiTextures_[textureIndex]->descriptorSet;
        }
    }

    uiDrawCalls_.push_back(drawCall);
}


void VulkanContext::RenderUiDrawCalls(
    vk::raii::CommandBuffer& commandBuffer) const {
    if (uiDrawCalls_.empty()) {
        return;
    }

    vk::DescriptorSet fallbackDescriptorSet{nullptr};
    for (const auto& texture : uiTextures_) {
        if (texture) {
            fallbackDescriptorSet = *texture->descriptorSet;
            break;
        }
    }

    const vk::Rect2D fullScissor{
        .offset = {.x = 0, .y = 0}, .extent = swapChainExtent_};

    for (const auto& drawCall : uiDrawCalls_) {
        const auto descriptorSet = drawCall.descriptorSet_ != nullptr
                                       ? drawCall.descriptorSet_
                                       : fallbackDescriptorSet;
        if (descriptorSet == nullptr) {
            continue;
        }

        commandBuffer.setScissor(
            0, drawCall.scissorEnabled ? drawCall.scissor_ : fullScissor);

        commandBuffer.bindVertexBuffers(0, drawCall.vertexBuffer_, {0});
        commandBuffer.bindIndexBuffer(drawCall.indexBuffer_, 0,
                                      vk::IndexType::eUint32);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                         pipelineLayout2d_, 0, descriptorSet,
                                         nullptr);

        commandBuffer.pushConstants<PushConstantObject2d>(
            pipelineLayout2d_, vk::ShaderStageFlagBits::eAllGraphics, 0,
            PushConstantObject2d{.model=drawCall.mat, .view=view2d_, .proj=proj2d_,
                                 .color=glm::vec4(1.0f),
                                 .hasTexture=drawCall.hasTexture ? 1u : 0u});

        commandBuffer.drawIndexed(drawCall.indexCount, 1, 0, 0, 0);
    }

    commandBuffer.setScissor(0, fullScissor);
}


