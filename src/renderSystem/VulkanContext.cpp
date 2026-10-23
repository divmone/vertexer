#include "VulkanContext.hpp"

#include <cmath>
#include <fstream>
#include <limits>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <Logger.hpp>
#include <chrono>
#include <glm/gtx/norm.hpp>

#include "AssetLoader.hpp"
#include "EngineStats.hpp"
#include "PipelineBuilder.hpp"
#include "Scene.hpp"
#include "types/VulkanVertex.hpp"

struct EngineStats;

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

VulkanContext::VulkanContext(NativeWindowHandler windowHandler,
                             const ExtentProvider& extentProvider)
    : nativeWindowHandler_(windowHandler), extentProvider_(extentProvider) {
    initVulkan();
    createImGuiDescriptorPool();
    initImGui();
}

VulkanContext::~VulkanContext() {
    m_device.waitIdle();
    cleanupSwapChain();
}

void VulkanContext::initVulkan() {
    createInstance();
    createSurface();
    selectPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createImageViews();
    createDescriptorSetLayout();
    createBlendDescriptorSetLayout();
    createDepthResources();
    createGraphicsPipeline();
    createBlendingPipeline();
    createCommandPool();
    createUniformBuffers();

    createCommandBuffer();
    createSyncObjects();
}

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
        throw std::runtime_error("Required layer not supported: " +
                                 std::string(*unsupportedLayerIt));
    }

    auto sdlExtensions = std::visit(
        overloaded{
            [&](const Win32WindowHandler&) -> std::vector<const char*> {
                return {"VK_KHR_surface", "VK_KHR_win32_surface"};
            },
            [&](const WaylandWindowHandler&) -> std::vector<const char*> {
                return {};
            },
            [&](const X11WindowHandler&) -> std::vector<const char*> {
                return {};
            },
            [&](const MacOSWindowHandler&) -> std::vector<const char*> {
                return {};
            },
        },
        nativeWindowHandler_);

    auto extensionProperties = m_context.enumerateInstanceExtensionProperties();
    for (uint32_t i = 0; i < sdlExtensions.size(); ++i) {
        if (std::ranges::none_of(
                extensionProperties, [sdlExtension = sdlExtensions[i]](
                                         const auto& extensionProperty) {
                    return strcmp(extensionProperty.extensionName,
                                  sdlExtension) == 0;
                })) {
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
        throw std::runtime_error("Cannot find GPU");
    }

    for (auto& physicalDevice : physicalDevices) {
        auto features = physicalDevice.getFeatures();
        auto properties = physicalDevice.getProperties();
        if ((properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ||
             properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) &&
            features.geometryShader) {
            m_physicalDevice = physicalDevice;
        }
    }

    LOG_INFO("Selected physical device is: {}",
             m_physicalDevice.getProperties().deviceName.data());
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
                       vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
        featureChain = {{
                            .features = {.samplerAnisotropy = vk::True},
                        },
                        {.shaderDrawParameters = true},
                        {.synchronization2 = true, .dynamicRendering = true},
                        {.extendedDynamicState = true}};

    const std::vector<const char*> requiredDeviceFeatures{
        vk::KHRSwapchainExtensionName};

    vk::DeviceCreateInfo deviceCreateInfo{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &deviceQueueCreateInfo,
        .enabledExtensionCount =
            static_cast<uint32_t>(requiredDeviceFeatures.size()),
        .ppEnabledExtensionNames = requiredDeviceFeatures.data()};

    m_device = vk::raii::Device(m_physicalDevice, deviceCreateInfo);
    m_graphicsQueue = vk::raii::Queue(m_device, queueFamilyIndex, 0);
}

void VulkanContext::createSurface() {
    const VkSurfaceKHR surface = std::visit(
        overloaded{
            [&](const Win32WindowHandler& h) -> VkSurfaceKHR {
                VkSurfaceKHR s = VK_NULL_HANDLE;
#if defined(_WIN64)
                VkWin32SurfaceCreateInfoKHR ci{
                    VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
                ci.hinstance = static_cast<HINSTANCE>(h.hInstance);
                ci.hwnd = static_cast<HWND>(h.hWnd);
                vkCreateWin32SurfaceKHR(*instance_, &ci, nullptr, &s);
                return s;
#endif
            },
            [&](const WaylandWindowHandler&) -> VkSurfaceKHR {
                return nullptr;
            },
            [&](const X11WindowHandler&) -> VkSurfaceKHR { return nullptr; },
            [&](const MacOSWindowHandler&) -> VkSurfaceKHR { return nullptr; },
        },
        nativeWindowHandler_);

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

    m_swapchain = vk::raii::SwapchainKHR(m_device, swapchainCreateInfo);
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
        m_swapChainImageViews.emplace_back(m_device, imageViewCreateInfo);
    }
}

