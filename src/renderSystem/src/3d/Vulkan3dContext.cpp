#include <chrono>
#include <cmath>
#include <cstring>
#include <fstream>
#include <logSystem/Logger.hpp>
#include <renderSystem/3d/Vulkan3dContext.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <renderSystem/detail/DescriptorSetLayoutBuilder.hpp>
#include <renderSystem/detail/PipelineBuilder.hpp>
#include <renderSystem/types/3d/PushConstantObject3d.hpp>
#include <renderSystem/types/3d/UBObject.hpp>
#include <resourceSystem/AssetLoader.hpp>

#include "FileSystem.hpp"
#include <renderSystem/types/3d/VulkanVertex3d.hpp>

void Vulkan3dContext::Init() {
    CreateDescriptorSetLayout();
    CreateBlendDescriptorSetLayout();
    // Bindless descriptor sets are not used by the UI-only renderer yet.
    // Creating the layout without the required device features triggers
    // VUID-VkDescriptorSetLayoutBindingFlagsCreateInfo errors.
    CreateUniformBuffers();
    CreateGraphicsPipeline();
    CreateBlendingPipeline();
}

void Vulkan3dContext::SetViewMatrix(const glm::mat4& matrix) {
    base_->view_ = matrix;
}

void Vulkan3dContext::SetProjMatrix(const glm::mat4& matrix) {
    base_->proj_ = matrix;
}

void Vulkan3dContext::CreateGraphicsPipeline() {
    auto shaderModule = base_->CreateShaderModule(
        base_->LoadShader(FS::GetFullPath(FS::FileType::eShader, "shader")));

    base_->pipelineLayout_ = PipelineBuilder::CreatePipelineLayout(
        base_->device_,
        {{
            .stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
            .offset = 0,
            .size = sizeof(PushConstantObject3d),
        }},
        {*base_->descriptorSetLayout3d_, *base_->blendingDescriptorSetLayout_});

    PipelineBuilder builder;
    base_->graphicsPipeline_ =
        builder.Shaders(shaderModule)
            .ColorFormat(base_->swapChainSurfaceFormat_.format)
            .DepthFormat(base_->depthFormat_)
            .VertexType<VulkanVertex3d>()
            .DepthTestOn()
            .Layout(base_->pipelineLayout_)
            .Build(base_->device_);

    PipelineBuilder blendPipelineBuilder;
    base_->blendingPipeline_ =
        blendPipelineBuilder.Shaders(shaderModule)
            .ColorFormat(base_->swapChainSurfaceFormat_.format)
            .DepthFormat(base_->depthFormat_)
            .VertexType<VulkanVertex3d>()
            .BlendingOn()
            .DepthTestOff()
            .CullMode(vk::CullModeFlagBits::eBack)
            .FrontFace(vk::FrontFace::eCounterClockwise)
            .Layout(base_->pipelineLayout_)
            .Build(base_->device_);

    shaderModule = base_->CreateShaderModule(
        base_->LoadShader(FS::GetFullPath(FS::FileType::eShader, "sky")));

    base_->skyPipelineLayout_ = PipelineBuilder::CreatePipelineLayout(
        base_->device_,
        nullptr,
        {*base_->descriptorSetLayout3d_, *base_->blendingDescriptorSetLayout_});

    PipelineBuilder skyPipelineBuilder;
    base_->blendingPipeline_ =
        skyPipelineBuilder.Shaders(shaderModule)
            .ColorFormat(base_->swapChainSurfaceFormat_.format)
            .DepthFormat(base_->depthFormat_)
            .VertexType<VulkanVertex3d>()
            .CullMode(vk::CullModeFlagBits::eBack)
            .FrontFace(vk::FrontFace::eCounterClockwise)
            .Layout(base_->pipelineLayout_)
            .Build(base_->device_);
}

void Vulkan3dContext::CreateBlendingPipeline() {
    // Body was already commented out in VulkanContext; behaviour is preserved.
}

void Vulkan3dContext::CreateDescriptorSetLayout() {
    DescriptorSetLayoutBuilder builder;
    builder.Add({.binding = 0,
                 .descriptorType = vk::DescriptorType::eUniformBuffer,
                 .descriptorCount = 1,
                 .stageFlags = vk::ShaderStageFlagBits::eAllGraphics});

    builder.Add({.binding = 1,
                 .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                 .descriptorCount = 1,
                 .stageFlags = vk::ShaderStageFlagBits::eFragment});

    base_->descriptorSetLayout3d_ = builder.Build(base_->device_);
}

