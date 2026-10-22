//
// Created by divmone on 9/21/2026.
//

#include "SceneLoader.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

#include "Logger.hpp"
#include "VulkanContext.hpp"

glm::vec3 convertJsonToVec3(nlohmann::json& json) {
    glm::vec3 vec;
    vec.x = json["x"].get<float>();
    vec.y = json["y"].get<float>();
    vec.z = json["y"].get<float>();

    return vec;
}

Scene SceneLoader::loadSceneFromJSON(VulkanContext& vkContext,
                                     const std::string& path) {
    using nlohmann::json;

    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("SCENE file not found: " + path);
    }

    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("failed to load file: " + path);
    }

    Scene scene;
    try {
        json jsonScene = json::parse(file);

        auto sceneObj = jsonScene["scene"];
        for (auto& jsonObj : sceneObj["objects"]) {
            auto idx = vkContext.loadAsset(jsonObj["model"].get<std::string>());
            const auto& name = jsonObj["name"].get<std::string>();
            auto& obj = scene.AddObject(idx, name);

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

    vkContext.finishLoading();

    file.close();

    return scene;
}
