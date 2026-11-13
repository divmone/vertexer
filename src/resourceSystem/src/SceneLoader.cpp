//
// Created by divmone on 9/21/2026.
//

#include <fstream>
#include <logSystem/Logger.hpp>
#include <nlohmann/json.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <resourceSystem/SceneLoader.hpp>

#include "resourceSystem/AssetLoader.hpp"

glm::vec3 convertJsonToVec3(nlohmann::json& json) {
    glm::vec3 vec;
    vec.x = json["x"].get<float>();
    vec.y = json["y"].get<float>();
    vec.z = json["y"].get<float>();

    return vec;
}

Scene SceneLoader::LoadSceneFromJSON(std::string_view path) {
    using nlohmann::json;

    const std::filesystem::path filePath{std::string(path)};

    if (!std::filesystem::exists(filePath)) {
        throw std::runtime_error("SCENE file not found: " + filePath.string());
    }

    std::ifstream file;
    file.open(filePath);

    if (!file) {
        throw std::runtime_error("failed to load file: " + filePath.string());
    }

    Scene scene;
    try {
        json jsonScene = json::parse(file);

        auto sceneObj = jsonScene["scene"];
        for (auto& jsonObj : sceneObj["objects"]) {
            const auto& name = jsonObj["name"].get<std::string>();
            auto& obj = scene.AddObject(0 /*TODO*/, name);

            if (jsonObj.contains("position")) {
                obj.transform.pos = convertJsonToVec3(jsonObj["position"]);
            }

            if (jsonObj.contains("rotation")) {
                obj.transform.rotate({1, 0, 0},
                                     convertJsonToVec3(jsonObj["rotation"]).x);
                obj.transform.rotate({0, 1, 0},
                                     convertJsonToVec3(jsonObj["rotation"]).y);
                obj.transform.rotate({0, 0, 1},
                                     convertJsonToVec3(jsonObj["rotation"]).z);
            }

            if (jsonObj.contains("scale")) {
                obj.transform.scale = convertJsonToVec3(jsonObj["scale"]);
            }
        }
    } catch (const json::parse_error& e) {
        LOG_ERROR(e.what());
    } catch (const json::type_error& e) {
        LOG_ERROR(e.what());
    } catch (const std::runtime_error& e) {
        LOG_ERROR(e.what());
    }

    return scene;
}

SceneMeshData SceneLoader::LoadSceneMeshData(const std::string& path) {
    using nlohmann::json;

    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("SCENE file not found: " + path);
    }

    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("failed to load file: " + path);
    }

    SceneMeshData meshData;

    try {
        size_t modelsCount = 0;
        json jsonScene = json::parse(file);

        auto sceneObj = jsonScene["scene"];

        for (auto& jsonObj : sceneObj["objects"]) {
            if (jsonObj.contains("model")) {
                modelsCount++;
            }
        }

        meshData.models_.reserve(modelsCount);

        for (auto& jsonObj : sceneObj["objects"]) {
            if (jsonObj.contains("model")) {
                auto model = AssetLoader::loadModelFromGltf(
                    jsonObj["model"].get<std::string>());
                meshData.models_.emplace_back(std::move(model));
            }
        }
    } catch (const json::parse_error& e) {
        LOG_ERROR(e.what());
    } catch (const json::type_error& e) {
        LOG_ERROR(e.what());
    } catch (const std::runtime_error& e) {
        LOG_ERROR(e.what());
    }

    return meshData;
}