void VulkanContext::createGraphicsPipeline() {
    auto shaderModule =
        createShaderModule(loadShader("../../../shaders/shader.spv"));

    m_pipelineLayout = PipelineBuilder::createPipelineLayout(
        m_device,
        {{
            .stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
            .offset = 0,
            .size = sizeof(PushConstantObject),
        }},
        {*descriptorSetLayout, *blendingDescriptorSetLayout});
    PipelineBuilder builder;
    m_graphicsPipeline = builder.shaders(shaderModule)
                             .colorFormat(m_swapChainSurfaceFormat.format)
                             .depthFormat(depthFormat)
                             .vertexType<VulkanVertex>()
                             .depthTestOn()
                             .layout(m_pipelineLayout)
                             .build(m_device);

    PipelineBuilder blendPipelineBuilder;
    blendingPipeline = blendPipelineBuilder.shaders(shaderModule)
                           .colorFormat(m_swapChainSurfaceFormat.format)
                           .depthFormat(depthFormat)
                           .vertexType<VulkanVertex>()
                           .blendingOn()
                           .depthTestOff()
                           .layout(m_pipelineLayout)
                           .build(m_device);

    shaderModule = createShaderModule(loadShader("../../../shaders/sky.spv"));
    skyPipelineLayout = PipelineBuilder::createPipelineLayout(
        m_device, nullptr,
        {*descriptorSetLayout, *blendingDescriptorSetLayout});

    PipelineBuilder skyPipelineBuilder;
    blendingPipeline = skyPipelineBuilder.shaders(shaderModule)
                           .colorFormat(m_swapChainSurfaceFormat.format)
                           .depthFormat(depthFormat)
                           .vertexType<VulkanVertex>()
                           .layout(m_pipelineLayout)
                           .build(m_device);
}

void VulkanContext::createBlendingPipeline() {
    // auto shaderModule =
    // createShaderModule(loadShader("../shaders/shader.spv"));
    // vk::PipelineShaderStageCreateInfo vertexShaderStageCreateInfo{
    //     .stage = vk::ShaderStageFlagBits::eVertex,
    //     .module = shaderModule,
    //     .pName = "vertMain"
    // };
    //
    // vk::PipelineShaderStageCreateInfo fragmentShaderStageCreateInfo{
    //     .stage = vk::ShaderStageFlagBits::eFragment,
    //     .module = shaderModule,
    //     .pName = "fragMain"
    // };
    //
    // vk::PipelineShaderStageCreateInfo stages[] =
    // {vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo};
    //
    // std::vector<vk::DynamicState> dynamicStates =
    // {vk::DynamicState::eViewport, vk::DynamicState::eScissor};
    //
    // vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1,
    // .scissorCount = 1};
    //
    // vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{
    //     .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
    //     .pDynamicStates = dynamicStates.data()
    // };
    //
    // auto bindingDescription = Vertex::getBindingDescription();
    // auto attributeDescription = Vertex::getAttributeDescriptions();
    //
    // vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{
    //     .vertexBindingDescriptionCount = 1,
    //     .pVertexBindingDescriptions = &bindingDescription,
    //     .vertexAttributeDescriptionCount = attributeDescription.size(),
    //     .pVertexAttributeDescriptions = attributeDescription.data(),
    // };
    //
    // vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{
    //     .topology = vk::PrimitiveTopology::eTriangleList
    // };
    //
    // vk::Viewport viewport{
    //     0, 0, static_cast<float>(m_swapChainExtent.width),
    //     static_cast<float>(m_swapChainExtent.height), 0, 1
    // };
    //
    // vk::Rect2D scissor{vk::Offset2D{0, 0}, m_swapChainExtent};
    //
    // vk::PipelineRasterizationStateCreateInfo rasterizer{
    //     .depthClampEnable = vk::False,
    //     .rasterizerDiscardEnable = vk::False,
    //     .polygonMode = vk::PolygonMode::eFill,
    //     .cullMode = vk::CullModeFlagBits::eBack,
    //     .frontFace = vk::FrontFace::eCounterClockwise,
    //     .depthBiasEnable = vk::True,
    //     .lineWidth = 1.0f
    // };
    //
    // vk::PipelineMultisampleStateCreateInfo multisampling{
    //     .rasterizationSamples = vk::SampleCountFlagBits::e1,
    //     .sampleShadingEnable = vk::False
    // };
    //
    // vk::PipelineColorBlendAttachmentState colorBlendAttachment{
    //     .blendEnable = vk::True,
    //     .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
    //     .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
    //     .colorBlendOp = vk::BlendOp::eAdd,
    //     .srcAlphaBlendFactor = vk::BlendFactor::eOne,
    //     .dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
    //     .alphaBlendOp = vk::BlendOp::eAdd,
    //     .colorWriteMask = vk::ColorComponentFlagBits::eR |
    //     vk::ColorComponentFlagBits::eG |
    //                       vk::ColorComponentFlagBits::eB |
    //                       vk::ColorComponentFlagBits::eA
    // };
    //
    // vk::PipelineColorBlendStateCreateInfo colorBlending{
    //     .logicOpEnable = vk::False, .logicOp = vk::LogicOp::eCopy,
    //     .attachmentCount = 1, .pAttachments = &colorBlendAttachment
    // };
    //
    // vk::PushConstantRange range{
    //     .stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
    //     .offset = 0,
    //     .size = sizeof(PushConstantObject),
    // };
    //
    // vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
    //     .colorAttachmentCount = 1,
    //     .pColorAttachmentFormats = &m_swapChainSurfaceFormat.format
    // };
    //
    // vk::PipelineDepthStencilStateCreateInfo depthStencilStateInfo{
    //     .depthTestEnable = vk::True,
    //     .depthWriteEnable = vk::False,
    //     .depthCompareOp = vk::CompareOp::eLess,
    // };
    //
    // vk::StructureChain<vk::GraphicsPipelineCreateInfo,
    // vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain{
    //     {
    //         .stageCount = 2,
    //         .pStages = stages,
    //         .pVertexInputState = &vertexInputStateCreateInfo,
    //         .pInputAssemblyState = &inputAssemblyStateCreateInfo,
    //         .pViewportState = &viewportState,
    //         .pRasterizationState = &rasterizer,
    //         .pMultisampleState = &multisampling,
    //         .pDepthStencilState = &depthStencilStateInfo,
    //         .pColorBlendState = &colorBlending,
    //         .pDynamicState = &dynamicStateCreateInfo,
    //         .layout = m_pipelineLayout,
    //         .renderPass = nullptr
    //     },
    //     {
    //         .colorAttachmentCount = 1, .pColorAttachmentFormats =
    //         &m_swapChainSurfaceFormat.format, .depthAttachmentFormat =
    //         depthFormat
    //     }
    // };
    //
    // blendingPipeline = vk::raii::Pipeline(m_device, nullptr,
    //                                       pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
}

