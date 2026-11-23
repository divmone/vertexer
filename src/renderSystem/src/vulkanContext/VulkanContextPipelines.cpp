
#include <chrono>
#include <cmath>
#include <fstream>
#include <logSystem/Logger.hpp>
#include <platformSystem/NativeVkSurfaceKHR.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <resourceSystem/AssetLoader.hpp>
#include <sceneSystem/Scene.hpp>

#include "FileSystem.hpp"
#include "renderSystem/detail/DescriptorSetLayoutBuilder.hpp"
#include "renderSystem/detail/PipelineBuilder.hpp"
#include "types/VulkanVertex2d.hpp"
#include "types/VulkanVertex3d.hpp"

void VulkanContext::CreateGraphicsPipeline() {
    auto shaderModule = CreateShaderModule(
        LoadShader(FS::GetFullPath(FS::FileType::eShader, "shader")));

    pipelineLayout_ = PipelineBuilder::CreatePipelineLayout(
        device_,
        {{
            .stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
            .offset = 0,
            .size = sizeof(PushConstantObject3d),
        }},
        {*descriptorSetLayout3d_, *blendingDescriptorSetLayout_});

    PipelineBuilder builder;
    graphicsPipeline_ = builder.Shaders(shaderModule)
                            .ColorFormat(swapChainSurfaceFormat_.format)
                            .DepthFormat(depthFormat_)
                            .VertexType<VulkanVertex3d>()
                            .DepthTestOn()
                            .Layout(pipelineLayout_)
                            .Build(device_);

    PipelineBuilder blendPipelineBuilder;
    blendingPipeline_ = blendPipelineBuilder.Shaders(shaderModule)
                           .ColorFormat(swapChainSurfaceFormat_.format)
                           .DepthFormat(depthFormat_)
                           .VertexType<VulkanVertex3d>()
                           .BlendingOn()
                           .DepthTestOff()
                           .CullMode(vk::CullModeFlagBits::eBack)
                           .FrontFace(vk::FrontFace::eCounterClockwise)
                           .Layout(pipelineLayout_)
                           .Build(device_);

    shaderModule = CreateShaderModule(
        LoadShader(FS::GetFullPath(FS::FileType::eShader, "sky")));

    skyPipelineLayout_ = PipelineBuilder::CreatePipelineLayout(
        device_, nullptr,
        {*descriptorSetLayout3d_, *blendingDescriptorSetLayout_});

    PipelineBuilder skyPipelineBuilder;
    blendingPipeline_ = skyPipelineBuilder.Shaders(shaderModule)
                           .ColorFormat(swapChainSurfaceFormat_.format)
                           .DepthFormat(depthFormat_)
                           .VertexType<VulkanVertex3d>()
                           .CullMode(vk::CullModeFlagBits::eBack)
                           .FrontFace(vk::FrontFace::eCounterClockwise)
                           .Layout(pipelineLayout_)
                           .Build(device_);
}


void VulkanContext::CreateGraphicsPipeline2d() {
    auto shaderModule = CreateShaderModule(
        LoadShader(FS::GetFullPath(FS::FileType::eShader, "shader_2d")));

    pipelineLayout2d_ = PipelineBuilder::CreatePipelineLayout(
        device_,
        {{
            .stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
            .offset = 0,
            .size = sizeof(PushConstantObject2d),
        }},
        {*descriptorSetLayout2d_});

    PipelineBuilder builder;

    graphicsPipeline2d_ = builder.Shaders(shaderModule)
                              .ColorFormat(swapChainSurfaceFormat_.format)
                              .VertexType<VulkanVertex2d>()
                              .DepthTestOff()
                              .DepthFormat(depthFormat_)
                              .PremultipliedBlendingOn()
                              .Layout(pipelineLayout2d_)
                              .Build(device_);
}


