#include <cstddef>
#include <filesystem>
#include <fstream>
#include <logSystem/Logger.hpp>
#include <renderSystem/3d/Vulkan3dContext.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <resourceSystem/AssetLoader.hpp>
#include <sceneSystem/Scene.hpp>

void Vulkan3dContext::ProcessNodes(unsigned         root,
                                   const glm::mat4& mat,
                                   const VkModel&   model,
                                   uint32_t         modelIndex) {
    const auto& node = model.nodes[root];
    glm::mat4   world = mat * node.transformation;

    if (node.meshIndex != -1) {
        const Mesh& mesh = model.meshes[node.meshIndex];
        for (const auto& prim : mesh.primitives) {
            base_->drawCalls_.push_back(DrawCall3d{
                .textureIndex = prim.textureIndex + model.textureOffset,
                .mat = world,
                .hasTexture = prim.hasTexture,
                .color = prim.color,
                .indexCount = prim.indexCount,
                .firstIndex = prim.firstIndex,
                .distanceToCam = 0,
                .modelIndex = modelIndex});
        }
        for (const auto& prim : mesh.blendPrimitives) {
            base_->blendDrawCalls_.push_back(DrawCall3d{
                .textureIndex = prim.textureIndex + model.textureOffset,
                .mat = world,
                .hasTexture = prim.hasTexture,
                .color = prim.color,
                .indexCount = prim.indexCount,
                .firstIndex = prim.firstIndex,
                .distanceToCam = 0,
                .modelIndex = modelIndex});
        }
    }

    for (const auto& child : node.children) {
        ProcessNodes(child, world, model, modelIndex);
    }
}

void Vulkan3dContext::DrawBlendNodes(unsigned         root,
                                     const glm::mat4& mat,
                                     const VkModel&   model) {}

std::vector<VkTexture> Vulkan3dContext::CreateTexturesFromImages(
    vk::raii::CommandBuffer&             commandBuffer,
    std::vector<vk::raii::Buffer>&       stagingBuffers,
    std::vector<vk::raii::DeviceMemory>& stagingMemories,
    const Model&                         model) {
    std::vector<VkTexture> textures;

    for (const auto& imageData : model.images) {
        VkTexture      texture;
        vk::DeviceSize imageSize =
            imageData.width * imageData.height * imageData.channels;
        auto [stagingBuffer, stagingBufferMemory] =
            base_->CreateBuffer(imageSize,
                                vk::BufferUsageFlagBits::eTransferSrc,
                                vk::MemoryPropertyFlagBits::eHostVisible |
                                    vk::MemoryPropertyFlagBits::eHostCoherent);
        void* data = stagingBufferMemory.mapMemory(0, imageSize);
        memcpy(data, imageData.data, imageSize);
        stagingBufferMemory.unmapMemory();

        std::tie(texture.textureImage, texture.textureImageMemory) =
            base_->CreateImage(imageData.width,
                               imageData.height,
                               vk::Format::eR8G8B8A8Srgb,
                               vk::ImageTiling::eOptimal,
                               vk::ImageUsageFlagBits::eTransferDst |
                                   vk::ImageUsageFlagBits::eSampled,
                               vk::MemoryPropertyFlagBits::eDeviceLocal);

        base_->TransitionImageLayout(commandBuffer,
                                     texture.textureImage,
                                     vk::ImageLayout::eUndefined,
                                     vk::ImageLayout::eTransferDstOptimal);
        base_->CopyBufferToImage(commandBuffer,
                                 stagingBuffer,
                                 texture.textureImage,
                                 static_cast<uint32_t>(imageData.width),
                                 static_cast<uint32_t>(imageData.height));
        base_->TransitionImageLayout(commandBuffer,
                                     texture.textureImage,
                                     vk::ImageLayout::eTransferDstOptimal,
                                     vk::ImageLayout::eShaderReadOnlyOptimal);

        texture.textureImageView =
            base_->CreateImageView(texture.textureImage,
                                   vk::Format::eR8G8B8A8Srgb,
                                   vk::ImageAspectFlagBits::eColor);

        vk::SamplerCreateInfo samplerInfo{};
        texture.textureSampler = vk::raii::Sampler(base_->device_, samplerInfo);

        textures.push_back(std::move(texture));
        stagingBuffers.push_back(std::move(stagingBuffer));
        stagingMemories.push_back(std::move(stagingBufferMemory));
        base_->textureCounter_++;
    }

    return textures;
}

