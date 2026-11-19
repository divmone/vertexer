#include <PipelineBuilder.hpp>
#include <chrono>
#include <cmath>
#include <fstream>
#include <limits>
#include <logSystem/Logger.hpp>
#include <platformSystem/NativeSDLExtensions.hpp>
#include <platformSystem/NativeVkSurfaceKHR.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <resourceSystem/AssetLoader.hpp>
#include <sceneSystem/Scene.hpp>
#include <types/VulkanVertex3d.hpp>
#include <typesSystem/EngineStats.hpp>

#include "DescriptorSetLayoutBuilder.hpp"
#include "FileSystem.hpp"
#include "resourceSystem/types/Sprite.hpp"
#include "types/VulkanVertex2d.hpp"

struct EngineStats;

constexpr uint32_t MAX_BINDLESS_RESOURCES = 20000;

void VulkanContext::processNodes(unsigned root,
                                 const glm::mat4& mat,
                                 const VkModel& model,
                                 uint32_t modelIndex) {
    const auto& node = model.nodes[root];
    glm::mat4 world = mat * node.transformation;

    if (node.meshIndex != -1) {
        const Mesh& mesh = model.meshes[node.meshIndex];
        for (const auto& prim : mesh.primitives) {
            drawCalls.push_back(
                DrawCall {
                    .textureIndex=prim.textureIndex + model.textureOffset,
                    .mat=world,
                    .hasTexture=prim.hasTexture,
                    .color=prim.color,
                    .indexCount=prim.indexCount,
                    .firstIndex=prim.firstIndex,
                    .distanceToCam=0,
                    .modelIndex=modelIndex
                }
            );
        }
        for (const auto& prim : mesh.blendPrimitives) {
            blendDrawCalls.push_back(
                DrawCall {
                    .textureIndex=prim.textureIndex + model.textureOffset,
                    .mat=world,
                    .hasTexture=prim.hasTexture,
                    .color=prim.color,
                    .indexCount=prim.indexCount,
                    .firstIndex=prim.firstIndex,
                    .distanceToCam=0,
                    .modelIndex=modelIndex
                }
            );
        }
    }

    for (const auto& child : node.children) {
        processNodes(child, world, model, modelIndex);
    }
}


void VulkanContext::drawBlendNodes(unsigned root,
                                   const glm::mat4& mat,
                                   const VkModel& model) {}


