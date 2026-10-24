//
// Created by divmone on 7/23/2026.
//

#include <resourceSystem/AssetLoader.hpp>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <spdlog/spdlog.h>
#include <stb_image.h>

#include <future>
#include <iostream>
#include <logSystem/Logger.hpp>
#include <typesSystem/MeshData.hpp>
#include <typesSystem/Vertex.hpp>

namespace {
glm::mat4 getNodeLocalMatrix(const fastgltf::Node& node) {
    if (const auto* matrix =
            std::get_if<fastgltf::math::fmat4x4>(&node.transform)) {
        return glm::make_mat4(matrix->data());
    }

    const auto& trs = std::get<fastgltf::TRS>(node.transform);
    glm::mat4 t = glm::translate(
        glm::mat4(1.0f), glm::vec3(trs.translation.x(), trs.translation.y(),
                                   trs.translation.z()));
    glm::mat4 r = glm::mat4_cast(glm::quat(trs.rotation.w(), trs.rotation.x(),
                                           trs.rotation.y(), trs.rotation.z()));
    glm::mat4 s =
        glm::scale(glm::mat4(1.0f),
                   glm::vec3(trs.scale.x(), trs.scale.y(), trs.scale.z()));

    return t * r * s;
}

[[maybe_unused]] glm::vec3 computeBoundsCenter(
    const std::vector<Vertex>& vertices) {
    if (vertices.empty()) {
        return glm::vec3(0.0f);
    }

    glm::vec3 minBounds(std::numeric_limits<float>::max());
    glm::vec3 maxBounds(std::numeric_limits<float>::lowest());

    for (const auto& v : vertices) {
        minBounds = glm::min(minBounds, v.pos);
        maxBounds = glm::max(maxBounds, v.pos);
    }

    return (minBounds + maxBounds) * 0.5f;
}

void processMeshes(const fastgltf::Asset& gltf, Model& model) {
    for (const auto& gltfMesh : gltf.meshes) {
        Mesh mesh;
        for (const auto& gltfPrimitive : gltfMesh.primitives) {
            Primitive primitive;

            primitive.materialIndex = gltfPrimitive.materialIndex
                                          ? gltfPrimitive.materialIndex.value()
                                          : -1;

            if (primitive.materialIndex >= 0) {
                const auto& baseColorTexture =
                    gltf.materials[primitive.materialIndex]
                        .pbrData.baseColorTexture;

                primitive.isBlend =
                    gltf.materials[primitive.materialIndex].alphaMode ==
                    fastgltf::AlphaMode::Blend;
                const auto& baseColorFactor =
                    gltf.materials[primitive.materialIndex]
                        .pbrData.baseColorFactor;
                primitive.color =
                    glm::vec4(baseColorFactor.x(), baseColorFactor.y(),
                              baseColorFactor.z(), baseColorFactor.w());

                if (baseColorTexture) {
                    const auto& texture =
                        gltf.textures[baseColorTexture->textureIndex];
                    if (texture.imageIndex) {
                        primitive.textureIndex = texture.imageIndex.value();
                        primitive.hasTexture = true;
                    }
                }
            }
            primitive.firstIndex = model.indices.size();

            auto posIt = gltfPrimitive.findAttribute("POSITION");
            if (posIt == gltfPrimitive.attributes.end()) {
                continue;
            }

            const fastgltf::Accessor& accessor =
                gltf.accessors[posIt->accessorIndex];
            size_t initSizeVert = model.vertices.size();
            model.vertices.resize(initSizeVert + accessor.count);

            fastgltf::iterateAccessorWithIndex<glm::vec3>(
                gltf, accessor, [&](const glm::vec3& position, size_t index) {
                    model.vertices[index + initSizeVert].pos = position;
                });

            auto normIt = gltfPrimitive.findAttribute("NORMAL");
            if (normIt != gltfPrimitive.attributes.end()) {
                const fastgltf::Accessor& normAccessor =
                    gltf.accessors[normIt->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                    gltf, normAccessor,
                    [&](const glm::vec3& normal, size_t index) {
                        model.vertices[index + initSizeVert].normal = normal;
                    });
            } else {
                for (size_t i = 0; i < accessor.count; ++i) {
                    model.vertices[i + initSizeVert].normal =
                        glm::vec3(0.0f, 0.0f, 1.0f);
                }
            }

            auto texCoordIt = gltfPrimitive.findAttribute("TEXCOORD_0");
            if (texCoordIt != gltfPrimitive.attributes.end()) {
                const fastgltf::Accessor& texCoordAccessor =
                    gltf.accessors[texCoordIt->accessorIndex];
                fastgltf::iterateAccessorWithIndex<glm::vec2>(
                    gltf, texCoordAccessor,
                    [&](const glm::vec2& texCoord, size_t index) {
                        model.vertices[index + initSizeVert].texCoord =
                            texCoord;
                    });
            } else {
                for (size_t i = 0; i < accessor.count; ++i) {
                    model.vertices[i + initSizeVert].texCoord =
                        glm::vec2(0.0f, 0.0f);
                }
            }

            if (!gltfPrimitive.indicesAccessor.has_value()) {
                continue;
            }

            const fastgltf::Accessor& indicesAccessor =
                gltf.accessors[gltfPrimitive.indicesAccessor.value()];
            auto initSizeInd = model.indices.size();
            model.indices.resize(initSizeInd + indicesAccessor.count);

            fastgltf::iterateAccessorWithIndex<uint32_t>(
                gltf, indicesAccessor, [&](uint32_t value, size_t index) {
                    model.indices[index + initSizeInd] =
                        value + static_cast<uint32_t>(initSizeVert);
                });

            primitive.indexCount = indicesAccessor.count;
            if (primitive.isBlend) {
                mesh.blendPrimitives.push_back(primitive);
            } else {
                mesh.primitives.push_back(primitive);
            }
        }

        model.meshes.push_back(std::move(mesh));
    }
}

void processNodes(const fastgltf::Asset& gltf, Model& model) {
    for (auto& gltfNode : gltf.nodes) {
        Node node;
        node.meshIndex =
            gltfNode.meshIndex.has_value() ? gltfNode.meshIndex.value() : -1;
        node.transformation = getNodeLocalMatrix(gltfNode);
        node.children = std::vector<uint32_t>(gltfNode.children.begin(),
                                              gltfNode.children.end());
        model.nodes.push_back(std::move(node));
    }
}

ImageData decodeGltfImage(const fastgltf::Asset& gltf,
                          const fastgltf::Image& img,
                          const std::string& parentPath) {
    return std::visit(
        fastgltf::visitor{
            [](const auto& arg) -> ImageData {
                throw std::runtime_error(
                    std::string("unsupported glTF image data source: ") +
                    typeid(arg).name());
            },
            [&parentPath](const fastgltf::sources::URI& uri) -> ImageData {
                std::string relative(uri.uri.path().begin(),
                                     uri.uri.path().end());
                std::string path = parentPath + "/" + relative;
                return AssetLoader::loadImageDataFromFile(path);
            },
            [](const fastgltf::sources::Array& arr) -> ImageData {
                int x, y, n;
                unsigned char* data = stbi_load_from_memory(
                    reinterpret_cast<const unsigned char*>(arr.bytes.data()),
                    static_cast<int>(arr.bytes.size()), &x, &y, &n,
                    STBI_rgb_alpha);
                if (!data) {
                    throw std::runtime_error("failed to decode embedded image");
                }
                ImageData img;
                img.data = data;
                img.width = x;
                img.height = y;
                img.channels = 4;
                return img;
            },
            [&gltf](const fastgltf::sources::BufferView& view) -> ImageData {
                const auto& bufferView = gltf.bufferViews[view.bufferViewIndex];
                const auto& buffer = gltf.buffers[bufferView.bufferIndex];
                const auto& bufferBytes =
                    std::get<fastgltf::sources::Array>(buffer.data);

                const unsigned char* ptr =
                    reinterpret_cast<const unsigned char*>(
                        bufferBytes.bytes.data() + bufferView.byteOffset);

                int x, y, n;
                unsigned char* data = stbi_load_from_memory(
                    ptr, static_cast<int>(bufferView.byteLength), &x, &y, &n,
                    STBI_rgb_alpha);
                if (!data) {
                    throw std::runtime_error("failed to decode embedded image");
                }
                ImageData img;
                img.data = data;
                img.width = x;
                img.height = y;
                img.channels = 4;
                return img;
            }},
        img.data);
}

void processImages(const fastgltf::Asset& gltf,
                   Model& model,
                   const std::string& parentPath) {
    std::vector<std::future<ImageData>> futures;
    futures.reserve(gltf.images.size());

    for (const auto& img : gltf.images) {
        futures.push_back(std::async(
            std::launch::async, [&gltf, &img, &parentPath]() -> ImageData {
                return decodeGltfImage(gltf, img, parentPath);
            }));
    }

    std::vector<ImageData> images;
    images.reserve(futures.size());
    for (auto& future : futures) {
        images.push_back(future.get());
    }

    model.images = std::move(images);
}
}  // namespace

