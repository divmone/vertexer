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

        CreateDescriptorSetLayout(device, bindings);
        CreateDescriptorPool(device, poolSizes);
        CreateDescriptorSets(device, writes);
    }

    [[nodiscard]] vk::DescriptorSetLayout GetDescriptorSetLayout() const {
        return *descriptorSetLayout_;
    }

    [[nodiscard]] vk::DescriptorSet GetDescriptorSet(int index) const {
        return *descriptorSets_[index];
    }

    [[nodiscard]] vk::DescriptorPool GetDescriptorPool() const {
        return *descriptorPool_;
    }
private:
    uint32_t framesCount_{0};
    vk::raii::DescriptorPool descriptorPool_{nullptr};
    std::vector<vk::raii::DescriptorSet> descriptorSets_;
    vk::raii::DescriptorSetLayout descriptorSetLayout_{nullptr};

    void CreateDescriptorSetLayout(vk::raii::Device &device,
                                   vk::ArrayProxy<const vk::DescriptorSetLayoutBinding> bindings);

    void CreateDescriptorPool(vk::raii::Device &device, vk::ArrayProxy<const vk::DescriptorPoolSize> poolSizes);

    void CreateDescriptorSets(vk::raii::Device &device, vk::ArrayProxy<const vk::WriteDescriptorSet> writes);
};