void VulkanContext::createCommandPool() {
    vk::CommandPoolCreateInfo commandPoolCreateInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queueFamilyIndex};

    m_commandPool = vk::raii::CommandPool(m_device, commandPoolCreateInfo);
}

void VulkanContext::createCommandBuffer() {
    vk::CommandBufferAllocateInfo commandBufferAllocateInfo{
        .commandPool = m_commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT};

    m_commandBuffer =
        vk::raii::CommandBuffers(m_device, commandBufferAllocateInfo);
}

void VulkanContext::processNodes(unsigned root,
                                 const glm::mat4& mat,
                                 const VkModel& model,
                                 uint32_t modelIndex) {
    const auto& node = model.nodes[root];
    glm::mat4 world = mat * node.transformation;

    if (node.meshIndex != -1) {
        const Mesh& mesh = model.meshes[node.meshIndex];
        for (const auto& prim : mesh.primitives) {
            drawCalls.push_back(DrawCall{
                prim.textureIndex + model.textureOffset, world, prim.hasTexture,
                prim.color, prim.indexCount, prim.firstIndex, 0, modelIndex});
        }
        for (const auto& prim : mesh.blendPrimitives) {
            blendDrawCalls.push_back(DrawCall{
                prim.textureIndex + model.textureOffset, world, prim.hasTexture,
                prim.color, prim.indexCount, prim.firstIndex, 0, modelIndex});
        }
    }

    for (const auto& child : node.children) {
        processNodes(child, world, model, modelIndex);
    }
}

void VulkanContext::drawBlendNodes(unsigned root,
                                   const glm::mat4& mat,
                                   const VkModel& model) {}

