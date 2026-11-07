//
// Created by Антон Мухин on 07.11.2026.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

class DescriptorSetLayoutBuilder {
public:
    DescriptorSetLayoutBuilder() = default;
    ~DescriptorSetLayoutBuilder() = default;

    vk::raii::DescriptorSetLayout Build(const vk::raii::Device& device);
    DescriptorSetLayoutBuilder& Add(const vk::DescriptorSetLayoutBinding&& binding);
private:
    std::vector<vk::DescriptorSetLayoutBinding> bindings_;

};