Model AssetLoader::loadModelFromGltf(const std::string& pathToFile) {
    Model model;
    fastgltf::Parser parser;
    auto gltfFile = fastgltf::GltfDataBuffer::FromPath(pathToFile);

    if (gltfFile.error() != fastgltf::Error::None) {
        throw std::runtime_error(getErrorMessage(gltfFile.error()).data());
    }
    std::filesystem::path path(pathToFile);
    std::string parentPath = path.parent_path().string();

    auto asset = parser.loadGltf(gltfFile.get(), path.parent_path(),
                                 fastgltf::Options::LoadExternalBuffers);

    if (asset.error() != fastgltf::Error::None) {
        throw std::runtime_error(getErrorMessage(asset.error()).data());
    }

    auto& gltfAsset = asset.get();

    if (gltfAsset.scenes.empty()) {
        throw std::runtime_error("glTF has no scenes");
    }

    size_t sceneIndex = gltfAsset.defaultScene.value_or(0);
    const auto& scene = gltfAsset.scenes[sceneIndex];

    processMeshes(gltfAsset, model);
    processNodes(gltfAsset, model);
    processImages(gltfAsset, model, parentPath);

    for (auto rootNodeIndex : scene.nodeIndices) {
        model.rootNodes.push_back(rootNodeIndex);
    }

    LOG_INFO("Loaded model from file: {}", pathToFile);
    return model;
}

