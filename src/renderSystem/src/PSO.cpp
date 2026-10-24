//
// Created by divmone on 9/24/2026.
//

#include <renderSystem/PSO.hpp>

void PSO::createDescriptorSetLayout(
    vk::raii::Device& device,
    vk::ArrayProxy<const vk::DescriptorSetLayoutBinding> bindings) {
    vk::DescriptorSetLayoutCreateInfo createInfo{
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()};

    descriptorSetLayout_ = vk::raii::DescriptorSetLayout(device, createInfo);
}

void PSO::createDescriptorPool(
    vk::raii::Device& device,
    vk::ArrayProxy<const vk::DescriptorPoolSize> poolSizes) {
    uint32_t maxSets = 0;
    for (const auto& poolSize : poolSizes) {
        maxSets += poolSize.descriptorCount;
    }

    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = maxSets,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()};
    descriptorPool_ = vk::raii::DescriptorPool(device, poolInfo);
}

void PSO::createDescriptorSets(
    vk::raii::Device& device,
    vk::ArrayProxy<const vk::WriteDescriptorSet> writes) {
    std::vector layouts(framesCount_, *descriptorSetLayout_);
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = descriptorPool_,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data()};

    descriptorSets_ = device.allocateDescriptorSets(allocInfo);

    for (size_t i = 0; i < framesCount_; i++) {
        // std::array writes{
        //     vk::WriteDescriptorSet{
        //         .dstSet = descriptorSets_[i], .dstBinding = 0,
        //         .dstArrayElement = 0, .descriptorCount = 1, .descriptorType =
        //         vk::DescriptorType::eUniformBuffer, .pBufferInfo =
        //         &bufferInfo
        //     }
        // };
        device.updateDescriptorSets(writes, {});
    }
}
