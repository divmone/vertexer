//
// Created by divmone on 9/24/2026.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

class PipelineBuilder {
public:
    template <typename T>
    PipelineBuilder& VertexType() {
        vertexInputBindingDescription_ = T::GetBindingDescription();
        auto attrs = T::GetAttributeDescriptions();
        vertexInputAttributeDescription_.assign(attrs.begin(), attrs.end());
        return *this;
    }

    PipelineBuilder& Layout(const vk::PipelineLayout& pipelineLayout);
    PipelineBuilder& ColorFormat(vk::Format f);
    PipelineBuilder& DepthFormat(vk::Format f);
    PipelineBuilder& BlendingOn();
    PipelineBuilder& PremultipliedBlendingOn();
    PipelineBuilder& BlendingOff();
    PipelineBuilder& DepthTestOn();
    PipelineBuilder& DepthTestOff();
    PipelineBuilder& FrontFace(vk::FrontFace f);
    PipelineBuilder& CullMode(vk::CullModeFlags f);

    PipelineBuilder& Shaders(const vk::ShaderModule& shaderModule,
                             const char* vertName = "vertMain",
                             const char* fragName = "fragMain");
    vk::raii::Pipeline Build(const vk::raii::Device& device);

    static vk::raii::PipelineLayout CreatePipelineLayout(
        const vk::raii::Device& device,
        vk::ArrayProxy<const vk::PushConstantRange> ranges,
        vk::ArrayProxy<const vk::DescriptorSetLayout> layouts);
private:
    vk::VertexInputBindingDescription vertexInputBindingDescription_;
    std::vector<vk::VertexInputAttributeDescription>
        vertexInputAttributeDescription_;
    vk::PipelineLayout layout_;
    vk::Format colorFormat_{vk::Format::eUndefined};
    vk::Format depthFormat_{vk::Format::eUndefined};

    vk::CullModeFlags cullMode_{vk::CullModeFlagBits::eNone};
    vk::FrontFace frontFace_{vk::FrontFace::eCounterClockwise};

    std::array<vk::PipelineShaderStageCreateInfo, 2> stages_;
    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo_;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment_{
        .blendEnable = vk::False,
        .colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
    vk::PipelineDepthStencilStateCreateInfo depthStencilStateInfo_{
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::False,
        .depthCompareOp = vk::CompareOp::eLess,
    };
};
