//
// Created by divmone on 9/24/2026.
//

#include <renderSystem/detail/PipelineBuilder.hpp>

PipelineBuilder& PipelineBuilder::Layout(
    const vk::PipelineLayout& pipelineLayout) {
    layout_ = pipelineLayout;
    return *this;
}

PipelineBuilder& PipelineBuilder::ColorFormat(vk::Format f) {
    colorFormat_ = f;
    return *this;
}

PipelineBuilder& PipelineBuilder::DepthFormat(vk::Format f) {
    depthFormat_ = f;
    return *this;
}

PipelineBuilder& PipelineBuilder::BlendingOn() {
    colorBlendAttachment_ = {
        .blendEnable = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eSrcAlpha,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

    return *this;
}

PipelineBuilder& PipelineBuilder::PremultipliedBlendingOn() {
    colorBlendAttachment_ = {
        .blendEnable = vk::True,
        .srcColorBlendFactor = vk::BlendFactor::eOne,
        .dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .colorBlendOp = vk::BlendOp::eAdd,
        .srcAlphaBlendFactor = vk::BlendFactor::eOne,
        .dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha,
        .alphaBlendOp = vk::BlendOp::eAdd,
        .colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

    return *this;
}

PipelineBuilder& PipelineBuilder::BlendingOff() {
    colorBlendAttachment_ = {.blendEnable = vk::False,
                             .colorWriteMask = vk::ColorComponentFlagBits::eR |
                                               vk::ColorComponentFlagBits::eG |
                                               vk::ColorComponentFlagBits::eB |
                                               vk::ColorComponentFlagBits::eA};
    return *this;
}

PipelineBuilder& PipelineBuilder::DepthTestOn() {
    depthStencilStateInfo_ = {.depthTestEnable = vk::True,
                              .depthWriteEnable = vk::True,
                              .depthCompareOp = vk::CompareOp::eLess,
                              .depthBoundsTestEnable = vk::False,
                              .stencilTestEnable = vk::False};

    return *this;
}

PipelineBuilder& PipelineBuilder::DepthTestOff() {
    depthStencilStateInfo_ = {
        .depthTestEnable = vk::False,
        .depthWriteEnable = vk::False,
        .depthCompareOp = vk::CompareOp::eAlways,
    };

    return *this;
}

PipelineBuilder& PipelineBuilder::FrontFace(vk::FrontFace f) {
    frontFace_ = f;
    return *this;
}

PipelineBuilder& PipelineBuilder::CullMode(vk::CullModeFlags f) {
    cullMode_ |= f;
    return *this;
}

PipelineBuilder& PipelineBuilder::Shaders(const vk::ShaderModule& shaderModule,
                                          const char*             vertName,
                                          const char*             fragName) {
    stages_ = {{{.stage = vk::ShaderStageFlagBits::eVertex,
                 .module = shaderModule,
                 .pName = vertName},
                {.stage = vk::ShaderStageFlagBits::eFragment,
                 .module = shaderModule,
                 .pName = fragName}}};

    return *this;
}

vk::raii::Pipeline PipelineBuilder::Build(const vk::raii::Device& device) {
    constexpr std::array dynamicStates{vk::DynamicState::eViewport,
                                       vk::DynamicState::eScissor};

    vk::PipelineViewportStateCreateInfo viewportState{.viewportCount = 1,
                                                      .scissorCount = 1};

    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo{
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()};

    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{
        .topology = vk::PrimitiveTopology::eTriangleList};

    vk::PipelineRasterizationStateCreateInfo rasterizer{
        .depthClampEnable = vk::False,
        .rasterizerDiscardEnable = vk::False,
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = cullMode_,
        .frontFace = frontFace_,
        .depthBiasEnable = vk::True,
        .lineWidth = 1.0f};

    vk::PipelineMultisampleStateCreateInfo multisampling{
        .rasterizationSamples = vk::SampleCountFlagBits::e1,
        .sampleShadingEnable = vk::False};

    vk::PipelineColorBlendStateCreateInfo colorBlending{
        .logicOpEnable = vk::False,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment_};

    vk::PipelineRenderingCreateInfo pipelineRenderingCreateInfo{
        .colorAttachmentCount = 1, .pColorAttachmentFormats = &colorFormat_};

    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexInputBindingDescription_,
        .vertexAttributeDescriptionCount =
            static_cast<uint32_t>(vertexInputAttributeDescription_.size()),
        .pVertexAttributeDescriptions = vertexInputAttributeDescription_.data(),
    };

    vk::StructureChain<vk::GraphicsPipelineCreateInfo,
                       vk::PipelineRenderingCreateInfo>
        pipelineCreateInfoChain{
            {.stageCount = static_cast<uint32_t>(stages_.size()),
             .pStages = stages_.data(),
             .pVertexInputState = &vertexInputStateCreateInfo,
             .pInputAssemblyState = &inputAssemblyStateCreateInfo,
             .pViewportState = &viewportState,
             .pRasterizationState = &rasterizer,
             .pMultisampleState = &multisampling,
             .pDepthStencilState = &depthStencilStateInfo_,
             .pColorBlendState = &colorBlending,
             .pDynamicState = &dynamicStateCreateInfo,
             .layout = layout_,
             .renderPass = nullptr},
            {.colorAttachmentCount = 1,
             .pColorAttachmentFormats = &colorFormat_,
             .depthAttachmentFormat = depthFormat_}};

    return {device,
            nullptr,
            pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>()};
}

vk::raii::PipelineLayout PipelineBuilder::CreatePipelineLayout(
    const vk::raii::Device&                       device,
    vk::ArrayProxy<const vk::PushConstantRange>   ranges,
    vk::ArrayProxy<const vk::DescriptorSetLayout> layouts) {
    vk::PipelineLayoutCreateInfo createInfo{
        .setLayoutCount = layouts.size(),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = ranges.size(),
        .pPushConstantRanges = ranges.data()};

    return {device, createInfo};
}
