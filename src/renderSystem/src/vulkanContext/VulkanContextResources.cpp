
#include <chrono>
#include <cmath>
#include <fstream>
#include <logSystem/Logger.hpp>
#include <platformSystem/NativeVkSurfaceKHR.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <resourceSystem/AssetLoader.hpp>

void VulkanContext::CreateDepthResources() {
    depthFormat_ = FindDepthFormat();
    std::tie(depthImage_, depthImageMemory_) =
        CreateImage(swapChainExtent_.width,
                    swapChainExtent_.height,
                    depthFormat_,
                    vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eDepthStencilAttachment,
                    vk::MemoryPropertyFlagBits::eDeviceLocal);
    depthImageView_ = CreateImageView(
        depthImage_, depthFormat_, vk::ImageAspectFlagBits::eDepth);
}

std::pair<vk::raii::Image, vk::raii::DeviceMemory> VulkanContext::CreateImage(
    uint32_t                width,
    uint32_t                height,
    vk::Format              format,
    vk::ImageTiling         tiling,
    vk::ImageUsageFlags     usage,
    vk::MemoryPropertyFlags properties) {
    vk::ImageCreateInfo imageInfo{
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {.width = width, .height = height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = tiling,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive};

    auto image = vk::raii::Image(device_, imageInfo);

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            FindMemoryType(memRequirements.memoryTypeBits, properties)};
    auto imageMemory = vk::raii::DeviceMemory(device_, allocInfo);
    image.bindMemory(imageMemory, 0);

    return {std::move(image), std::move(imageMemory)};
}

vk::raii::ImageView VulkanContext::CreateImageView(
    const vk::Image&     image,
    vk::Format           format,
    vk::ImageAspectFlags aspectFlags) const {
    vk::ImageViewCreateInfo viewInfo{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {.aspectMask = aspectFlags,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};
    return {device_, viewInfo};
}

uint32_t VulkanContext::FindMemoryType(
    uint32_t typeFilter, vk::MemoryPropertyFlags properties) const {
    vk::PhysicalDeviceMemoryProperties memProperties =
        physicalDevice_.getMemoryProperties();

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1u << i) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) ==
                properties) {
            return i;
        }
    }

    LOG_ERROR("failed to find suitable memory type!");
    throw std::runtime_error("failed to find suitable memory type!");
}

void VulkanContext::CopyBufferToImage(vk::raii::CommandBuffer& commandBuffer,
                                      const vk::raii::Buffer&  buffer,
                                      vk::raii::Image&         image,
                                      uint32_t                 width,
                                      uint32_t                 height) {
    vk::BufferImageCopy region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                             .mipLevel = 0,
                             .baseArrayLayer = 0,
                             .layerCount = 1},
        .imageOffset = {0, 0, 0},
        .imageExtent = {width, height, 1}};
    commandBuffer.copyBufferToImage(
        buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
}

void VulkanContext::TransitionImageLayout(
    vk::Image               image,
    vk::ImageLayout         oldLayout,
    vk::ImageLayout         newLayout,
    vk::AccessFlags2        srcAccessMask,
    vk::AccessFlags2        dstAccessMask,
    vk::PipelineStageFlags2 srcStageMask,
    vk::PipelineStageFlags2 dstStageMask,
    vk::ImageAspectFlags    imageAspectFlags) const {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = srcStageMask,
        .srcAccessMask = srcAccessMask,
        .dstStageMask = dstStageMask,
        .dstAccessMask = dstAccessMask,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {.aspectMask = imageAspectFlags,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};
    vk::DependencyInfo dependencyInfo = {.dependencyFlags = {},
                                         .imageMemoryBarrierCount = 1,
                                         .pImageMemoryBarriers = &barrier};
    commandBuffer_[frameIndex_].pipelineBarrier2(dependencyInfo);
}

