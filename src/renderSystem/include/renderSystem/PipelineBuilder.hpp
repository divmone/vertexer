//
// Created by divmone on 9/24/2026.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

class PipelineBuilder {
public:
    template <typename T>
    PipelineBuilder& vertexType() {
        vertexInputBindingDescription_ = T::getBindingDescription();
        auto attrs                     = T::getAttributeDescriptions();
        vertexInputAttributeDescription_.assign(attrs.begin(), attrs.end());
        return *this;
    }

    PipelineBuilder&   layout(const vk::PipelineLayout& pipelineLayout);
    PipelineBuilder&   colorFormat(vk::Format f);
    PipelineBuilder&   depthFormat(vk::Format f);
    PipelineBuilder&   blendingOn();
    PipelineBuilder&   blendingOff();
    PipelineBuilder&   depthTestOn();
    PipelineBuilder&   depthTestOff();
    PipelineBuilder&   shaders(const vk::ShaderModule& shaderModule,
                               const char*             vertName = "vertMain",
                               const char*             fragName = "fragMain");
    vk::raii::Pipeline build(const vk::raii::Device& device);

    static vk::raii::PipelineLayout createPipelineLayout(
        const vk::raii::Device&                       device,
        vk::ArrayProxy<const vk::PushConstantRange>   ranges,
        vk::ArrayProxy<const vk::DescriptorSetLayout> layouts);
private:
    vk::VertexInputBindingDescription vertexInputBindingDescription_;
    std::vector<vk::VertexInputAttributeDescription>
                       vertexInputAttributeDescription_;
    vk::PipelineLayout layout_;
    vk::Format         colorFormat_{vk::Format::eUndefined};
    vk::Format         depthFormat_{vk::Format::eUndefined};
    std::array<vk::PipelineShaderStageCreateInfo, 2> stages_;
    vk::PipelineVertexInputStateCreateInfo vertexInputStateCreateInfo_;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment_{
        .blendEnable = vk::False,
        .colorWriteMask =
            vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};
    vk::PipelineDepthStencilStateCreateInfo depthStencilStateInfo_{
        .depthTestEnable  = vk::True,
        .depthWriteEnable = vk::False,
        .depthCompareOp   = vk::CompareOp::eLess,
    };
};