VkModel Vulkan3dContext::LoadModel(const std::string& path) {
    VkModel vkModel;
    auto    model = AssetLoader::LoadModelFromGltf(path);

    vk::raii::CommandBuffer commandBuffer = base_->BeginSingleTimeCommand();
    std::vector<vk::raii::Buffer>       stagingBuffers;
    std::vector<vk::raii::DeviceMemory> stagingMemories;

    auto [vertexBuffer, vertexBufferMemory] =
        base_->CreateUniversalBuffer<Vertex3d>(
            commandBuffer,
            stagingBuffers,
            stagingMemories,
            model.vertices,
            vk::BufferUsageFlagBits::eVertexBuffer);
    vkModel.vertexBuffer = std::move(vertexBuffer);
    vkModel.vertexBufferMemory = std::move(vertexBufferMemory);

    auto [indexBuffer, indexBufferMemory] =
        base_->CreateUniversalBuffer<uint32_t>(
            commandBuffer,
            stagingBuffers,
            stagingMemories,
            model.indices,
            vk::BufferUsageFlagBits::eIndexBuffer);
    vkModel.indexBuffer = std::move(indexBuffer);
    vkModel.indexBufferMemory = std::move(indexBufferMemory);
    vkModel.indicesCount = model.indices.size();

    vkModel.meshes = std::move(model.meshes);
    vkModel.nodes = std::move(model.nodes);
    vkModel.rootNodes = std::move(model.rootNodes);
    vkModel.textureOffset = base_->textureCounter_;
    vkModel.textures = CreateTexturesFromImages(
        commandBuffer, stagingBuffers, stagingMemories, model);

    base_->EndSingleTimeCommand(std::move(commandBuffer));

    return vkModel;
}

SpriteHandle Vulkan3dContext::AddModel(const Model& model) {
    VkModel                 vkModel;
    vk::raii::CommandBuffer commandBuffer = base_->BeginSingleTimeCommand();
    std::vector<vk::raii::Buffer>       stagingBuffers;
    std::vector<vk::raii::DeviceMemory> stagingMemories;

    auto [vertexBuffer, vertexBufferMemory] =
        base_->CreateUniversalBuffer<Vertex3d>(
            commandBuffer,
            stagingBuffers,
            stagingMemories,
            model.vertices,
            vk::BufferUsageFlagBits::eVertexBuffer);
    vkModel.vertexBuffer = std::move(vertexBuffer);
    vkModel.vertexBufferMemory = std::move(vertexBufferMemory);

    auto [indexBuffer, indexBufferMemory] =
        base_->CreateUniversalBuffer<uint32_t>(
            commandBuffer,
            stagingBuffers,
            stagingMemories,
            model.indices,
            vk::BufferUsageFlagBits::eIndexBuffer);
    vkModel.indexBuffer = std::move(indexBuffer);
    vkModel.indexBufferMemory = std::move(indexBufferMemory);
    vkModel.indicesCount = model.indices.size();

    vkModel.meshes = model.meshes;
    vkModel.nodes = model.nodes;
    vkModel.rootNodes = model.rootNodes;
    vkModel.textureOffset = base_->textureCounter_;
    vkModel.textures = CreateTexturesFromImages(
        commandBuffer, stagingBuffers, stagingMemories, model);

    base_->EndSingleTimeCommand(std::move(commandBuffer));

    base_->models_.emplace_back(std::move(vkModel));

    return {static_cast<uint32_t>(base_->models_.size() - 1)};
}

uint32_t Vulkan3dContext::LoadAsset(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        LOG_ERROR("file not found: {}", path);
        throw std::runtime_error("file not found: " + path);
    }

    std::ifstream file(path);
    if (!file) {
        LOG_ERROR("Failed to open file: {}", path);
        throw std::runtime_error("Failed to open file: " + path);
    }

    base_->models_.push_back(LoadModel(path));
    return base_->models_.size() - 1;
}