void VulkanContext::recordCommandBuffer(uint32_t index,
                                        Scene& scene,
                                        EngineStats& stats) {
    stats.drawcallCount = 0;
    stats.triangleCount = 0;

    drawCalls.clear();
    blendDrawCalls.clear();

    auto& commandBuffer = m_commandBuffer[frameIndex];

    commandBuffer.begin({});
    transition_image_layout(m_swapChainImages[index],
                            vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eColorAttachmentOptimal, {},
                            vk::AccessFlagBits2::eColorAttachmentWrite,
                            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                            vk::ImageAspectFlagBits::eColor);

    vk::ClearValue clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.9f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderingAttachmentInfo renderingAttachmentInfo{
        .imageView = m_swapChainImageViews[index],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearValue};

    vk::RenderingAttachmentInfo depthAttachmentInfo = {
        .imageView = depthImageView,
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = clearDepth};

    vk::RenderingInfo renderingInfo{
        .renderArea = {.offset = {0, 0}, .extent = m_swapChainExtent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &renderingAttachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo};

    transition_image_layout(*depthImage, vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eDepthAttachmentOptimal,
                            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                            vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                                vk::PipelineStageFlagBits2::eLateFragmentTests,
                            vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                                vk::PipelineStageFlagBits2::eLateFragmentTests,
                            vk::ImageAspectFlagBits::eDepth);

    commandBuffer.beginRendering(renderingInfo);

    commandBuffer.setViewport(
        0,
        vk::Viewport(0.0f, 0.0f, static_cast<float>(m_swapChainExtent.width),
                     static_cast<float>(m_swapChainExtent.height), 0.0f, 1.0f));
    commandBuffer.setScissor(0,
                             vk::Rect2D(vk::Offset2D(0, 0), m_swapChainExtent));

    for (const auto& obj : scene.GetObjects()) {
        const auto& model = models_[obj.modelIndex];
        for (const auto& root : model.rootNodes) {
            processNodes(root, obj.transform.getModelMat(), model,
                         obj.modelIndex);
        }
    }

    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                   m_graphicsPipeline);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                         m_pipelineLayout, 0,
                                         *descriptorSets[frameIndex], nullptr);

        for (const auto& obj : scene.GetObjects()) {
            const auto& model = models_[obj.modelIndex];
            commandBuffer.bindVertexBuffers(0, *model.vertexBuffer, {0});
            commandBuffer.bindIndexBuffer(*model.indexBuffer, 0,
                                          vk::IndexType::eUint32);

            for (const auto& drawCall : drawCalls) {
                if (drawCall.modelIndex == obj.modelIndex) {
                    m_commandBuffer[frameIndex].bindDescriptorSets(
                        vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 1,
                        *blendDescriptorSets[drawCall.textureIndex], nullptr);

                    m_commandBuffer[frameIndex]
                        .pushConstants<PushConstantObject>(
                            m_pipelineLayout,
                            vk::ShaderStageFlagBits::eAllGraphics, 0,
                            PushConstantObject{
                                drawCall.mat,
                                drawCall.color,
                                drawCall.hasTexture,
                            });

                    m_commandBuffer[frameIndex].drawIndexed(
                        drawCall.indexCount, 1, drawCall.firstIndex, 0, 0);
                    stats.drawcallCount++;
                    stats.triangleCount += drawCall.indexCount / 3;
                }
            }
        }
    }

    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                   blendingPipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                         m_pipelineLayout, 0,
                                         *descriptorSets[frameIndex], nullptr);

        auto cameraPos = glm::vec3(glm::inverse(view_)[3]);

        for (auto& drawCall : blendDrawCalls) {
            drawCall.distanceToCam =
                glm::length2(cameraPos - glm::vec3(drawCall.mat[3]));
        }

        std::ranges::sort(blendDrawCalls,
                          [&](const DrawCall& a, const DrawCall& b) {
                              return a.distanceToCam > b.distanceToCam;
                          });

        uint32_t boundModelIndex = std::numeric_limits<uint32_t>::max();
        for (auto& drawCall : blendDrawCalls) {
            if (drawCall.modelIndex != boundModelIndex) {
                const auto& model = models_[drawCall.modelIndex];
                commandBuffer.bindVertexBuffers(0, *model.vertexBuffer, {0});
                commandBuffer.bindIndexBuffer(*model.indexBuffer, 0,
                                              vk::IndexType::eUint32);
                boundModelIndex = drawCall.modelIndex;
            }

            m_commandBuffer[frameIndex].bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 1,
                *blendDescriptorSets[drawCall.textureIndex], nullptr);

            m_commandBuffer[frameIndex].pushConstants<PushConstantObject>(
                m_pipelineLayout, vk::ShaderStageFlagBits::eAllGraphics, 0,
                PushConstantObject{drawCall.mat, drawCall.color,
                                   drawCall.hasTexture});

            m_commandBuffer[frameIndex].drawIndexed(drawCall.indexCount, 1,
                                                    drawCall.firstIndex, 0, 0);
            stats.drawcallCount++;
            stats.triangleCount += drawCall.indexCount / 3;
        }
    }

    commandBuffer.endRendering();

    vk::RenderingAttachmentInfo imguiColorAttachmentInfo = {
        .imageView = m_swapChainImageViews[index],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eLoad,
        .storeOp = vk::AttachmentStoreOp::eStore,
    };

    vk::RenderingInfo imguiRenderingInfo{
        .renderArea = {.offset = {0, 0}, .extent = m_swapChainExtent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &imguiColorAttachmentInfo,
    };

    commandBuffer.beginRendering(imguiRenderingInfo);
    // ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
    //                                 *m_commandBuffer[frameIndex]);
    commandBuffer.endRendering();

    transition_image_layout(
        m_swapChainImages[index], vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,          // srcAccessMask
        {},                                                  // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,  // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe            // dstStage
        ,
        vk::ImageAspectFlagBits::eColor);

    commandBuffer.end();
}

