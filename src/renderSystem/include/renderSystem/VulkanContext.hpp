#pragma once

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <platformSystem/IExtentProvider.hpp>
#include <platformSystem/NativeWindowHandler.hpp>
#include <renderSystem/types/DrawCall.hpp>
#include <renderSystem/types/DrawCall2d.hpp>
#include <renderSystem/types/UiDrawCall.hpp>
#include <renderSystem/types/VulkanModel.hpp>
#include <resourceSystem/types/Model.hpp>
#include <span>
#include <vulkan/vulkan_raii.hpp>

#include "resourceSystem/types/Sprite.hpp"
#include "resourceSystem/types/SpriteData.hpp"
#include "types/VulkanSprite.hpp"

struct EngineStats;
class Scene;

static constexpr int MAX_FRAMES_IN_FLIGHT{2};
static constexpr int MAX_BINDLESS_RESOURCES{20000};

struct PushConstantObject3d {
    alignas(16) glm::mat4 model;
    alignas(16) glm::vec4 color;
    alignas(16) uint32_t hasTexture;
};

struct PushConstantObject2d {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec4 color;
    alignas(16) uint32_t hasTexture;
};

struct PushConstantObjectBlend {
    alignas(16) glm::mat4 model;
    alignas(16) glm::vec4 color;
};

