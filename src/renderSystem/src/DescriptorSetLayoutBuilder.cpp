//
// Created by Антон Мухин on 07.11.2026.
//

#include "DescriptorSetLayoutBuilder.hpp"

vk::raii::DescriptorSetLayout DescriptorSetLayoutBuilder::Build(
    const vk::raii::Device& device) {
    vk::DescriptorSetLayoutCreateInfo createInfo = {
        .bindingCount = static_cast<uint32_t>(bindings_.size()),
        .pBindings = bindings_.data()};

    return {device, createInfo};
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::Add(
    vk::DescriptorSetLayoutBinding binding) {
    bindings_.push_back(binding);
    return *this;
}
