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

void VulkanContext::createInstance() {
    vk::ApplicationInfo appInfo{.pApplicationName = "Test",
                                .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                .pEngineName = "No Engine",
                                .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                                .apiVersion = vk::ApiVersion14};

    auto layerProperties = m_context.enumerateInstanceLayerProperties();

#ifdef NDEBUG
    const std::vector<char const*> validationLayers = {};
#else
    const std::vector<char const*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"};
#endif

    auto unsupportedLayerIt = std::ranges::find_if(
        validationLayers, [&layerProperties](auto const& requiredLayer) {
            return std::ranges::none_of(
                layerProperties, [requiredLayer](const auto& layerProperty) {
                    return strcmp(layerProperty.layerName, requiredLayer) == 0;
                });
        });

    if (unsupportedLayerIt != validationLayers.end()) {
        LOG_ERROR("Required layer not supported: {}",
                  *unsupportedLayerIt);
        throw std::runtime_error("Required layer not supported: " +
                                 std::string(*unsupportedLayerIt));
    }

    auto sdlExtensions = GetNativeSDLExtensions(nativeWindowHandler_);

    auto extensionProperties = m_context.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < sdlExtensions.size(); ++i) {
        if (std::ranges::none_of(
                extensionProperties, [sdlExtension = sdlExtensions[i]](
                                         const auto& extensionProperty) {
                    return strcmp(extensionProperty.extensionName,
                                  sdlExtension) == 0;
                })) {
            LOG_ERROR("Required SDL extension not supported: {}",
                      sdlExtensions[i]);
            throw std::runtime_error("Required SDL extension not supported: " +
                                     std::string(sdlExtensions[i]));
        }
    }

    vk::InstanceCreateInfo instanceCreateInfo{
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(sdlExtensions.size()),
        .ppEnabledExtensionNames = sdlExtensions.data()};

    instance_ = vk::raii::Instance(m_context, instanceCreateInfo);
}


void VulkanContext::selectPhysicalDevice() {
    auto physicalDevices = instance_.enumeratePhysicalDevices();
    if (physicalDevices.empty()) {
        LOG_ERROR("Cannot find GPU");
        throw std::runtime_error("Cannot find GPU");
    }

    if (m_physicalDevice == nullptr) {
        LOG_INFO("physical device is null");
    }
    for (auto& physicalDevice : physicalDevices) {
        auto features = physicalDevice.getFeatures();
        auto properties = physicalDevice.getProperties();
        if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ||
            properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
            m_physicalDevice = physicalDevice;
        }
    }

    if (m_physicalDevice == nullptr) {
        LOG_ERROR("Physical device is null");
        throw std::runtime_error("Physical device is null");
    }

    LOG_INFO("Selected physical device is: {}",
             m_physicalDevice.getProperties().deviceName.data());

    LOG_INFO("Max PushConstantSize is: {}",
             m_physicalDevice.getProperties().limits.maxPushConstantsSize);
}


void VulkanContext::createLogicalDevice() {
    std::vector<vk::QueueFamilyProperties> queueFamilyProperties =
        m_physicalDevice.getQueueFamilyProperties();
    auto graphicsQueueFamilyProperty =
        std::ranges::find_if(queueFamilyProperties, [](const auto& qfp) {
            return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) !=
                   static_cast<vk::QueueFlags>(0);
        });
    queueFamilyIndex = static_cast<uint32_t>(std::distance(
        queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

    float queuePriority = 0.5f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{
        .queueFamilyIndex = queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority};

    vk::PhysicalDeviceFeatures deviceFeatures;
    vk::StructureChain<vk::PhysicalDeviceFeatures2,
                       vk::PhysicalDeviceVulkan11Features,
                       vk::PhysicalDeviceVulkan13Features,
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
                       vk::PhysicalDeviceDescriptorIndexingFeatures>
        featureChain = {{
                            .features = {.samplerAnisotropy = vk::True},
                        },
                        {.shaderDrawParameters = true},
                        {.synchronization2 = true, .dynamicRendering = true},
                        {.extendedDynamicState = true},
                        {.descriptorBindingPartiallyBound = true,
                         .runtimeDescriptorArray = true}};

    const std::vector<const char*> requiredDeviceFeatures{
        vk::KHRSwapchainExtensionName};

    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount =
            static_cast<uint32_t>(requiredDeviceFeatures.size()),
        .ppEnabledExtensionNames = requiredDeviceFeatures.data()};

    device_ = vk::raii::Device(m_physicalDevice, deviceCreateInfo);
    m_graphicsQueue = vk::raii::Queue(device_, queueFamilyIndex, 0);
}


void VulkanContext::createSurface() {
    const VkSurfaceKHR surface =
        GetNativeVkSurfaceKHR(instance_, nativeWindowHandler_);
    m_surfaceKHR = vk::raii::SurfaceKHR(instance_, surface);
}


vk::SurfaceFormatKHR VulkanContext::chooseSwapSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    const auto formatIt =
        std::ranges::find_if(availableFormats, [](const auto& format) {
            return format.format == vk::Format::eB8G8R8A8Srgb &&
                   format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
        });
    return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
}


