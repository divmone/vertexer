//
// Created by divmone on 10/16/2026.
//

#pragma once

#include <inputSystem/InputSystem.hpp>
#include <logSystem/Logger.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <uiSystem/UICore.hpp>
#include <windowSystem/SDLWindow.hpp>

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
    InputSystem inputSystem_;
    UI ui_;

    void Init();
    void Update(float deltaTime);
};
