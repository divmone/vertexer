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
#include <renderSystem/types/2d/VulkanVertex2d.hpp>
#include <renderSystem/types/3d/VulkanVertex3d.hpp>

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
