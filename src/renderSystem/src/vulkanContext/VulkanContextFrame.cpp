#include <logSystem/Logger.hpp>
#include <renderSystem/2d/Vulkan2dContext.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <typesSystem/EngineStats.hpp>

void VulkanContext::DrawUi(EngineStats& stats, Vulkan2dContext& context) {
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

    context.RenderFrame(commandBuffer_[frameIndex_], imageIndex, stats);

    vk::PipelineStageFlags waitDestinationStageMask{
        vk::PipelineStageFlagBits::eColorAttachmentOutput};

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
