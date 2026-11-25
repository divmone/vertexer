#include <renderSystem/VulkanContext.hpp>
#include <renderSystem/detail/DescriptorPool.hpp>
#include <resourceSystem/AssetLoader.hpp>
#include <sceneSystem/Scene.hpp>
#include <utility>

#include "logSystem/Logger.hpp"
#include "platformSystem/NativeSDLExtensions.hpp"

VulkanContext::VulkanContext(const NativeWindowHandler& windowHandler,
                             ExtentProvider             extentProvider)
    : nativeWindowHandler_(windowHandler),
      extentProvider_(std::move(extentProvider)) {
    InitVulkan();
}

VulkanContext::~VulkanContext() {
    device_.waitIdle();
    CleanupSwapChain();
}

void VulkanContext::InitVulkan() {
    CreateInstance();
    CreateSurface();
    SelectPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
    CreateDepthResources();
    CreateCommandPool();
    CreateCommandBuffer();
    CreateSyncObjects();
    CreateDescriptorPool();
}

void VulkanContext::CreateInstance() {
    vk::ApplicationInfo appInfo{.pApplicationName = "Test",
                                .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
                                .pEngineName = "No Engine",
                                .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                                .apiVersion = vk::ApiVersion14};

    auto layerProperties = context_.enumerateInstanceLayerProperties();

#ifdef NDEBUG
    const std::vector<const char*> validationLayers = {};
#else
    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"};
#endif

    auto unsupportedLayerIt = std::ranges::find_if(
        validationLayers, [&layerProperties](const auto& requiredLayer) {
            return std::ranges::none_of(
                layerProperties, [requiredLayer](const auto& layerProperty) {
                    return strcmp(layerProperty.layerName, requiredLayer) == 0;
                });
        });

    if (unsupportedLayerIt != validationLayers.end()) {
        LOG_ERROR("Required layer not supported: {}", *unsupportedLayerIt);
        throw std::runtime_error("Required layer not supported: " +
                                 std::string(*unsupportedLayerIt));
    }

    auto sdlExtensions = GetNativeSDLExtensions(nativeWindowHandler_);

    auto extensionProperties = context_.enumerateInstanceExtensionProperties();
    for (auto& i : sdlExtensions) {
        if (std::ranges::none_of(
                extensionProperties,
                [sdlExtension = i](const auto& extensionProperty) {
                    return strcmp(extensionProperty.extensionName,
                                  sdlExtension) == 0;
                })) {
            LOG_ERROR("Required SDL extension not supported: {}", i);
            throw std::runtime_error("Required SDL extension not supported: " +
                                     std::string(i));
        }
    }

    vk::InstanceCreateInfo instanceCreateInfo{
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
        .ppEnabledLayerNames = validationLayers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(sdlExtensions.size()),
        .ppEnabledExtensionNames = sdlExtensions.data()};

    instance_ = vk::raii::Instance(context_, instanceCreateInfo);
}

void VulkanContext::ProcessMinimizedWindow() {
    // int width = 0, height = 0;
    // while (width == 0 || height == 0) {
    //     SDL_GetWindowSizeInPixels(m_window, &width, &height);
    //     SDL_WaitEvent(nullptr);
    // }
}

void VulkanContext::CreateTexture() {}
