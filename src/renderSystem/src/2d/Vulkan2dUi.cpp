#include <array>
#include <memory>
#include <renderSystem/2d/Vulkan2dContext.hpp>
#include <renderSystem/VulkanContext.hpp>

uint32_t Vulkan2dContext::AddUiGeometry(std::span<const Vertex2d> vertices,
                                        std::span<const uint32_t> indices) {
    auto geometry = std::make_unique<UiGeometryEntry>();

    vk::raii::CommandBuffer commandBuffer = base_->BeginSingleTimeCommand();
    std::vector<vk::raii::Buffer>       stagingBuffers;
    std::vector<vk::raii::DeviceMemory> stagingMemories;

    auto [vertexBuffer, vertexBufferMemory] =
        base_->CreateUniversalBuffer<Vertex2d>(
            commandBuffer,
            stagingBuffers,
            stagingMemories,
            vertices,
            vk::BufferUsageFlagBits::eVertexBuffer);
    geometry->vertexBuffer = std::move(vertexBuffer);
    geometry->vertexBufferMemory = std::move(vertexBufferMemory);

    auto [indexBuffer, indexBufferMemory] =
        base_->CreateUniversalBuffer<uint32_t>(
            commandBuffer,
            stagingBuffers,
            stagingMemories,
            indices,
            vk::BufferUsageFlagBits::eIndexBuffer);
    geometry->indexBuffer = std::move(indexBuffer);
    geometry->indexBufferMemory = std::move(indexBufferMemory);
    geometry->indexCount = static_cast<uint32_t>(indices.size());

    base_->EndSingleTimeCommand(std::move(commandBuffer));

    uiGeometries_.push_back(std::move(geometry));

    // Handle 0 is reserved as "invalid".
    return static_cast<uint32_t>(uiGeometries_.size());
}

void Vulkan2dContext::ReleaseUiGeometry(uint32_t geometryHandle) {
    [[unlikely]]if (geometryHandle == 0) {
        return;
    }

    const auto index = static_cast<size_t>(geometryHandle - 1);
    if (index >= uiGeometries_.size()) {
        return;
    }

    base_->device_.waitIdle();
    uiGeometries_[index].reset();
}

vk::raii::DescriptorSet Vulkan2dContext::CreateUiTextureDescriptorSet(
    const VkTexture& texture) const {
    vk::DescriptorSetAllocateInfo allocInfo{
        .descriptorPool = base_->generalDescriptorPool_,
        .descriptorSetCount = 1,
        .pSetLayouts = &*descriptorSetLayout2d_};

    auto descriptorSet =
        std::move(base_->device_.allocateDescriptorSets(allocInfo)[0]);

    vk::DescriptorImageInfo imageInfo{
        .sampler = texture.textureSampler,
        .imageView = texture.textureImageView,
        .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal
    };

    std::array writes{
        vk::WriteDescriptorSet{
            .dstSet = descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = vk::DescriptorType::eCombinedImageSampler,
            .pImageInfo = &imageInfo
        }
    };
    base_->device_.updateDescriptorSets(writes, {});

    return descriptorSet;
}

Vulkan2dContext::UiTextureHandle Vulkan2dContext::AddUiTexture(const ImageData& image) {
    auto texture = std::make_unique<UiTextureEntry>();

    vk::raii::CommandBuffer commandBuffer = base_->BeginSingleTimeCommand();
    std::vector<vk::raii::Buffer>       stagingBuffers;
    std::vector<vk::raii::DeviceMemory> stagingMemories;

    texture->texture = base_->CreateVkTextureFromImageData(
        commandBuffer, stagingBuffers, stagingMemories, image);
    texture->width = image.width;
    texture->height = image.height;
    texture->descriptorSet = CreateUiTextureDescriptorSet(texture->texture);

    base_->EndSingleTimeCommand(std::move(commandBuffer));

    uiTextures_.push_back(std::move(texture));

    return uiTextures_.size();
}

void Vulkan2dContext::ReleaseUiTexture(UiTextureHandle textureHandle) {
    if (textureHandle == 0) {
        return;
    }

    const auto index = static_cast<size_t>(textureHandle - 1);
    if (index >= uiTextures_.size()) {
        return;
    }

    base_->device_.waitIdle();
    uiTextures_[index].reset();
}

void Vulkan2dContext::AddUiDrawCall(uint32_t         geometryHandle,
                                    const glm::mat4& model,
                                    UiTextureHandle  textureHandle,
                                    bool             hasTexture,
                                    bool             scissorEnabled,
                                    vk::Rect2D       scissor) {
    if (geometryHandle == 0) {
        return;
    }

    const auto geometryIndex = static_cast<size_t>(geometryHandle - 1);
    if (geometryIndex >= uiGeometries_.size() ||
        !uiGeometries_[geometryIndex]) {
        return;
    }

    const auto& geometry = *uiGeometries_[geometryIndex];

    DrawCall2d drawCall;
    drawCall.mat = model;
    drawCall.color = glm::vec4(1.0f);
    drawCall.hasTexture = hasTexture;
    drawCall.indexCount = geometry.indexCount;
    drawCall.vertexBuffer_ = *geometry.vertexBuffer;
    drawCall.indexBuffer_ = *geometry.indexBuffer;
    drawCall.scissorEnabled = scissorEnabled;
    drawCall.scissor_ = scissor;

    if (textureHandle != 0) {
        const auto textureIndex = textureHandle - 1;
        if (textureIndex < uiTextures_.size() && uiTextures_[textureIndex]) {
            drawCall.descriptorSet_ = *uiTextures_[textureIndex]->descriptorSet;
        }
    }

    drawCalls2d_.push_back(drawCall);
}