ImageData AssetLoader::loadImageDataFromFile(const std::string& pathToFile) {
    int x, y, n;
    unsigned char* data =
        stbi_load(pathToFile.c_str(), &x, &y, &n, STBI_rgb_alpha);
    if (!data) {
        LOG_ERROR("Failed to load texture from: {}", pathToFile.c_str());
        LOG_ERROR("Reason: {}", stbi_failure_reason());
        static unsigned char white[4] = {255, 255, 255, 255};
        data = white;
        x = y = 1;
    }

    ImageData imageData;
    imageData.data = data;
    imageData.width = x;
    imageData.height = y;
    imageData.channels = 4;
    return imageData;
}

// std::vector<VkMaterial> AssetLoader::LoadMaterialsFromGltf(
//     const std::string& pathToFile) {
//     std::vector<VkMaterial> materials;
//
//     fastgltf::Parser parser;
//     auto gltfFile = fastgltf::GltfDataBuffer::FromPath(pathToFile);
//
//     if (gltfFile.error() != fastgltf::Error::None) {
//         throw std::runtime_error(getErrorMessage(gltfFile.error()).data());
//     }
//     std::filesystem::path path(pathToFile);
//
//     auto asset = parser.loadGltf(gltfFile.get(), path.parent_path(),
//                                  fastgltf::Options::LoadExternalBuffers);
//
//     if (asset.error() != fastgltf::Error::None) {
//         throw std::runtime_error(getErrorMessage(asset.error()).data());
//     }
//
//     auto& gltfAsset = asset.get();
// }
