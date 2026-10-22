//
// Created by divmone on 9/24/2026.
//

#pragma once

#include <vulkan/vulkan_raii.hpp>

class PSO {
public:
    PSO(vk::raii::Device &device,
            const vk::ArrayProxy<const vk::DescriptorSetLayoutBinding> bindings,
            const vk::ArrayProxy<const vk::DescriptorPoolSize> poolSizes,
            const vk::ArrayProxy<const vk::WriteDescriptorSet> writes, uint32_t framesCount)
        : framesCount_(framesCount) {

        createDescriptorSetLayout(device, bindings);
        createDescriptorPool(device, poolSizes);
        createDescriptorSets(device, writes);
    }

    [[nodiscard]] vk::DescriptorSetLayout getDescriptorSetLayout() const {
        return *descriptorSetLayout_;
    }

    [[nodiscard]] vk::DescriptorSet getDescriptorSet(int index) const {
        return *descriptorSets_[index];
    }

    [[nodiscard]] vk::DescriptorPool getDescriptorPool() const {
        return *descriptorPool_;
    }
private:
    uint32_t framesCount_{0};
    vk::raii::DescriptorPool descriptorPool_{nullptr};
    std::vector<vk::raii::DescriptorSet> descriptorSets_;
    vk::raii::DescriptorSetLayout descriptorSetLayout_{nullptr};

    void createDescriptorSetLayout(vk::raii::Device &device,
                                   vk::ArrayProxy<const vk::DescriptorSetLayoutBinding> bindings);

    void createDescriptorPool(vk::raii::Device &device, vk::ArrayProxy<const vk::DescriptorPoolSize> poolSizes);

    void createDescriptorSets(vk::raii::Device &device, vk::ArrayProxy<const vk::WriteDescriptorSet> writes);
};