void VulkanContext::createSyncObjects() {
    assert(presentCompleteSemaphore.empty() &&
           renderFinishedSemaphore.empty() && drawFence.empty());

    for (int i = 0; i < m_swapChainImages.size(); i++) {
        presentCompleteSemaphore.emplace_back(
            vk::raii::Semaphore(m_device, vk::SemaphoreCreateInfo()));
        renderFinishedSemaphore.emplace_back(
            vk::raii::Semaphore(m_device, vk::SemaphoreCreateInfo()));
    }

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        drawFence.emplace_back(vk::raii::Fence(
            m_device, {.flags = vk::FenceCreateFlagBits::eSignaled}));
    }
}

void VulkanContext::createDescriptorSetLayout() {
    vk::DescriptorSetLayoutBinding binding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eUniformBuffer,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eAllGraphics};

    vk::DescriptorSetLayoutBinding samplerBinding{
        .binding = 1,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment};

    std::array bindings{binding, samplerBinding};

    vk::DescriptorSetLayoutCreateInfo createInfo{
        .bindingCount = bindings.size(), .pBindings = bindings.data()};

    descriptorSetLayout = vk::raii::DescriptorSetLayout(m_device, createInfo);
    // graphicsPSO_ = std::make_unique<PSO>(m_device, MAX_FRAMES_IN_FLIGHT);
}

void VulkanContext::createUniformBuffers() {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DeviceSize bufferSize = sizeof(UBObject);
        auto [buffer, buffMemory] =
            createBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                         vk::MemoryPropertyFlagBits::eHostVisible |
                             vk::MemoryPropertyFlagBits::eHostCoherent);
        uniformBuffers.emplace_back(std::move(buffer));
        uniformDevicesMemory.emplace_back(std::move(buffMemory));
        uniformBuffersMapped.emplace_back(
            uniformDevicesMemory.back().mapMemory(0, bufferSize));
    }
}

void VulkanContext::updateUniformBuffer(uint32_t index) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    UBObject ubo{};
    ubo.view = view_;
    ubo.proj = proj_;
    ubo.proj[1][1] *= -1;

    float lightRadius = 5.0f;
    float lightHeight = 3.0f;
    float lightSpeed = glm::radians(90.0f);
    ubo.lightPos =
        glm::vec3(lightRadius * std::cos(time * lightSpeed),
                  lightRadius * std::sin(time * lightSpeed), lightHeight);

    ubo.lightColor = {1.0, 1.0, 1.0};
    ubo.ambient = {0.1, 0.1, 0.1};
    ubo.cameraPos = glm::vec3(glm::inverse(view_)[3]);
    ;

    std::memcpy(uniformBuffersMapped[index], &ubo, sizeof(ubo));
}

void VulkanContext::createDescriptorPool() {
    std::array poolSizes{
        vk::DescriptorPoolSize{.type = vk::DescriptorType::eUniformBuffer,
                               .descriptorCount = MAX_FRAMES_IN_FLIGHT},
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = textureCounter},
    };

    uint32_t maxSets = 0;
    for (const auto& poolSize : poolSizes) {
        maxSets += poolSize.descriptorCount;
    }

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = maxSets,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()};
    descriptorPool = vk::raii::DescriptorPool(m_device, poolInfo);
}

void VulkanContext::initImGui() {
    // ImGui_ImplVulkan_InitInfo initInfo{};
    // initInfo.Instance       = *m_instance;
    // initInfo.PhysicalDevice = *m_physicalDevice;
    // initInfo.Device         = *m_device;
    // initInfo.QueueFamily    = queueFamilyIndex;
    // initInfo.Queue          = *m_graphicsQueue;
    // initInfo.PipelineCache  = VK_NULL_HANDLE;
    // initInfo.DescriptorPool = *imguiDescriptorPool;
    // initInfo.MinImageCount  = MAX_FRAMES_IN_FLIGHT;
    // initInfo.ImageCount     = m_swapChainImages.size();
    // initInfo.Allocator      = VK_NULL_HANDLE;
    //
    // initInfo.UseDynamicRendering = true;
    // VkFormat colorFormat =
    //     static_cast<VkFormat>(m_swapChainSurfaceFormat.format);
    // initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.sType =
    //     VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    // initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount
    // =
    //     1;
    // initInfo.PipelineInfoMain.PipelineRenderingCreateInfo
    //     .pColorAttachmentFormats          = &colorFormat;
    // initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    // ImGui_ImplVulkan_Init(&initInfo);
    //
    // ImGui::GetIO().FontGlobalScale = 2.0f;
}