/*
void VulkanContext::recordCommandBuffer(uint32_t index,
                                        Scene& scene,
                                        EngineStats& stats) {
    stats.drawcallCount = 0;
    stats.triangleCount = 0;

    auto& commandBuffer = m_commandBuffer[frameIndex];

    commandBuffer.begin({});
    transition_image_layout(m_swapChainImages[index],
                            vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eColorAttachmentOptimal, {},
                            vk::AccessFlagBits2::eColorAttachmentWrite,
                            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
                            vk::ImageAspectFlagBits::eColor);

    vk::ClearValue clearValue = vk::ClearColorValue(0.0f, 0.0f, 0.9f, 1.0f);
    vk::ClearValue clearDepth = vk::ClearDepthStencilValue(1.0f, 0);

    vk::RenderingAttachmentInfo renderingAttachmentInfo{
        .imageView = m_swapChainImageViews[index],
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = clearValue};

    vk::RenderingAttachmentInfo depthAttachmentInfo = {
        .imageView = depthImageView,
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = clearDepth};

    vk::RenderingInfo renderingInfo{
        .renderArea = {.offset = {.x = 0, .y = 0}, .extent = m_swapChainExtent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &renderingAttachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo};

    transition_image_layout(*depthImage, vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eDepthAttachmentOptimal,
                            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                            vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                            vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                                vk::PipelineStageFlagBits2::eLateFragmentTests,
                            vk::PipelineStageFlagBits2::eEarlyFragmentTests |
                                vk::PipelineStageFlagBits2::eLateFragmentTests,
                            vk::ImageAspectFlagBits::eDepth);

    commandBuffer.beginRendering(renderingInfo);

    commandBuffer.setViewport(
        0,
        vk::Viewport(0.0f, 0.0f, static_cast<float>(m_swapChainExtent.width),
                     static_cast<float>(m_swapChainExtent.height), 0.0f, 1.0f));
    commandBuffer.setScissor(0,
                             vk::Rect2D(vk::Offset2D(0, 0), m_swapChainExtent));

    for (const auto& obj : scene.GetObjects()) {
        const auto& model = models_[obj.modelIndex];
        for (const auto& root : model.rootNodes) {
            processNodes(root, obj.transform.getModelMat(), model,
                         obj.modelIndex);
        }
    }

    // for (const auto& sprite: sprites_) {
    //     DrawCall drawCall{
    //         .color = sprite.color_,
    //         .distanceToCam = 0,
    //         .hasTexture = sprite.hasTexture_,
    //         .firstIndex =
    //     };
    // }

    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                               graphicsPipeline2d_);

    RenderDrawCalls2d(commandBuffer);

    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                   graphicsPipeline_);

        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                         m_pipelineLayout, 0,
                                         *descriptorSets[frameIndex], nullptr);

        for (const auto& obj : scene.GetObjects()) {
            const auto& model = models_[obj.modelIndex];
            commandBuffer.bindVertexBuffers(0, *model.vertexBuffer, {0});
            commandBuffer.bindIndexBuffer(*model.indexBuffer, 0,
                                          vk::IndexType::eUint32);

            for (const auto& drawCall : drawCalls) {
                if (drawCall.modelIndex == obj.modelIndex) {
                    commandBuffer.bindDescriptorSets(
                        vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 1,
                        *blendDescriptorSets[drawCall.textureIndex], nullptr);

                    commandBuffer.pushConstants<PushConstantObject3d>(
                        m_pipelineLayout, vk::ShaderStageFlagBits::eAllGraphics,
                        0,
                        PushConstantObject3d{
                            drawCall.mat,
                            drawCall.color,
                            drawCall.hasTexture,
                        });

                    commandBuffer.drawIndexed(drawCall.indexCount, 1,
                                              drawCall.firstIndex, 0, 0);
                    stats.drawcallCount++;
                    stats.triangleCount += drawCall.indexCount / 3;
                }
            }
        }
    }

    {
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                   blendingPipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                         m_pipelineLayout, 0,
                                         *descriptorSets[frameIndex], nullptr);

        auto cameraPos = glm::vec3(glm::inverse(view_)[3]);

        for (auto& drawCall : blendDrawCalls) {
            drawCall.distanceToCam =
                glm::length2(cameraPos - glm::vec3(drawCall.mat[3]));
        }

        std::ranges::sort(blendDrawCalls,
                          [&](const DrawCall& a, const DrawCall& b) {
                              return a.distanceToCam > b.distanceToCam;
                          });

        uint32_t boundModelIndex = std::numeric_limits<uint32_t>::max();
        for (auto& drawCall : blendDrawCalls) {
            if (drawCall.modelIndex != boundModelIndex) {
                const auto& model = models_[drawCall.modelIndex];
                commandBuffer.bindVertexBuffers(0, *model.vertexBuffer, {0});
                commandBuffer.bindIndexBuffer(*model.indexBuffer, 0,
                                              vk::IndexType::eUint32);
                boundModelIndex = drawCall.modelIndex;
            }

            commandBuffer.bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 1,
                *blendDescriptorSets[drawCall.textureIndex], nullptr);

            commandBuffer.pushConstants<PushConstantObject3d>(
                m_pipelineLayout, vk::ShaderStageFlagBits::eAllGraphics, 0,
                PushConstantObject3d{drawCall.mat, drawCall.color,
                                     drawCall.hasTexture});

            commandBuffer.drawIndexed(drawCall.indexCount, 1,
                                      drawCall.firstIndex, 0, 0);
            stats.drawcallCount++;
            stats.triangleCount += drawCall.indexCount / 3;
        }
    }

    commandBuffer.endRendering();

    transition_image_layout(
        m_swapChainImages[index], vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,
        vk::AccessFlagBits2::eColorAttachmentWrite,          // srcAccessMask
        {},                                                  // dstAccessMask
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,  // srcStage
        vk::PipelineStageFlagBits2::eBottomOfPipe            // dstStage
        ,
        vk::ImageAspectFlagBits::eColor);

    commandBuffer.end();

    drawCalls.clear();
    blendDrawCalls.clear();
    drawCalls2d_.clear();
}*/