void VulkanContext::TransitionImageLayout(
    vk::raii::CommandBuffer& commandBuffer,
    const vk::raii::Image&   image,
    vk::ImageLayout          oldLayout,
    vk::ImageLayout          newLayout) {
    vk::ImageMemoryBarrier barrier = {
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
        .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
        .image = image,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                             .levelCount = 1,
                             .layerCount = 1}};

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined &&
        newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
               newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (oldLayout == vk::ImageLayout::eUndefined &&
               newLayout == vk::ImageLayout::eColorAttachmentOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    } else if (oldLayout == vk::ImageLayout::eColorAttachmentOptimal &&
               newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        sourceStage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        LOG_ERROR("unsupported layout transition!");
        throw std::invalid_argument("unsupported layout transition!");
    }
    commandBuffer.pipelineBarrier(
        sourceStage, destinationStage, {}, {}, nullptr, barrier);
}

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> VulkanContext::CreateBuffer(
    vk::DeviceSize          size,
    vk::BufferUsageFlags    usage,
    vk::MemoryPropertyFlags properties) const {
    vk::BufferCreateInfo bufferCreateInfo{
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    auto buffer = vk::raii::Buffer(device_, bufferCreateInfo);

    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo memoryAllocateInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            FindMemoryType(memRequirements.memoryTypeBits, properties)};

    auto bufferMemory = vk::raii::DeviceMemory(device_, memoryAllocateInfo);
    buffer.bindMemory(*bufferMemory, 0);

    return {std::move(buffer), std::move(bufferMemory)};
}

vk::raii::CommandBuffer VulkanContext::BeginSingleTimeCommand() const {
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = commandPool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1};
    vk::raii::CommandBuffer commandBuffer =
        std::move(vk::raii::CommandBuffers(device_, allocInfo).front());

    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void VulkanContext::EndSingleTimeCommand(
    vk::raii::CommandBuffer&& commandBuffer) const {
    commandBuffer.end();

    vk::SubmitInfo submitInfo{.commandBufferCount = 1,
                              .pCommandBuffers = &*commandBuffer};
    graphicsQueue_.submit(submitInfo, nullptr);
    graphicsQueue_.waitIdle();
}

vk::Format VulkanContext::FindSupportedFormat(
    const std::vector<vk::Format>& candidates,
    vk::ImageTiling                tiling,
    vk::FormatFeatureFlags         features) const {
    for (const auto format : candidates) {
        vk::FormatProperties props =
            physicalDevice_.getFormatProperties(format);

        if (((tiling == vk::ImageTiling::eLinear) &&
             ((props.linearTilingFeatures & features) == features)) ||
            ((tiling == vk::ImageTiling::eOptimal) &&
             ((props.optimalTilingFeatures & features) == features))) {
            return format;
        }
    }

    LOG_ERROR("failed to find supported format!");
    throw std::runtime_error("failed to find supported format!");
}

vk::Format VulkanContext::FindDepthFormat() const {
    return FindSupportedFormat(
        {vk::Format::eD32Sfloat,
         vk::Format::eD32SfloatS8Uint,
         vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

VkTexture VulkanContext::CreateVkTextureFromImageData(
    vk::raii::CommandBuffer&             commandBuffer,
    std::vector<vk::raii::Buffer>&       stagingBuffers,
    std::vector<vk::raii::DeviceMemory>& stagingMemories,
    const ImageData&                     imageData) {
    VkTexture texture;
    auto      imageSize = static_cast<vk::DeviceSize>(
        imageData.width * imageData.height * imageData.channels);

    auto [stagingBuffer, stagingBufferMemory] =
        CreateBuffer(imageSize,
                     vk::BufferUsageFlagBits::eTransferSrc,
                     vk::MemoryPropertyFlagBits::eHostVisible |
                         vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data = stagingBufferMemory.mapMemory(0, imageSize);

    memcpy(data, imageData.data, imageSize);
    stagingBufferMemory.unmapMemory();

    std::tie(texture.textureImage, texture.textureImageMemory) = CreateImage(
        imageData.width,
        imageData.height,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    TransitionImageLayout(commandBuffer,
                          texture.textureImage,
                          vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eTransferDstOptimal);
    CopyBufferToImage(commandBuffer,
                      stagingBuffer,
                      texture.textureImage,
                      imageData.width,
                      imageData.height);
    TransitionImageLayout(commandBuffer,
                          texture.textureImage,
                          vk::ImageLayout::eTransferDstOptimal,
                          vk::ImageLayout::eShaderReadOnlyOptimal);

    texture.textureImageView = CreateImageView(texture.textureImage,
                                               vk::Format::eR8G8B8A8Srgb,
                                               vk::ImageAspectFlagBits::eColor);

    vk::SamplerCreateInfo samplerInfo{};
    texture.textureSampler = vk::raii::Sampler(device_, samplerInfo);

    stagingBuffers.push_back(std::move(stagingBuffer));
    stagingMemories.push_back(std::move(stagingBufferMemory));
    textureCounter_++;

    return texture;
}