void VulkanContext::createImGuiDescriptorPool() {
    // constexpr uint32_t imguiExtraSets =
    //     IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE +
    //     IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE;
    //
    // std::array poolSizes{
    //     vk::DescriptorPoolSize{
    //         .type = vk::DescriptorType::eSampledImage,
    //         .descriptorCount =
    //             IMGUI_IMPL_VULKAN_MINIMUM_SAMPLED_IMAGE_POOL_SIZE},
    //     vk::DescriptorPoolSize{
    //         .type            = vk::DescriptorType::eSampler,
    //         .descriptorCount = IMGUI_IMPL_VULKAN_MINIMUM_SAMPLER_POOL_SIZE}};
    //
    // uint32_t maxSets = 0;
    // for (const auto& poolSize : poolSizes) {
    //     maxSets += poolSize.descriptorCount;
    // }
    //
    // vk::DescriptorPoolCreateInfo poolInfo{
    //     .flags         =
    //     vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, .maxSets =
    //     maxSets, .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
    //     .pPoolSizes    = poolSizes.data()};
    // imguiDescriptorPool = vk::raii::DescriptorPool(m_device, poolInfo);
}

void VulkanContext::createDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                                 *descriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()};

    descriptorSets = m_device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo{.buffer = uniformBuffers[i],
                                            .offset = 0,
                                            .range = sizeof(UBObject)};
        std::array writes{vk::WriteDescriptorSet{
            .dstSet = descriptorSets[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo}};
        m_device.updateDescriptorSets(writes, {});
    }
}

void VulkanContext::createDepthResources() {
    depthFormat = findDepthFormat();
    std::tie(depthImage, depthImageMemory) =
        createImage(m_swapChainExtent.width, m_swapChainExtent.height,
                    depthFormat, vk::ImageTiling::eOptimal,
                    vk::ImageUsageFlagBits::eDepthStencilAttachment,
                    vk::MemoryPropertyFlagBits::eDeviceLocal);
    depthImageView = createImageView(depthImage, depthFormat,
                                     vk::ImageAspectFlagBits::eDepth);
}

std::pair<vk::raii::Image, vk::raii::DeviceMemory> VulkanContext::createImage(
    uint32_t width,
    uint32_t height,
    vk::Format format,
    vk::ImageTiling tiling,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties) {
    vk::ImageCreateInfo imageInfo{.imageType = vk::ImageType::e2D,
                                  .format = format,
                                  .extent = {width, height, 1},
                                  .mipLevels = 1,
                                  .arrayLayers = 1,
                                  .samples = vk::SampleCountFlagBits::e1,
                                  .tiling = tiling,
                                  .usage = usage,
                                  .sharingMode = vk::SharingMode::eExclusive};

    vk::raii::Image image = vk::raii::Image(m_device, imageInfo);

    vk::MemoryRequirements memRequirements = image.getMemoryRequirements();
    vk::MemoryAllocateInfo allocInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            findMemoryType(memRequirements.memoryTypeBits, properties)};
    vk::raii::DeviceMemory imageMemory =
        vk::raii::DeviceMemory(m_device, allocInfo);
    image.bindMemory(imageMemory, 0);

    return {std::move(image), std::move(imageMemory)};
}

vk::raii::ImageView VulkanContext::createImageView(
    const vk::Image& image,
    vk::Format format,
    vk::ImageAspectFlags aspectFlags) {
    vk::ImageViewCreateInfo viewInfo{
        .image = image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {.aspectMask = aspectFlags,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};
    return vk::raii::ImageView(m_device, viewInfo);
}

uint32_t VulkanContext::findMemoryType(uint32_t typeFilter,
                                       vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties =
        m_physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) ==
                properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}

void VulkanContext::createBlendDescriptorSetLayout() {
    vk::DescriptorSetLayoutBinding binding{
        .binding = 0,
        .descriptorType = vk::DescriptorType::eCombinedImageSampler,
        .descriptorCount = 1,
        .stageFlags = vk::ShaderStageFlagBits::eFragment};

    vk::DescriptorSetLayoutCreateInfo createInfo{
        .bindingCount = 1,
        .pBindings = &binding,
    };

    blendingDescriptorSetLayout =
        vk::raii::DescriptorSetLayout(m_device, createInfo);
}

void VulkanContext::createBlendDescriptorSet() {
    std::vector<vk::DescriptorSetLayout> layouts(textureCounter,
                                                 *blendingDescriptorSetLayout);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()};

    blendDescriptorSets = m_device.allocateDescriptorSets(allocInfo);

    size_t index = 0;
    for (int i = 0; i < models_.size(); i++) {
        for (int j = 0; j < models_[i].textures.size(); j++) {
            const auto& texture = models_[i].textures[j];
            vk::DescriptorImageInfo imageInfo{
                .sampler = texture.textureSampler,
                .imageView = texture.textureImageView,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

            std::array writes{vk::WriteDescriptorSet{
                .dstSet = blendDescriptorSets[index++],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &imageInfo}};
            m_device.updateDescriptorSets(writes, {});
        }
    }
}