void VulkanContext::CreateBlendingPipeline() {
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


void VulkanContext::CreateDescriptorSetLayout() {
    DescriptorSetLayoutBuilder builder;
    builder.Add({.binding = 0,
                 .descriptorType = vk::DescriptorType::eUniformBuffer,
                 .descriptorCount = 1,
                 .stageFlags = vk::ShaderStageFlagBits::eAllGraphics});

    builder.Add({.binding = 1,
                 .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                 .descriptorCount = 1,
                 .stageFlags = vk::ShaderStageFlagBits::eFragment});

    descriptorSetLayout3d_ = builder.Build(device_);
    // graphicsPSO_ = std::make_unique<PSO>(m_device, MAX_FRAMES_IN_FLIGHT);
}


void VulkanContext::CreateDescriptorSetLayout2d() {
    DescriptorSetLayoutBuilder builder;
    descriptorSetLayout2d_ =
        builder
            .Add({.binding = 0,
                  .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                  .descriptorCount = 1,
                  .stageFlags = vk::ShaderStageFlagBits::eFragment})
            .Build(device_);
}


void VulkanContext::CreateUniformBuffers() {
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DeviceSize bufferSize = sizeof(UBObject);
        auto [buffer, buffMemory] =
            CreateBuffer(bufferSize, vk::BufferUsageFlagBits::eUniformBuffer,
                         vk::MemoryPropertyFlagBits::eHostVisible |
                             vk::MemoryPropertyFlagBits::eHostCoherent);
        uniformBuffers_.emplace_back(std::move(buffer));
        uniformDevicesMemory_.emplace_back(std::move(buffMemory));
        uniformBuffersMapped_.emplace_back(
            uniformDevicesMemory_.back().mapMemory(0, bufferSize));
    }
}


void VulkanContext::UpdateUniformBuffer(uint32_t index) const {
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

    ubo.lightPos = glm::vec3(
        lightRadius * std::cos(time * lightSpeed),
        lightRadius * std::sin(time * lightSpeed), lightHeight);

    ubo.lightColor = {1.0, 1.0, 1.0};
    ubo.ambient = {0.1, 0.1, 0.1};
    ubo.cameraPos = glm::vec3(glm::inverse(view_)[3]);

    std::memcpy(uniformBuffersMapped_[index], &ubo, sizeof(ubo));
}


void VulkanContext::CreateDescriptorPool() {
    std::array poolSizes{
        vk::DescriptorPoolSize{.type = vk::DescriptorType::eUniformBuffer,
                               .descriptorCount = MAX_FRAMES_IN_FLIGHT},
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = MAX_BINDLESS_RESOURCES},
    };

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = MAX_BINDLESS_RESOURCES,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()};
    generalDescriptorPool_ = vk::raii::DescriptorPool(device_, poolInfo);
}


void VulkanContext::CreateBindlessDescriptorPool() {
    std::array poolSizes{
        vk::DescriptorPoolSize{
            .type = vk::DescriptorType::eCombinedImageSampler,
            .descriptorCount = MAX_BINDLESS_RESOURCES},
    };

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = MAX_BINDLESS_RESOURCES * sizeof(poolSizes[0]),
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()};
    generalDescriptorPool_ = vk::raii::DescriptorPool(device_, poolInfo);
}


void VulkanContext::CreateDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                                 *descriptorSetLayout3d_);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = generalDescriptorPool_,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()};

    descriptorSets_ = device_.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vk::DescriptorBufferInfo bufferInfo{.buffer = uniformBuffers_[i],
                                            .offset = 0,
                                            .range = sizeof(UBObject)};
        std::array writes{vk::WriteDescriptorSet{
            .dstSet = descriptorSets_[i],
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eUniformBuffer,
            .pBufferInfo = &bufferInfo}};
        device_.updateDescriptorSets(writes, {});
    }
}


void VulkanContext::CreateBlendDescriptorSetLayout() {
    DescriptorSetLayoutBuilder builder;

    blendingDescriptorSetLayout_ =
        builder
            .Add({.binding = 0,
                  .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                  .descriptorCount = 1,
                  .stageFlags = vk::ShaderStageFlagBits::eFragment})
            .Build(device_);
}