void Vulkan3dContext::CreateUniformBuffers() {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DeviceSize bufferSize = sizeof(UBObject);
        auto [buffer, buffMemory] =
            base_->CreateBuffer(bufferSize,
                                vk::BufferUsageFlagBits::eUniformBuffer,
                                vk::MemoryPropertyFlagBits::eHostVisible |
                                    vk::MemoryPropertyFlagBits::eHostCoherent);
        base_->uniformBuffers_.emplace_back(std::move(buffer));
        base_->uniformDevicesMemory_.emplace_back(std::move(buffMemory));
        base_->uniformBuffersMapped_.emplace_back(
            base_->uniformDevicesMemory_.back().mapMemory(0, bufferSize));
    }
}

void Vulkan3dContext::UpdateUniformBuffer(uint32_t index) const {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto  currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float>(currentTime - startTime).count();

    UBObject ubo{};
    ubo.view = base_->view_;
    ubo.proj = base_->proj_;
    ubo.proj[1][1] *= -1;

    float lightRadius = 5.0f;
    float lightHeight = 3.0f;
    float lightSpeed = glm::radians(90.0f);

    ubo.lightPos = glm::vec3(lightRadius * std::cos(time * lightSpeed),
                             lightRadius * std::sin(time * lightSpeed),
                             lightHeight);

    ubo.lightColor = {1.0, 1.0, 1.0};
    ubo.ambient = {0.1, 0.1, 0.1};
    ubo.cameraPos = glm::vec3(glm::inverse(base_->view_)[3]);

    std::memcpy(base_->uniformBuffersMapped_[index], &ubo, sizeof(ubo));
}

void Vulkan3dContext::CreateDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(
        MAX_FRAMES_IN_FLIGHT, *base_->descriptorSetLayout3d_);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = base_->generalDescriptorPool_,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()};

    base_->descriptorSets_ = base_->device_.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo{.buffer = base_->uniformBuffers_[i],
                                            .offset = 0,
                                            .range = sizeof(UBObject)};
        std::array               writes{vk::WriteDescriptorSet{
            .dstSet = base_->descriptorSets_[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo}};
        base_->device_.updateDescriptorSets(writes, {});
    }
}

void Vulkan3dContext::CreateBlendDescriptorSetLayout() {
    DescriptorSetLayoutBuilder builder;

    base_->blendingDescriptorSetLayout_ =
        builder
            .Add({.binding = 0,
                  .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                  .descriptorCount = 1,
                  .stageFlags = vk::ShaderStageFlagBits::eFragment})
            .Build(base_->device_);
}

void Vulkan3dContext::CreateTextureDescriptorSet() {
    std::vector<vk::DescriptorSetLayout> layouts(
        base_->textureCounter_, *base_->blendingDescriptorSetLayout_);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = base_->generalDescriptorPool_,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()};

    base_->blendDescriptorSets_ =
        base_->device_.allocateDescriptorSets(allocInfo);

    size_t index = 0;
    for (auto& model : base_->models_) {
        for (const auto& texture : model.textures) {
            vk::DescriptorImageInfo imageInfo{
                .sampler = texture.textureSampler,
                .imageView = texture.textureImageView,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

            std::array writes{vk::WriteDescriptorSet{
                .dstSet = base_->blendDescriptorSets_[index++],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &imageInfo}};
            base_->device_.updateDescriptorSets(writes, {});
        }
    }
}

void Vulkan3dContext::CreateBindlessDescriptorSetLayout() {
    DescriptorSetLayoutBuilder builder;
    base_->bindlessDescriptorSetLayout_ =
        builder
            .Add({.binding = 100,
                  .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                  .descriptorCount = MAX_BINDLESS_RESOURCES})
            .BuildBindLess(base_->device_);
}

void Vulkan3dContext::CreateBindlessDescriptorSet() {
    // Body was already commented out in VulkanContext; behaviour is preserved.
}

void Vulkan3dContext::FinishLoading() {
    CreateDescriptorSets();
    CreateTextureDescriptorSet();
}