std::vector<VkTexture> VulkanContext::createTexturesFromImages(
    vk::raii::CommandBuffer& commandBuffer,
    std::vector<vk::raii::Buffer>& stagingBuffers,
    std::vector<vk::raii::DeviceMemory>& stagingMemories,
    const Model& model) {
    std::vector<VkTexture> textures;

    for (const auto& imageData : model.images_) {
        VkTexture texture;
        vk::DeviceSize imageSize =
            imageData.width * imageData.height * imageData.channels;
        auto [stagingBuffer, stagingBufferMemory] =
            createBuffer(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eHostVisible |
                             vk::MemoryPropertyFlagBits::eHostCoherent);
        void* data = stagingBufferMemory.mapMemory(0, imageSize);
        memcpy(data, imageData.data, imageSize);
        stagingBufferMemory.unmapMemory();

        std::tie(texture.textureImage, texture.textureImageMemory) =
            createImage(imageData.width, imageData.height,
                        vk::Format::eR8G8B8A8Srgb, vk::ImageTiling::eOptimal,
                        vk::ImageUsageFlagBits::eTransferDst |
                            vk::ImageUsageFlagBits::eSampled,
                        vk::MemoryPropertyFlagBits::eDeviceLocal);

        transitionImageLayout(commandBuffer, texture.textureImage,
                              vk::ImageLayout::eUndefined,
                              vk::ImageLayout::eTransferDstOptimal);
        copyBufferToImage(commandBuffer, stagingBuffer, texture.textureImage,
                          static_cast<uint32_t>(imageData.width),
                          static_cast<uint32_t>(imageData.height));
        transitionImageLayout(commandBuffer, texture.textureImage,
                              vk::ImageLayout::eTransferDstOptimal,
                              vk::ImageLayout::eShaderReadOnlyOptimal);

        texture.textureImageView =
            createImageView(texture.textureImage, vk::Format::eR8G8B8A8Srgb,
                            vk::ImageAspectFlagBits::eColor);

        vk::SamplerCreateInfo samplerInfo{};
        texture.textureSampler = vk::raii::Sampler(device_, samplerInfo);

        textures.push_back(std::move(texture));
        stagingBuffers.push_back(std::move(stagingBuffer));
        stagingMemories.push_back(std::move(stagingBufferMemory));
        textureCounter++;
    }

    return textures;
}


VkModel VulkanContext::loadModel(const std::string& path) {
    VkModel vkModel;
    auto model = AssetLoader::loadModelFromGltf(path);

    vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommand();
    std::vector<vk::raii::Buffer> stagingBuffers;
    std::vector<vk::raii::DeviceMemory> stagingMemories;

    auto [vertexBuffer, vertexBufferMemory] = createUniversalBuffer<Vertex3d>(
        commandBuffer, stagingBuffers, stagingMemories, model.vertices_,
        vk::BufferUsageFlagBits::eVertexBuffer);
    vkModel.vertexBuffer = std::move(vertexBuffer);
    vkModel.vertexBufferMemory = std::move(vertexBufferMemory);

    auto [indexBuffer, indexBufferMemory] = createUniversalBuffer<uint32_t>(
        commandBuffer, stagingBuffers, stagingMemories, model.indices,
        vk::BufferUsageFlagBits::eIndexBuffer);
    vkModel.indexBuffer = std::move(indexBuffer);
    vkModel.indexBufferMemory = std::move(indexBufferMemory);
    vkModel.indicesCount = model.indices.size();

    vkModel.meshes = std::move(model.meshes_);
    vkModel.nodes = std::move(model.nodes_);
    vkModel.rootNodes = std::move(model.rootNodes_);
    vkModel.textureOffset = textureCounter;
    vkModel.textures = createTexturesFromImages(commandBuffer, stagingBuffers,
                                                stagingMemories, model);

    endSingleTimeCommand(std::move(commandBuffer));

    return vkModel;
}


