//
// Created by divmone on 10/16/2026.
//

#pragma once

#include "Camera.hpp"
#include "Scene.hpp"
#include "VulkanContext.hpp"
#include "inputSystem/InputSystem.hpp"
#include "logSystem/Logger.hpp"
#include "windowSystem/SDLWindow.hpp"

class App {
public:
    App();
    ~App() = default;

    void Run();
private:
    bool isRunning_;
    Logger logger_;
    SDLWindow window_;
    VulkanContext vulkanContext_;
    Camera camera_{glm::vec3(5, 5, 0), {0, 1, 0}, 180, -45};
    Scene scene_;
    InputSystem inputSystem_;

    bool pressedKeys_[256]{false};
    float lastX{0.0f};
    float lastY{0.0f};
    // std::unordered_set<SDL_Keycode> pressedKeys_;
    void init();
    void initScene();
    void update(float deltaTime);
};