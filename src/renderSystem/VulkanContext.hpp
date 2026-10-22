#pragma once

#include <glm/glm.hpp>

#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#endif
#include <vulkan/vulkan_raii.hpp>

#include "IExtentProvider.hpp"
#include "NativeWindowHandler.hpp"
#include "PSO.hpp"
#include "types/DrawCall.hpp"
#include "types/Model.hpp"
#include "types/Providers.hpp"
#include "types/VulkanModel.hpp"

struct EngineStats;
class Scene;

constexpr int MAX_FRAMES_IN_FLIGHT = 2;

struct PushConstantObject {
    alignas(16) glm::mat4 model;
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
    VulkanContext(NativeWindowHandler windowHandler,
                  const ExtentProvider& extentProvider);
    ~VulkanContext();

    void drawScene(Scene& scene, EngineStats& stats);

    void recreateContext();
    void processMinimizedWindow();

    void setViewMatrix(const glm::mat4&);
    void setProjMatrix(const glm::mat4&);
    uint32_t loadAsset(const std::string& path);
    void finishLoading();
private:
    void initVulkan();
    void createInstance();
    void selectPhysicalDevice();
    void createLogicalDevice();
    void createSurface();
    void createSwapchain();
    void createImageViews();
    void createDescriptorSetLayout();

    void createGraphicsPipeline();
    void createBlendingPipeline();

    void createCommandPool();
    void createCommandBuffer();

    void processNodes(unsigned root,
                      const glm::mat4& mat,
                      const VkModel& model,
                      uint32_t modelIndex);
    void drawBlendNodes(unsigned root,
                        const glm::mat4& mat,
                        const VkModel& model);

    void recordCommandBuffer(uint32_t index, Scene& scene, EngineStats& stats);
    void createSyncObjects();
    void createUniformBuffers();
    void updateUniformBuffer(uint32_t index);
    void createDescriptorPool();
    void initImGui();
    void createImGuiDescriptorPool();

    void createDescriptorSets();
    void createDepthResources();

    uint32_t findMemoryType(uint32_t typeFilter,
                            vk::MemoryPropertyFlags properties);
    void copyBufferToImage(vk::raii::CommandBuffer& commandBuffer,
                           const vk::raii::Buffer& buffer,
                           vk::raii::Image& image,
                           uint32_t width,
                           uint32_t height);

    // SDL_Window* m_window{nullptr};
    int width_{0};
    int height_{0};

    NativeWindowHandler nativeWindowHandler_;
    ExtentProvider extentProvider_;

    vk::raii::Context m_context;
    vk::raii::Instance instance_{nullptr};
    vk::raii::PhysicalDevice m_physicalDevice{nullptr};
    vk::raii::Device m_device{nullptr};
    vk::raii::Queue m_graphicsQueue{nullptr};

    vk::raii::SurfaceKHR m_surfaceKHR{nullptr};

    vk::raii::SwapchainKHR m_swapchain{nullptr};
    std::vector<vk::Image> m_swapChainImages{nullptr};
    ;
    vk::SurfaceFormatKHR m_swapChainSurfaceFormat;
    vk::Extent2D m_swapChainExtent;
    std::vector<vk::raii::ImageView> m_swapChainImageViews;

    vk::raii::DescriptorSetLayout descriptorSetLayout{nullptr};
    vk::raii::DescriptorSetLayout blendingDescriptorSetLayout{nullptr};

    std::unique_ptr<PSO> graphicsPSO_;

    vk::raii::PipelineLayout m_pipelineLayout{nullptr};
    vk::raii::Pipeline m_graphicsPipeline{nullptr};

    vk::raii::Pipeline blendingPipeline{nullptr};

    vk::raii::PipelineLayout skyPipelineLayout{nullptr};
    vk::raii::Pipeline skyPipeline{nullptr};

    vk::raii::CommandPool m_commandPool{nullptr};
    uint32_t queueFamilyIndex{0};
    std::vector<vk::raii::CommandBuffer> m_commandBuffer;
    std::vector<vk::raii::Semaphore> presentCompleteSemaphore;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphore;
    std::vector<vk::raii::Fence> drawFence;

    uint32_t frameIndex{0};
    std::vector<vk::raii::Buffer> uniformBuffers;
    std::vector<vk::raii::DeviceMemory> uniformDevicesMemory;
    std::vector<void*> uniformBuffersMapped;
    vk::raii::DescriptorPool descriptorPool{nullptr};
    vk::raii::DescriptorPool imguiDescriptorPool{nullptr};

    std::vector<vk::raii::DescriptorSet> descriptorSets;