struct UBObject {
    alignas(16) glm::vec3 lightPos;
    alignas(16) glm::vec3 lightColor;
    alignas(16) glm::vec3 ambient;
    alignas(16) glm::vec3 cameraPos;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class VulkanContext {
public:
    VulkanContext(const NativeWindowHandler& windowHandler,
                  ExtentProvider             extentProvider);
    ~VulkanContext();

    void RenderFrame(Scene& scene, EngineStats& stats);

    void RecreateContext();
    void ProcessMinimizedWindow();

    void SetViewMatrix(const glm::mat4&);
    void SetProjMatrix(const glm::mat4&);
    void SetProj2dMatrix(const glm::mat4&);

    uint32_t LoadAsset(const std::string& path);
    void     FinishLoading();

    SpriteHandle AddModel(const Model& model);
    SpriteHandle AddSprite(const SpriteData& sprite);

    void AddSpriteToPipeline(const Sprite& sprite);
    void DrawSprites(EngineStats& stats);

    // --- RmlUi / 2D UI rendering ---
    uint32_t AddUiGeometry(std::span<const Vertex2d> vertices,
                           std::span<const uint32_t> indices);
    void     ReleaseUiGeometry(uint32_t geometryHandle);

    uintptr_t AddUiTexture(const ImageData& image);
    void      ReleaseUiTexture(uintptr_t textureHandle);

    uintptr_t CreateOffscreenTarget(uint32_t width, uint32_t height);
    void RenderSpriteToOffscreen(SpriteHandle sprite, uintptr_t targetHandle);
    std::pair<uint32_t, uint32_t> GetUiTextureSize(
        uintptr_t textureHandle) const;

    void AddUiDrawCall(uint32_t         geometryHandle,
                       const glm::mat4& model,
                       uintptr_t        textureHandle,
                       bool             hasTexture,
                       bool             scissorEnabled,
                       vk::Rect2D       scissor);

private:
    void InitVulkan();
    void CreateInstance();
    void SelectPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSurface();
    void CreateSwapchain();
    void CreateImageViews();

    void CreateDescriptorSetLayout();
    void CreateDescriptorSetLayout2d();

    void CreateGraphicsPipeline();
    void CreateGraphicsPipeline2d();

    void CreateBlendingPipeline();

    void CreateCommandPool();
    void CreateCommandBuffer();

    void ProcessNodes(unsigned         root,
                      const glm::mat4& mat,
                      const VkModel&   model,
                      uint32_t         modelIndex);
    void DrawBlendNodes(unsigned         root,
                        const glm::mat4& mat,
                        const VkModel&   model);

    // void recordCommandBuffer(uint32_t index, Scene& scene, EngineStats&
    // stats);
    void RecordCommandBuffer2d(uint32_t imageIndex, EngineStats& stats);
    void CreateSyncObjects();
    void CreateUniformBuffers();
    void UpdateUniformBuffer(uint32_t index) const;

    void CreateDescriptorSets();
    void CreateDepthResources();

    uint32_t FindMemoryType(uint32_t                typeFilter,
                            vk::MemoryPropertyFlags properties) const;
    void     CopyBufferToImage(vk::raii::CommandBuffer& commandBuffer,
                               const vk::raii::Buffer&  buffer,
                               vk::raii::Image&         image,
                               uint32_t                 width,
                               uint32_t                 height);

    // SDL_Window* m_window{nullptr};
    int width_{0};
    int height_{0};

    NativeWindowHandler nativeWindowHandler_;
    ExtentProvider      extentProvider_;

    vk::raii::Context        context_;
    vk::raii::Instance       instance_{nullptr};
    vk::raii::PhysicalDevice physicalDevice_{nullptr};
    vk::raii::Device         device_{nullptr};
    vk::raii::Queue          graphicsQueue_{nullptr};

    vk::raii::SurfaceKHR surfaceKHR_{nullptr};

    vk::raii::SwapchainKHR swapchain_{nullptr};
    std::vector<vk::Image> swapChainImages_{nullptr};

    vk::SurfaceFormatKHR             swapChainSurfaceFormat_;
    vk::Extent2D                     swapChainExtent_;
    std::vector<vk::raii::ImageView> swapChainImageViews_;

    vk::raii::DescriptorSetLayout descriptorSetLayout3d_{nullptr};
    vk::raii::DescriptorSetLayout blendingDescriptorSetLayout_{nullptr};
    vk::raii::DescriptorSetLayout descriptorSetLayout2d_{nullptr};

    vk::raii::Pipeline       graphicsPipeline_{nullptr};
    vk::raii::PipelineLayout pipelineLayout_{nullptr};

    vk::raii::Pipeline       graphicsPipeline2d_{nullptr};
    vk::raii::PipelineLayout pipelineLayout2d_{nullptr};

    vk::raii::Pipeline blendingPipeline_{nullptr};

    vk::raii::PipelineLayout skyPipelineLayout_{nullptr};
    vk::raii::Pipeline       skyPipeline_{nullptr};

    vk::raii::CommandPool                commandPool_{nullptr};
    uint32_t                             queueFamilyIndex_{0};
    std::vector<vk::raii::CommandBuffer> commandBuffer_;
    std::vector<vk::raii::Semaphore>     presentCompleteSemaphore_;
    std::vector<vk::raii::Semaphore>     renderFinishedSemaphore_;
    std::vector<vk::raii::Fence>         drawFence_;

    uint32_t                            frameIndex_{0};
    std::vector<vk::raii::Buffer>       uniformBuffers_;
    std::vector<vk::raii::DeviceMemory> uniformDevicesMemory_;
    std::vector<void*>                  uniformBuffersMapped_;

    vk::raii::DescriptorPool generalDescriptorPool_{nullptr};
    vk::raii::DescriptorPool bindlessDescriptorPool_{nullptr};
    void                     CreateDescriptorPool();
    void                     CreateBindlessDescriptorPool();

    vk::raii::DescriptorPool imguiDescriptorPool_{nullptr};

    std::vector<vk::raii::DescriptorSet> descriptorSets_;

    vk::raii::Image depthImage_{nullptr};
    vk::raii::DeviceMemory depthImageMemory_{nullptr};
    vk::raii::ImageView depthImageView_{nullptr};
    vk::Format depthFormat_;

    std::vector<VkTexture> textures_;
    std::vector<vk::raii::DescriptorSet> blendDescriptorSets_;
    void CreateBlendDescriptorSetLayout();
    void CreateTextureDescriptorSet();

    uint32_t     textureCounter_{0};
    void         CreateTexture();
    vk::Extent2D ChooseSwapExtent(
        const vk::SurfaceCapabilitiesKHR& capabilities);

    vk::raii::ShaderModule CreateShaderModule(
        const std::vector<char>& code) const;

    static vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR>& availableFormats);

    static vk::PresentModeKHR ChooseSwapPresentMode(
        const std::vector<vk::PresentModeKHR>& availablePresentModes);