void VulkanContext::copyBufferToImage(vk::raii::CommandBuffer& commandBuffer,
                                      const vk::raii::Buffer& buffer,
                                      vk::raii::Image& image,
                                      uint32_t width,
                                      uint32_t height) {
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

vk::raii::ShaderModule VulkanContext::createShaderModule(
    const std::vector<char>& code) {
    vk::ShaderModuleCreateInfo shaderModuleCreateInfo{
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())};

    vk::raii::ShaderModule shaderModule{m_device, shaderModuleCreateInfo};
    return shaderModule;
}

std::vector<char> VulkanContext::loadShader(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    file.close();
    return buffer;
}

void VulkanContext::transition_image_layout(
    vk::Image image,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::AccessFlags2 src_access_mask,
    vk::AccessFlags2 dst_access_mask,
    vk::PipelineStageFlags2 src_stage_mask,
    vk::PipelineStageFlags2 dst_stage_mask,
    vk::ImageAspectFlags image_aspect_flags) {
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = src_stage_mask,
        .srcAccessMask = src_access_mask,
        .dstStageMask = dst_stage_mask,
        .dstAccessMask = dst_access_mask,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = {.aspectMask = image_aspect_flags,
                             .baseMipLevel = 0,
                             .levelCount = 1,
                             .baseArrayLayer = 0,
                             .layerCount = 1}};
    vk::DependencyInfo dependency_info = {.dependencyFlags = {},
                                          .imageMemoryBarrierCount = 1,
                                          .pImageMemoryBarriers = &barrier};
    m_commandBuffer[frameIndex].pipelineBarrier2(dependency_info);
}

void VulkanContext::transitionImageLayout(
    vk::raii::CommandBuffer& commandBuffer,
    const vk::raii::Image& image,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout) {
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
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }
    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {},
                                  nullptr, barrier);
}

std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> VulkanContext::createBuffer(
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties) {
    vk::BufferCreateInfo bufferCreateInfo{
        .size = size,
        .usage = usage,
        .sharingMode = vk::SharingMode::eExclusive,
    };
    vk::raii::Buffer buffer = vk::raii::Buffer(m_device, bufferCreateInfo);

    vk::MemoryRequirements memRequirements = buffer.getMemoryRequirements();

    vk::MemoryAllocateInfo memoryAllocateInfo{
        .allocationSize = memRequirements.size,
        .memoryTypeIndex =
            findMemoryType(memRequirements.memoryTypeBits, properties)};

    auto bufferMemory = vk::raii::DeviceMemory(m_device, memoryAllocateInfo);
    buffer.bindMemory(*bufferMemory, 0);

    return {std::move(buffer), std::move(bufferMemory)};
}

vk::raii::CommandBuffer VulkanContext::beginSingleTimeCommand() {
    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = m_commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1};
    vk::raii::CommandBuffer commandBuffer =
        std::move(vk::raii::CommandBuffers(m_device, allocInfo).front());

    vk::CommandBufferBeginInfo beginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

void VulkanContext::endSingleTimeCommand(
    vk::raii::CommandBuffer&& commandBuffer) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo{.commandBufferCount = 1,
                              .pCommandBuffers = &*commandBuffer};
    m_graphicsQueue.submit(submitInfo, nullptr);
    m_graphicsQueue.waitIdle();
}

vk::Format VulkanContext::findSupportedFormat(
    const std::vector<vk::Format>& candidates,
    vk::ImageTiling tiling,
    vk::FormatFeatureFlags features) {
    for (const auto format : candidates) {
        vk::FormatProperties props =
            m_physicalDevice.getFormatProperties(format);

        if (((tiling == vk::ImageTiling::eLinear) &&
             ((props.linearTilingFeatures & features) == features)) ||
            ((tiling == vk::ImageTiling::eOptimal) &&
             ((props.optimalTilingFeatures & features) == features))) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

vk::Format VulkanContext::findDepthFormat() {
    return findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint,
         vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment);
}

void VulkanContext::cleanupSwapChain() {
    depthImageView = nullptr;
    depthImage = nullptr;
    depthImageMemory = nullptr;

    m_swapChainImageViews.clear();
    m_swapchain = nullptr;
}

std::vector<Texture> VulkanContext::createTexturesFromImages(
    vk::raii::CommandBuffer& commandBuffer,
    std::vector<vk::raii::Buffer>& stagingBuffers,
    std::vector<vk::raii::DeviceMemory>& stagingMemories,
    const Model& model) {
    std::vector<Texture> textures;

    for (const auto& imageData : model.images) {
        Texture texture;
        vk::DeviceSize imageSize =
            imageData.width * imageData.height * imageData.channels;
        auto [stagingBuffer, stagingBufferMemory] =
            createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostVisible |
                             vk::MemoryPropertyFlagBits::eHostCoherent);
        void* data = stagingBufferMemory.mapMemory(0, imageSize);
        memcpy(data, imageData.data, imageSize);
        stagingBufferMemory.unmapMemory();

        std::tie(texture.textureImage, texture.textureImageMemory) =
            createImage(imageData.width, imageData.height,
                        vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
                        vk::ImageUsageFlagBits::eTransferDst |
                            vk::ImageUsageFlagBits::eSampled,
                        vk::MemoryPropertyFlagBits::eDeviceLocal);

        transitionImageLayout(commandBuffer, texture.textureImage,
                              vk::ImageLayout::eUndefined,
                              vk::ImageLayout::eTransferDstOptimal);
        copyBufferToImage(commandBuffer, stagingBuffer, texture.textureImage,
                          static_cast<uint32_t>(imageData.width),
                          static_cast<uint32_t>(imageData.height));
        transitionImageLayout(commandBuffer, texture.textureImage,
                              vk::ImageLayout::eTransferDstOptimal,
                              vk::ImageLayout::eShaderReadOnlyOptimal);

        texture.textureImageView =
            createImageView(texture.textureImage, vk::Format::eR8G8B8A8Srgb,
                            vk::ImageAspectFlagBits::eColor);

        vk::SamplerCreateInfo samplerInfo{};
        texture.textureSampler = vk::raii::Sampler(m_device, samplerInfo);

        textures.push_back(std::move(texture));
        stagingBuffers.push_back(std::move(stagingBuffer));
        stagingMemories.push_back(std::move(stagingBufferMemory));
        textureCounter++;
    }

    return textures;
}

