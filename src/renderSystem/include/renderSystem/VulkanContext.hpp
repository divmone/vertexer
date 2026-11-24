#pragma once

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#endif

#include <glm/glm.hpp>
#include <memory>
#include <platformSystem/IExtentProvider.hpp>
#include <platformSystem/NativeWindowHandler.hpp>
#include <renderSystem/types/3d/DrawCall3d.hpp>
#include <renderSystem/types/3d/VulkanModel.hpp>
#include <renderSystem/types/common/VulkanTexture.hpp>
#include <resourceSystem/types/ImageData.hpp>
#include <resourceSystem/types/Model.hpp>
#include <span>
#include <vulkan/vulkan_raii.hpp>

struct EngineStats;
class Scene;
class Vulkan2dContext;
class Vulkan3dContext;

static constexpr int MAX_FRAMES_IN_FLIGHT{2};
static constexpr int MAX_BINDLESS_RESOURCES{20000};

class VulkanContext {
    friend class Vulkan2dContext;
    friend class Vulkan3dContext;

public:
    VulkanContext(const NativeWindowHandler& windowHandler,
                  ExtentProvider             extentProvider);
    ~VulkanContext();

#pragma region PublicApi

    void RecreateContext();
    void ProcessMinimizedWindow();

    void DrawUi(EngineStats& stats, Vulkan2dContext& context);

#pragma endregion PublicApi

private:
#pragma region Initialization

    void InitVulkan();
    void CreateInstance();
    void SelectPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSurface();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateCommandPool();
    void CreateCommandBuffer();
    void CreateSyncObjects();
    void CreateDepthResources();
    void CreateDescriptorPool();
    void CreateBindlessDescriptorPool();

#pragma endregion Initialization

#pragma region Resources

    uint32_t FindMemoryType(uint32_t                typeFilter,
                            vk::MemoryPropertyFlags properties) const;

    void CopyBufferToImage(vk::raii::CommandBuffer& commandBuffer,
                           const vk::raii::Buffer&  buffer,
                           vk::raii::Image&         image,
                           uint32_t                 width,
                           uint32_t                 height);

    void CreateTexture();

    VkTexture CreateVkTextureFromImageData(
        vk::raii::CommandBuffer&             commandBuffer,
        std::vector<vk::raii::Buffer>&       stagingBuffers,
        std::vector<vk::raii::DeviceMemory>& stagingMemories,
        const ImageData&                     imageData);

#pragma endregion Resources

#pragma region MemoryAndImages

    std::pair<vk::raii::Image, vk::raii::DeviceMemory> CreateImage(
        uint32_t                width,
        uint32_t                height,
        vk::Format              format,
        vk::ImageTiling         tiling,
        vk::ImageUsageFlags     usage,
        vk::MemoryPropertyFlags properties);

    vk::raii::ImageView CreateImageView(const vk::Image&     image,
                                        vk::Format           format,
                                        vk::ImageAspectFlags aspectFlags) const;

    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CreateBuffer(
        vk::DeviceSize          size,
        vk::BufferUsageFlags    usage,
        vk::MemoryPropertyFlags properties) const;

    template <typename T>
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> CreateUniversalBuffer(
        vk::raii::CommandBuffer&             commandBuffer,
        std::vector<vk::raii::Buffer>&       stagingBuffersKeepAlive,
        std::vector<vk::raii::DeviceMemory>& stagingMemoriesKeepAlive,
        const std::span<const T>&            arr,
        vk::BufferUsageFlagBits              flag) {
        vk::DeviceSize bufferSize = arr.size() * sizeof(arr[0]);
        auto [stagingBuffer, stagingDeviceMemory] =
            CreateBuffer(bufferSize,
                         vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostVisible |
                             vk::MemoryPropertyFlagBits::eHostCoherent);
        void* data = stagingDeviceMemory.mapMemory(0, bufferSize);
        memcpy(data, arr.data(), bufferSize);
        stagingDeviceMemory.unmapMemory();

        auto [buffer, bufferMemory] =
            CreateBuffer(bufferSize,
                         flag | vk::BufferUsageFlagBits::eTransferDst,
                         vk::MemoryPropertyFlagBits::eDeviceLocal);
        commandBuffer.copyBuffer(
            *stagingBuffer, *buffer, vk::BufferCopy{.size = bufferSize});

        stagingBuffersKeepAlive.push_back(std::move(stagingBuffer));
        stagingMemoriesKeepAlive.push_back(std::move(stagingDeviceMemory));

        return {std::move(buffer), std::move(bufferMemory)};
    }

    vk::raii::CommandBuffer BeginSingleTimeCommand() const;
    void EndSingleTimeCommand(vk::raii::CommandBuffer&& commandBuffer) const;

#pragma endregion MemoryAndImages

#pragma region ImageLayoutAndFormats

    void TransitionImageLayout(vk::Image               image,
                               vk::ImageLayout         oldLayout,
                               vk::ImageLayout         newLayout,
                               vk::AccessFlags2        srcAccessMask,
                               vk::AccessFlags2        dstAccessMask,
                               vk::PipelineStageFlags2 srcStageMask,
                               vk::PipelineStageFlags2 dstStageMask,
                               vk::ImageAspectFlags    imageAspectFlags) const;

