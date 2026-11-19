#include <PipelineBuilder.hpp>
#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>
#include <logSystem/Logger.hpp>
#include <platformSystem/NativeSDLExtensions.hpp>
#include <platformSystem/NativeVkSurfaceKHR.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <resourceSystem/AssetLoader.hpp>
#include <sceneSystem/Scene.hpp>
#include <types/VulkanVertex3d.hpp>
#include <typesSystem/EngineStats.hpp>

#include "DescriptorSetLayoutBuilder.hpp"
#include "FileSystem.hpp"
#include "resourceSystem/types/Sprite.hpp"
#include "types/VulkanVertex2d.hpp"

struct EngineStats;

constexpr uint32_t MAX_BINDLESS_RESOURCES = 20000;

void VulkanContext::recordCommandBuffer2d(uint32_t imageIndex,
                                          EngineStats& stats) {
    stats.drawcallCount = 0;
    stats.triangleCount = 0;

    auto& cmd = m_commandBuffer[frameIndex];

    cmd.begin({});

    transition_image_layout(m_swapChainImages[imageIndex],
                            vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eColorAttachmentOptimal, {},
                            vk::AccessFlagBits2::eColorAttachmentWrite,
                            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                            vk::ImageAspectFlagBits::eColor);

    vk::ClearValue clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.9f, 1.0f);

    vk::RenderingAttachmentInfo colorAttachment{
        .imageView = m_swapChainImageViews[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearValue};

    vk::RenderingInfo renderingInfo{
        .renderArea = {.offset = {.x = 0, .y = 0}, .extent = m_swapChainExtent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = nullptr};

    cmd.beginRendering(renderingInfo);

    cmd.setViewport(0, vk::Viewport(
        0.0f, 0.0f,
        static_cast<float>(m_swapChainExtent.width),
        static_cast<float>(m_swapChainExtent.height), 0.0f, 1.0f));
    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), m_swapChainExtent));

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline2d_);

    RenderDrawCalls2d(cmd);
    RenderUiDrawCalls(cmd);

    stats.drawcallCount = static_cast<uint32_t>(drawCalls2d_.size() +
                                                uiDrawCalls_.size());
    for (const auto& dc : drawCalls2d_) {
        stats.triangleCount += dc.indexCount / 3;
    }
    for (const auto& dc : uiDrawCalls_) {
        stats.triangleCount += dc.indexCount / 3;
    }

    cmd.endRendering();

    // swapchain image: color attachment → present
    transition_image_layout(m_swapChainImages[imageIndex],
                            vk::ImageLayout::eColorAttachmentOptimal,
                            vk::ImageLayout::ePresentSrcKHR,
                            vk::AccessFlagBits2::eColorAttachmentWrite, {},
                            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                            vk::PipelineStageFlagBits2::eBottomOfPipe,
                            vk::ImageAspectFlagBits::eColor);

    cmd.end();

    drawCalls2d_.clear();
    uiDrawCalls_.clear();
}