    vk::raii::Image depthImage{nullptr};
    vk::raii::DeviceMemory depthImageMemory{nullptr};
    vk::raii::ImageView depthImageView{nullptr};
    vk::Format depthFormat;

    std::vector<Texture> textures_;
    std::vector<vk::raii::DescriptorSet> blendDescriptorSets;
    void createBlendDescriptorSetLayout();
    void createBlendDescriptorSet();

    uint32_t textureCounter{0};
    void createTexture();
    vk::Extent2D chooseSwapExtent(
        const vk::SurfaceCapabilitiesKHR& capabilities);

    vk::raii::ShaderModule createShaderModule(const std::vector<char>& code);

    static vk::SurfaceFormatKHR chooseSwapSurfaceFormat(
        const std::vector<vk::SurfaceFormatKHR>& availableFormats);

    static vk::PresentModeKHR chooseSwapPresentMode(
        const std::vector<vk::PresentModeKHR>& availablePresentModes);

    static uint32_t chooseSwapMinImageCount(
        const vk::SurfaceCapabilitiesKHR& surfaceCapabilities);

    static std::vector<char> loadShader(const std::string& filename);

    void transition_image_layout(vk::Image image,
                                 vk::ImageLayout old_layout,
                                 vk::ImageLayout new_layout,
                                 vk::AccessFlags2 src_access_mask,
                                 vk::AccessFlags2 dst_access_mask,
                                 vk::PipelineStageFlags2 src_stage_mask,
                                 vk::PipelineStageFlags2 dst_stage_mask,
                                 vk::ImageAspectFlags image_aspect_flags);

    void transitionImageLayout(vk::raii::CommandBuffer& commandBuffer,
                               const vk::raii::Image& image,
                               vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout);

    std::pair<vk::raii::Image, vk::raii::DeviceMemory> createImage(
        uint32_t width,
        uint32_t height,
        vk::Format format,
        vk::ImageTiling tiling,
        vk::ImageUsageFlags usage,
        vk::MemoryPropertyFlags properties);
    vk::raii::ImageView createImageView(const vk::Image& image,
                                        vk::Format format,
                                        vk::ImageAspectFlags aspectFlags);
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createBuffer(
        vk::DeviceSize size,
        vk::BufferUsageFlags usage,
        vk::MemoryPropertyFlags properties);

    vk::raii::CommandBuffer beginSingleTimeCommand();
    void endSingleTimeCommand(vk::raii::CommandBuffer&& commandBuffer);

    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates,
                                   vk::ImageTiling tiling,
                                   vk::FormatFeatureFlags features);
    vk::Format findDepthFormat();

    void cleanupSwapChain();

    template <typename T>
    std::pair<vk::raii::Buffer, vk::raii::DeviceMemory> createUniversalBuffer(
        vk::raii::CommandBuffer& commandBuffer,
        std::vector<vk::raii::Buffer>& stagingBuffersKeepAlive,
        std::vector<vk::raii::DeviceMemory>& stagingMemoriesKeepAlive,
        const std::vector<T>& arr,
        vk::BufferUsageFlagBits flag) {
        vk::DeviceSize bufferSize = arr.size() * sizeof(arr[0]);
        auto [stagingBuffer, stagingDeviceMemory] =
            createBuffer(bufferSize, vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostVisible |
                             vk::MemoryPropertyFlagBits::eHostCoherent);
        void* data = stagingDeviceMemory.mapMemory(0, bufferSize);
        memcpy(data, arr.data(), bufferSize);
        stagingDeviceMemory.unmapMemory();

        auto [buffer, bufferMemory] = createBuffer(
            bufferSize, flag | vk::BufferUsageFlagBits::eTransferDst,
            vk::MemoryPropertyFlagBits::eDeviceLocal);
        commandBuffer.copyBuffer(*stagingBuffer, *buffer,
                                 vk::BufferCopy{.size = bufferSize});

        stagingBuffersKeepAlive.push_back(std::move(stagingBuffer));
        stagingMemoriesKeepAlive.push_back(std::move(stagingDeviceMemory));

        return {std::move(buffer), std::move(bufferMemory)};
    }

    glm::mat4 view_;
    glm::mat4 proj_;

    std::vector<VkModel> models_;

    std::vector<Texture> createTexturesFromImages(
        vk::raii::CommandBuffer& commandBuffer,
        std::vector<vk::raii::Buffer>& stagingBuffersKeepAlive,
        std::vector<vk::raii::DeviceMemory>& stagingMemoriesKeepAlive,
        const Model& model);

    VkModel loadModel(const std::string& path);

    std::vector<DrawCall> drawCalls;
    std::vector<DrawCall> blendDrawCalls;
};
