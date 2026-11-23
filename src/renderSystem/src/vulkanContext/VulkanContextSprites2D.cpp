#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>
#include <logSystem/Logger.hpp>
#include <platformSystem/NativeVkSurfaceKHR.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <resourceSystem/AssetLoader.hpp>
#include <sceneSystem/Scene.hpp>
#include <typesSystem/EngineStats.hpp>
#include "resourceSystem/types/Sprite.hpp"


void VulkanContext::RecordCommandBuffer2d(uint32_t imageIndex,
                                          EngineStats& stats) {
    stats.drawcallCount = 0;
    stats.triangleCount = 0;

    auto& cmd = commandBuffer_[frameIndex_];

    cmd.begin({});

    TransitionImageLayout(swapChainImages_[imageIndex],
                            vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eColorAttachmentOptimal, {},
                            vk::AccessFlagBits2::eColorAttachmentWrite,
                            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                            vk::ImageAspectFlagBits::eColor);

    vk::ClearValue clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.9f, 1.0f);

    vk::RenderingAttachmentInfo colorAttachment{
        .imageView = swapChainImageViews_[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearValue};

    vk::RenderingInfo renderingInfo{
        .renderArea = {.offset = {.x = 0, .y = 0}, .extent = swapChainExtent_},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = nullptr};

    cmd.beginRendering(renderingInfo);

    cmd.setViewport(0, vk::Viewport(
        0.0f, 0.0f,
        static_cast<float>(swapChainExtent_.width),
        static_cast<float>(swapChainExtent_.height), 0.0f, 1.0f));
    cmd.setScissor(0, vk::Rect2D(vk::Offset2D(0, 0), swapChainExtent_));

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline2d_);

   // RenderDrawCalls2d(cmd);
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
    TransitionImageLayout(swapChainImages_[imageIndex],
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
        device_.waitForFences(*drawFence_[frameIndex_], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
        LOG_ERROR("failed to wait for fence!");
        throw std::runtime_error("failed to wait for fence!");
    }

    auto [result, imageIndex] = swapChain_.acquireNextImage(
        UINT64_MAX, *presentCompleteSemaphore_[frameIndex_], nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        RecreateContext();
        return;
    }
    if (result != vk::Result::eSuccess &&
        result != vk::Result::eSuboptimalKHR) {
        assert(result == vk::Result::eTimeout ||
               result == vk::Result::eNotReady);
        LOG_ERROR("failed to acquire swap chain image!");
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    device_.resetFences(*drawFence_[frameIndex_]);
    commandBuffer_[frameIndex_].reset();

    RecordCommandBuffer2d(imageIndex, stats);

    vk::PipelineStageFlags waitDestinationStageMask {vk::PipelineStageFlagBits::eColorAttachmentOutput};

    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*presentCompleteSemaphore_[frameIndex_],
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*commandBuffer_[frameIndex_],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphore_[imageIndex]};

    graphicsQueue_.submit(submitInfo, *drawFence_[frameIndex_]);

    vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphore_[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &*swapChain_,
        .pImageIndices = &imageIndex};

    result = graphicsQueue_.presentKHR(presentInfoKHR);
    if ((result == vk::Result::eSuboptimalKHR) ||
        (result == vk::Result::eErrorOutOfDateKHR)) {
        RecreateContext();
    } else {
        assert(result == vk::Result::eSuccess);
    }
    frameIndex_ = (frameIndex_ + 1) % MAX_FRAMES_IN_FLIGHT;
}


void VulkanContext::AddSpriteToPipeline(const Sprite& sprite) {
    auto& vkSprite = sprites_.at(sprite.handle);

    drawCalls2d_.push_back({
        .mat=sprite.transform.GetModelMat(),
        .hasTexture=vkSprite.hasTexture,
        .color=vkSprite.color,
        .indexCount=vkSprite.indicesCount,
        .vertexBuffer_=*vkSprite.vertexBuffer,
        .indexBuffer_=*vkSprite.indexBuffer,
        .descriptorSet_=*vkSprite.descriptorSet
        }
    );
}


void VulkanContext::CreateDescriptorSetForSprite(VkSprite& sprite) const {
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = generalDescriptorPool_,
        .descriptorSetCount = 1,
        .pSetLayouts = &*descriptorSetLayout2d_};

    sprite.descriptorSet =
        std::move(device_.allocateDescriptorSets(allocInfo)[0]);

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
    device_.updateDescriptorSets(writes, {});
}


void VulkanContext::RenderDrawCalls2d(vk::raii::CommandBuffer& commandBuffer) const {
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
    vk::raii::CommandBuffer commandBuffer = BeginSingleTimeCommand();

    std::vector<vk::raii::Buffer> stagingBuffers;
    std::vector<vk::raii::DeviceMemory> stagingMemories;

    auto [vertexBuffer, vertexBufferMemory] = CreateUniversalBuffer<Vertex2d>(
        commandBuffer, stagingBuffers, stagingMemories, sprite.vertices,
        vk::BufferUsageFlagBits::eVertexBuffer);

    vkSprite.vertexBuffer = std::move(vertexBuffer);
    vkSprite.vertexBufferMemory = std::move(vertexBufferMemory);

    auto [indexBuffer, indexBufferMemory] = CreateUniversalBuffer<uint32_t>(
        commandBuffer, stagingBuffers, stagingMemories, sprite.indices,
        vk::BufferUsageFlagBits::eIndexBuffer);

    vkSprite.indexBuffer = std::move(indexBuffer);
    vkSprite.indexBufferMemory = std::move(indexBufferMemory);
    vkSprite.indicesCount = sprite.indices.size();

    VkTexture texture = CreateVkTextureFromImageData(
        commandBuffer, stagingBuffers, stagingMemories, sprite.image);

    vkSprite.texture = std::move(texture);

    CreateDescriptorSetForSprite(vkSprite);

    EndSingleTimeCommand(std::move(commandBuffer));

    sprites_[spriteHandle_] = std::move(vkSprite);

    return spriteHandle_++;
}