void VulkanContext::DrawSprites(EngineStats& stats) {
    auto fenceResult =
        device_.waitForFences(*drawFence[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
        LOG_ERROR("failed to wait for fence!");
        throw std::runtime_error("failed to wait for fence!");
    }

    auto [result, imageIndex] = m_swapchain.acquireNextImage(
        UINT64_MAX, *presentCompleteSemaphore[frameIndex], nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateContext();
        return;
    }
    if (result != vk::Result::eSuccess &&
        result != vk::Result::eSuboptimalKHR) {
        assert(result == vk::Result::eTimeout ||
               result == vk::Result::eNotReady);
        LOG_ERROR("failed to acquire swap chain image!");
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    device_.resetFences(*drawFence[frameIndex]);
    m_commandBuffer[frameIndex].reset();

    recordCommandBuffer2d(imageIndex, stats);

    vk::PipelineStageFlags waitDestinationStageMask {vk::PipelineStageFlagBits::eColorAttachmentOutput};

    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*presentCompleteSemaphore[frameIndex],
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*m_commandBuffer[frameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphore[imageIndex]};

    m_graphicsQueue.submit(submitInfo, *drawFence[frameIndex]);

    vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphore[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &*m_swapchain,
        .pImageIndices = &imageIndex};

    result = m_graphicsQueue.presentKHR(presentInfoKHR);
    if ((result == vk::Result::eSuboptimalKHR) ||
        (result == vk::Result::eErrorOutOfDateKHR)) {
        recreateContext();
    } else {
        assert(result == vk::Result::eSuccess);
    }
    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}


void VulkanContext::AddSpriteToPipeline(const Sprite& sprite) {
    auto& vkSprite = sprites_.at(sprite.handle_);

    drawCalls2d_.push_back({
        .mat=sprite.transform_.GetModelMat(),
        .hasTexture=vkSprite.hasTexture_,
        .color=vkSprite.color_,
        .indexCount=vkSprite.indicesCount_,
        .vertexBuffer_=*vkSprite.vertexBuffer_,
        .indexBuffer_=*vkSprite.indexBuffer_,
        .descriptorSet_=*vkSprite.descriptorSet_
        }
    );
}


void VulkanContext::CreateDescriptorSetForSprite(VkSprite& sprite) {
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = generalDescriptorPool_,
        .descriptorSetCount = 1,
        .pSetLayouts = &*descriptorSetLayout2d_};

    sprite.descriptorSet_ =
        std::move(device_.allocateDescriptorSets(allocInfo)[0]);

    const auto& texture = sprite.texture_;

    vk::DescriptorImageInfo imageInfo{
        .sampler = texture.textureSampler,
        .imageView = texture.textureImageView,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

    std::array writes{vk::WriteDescriptorSet{
        .dstSet = sprite.descriptorSet_,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .pImageInfo = &imageInfo}};
    device_.updateDescriptorSets(writes, {});
}


void VulkanContext::RenderDrawCalls2d(vk::raii::CommandBuffer& commandBuffer) {
    for (const auto& drawCall : drawCalls2d_) {
        commandBuffer.bindVertexBuffers(0, drawCall.vertexBuffer_, {0});
        commandBuffer.bindIndexBuffer(drawCall.indexBuffer_, 0,
                                      vk::IndexType::eUint32);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                         pipelineLayout2d_, 0,
                                         drawCall.descriptorSet_, nullptr);

        commandBuffer.pushConstants<PushConstantObject2d>(
            pipelineLayout2d_, vk::ShaderStageFlagBits::eAllGraphics, 0,
            PushConstantObject2d{drawCall.mat, view2d_, proj2d_, drawCall.color,
                                 1});

        commandBuffer.drawIndexed(drawCall.indexCount, 1, 0, 0, 0);
    }
}


SpriteHandle VulkanContext::AddSprite(const SpriteData& sprite) {
    VkSprite vkSprite;
    vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommand();

    std::vector<vk::raii::Buffer> stagingBuffers;
    std::vector<vk::raii::DeviceMemory> stagingMemories;

    auto [vertexBuffer, vertexBufferMemory] = createUniversalBuffer<Vertex2d>(
        commandBuffer, stagingBuffers, stagingMemories, sprite.vertices_,
        vk::BufferUsageFlagBits::eVertexBuffer);

    vkSprite.vertexBuffer_ = std::move(vertexBuffer);
    vkSprite.vertexBufferMemory_ = std::move(vertexBufferMemory);

    auto [indexBuffer, indexBufferMemory] = createUniversalBuffer<uint32_t>(
        commandBuffer, stagingBuffers, stagingMemories, sprite.indices_,
        vk::BufferUsageFlagBits::eIndexBuffer);

    vkSprite.indexBuffer_ = std::move(indexBuffer);
    vkSprite.indexBufferMemory_ = std::move(indexBufferMemory);
    vkSprite.indicesCount_ = sprite.indices_.size();

    VkTexture texture = CreateVkTextureFromImageData(
        commandBuffer, stagingBuffers, stagingMemories, sprite.image_);

    vkSprite.texture_ = std::move(texture);

    CreateDescriptorSetForSprite(vkSprite);

    endSingleTimeCommand(std::move(commandBuffer));

    sprites_[spriteHandle_] = std::move(vkSprite);

    return spriteHandle_++;
}