    void TransitionImageLayout(vk::raii::CommandBuffer& commandBuffer,
                               const vk::raii::Image&   image,
                               vk::ImageLayout          oldLayout,
                               vk::ImageLayout          newLayout);

    vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates,
                                   vk::ImageTiling                tiling,
                                   vk::FormatFeatureFlags features) const;
    vk::Format FindDepthFormat() const;

#pragma endregion ImageLayoutAndFormats

#pragma region SwapchainHelpers

    vk::Extent2D ChooseSwapExtent(
        const vk::SurfaceCapabilitiesKHR& capabilities);

    static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR>& availableFormats);

    static vk::PresentModeKHR ChooseSwapPresentMode(
        const std::vector<vk::PresentModeKHR>& availablePresentModes);

    static uint32_t ChooseSwapMinImageCount(
        const vk::SurfaceCapabilitiesKHR& surfaceCapabilities);

    void CleanupSwapChain();

#pragma endregion SwapchainHelpers

#pragma region ShaderHelpers

    vk::raii::ShaderModule CreateShaderModule(
        const std::vector<char>& code) const;

    static std::vector<char> LoadShader(const std::string& filename);

#pragma endregion ShaderHelpers

#pragma region Fields

    // SDL_Window* m_window{nullptr};
    int width_{0};
    int height_{0};

    NativeWindowHandler nativeWindowHandler_;
    ExtentProvider      extentProvider_;

#pragma region VulkanCore

    vk::raii::Context        context_;
    vk::raii::Instance       instance_{nullptr};
    vk::raii::PhysicalDevice physicalDevice_{nullptr};
    vk::raii::Device         device_{nullptr};
    vk::raii::Queue          graphicsQueue_{nullptr};

    vk::raii::SurfaceKHR surfaceKHR_{nullptr};

#pragma endregion VulkanCore

#pragma region Swapchain

    vk::raii::SwapchainKHR swapChain_{nullptr};
    std::vector<vk::Image> swapChainImages_{nullptr};

    vk::SurfaceFormatKHR             swapChainSurfaceFormat_;
    vk::Extent2D                     swapChainExtent_;
    std::vector<vk::raii::ImageView> swapChainImageViews_;

#pragma endregion Swapchain

#pragma region DescriptorSetLayouts

    vk::raii::DescriptorSetLayout descriptorSetLayout3d_{nullptr};
    vk::raii::DescriptorSetLayout blendingDescriptorSetLayout_{nullptr};
    vk::raii::DescriptorSetLayout bindlessDescriptorSetLayout_{nullptr};

#pragma endregion DescriptorSetLayouts

#pragma region PipelinesAndLayouts

    vk::raii::Pipeline       graphicsPipeline_{nullptr};
    vk::raii::PipelineLayout pipelineLayout_{nullptr};

    vk::raii::Pipeline blendingPipeline_{nullptr};

    vk::raii::PipelineLayout skyPipelineLayout_{nullptr};
    vk::raii::Pipeline       skyPipeline_{nullptr};

#pragma endregion PipelinesAndLayouts

#pragma region CommandAndSync

    vk::raii::CommandPool                commandPool_{nullptr};
    uint32_t                             queueFamilyIndex_{0};
    std::vector<vk::raii::CommandBuffer> commandBuffer_;
    std::vector<vk::raii::Semaphore>     presentCompleteSemaphore_;
    std::vector<vk::raii::Semaphore>     renderFinishedSemaphore_;
    std::vector<vk::raii::Fence>         drawFence_;

    uint32_t frameIndex_{0};

#pragma endregion CommandAndSync

#pragma region UniformBuffers

    std::vector<vk::raii::Buffer>       uniformBuffers_;
    std::vector<vk::raii::DeviceMemory> uniformDevicesMemory_;
    std::vector<void*>                  uniformBuffersMapped_;

#pragma endregion UniformBuffers

#pragma region DescriptorPoolsAndSets

    vk::raii::DescriptorPool generalDescriptorPool_{nullptr};
    vk::raii::DescriptorPool bindlessDescriptorPool_{nullptr};

    vk::raii::DescriptorPool imguiDescriptorPool_{nullptr};

    std::vector<vk::raii::DescriptorSet> descriptorSets_;
    std::vector<vk::raii::DescriptorSet> blendDescriptorSets_;
    vk::raii::DescriptorSet              bindlessDescriptorSet_{nullptr};

#pragma endregion DescriptorPoolsAndSets

#pragma region DepthResources

    vk::raii::Image        depthImage_{nullptr};
    vk::raii::DeviceMemory depthImageMemory_{nullptr};
    vk::raii::ImageView    depthImageView_{nullptr};
    vk::Format             depthFormat_;

#pragma endregion DepthResources

#pragma region TexturesAndModels

    std::vector<VkTexture> textures_;
    uint32_t               textureCounter_{0};
    size_t                 texture2dIndex_{0};

    std::vector<VkModel> models_;

#pragma endregion TexturesAndModels

#pragma region Matrices

    glm::mat4 view_;
    glm::mat4 proj_;

#pragma endregion Matrices

#pragma region DrawCalls

    std::vector<DrawCall3d> drawCalls_;
    std::vector<DrawCall3d> blendDrawCalls_;

#pragma endregion DrawCalls

#pragma endregion Fields
};