void VulkanContext::CreateTextureDescriptorSet() {
    std::vector<vk::DescriptorSetLayout> layouts(textureCounter_,
                                                 *blendingDescriptorSetLayout_);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = generalDescriptorPool_,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()};

    blendDescriptorSets_ = device_.allocateDescriptorSets(allocInfo);

    size_t index = 0;
    for (auto & model : models_) {
        for (const auto & texture : model.textures) {
            vk::DescriptorImageInfo imageInfo{
                .sampler = texture.textureSampler,
                .imageView = texture.textureImageView,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};

            std::array writes{vk::WriteDescriptorSet{
                .dstSet = blendDescriptorSets_[index++],
                .dstBinding = 0,
                .dstArrayElement = 0,
                .descriptorCount = 1,
                .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                .pImageInfo = &imageInfo}};
            device_.updateDescriptorSets(writes, {});
        }
    }
}


vk::raii::ShaderModule VulkanContext::CreateShaderModule(
    const std::vector<char>& code) const {
    if (code.empty() || code.size() % sizeof(uint32_t) != 0) {
        LOG_ERROR("Invalid SPIR-V shader code size");
        throw std::runtime_error("Invalid SPIR-V shader code size");
    }

    vk::ShaderModuleCreateInfo shaderModuleCreateInfo{
        .codeSize = code.size() * sizeof(char),
        .pCode = reinterpret_cast<const uint32_t*>(code.data())};

    try {
        return vk::raii::ShaderModule{device_, shaderModuleCreateInfo};
    } catch (const vk::SystemError& e) {
        LOG_ERROR("Failed to create shader module: {}", e.what());
        throw std::runtime_error(
            std::string("Failed to create shader module: ") + e.what());
    }
};


std::vector<char> VulkanContext::LoadShader(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file) {
        LOG_ERROR("Failed to open file: {}", filename);
        throw std::runtime_error("Failed to open file: " + filename);
    }

    std::vector<char> buffer(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));

    file.close();
    return buffer;
}


void VulkanContext::CreateDescriptorSet2d() {

}


void VulkanContext::CreateBindlessDescriptorSetLayout() {
    DescriptorSetLayoutBuilder builder;
    bindlessDescriptorSetLayout_ =
        builder
            .Add({.binding = 100,
                  .descriptorType = vk::DescriptorType::eCombinedImageSampler,
                  .descriptorCount = MAX_BINDLESS_RESOURCES})
            .BuildBindLess(device_);
}


void VulkanContext::CreateBindlessDescriptorSet() {
    //
    //
    // vk::DescriptorSetVariableDescriptorCountAllocateInfo variableInfo{
    //     .descriptorSetCount = 1, .pDescriptorCounts = };
    //
    // vk::DescriptorSetAllocateInfo allocInfo{
    //     .descriptorPool = descriptorPool_,
    //     .descriptorSetCount = 1,
    //     .pSetLayouts = &*bindlessDescriptorSetLayout_,
    //     .pNext = };
    //
    // bindlessDescriptorSet_ =
    //     std::move(device_.allocateDescriptorSets(allocInfo)[0]);
    //
    // size_t index = 0;

    // const auto& texture = model2d.textures[0];
    // vk::DescriptorImageInfo imageInfo{
    //     .sampler = texture.textureSampler,
    //     .imageView = texture.textureImageView,
    //     .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal};
    //
    // std::array writes{vk::WriteDescriptorSet{
    //     .dstSet = descriptorSets2d_[index++],
    //     .dstBinding = 0,
    //     .dstArrayElement = 0,
    //     .descriptorCount = 1,
    //     .descriptorType = vk::DescriptorType::eCombinedImageSampler,
    //     .pImageInfo = &imageInfo}};
    // device_.updateDescriptorSets(writes, {});
}

//void VulkanContext::DrawSprites(vk::raii::CommandBuffer& commandBuffer) {}


void VulkanContext::FinishLoading() {
    CreateDescriptorSets();
    CreateTextureDescriptorSet();
    // createDescriptorSet2d();
    //  initImGui();
}