    static uint32_t ChooseSwapMinImageCount(
        const vk::SurfaceCapabilitiesKHR& surfaceCapabilities);

    static std::vector<char> LoadShader(const std::string& filename);

    void TransitionImageLayout(vk::Image image,
                                 vk::ImageLayout         oldLayout,
                                 vk::ImageLayout         newLayout,
                                 vk::AccessFlags2        srcAccessMask,
                                 vk::AccessFlags2        dstAccessMask,
                                 vk::PipelineStageFlags2 srcStageMask,
                                 vk::PipelineStageFlags2 dstStageMask,
                                 vk::ImageAspectFlags imageAspectFlags) const;

    void TransitionImageLayout(vk::raii::CommandBuffer& commandBuffer,
                               const vk::raii::Image&   image,
                               vk::ImageLayout          oldLayout,
                               vk::ImageLayout          newLayout);

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

    vk::raii::CommandBuffer BeginSingleTimeCommand() const;
    void EndSingleTimeCommand(vk::raii::CommandBuffer&& commandBuffer) const;

    vk::Format FindSupportedFormat(const std::vector<vk::Format>& candidates,
                                   vk::ImageTiling                tiling,
                                   vk::FormatFeatureFlags features) const;
    vk::Format FindDepthFormat() const;

    void CleanupSwapChain();

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

    glm::mat4 view_;
    glm::mat4 proj_;

    glm::mat4 view2d_{1.0f};
    glm::mat4 proj2d_;

    std::vector<VkTexture> CreateTexturesFromImages(
        vk::raii::CommandBuffer&             commandBuffer,
        std::vector<vk::raii::Buffer>&       stagingBuffers,
        std::vector<vk::raii::DeviceMemory>& stagingMemories,
        const Model&                         model);

    VkTexture CreateVkTextureFromImageData(
        vk::raii::CommandBuffer&             commandBuffer,
        std::vector<vk::raii::Buffer>&       stagingBuffers,
        std::vector<vk::raii::DeviceMemory>& stagingMemories,
        const ImageData&                     imageData);

    VkModel LoadModel(const std::string& path);

    std::vector<DrawCall> drawCalls_;
    std::vector<DrawCall> blendDrawCalls_;

    void CreateDescriptorSet2d();
    std::vector<vk::raii::DescriptorSet> descriptorSets2d_;

    vk::raii::DescriptorSetLayout bindlessDescriptorSetLayout_{nullptr};
    vk::raii::DescriptorSet       bindlessDescriptorSet_{nullptr};

    void CreateBindlessDescriptorSetLayout();
    void CreateBindlessDescriptorSet();

    std::unordered_map<SpriteHandle, VkSprite> sprites_;
    SpriteHandle                               spriteHandle_{0};

    // void DrawSprites(vk::raii::CommandBuffer& commandBuffer);

    void CreateDescriptorSetForSprite(VkSprite& sprite) const;

    std::vector<DrawCall2d> drawCalls2d_;
    void RenderDrawCalls2d(vk::raii::CommandBuffer& commandBuffer) const;

    std::vector<VkModel> models_;

    size_t texture2dIndex_{0};

    struct UiTextureEntry {
        VkTexture               texture;
        vk::raii::DescriptorSet descriptorSet{nullptr};
        uint32_t                width{0};
        uint32_t                height{0};
    };

    struct UiGeometryEntry {
        vk::raii::Buffer       vertexBuffer{nullptr};
        vk::raii::DeviceMemory vertexBufferMemory{nullptr};
        vk::raii::Buffer       indexBuffer{nullptr};
        vk::raii::DeviceMemory indexBufferMemory{nullptr};
        uint32_t               indexCount{0};
    };

    vk::raii::DescriptorSet CreateUiTextureDescriptorSet(
        const VkTexture& texture) const;
    void RenderUiDrawCalls(vk::raii::CommandBuffer& commandBuffer) const;

    std::vector<std::unique_ptr<UiTextureEntry>>  uiTextures_;
    std::vector<std::unique_ptr<UiGeometryEntry>> uiGeometries_;
    std::vector<UiDrawCall>                       uiDrawCalls_;
};