VkModel VulkanContext::loadModel(const std::string& path) {
    VkModel vkModel;
    auto model = AssetLoader::loadModelFromGltf(path);

    vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommand();
    std::vector<vk::raii::Buffer> stagingBuffers;
    std::vector<vk::raii::DeviceMemory> stagingMemories;

    auto [vertexBuffer, vertexBufferMemory] = createUniversalBuffer(
        commandBuffer, stagingBuffers, stagingMemories, model.vertices,
        vk::BufferUsageFlagBits::eVertexBuffer);
    vkModel.vertexBuffer = std::move(vertexBuffer);
    vkModel.vertexBufferMemory = std::move(vertexBufferMemory);
    auto [indexBuffer, indexBufferMemory] = createUniversalBuffer(
        commandBuffer, stagingBuffers, stagingMemories, model.indices,
        vk::BufferUsageFlagBits::eIndexBuffer);
    vkModel.indexBuffer = std::move(indexBuffer);
    vkModel.indexBufferMemory = std::move(indexBufferMemory);
    vkModel.indicesCount = model.indices.size();

    vkModel.meshes = std::move(model.meshes);
    vkModel.nodes = std::move(model.nodes);
    vkModel.rootNodes = std::move(model.rootNodes);
    vkModel.textureOffset = textureCounter;
    vkModel.textures = createTexturesFromImages(commandBuffer, stagingBuffers,
                                                stagingMemories, model);

    endSingleTimeCommand(std::move(commandBuffer));

    return vkModel;
}

void VulkanContext::finishLoading() {
    createDescriptorPool();
    createDescriptorSets();
    createBlendDescriptorSet();
    // initImGui();
}

void VulkanContext::drawScene(Scene& scene, EngineStats& stats) {
    auto fenceResult =
        m_device.waitForFences(*drawFence[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
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
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    updateUniformBuffer(frameIndex);

    m_device.resetFences(*drawFence[frameIndex]);
    m_commandBuffer[frameIndex].reset();

    recordCommandBuffer(imageIndex, scene, stats);

    vk::PipelineStageFlags waitDestinationStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput);

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

void VulkanContext::processMinimizedWindow() {
    // int width = 0, height = 0;
    // while (width == 0 || height == 0) {
    //     SDL_GetWindowSizeInPixels(m_window, &width, &height);
    //     SDL_WaitEvent(nullptr);
    // }
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

    m_device.waitIdle();
    cleanupSwapChain();
    createSwapchain();
    createImageViews();
    createDepthResources();
}

void VulkanContext::setViewMatrix(const glm::mat4& viewMat) {
    view_ = viewMat;
}

void VulkanContext::setProjMatrix(const glm::mat4& m) {
    proj_ = m;
}

void VulkanContext::createTexture() {}

uint32_t VulkanContext::loadAsset(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("file not found: " + path);
    }

    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path);
    }

    models_.push_back(loadModel(path));
    return models_.size() - 1;
}

//
// meshes.push_back(loadMesh("../assets/CarConcept.glb"));
// objects.push_back({meshes.size() - 1});
// objects[2].transform.pos = glm::vec3(3.0, 0.0, 0.0);
// objects[2].transform.rotate({1.0, 0.0, 0.0}, -90.0);
// objects[2].color = glm::vec3(1.0, 1.0, 0.0