SpriteHandle VulkanContext::AddModel(const Model& model) {
    VkModel vkModel;
    vk::raii::CommandBuffer commandBuffer = beginSingleTimeCommand();
    std::vector<vk::raii::Buffer> stagingBuffers;
    std::vector<vk::raii::DeviceMemory> stagingMemories;

    auto [vertexBuffer, vertexBufferMemory] = createUniversalBuffer<Vertex3d>(
        commandBuffer, stagingBuffers, stagingMemories, model.vertices_,
        vk::BufferUsageFlagBits::eVertexBuffer);
    vkModel.vertexBuffer = std::move(vertexBuffer);
    vkModel.vertexBufferMemory = std::move(vertexBufferMemory);

    auto [indexBuffer, indexBufferMemory] = createUniversalBuffer<uint32_t>(
        commandBuffer, stagingBuffers, stagingMemories, model.indices,
        vk::BufferUsageFlagBits::eIndexBuffer);
    vkModel.indexBuffer = std::move(indexBuffer);
    vkModel.indexBufferMemory = std::move(indexBufferMemory);
    vkModel.indicesCount = model.indices.size();

    vkModel.meshes = model.meshes_;
    vkModel.nodes = model.nodes_;
    vkModel.rootNodes = model.rootNodes_;
    vkModel.textureOffset = textureCounter;
    vkModel.textures = createTexturesFromImages(commandBuffer, stagingBuffers,
                                                stagingMemories, model);

    endSingleTimeCommand(std::move(commandBuffer));

    models_.emplace_back(std::move(vkModel));

    return {static_cast<uint32_t>(models_.size() - 1)};
}


uint32_t VulkanContext::loadAsset(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        LOG_ERROR("file not found: {}", path);
        throw std::runtime_error("file not found: " + path);
    }

    std::ifstream file(path);
    if (!file) {
        LOG_ERROR("Failed to open file: {}", path);
        throw std::runtime_error("Failed to open file: " + path);
    }

    models_.push_back(loadModel(path));
    return models_.size() - 1;
}

//
// meshes.push_back(loadMesh("../assets/CarConcept.glb"));
// objects.push_back({meshes.size() - 1});
// objects[2].transform.pos = glm::vec3(3.0, 0.0, 0.0);
// objects[2].transform.rotate({1.0, 0.0, 0.0}, -90.0);
// objects[2].color = glm::vec3(1.0, 1.0, 0.0


/*
void VulkanContext::renderFrame(Scene& scene, EngineStats& stats) {
    auto fenceResult =
        device_.waitForFences(*drawFence[frameIndex], vk::True, UINT64_MAX);
    if (fenceResult != vk::Result::eSuccess) {
        LOG_ERROR("failed to wait for fence!");
        throw std::runtime_error("failed to wait for fence!");
    }

    auto [result, imageIndex] = m_swapchain.acquireNextImage(
        UINT64_MAX, *presentCompleteSemaphore[frameIndex], nullptr);

    if (result == vk::Result::eErrorOutOfDateKHR) {
        recreateContext();
        return;
    }

    if (result != vk::Result::eSuccess &&
        result != vk::Result::eSuboptimalKHR) {
        assert(result == vk::Result::eTimeout ||
               result == vk::Result::eNotReady);
        LOG_ERROR("failed to acquire swap chain image!");
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    updateUniformBuffer(frameIndex);

    device_.resetFences(*drawFence[frameIndex]);
    m_commandBuffer[frameIndex].reset();

    recordCommandBuffer(imageIndex, scene, stats);

    vk::PipelineStageFlags waitDestinationStageMask(
        vk::PipelineStageFlagBits::eColorAttachmentOutput);

    const vk::SubmitInfo submitInfo{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*presentCompleteSemaphore[frameIndex],
        .pWaitDstStageMask = &waitDestinationStageMask,
        .commandBufferCount = 1,
        .pCommandBuffers = &*m_commandBuffer[frameIndex],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &*renderFinishedSemaphore[imageIndex]};

    m_graphicsQueue.submit(submitInfo, *drawFence[frameIndex]);

    vk::PresentInfoKHR presentInfoKHR{
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &*renderFinishedSemaphore[imageIndex],
        .swapchainCount = 1,
        .pSwapchains = &*m_swapchain,
        .pImageIndices = &imageIndex};

    result = m_graphicsQueue.presentKHR(presentInfoKHR);
    if ((result == vk::Result::eSuboptimalKHR) ||
        (result == vk::Result::eErrorOutOfDateKHR)) {
        recreateContext();
    } else {
        assert(result == vk::Result::eSuccess);
    }
    frameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
}*/


