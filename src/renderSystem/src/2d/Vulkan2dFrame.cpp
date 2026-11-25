#include <renderSystem/2d/Vulkan2dContext.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <typesSystem/EngineStats.hpp>

void Vulkan2dContext::RenderFrame(vk::raii::CommandBuffer& commandBuffer,
                                  uint32_t                 imageIndex,
                                  EngineStats&             stats) {
    stats.drawcallCount = 0;
    stats.triangleCount = 0;

    commandBuffer.begin({});

    base_->TransitionImageLayout(
        base_->swapChainImages_[imageIndex],
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::ImageAspectFlagBits::eColor);

    vk::ClearValue clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.9f, 1.0f);

    vk::RenderingAttachmentInfo colorAttachment{
        .imageView = base_->swapChainImageViews_[imageIndex],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearValue};

    vk::RenderingInfo renderingInfo{
        .renderArea = {.offset = {.x = 0, .y = 0},
                       .extent = base_->swapChainExtent_},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = nullptr};

    commandBuffer.beginRendering(renderingInfo);

    commandBuffer.setViewport(
        0,
        vk::Viewport(0.0f,
                     0.0f,
                     static_cast<float>(base_->swapChainExtent_.width),
                     static_cast<float>(base_->swapChainExtent_.height),
                     0.0f,
                     1.0f));
    commandBuffer.setScissor(
        0, vk::Rect2D(vk::Offset2D(0, 0), base_->swapChainExtent_));

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                               graphicsPipeline2d_);

    // Sprites are currently not submitted; the queue holds UI draw calls.
    RenderDrawCalls2d(commandBuffer);

    stats.drawcallCount = static_cast<uint32_t>(drawCalls2d_.size());
    for (const auto& dc : drawCalls2d_) {
        stats.triangleCount += dc.indexCount / 3;
    }

    commandBuffer.endRendering();

    // swapchain image: color attachment → present
    base_->TransitionImageLayout(
        base_->swapChainImages_[imageIndex],
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::ImageAspectFlagBits::eColor);

    commandBuffer.end();

    drawCalls2d_.clear();
}
