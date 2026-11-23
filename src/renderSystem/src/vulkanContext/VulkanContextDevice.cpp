#include <chrono>
#include <cmath>
#include <limits>
#include <logSystem/Logger.hpp>
#include <platformSystem/NativeVkSurfaceKHR.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <resourceSystem/AssetLoader.hpp>

void VulkanContext::SelectPhysicalDevice() {
    const auto physicalDevices = instance_.enumeratePhysicalDevices();
    if (physicalDevices.empty()) {
        LOG_ERROR("Cannot find GPU");
        throw std::runtime_error("Cannot find GPU");
    }

    if (physicalDevice_ == nullptr) {
        LOG_INFO("physical device is null");
    }
    for (auto& physicalDevice : physicalDevices) {
        auto features = physicalDevice.getFeatures();
        if (auto properties = physicalDevice.getProperties();
            properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ||
            properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
            physicalDevice_ = physicalDevice;
        }
    }

    if (physicalDevice_ == nullptr) {
        LOG_ERROR("Physical device is null");
        throw std::runtime_error("Physical device is null");
    }

    LOG_INFO("Selected physical device is: {}",
             physicalDevice_.getProperties().deviceName.data());

    LOG_INFO("Max PushConstantSize is: {}",
             physicalDevice_.getProperties().limits.maxPushConstantsSize);
}


void VulkanContext::CreateLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        physicalDevice_.getQueueFamilyProperties();

    auto graphicsQueueFamilyProperty =
        std::ranges::find_if(queueFamilyProperties, [](const auto& qfp) {
            return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) !=
                   static_cast<vk::QueueFlags>(0);
        });

    queueFamilyIndex_ = static_cast<uint32_t>(std::distance(
        queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

    float queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo {
        .queueFamilyIndex = queueFamilyIndex_,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    vk::PhysicalDeviceFeatures deviceFeatures;
    vk::StructureChain<vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan11Features,
                       vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
                       vk::PhysicalDeviceDescriptorIndexingFeatures>
    featureChain = {
        {.features = {.samplerAnisotropy = vk::True}},
        {.shaderDrawParameters = true},
        {
            .synchronization2 = true,
            .dynamicRendering = true
        },
        {.extendedDynamicState = true},
        {
            .descriptorBindingPartiallyBound = true,
            .runtimeDescriptorArray = true
        }
    };

    const std::vector<const char*> requiredDeviceFeatures {
        vk::KHRSwapchainExtensionName
    };

    vk::DeviceCreateInfo deviceCreateInfo {
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount =
            static_cast<uint32_t>(requiredDeviceFeatures.size()),
        .ppEnabledExtensionNames = requiredDeviceFeatures.data()
    };

    device_ = vk::raii::Device(physicalDevice_, deviceCreateInfo);
    graphicsQueue_ = vk::raii::Queue(device_, queueFamilyIndex_, 0);
}


void VulkanContext::CreateSurface() {
    const VkSurfaceKHR surface =
        GetNativeVkSurfaceKHR(instance_, nativeWindowHandler_);
    surfaceKHR_ = vk::raii::SurfaceKHR(instance_, surface);
}


vk::SurfaceFormatKHR VulkanContext::ChooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    const auto formatIt =
        std::ranges::find_if(availableFormats, [](const auto& format) {
            return format.format == vk::Format::eB8G8R8A8Srgb &&
                   format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}


vk::PresentModeKHR VulkanContext::ChooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) {
        return presentMode == vk::PresentModeKHR::eFifo;
    }));

    return std::ranges::any_of(
        availablePresentModes,
        [](const vk::PresentModeKHR value) {
           return vk::PresentModeKHR::eMailbox == value;
        })
        ? vk::PresentModeKHR::eMailbox
        : vk::PresentModeKHR::eFifo;
}


vk::Extent2D VulkanContext::ChooseSwapExtent(
    const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    std::tie(width_, height_) = extentProvider_.getExtentSizes();
    LOG_ERROR("std::tie(width_, height_) = extentProvider_.GetExtentSizes();");

    return {
        .width = std::clamp<uint32_t>(width_, capabilities.minImageExtent.width,
                                      capabilities.maxImageExtent.width),
        .height =
            std::clamp<uint32_t>(height_, capabilities.minImageExtent.height,
                                 capabilities.maxImageExtent.height)
    };
}


