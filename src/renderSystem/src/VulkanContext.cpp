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

VulkanContext::VulkanContext(NativeWindowHandler windowHandler,
                             const ExtentProvider& extentProvider)
    : nativeWindowHandler_(windowHandler), extentProvider_(extentProvider) {
    initVulkan();
}


VulkanContext::~VulkanContext() {
    device_.waitIdle();
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
    createDescriptorSetLayout2d();
    createBlendDescriptorSetLayout();
    createDepthResources();
    createGraphicsPipeline();
    CreateGraphicsPipeline2d();
    createBlendingPipeline();
    createCommandPool();
    createUniformBuffers();
    createCommandBuffer();
    createSyncObjects();
    createDescriptorPool();
}


void VulkanContext::processMinimizedWindow() {
    // int width = 0, height = 0;
    // while (width == 0 || height == 0) {
    //     SDL_GetWindowSizeInPixels(m_window, &width, &height);
    //     SDL_WaitEvent(nullptr);
    // }
}


void VulkanContext::setViewMatrix(const glm::mat4& viewMat) {
    view_ = viewMat;
}


void VulkanContext::setProjMatrix(const glm::mat4& m) {
    proj_ = m;
}


void VulkanContext::SetProj2dMatrix(const glm::mat4& m) {
    proj2d_ = m;
}


void VulkanContext::createTexture() {}