vk::PresentModeKHR VulkanContext::chooseSwapPresentMode(
    const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    assert(std::ranges::any_of(availablePresentModes, [](auto presentMode) {
        return presentMode == vk::PresentModeKHR::eFifo;
    }));
    return std::ranges::any_of(availablePresentModes,
                               [](const vk::PresentModeKHR value) {
                                   return vk::PresentModeKHR::eMailbox == value;
                               })
               ? vk::PresentModeKHR::eMailbox
               : vk::PresentModeKHR::eFifo;
}


vk::Extent2D VulkanContext::chooseSwapExtent(
    const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    std::tie(width_, height_) = extentProvider_.GetExtentSizes();
    LOG_ERROR("std::tie(width_, height_) = extentProvider_.GetExtentSizes();");
    return {
        .width = std::clamp<uint32_t>(width_, capabilities.minImageExtent.width,
                                      capabilities.maxImageExtent.width),
        .height =
            std::clamp<uint32_t>(height_, capabilities.minImageExtent.height,
                                 capabilities.maxImageExtent.height)};
}


uint32_t VulkanContext::chooseSwapMinImageCount(
    const vk::SurfaceCapabilitiesKHR& surfaceCapabilities) {
    auto minImageCount = std::max(3u, surfaceCapabilities.minImageCount);
    if ((0 < surfaceCapabilities.maxImageCount) &&
        (surfaceCapabilities.maxImageCount < minImageCount)) {
        minImageCount = surfaceCapabilities.maxImageCount;
    }
    return minImageCount;
}


void VulkanContext::createSwapchain() {
    auto surfaceCapabilities =
        m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surfaceKHR);
    std::vector<vk::SurfaceFormatKHR> availableFormats =
        m_physicalDevice.getSurfaceFormatsKHR(*m_surfaceKHR);
    std::vector<vk::PresentModeKHR> availablePresentModes =
        m_physicalDevice.getSurfacePresentModesKHR(*m_surfaceKHR);

    m_swapChainSurfaceFormat = chooseSwapSurfaceFormat(availableFormats);
    auto mode = chooseSwapPresentMode(availablePresentModes);
    m_swapChainExtent = chooseSwapExtent(surfaceCapabilities);

    uint32_t imageCount = chooseSwapMinImageCount(surfaceCapabilities);

    vk::SwapchainCreateInfoKHR swapchainCreateInfo{
        .surface = *m_surfaceKHR,
        .minImageCount = imageCount,
        .imageFormat = m_swapChainSurfaceFormat.format,
        .imageColorSpace = m_swapChainSurfaceFormat.colorSpace,
        .imageExtent = m_swapChainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = chooseSwapPresentMode(availablePresentModes),
        .clipped = true};

    m_swapchain = vk::raii::SwapchainKHR(device_, swapchainCreateInfo);
    m_swapChainImages = m_swapchain.getImages();
}


void VulkanContext::createImageViews() {
    vk::ImageViewCreateInfo imageViewCreateInfo{
        .viewType = vk::ImageViewType::e2D,
        .format = m_swapChainSurfaceFormat.format,
        .components = {vk::ComponentSwizzle::eIdentity,
                       vk::ComponentSwizzle::eIdentity,
                       vk::ComponentSwizzle::eIdentity,
                       vk::ComponentSwizzle::eIdentity},
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eColor,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};

    for (const auto& image : m_swapChainImages) {
        imageViewCreateInfo.image = image;
        m_swapChainImageViews.emplace_back(device_, imageViewCreateInfo);
    }
}


void VulkanContext::createCommandPool() {
    vk::CommandPoolCreateInfo commandPoolCreateInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueFamilyIndex};

    m_commandPool = vk::raii::CommandPool(device_, commandPoolCreateInfo);
}


void VulkanContext::createCommandBuffer() {
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo{
        .commandPool = m_commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT};

    m_commandBuffer =
        vk::raii::CommandBuffers(device_, commandBufferAllocateInfo);
}


void VulkanContext::createSyncObjects() {
    assert(presentCompleteSemaphore.empty() &&
           renderFinishedSemaphore.empty() && drawFence.empty());

    for (size_t i = 0; i < m_swapChainImages.size(); i++) {
        presentCompleteSemaphore.emplace_back(
            vk::raii::Semaphore(device_, vk::SemaphoreCreateInfo()));
        renderFinishedSemaphore.emplace_back(
            vk::raii::Semaphore(device_, vk::SemaphoreCreateInfo()));
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        drawFence.emplace_back(vk::raii::Fence(
            device_, {.flags = vk::FenceCreateFlagBits::eSignaled}));
    }
}


void VulkanContext::cleanupSwapChain() {
    depthImageView = nullptr;
    depthImage = nullptr;
    depthImageMemory = nullptr;

    m_swapChainImageViews.clear();
    m_swapchain = nullptr;
}


void VulkanContext::recreateContext() {
    vk::Extent2D extent;
    do {
        auto surfaceCapabilities =
            m_physicalDevice.getSurfaceCapabilitiesKHR(*m_surfaceKHR);

        extent = chooseSwapExtent(surfaceCapabilities);

        if (extent.width == 0 || extent.height == 0) {
            // SDL_WaitEvent(nullptr);
        }
    } while (extent.width == 0 || extent.height == 0);

    device_.waitIdle();
    cleanupSwapChain();
    createSwapchain();
    createImageViews();
    createDepthResources();
}