uint32_t VulkanContext::ChooseSwapMinImageCount(
    const vk::SurfaceCapabilitiesKHR& surfaceCapabilities) {
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) &&
        (surfaceCapabilities.maxImageCount < minImageCount)) {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}


void VulkanContext::CreateSwapchain() {
    auto surfaceCapabilities =
        physicalDevice_.getSurfaceCapabilitiesKHR(*surfaceKHR_);

    std::vector<vk::SurfaceFormatKHR> availableFormats =
        physicalDevice_.getSurfaceFormatsKHR(*surfaceKHR_);

    std::vector<vk::PresentModeKHR> availablePresentModes =
        physicalDevice_.getSurfacePresentModesKHR(*surfaceKHR_);

    swapChainSurfaceFormat_ = ChooseSwapSurfaceFormat(availableFormats);
    auto mode = ChooseSwapPresentMode(availablePresentModes);
    swapChainExtent_ = ChooseSwapExtent(surfaceCapabilities);

    uint32_t imageCount = ChooseSwapMinImageCount(surfaceCapabilities);

    vk::SwapchainCreateInfoKHR swapchainCreateInfo{
        .surface = *surfaceKHR_,
        .minImageCount = imageCount,
        .imageFormat = swapChainSurfaceFormat_.format,
        .imageColorSpace = swapChainSurfaceFormat_.colorSpace,
        .imageExtent = swapChainExtent_,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = ChooseSwapPresentMode(availablePresentModes),
        .clipped = true};

    swapChain_ = vk::raii::SwapchainKHR(device_, swapchainCreateInfo);
    swapChainImages_ = swapChain_.getImages();
}


void VulkanContext::CreateImageViews() {
    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = swapChainSurfaceFormat_.format,
        .components = {vk::ComponentSwizzle::eIdentity,
                       vk::ComponentSwizzle::eIdentity,
                       vk::ComponentSwizzle::eIdentity,
                       vk::ComponentSwizzle::eIdentity},
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};

    for (const auto& image : swapChainImages_) {
        imageViewCreateInfo.image = image;
        swapChainImageViews_.emplace_back(device_, imageViewCreateInfo);
    }
}


void VulkanContext::CreateCommandPool() {
    vk::CommandPoolCreateInfo commandPoolCreateInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueFamilyIndex_};

    commandPool_ = vk::raii::CommandPool(device_, commandPoolCreateInfo);
}


void VulkanContext::CreateCommandBuffer() {
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo{
        .commandPool = commandPool_,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT};

    commandBuffer_ =
        vk::raii::CommandBuffers(device_, commandBufferAllocateInfo);
}


void VulkanContext::CreateSyncObjects() {
    assert(presentCompleteSemaphore_.empty() &&
           renderFinishedSemaphore_.empty() && drawFence_.empty());

    for (size_t i = 0; i < swapChainImages_.size(); i++) {
        presentCompleteSemaphore_.emplace_back(
            device_, vk::SemaphoreCreateInfo());
        renderFinishedSemaphore_.emplace_back(
            device_, vk::SemaphoreCreateInfo());
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        drawFence_.emplace_back(vk::raii::Fence(
            device_, {.flags = vk::FenceCreateFlagBits::eSignaled}));
    }
}


void VulkanContext::CleanupSwapChain() {
    depthImageView_ = nullptr;
    depthImage_ = nullptr;
    depthImageMemory_ = nullptr;

    swapChainImageViews_.clear();
    swapChain_ = nullptr;
}


void VulkanContext::RecreateContext() {
    vk::Extent2D extent;
    do {
        auto surfaceCapabilities =
            physicalDevice_.getSurfaceCapabilitiesKHR(*surfaceKHR_);

        extent = ChooseSwapExtent(surfaceCapabilities);

        if (extent.width == 0 || extent.height == 0) {
            // SDL_WaitEvent(nullptr);
        }
    } while (extent.width == 0 || extent.height == 0);

    device_.waitIdle();
    CleanupSwapChain();
    CreateSwapchain();
    CreateImageViews();
    CreateDepthResources();
}


