//
// Created by divmone on 10/16/2026.
//

#pragma once

#include <inputSystem/InputSystem.hpp>
#include <logSystem/Logger.hpp>
#include <renderSystem/2d/Vulkan2dContext.hpp>
#include <renderSystem/3d/Vulkan3dContext.hpp>
#include <renderSystem/VulkanContext.hpp>
#include <uiSystem/UICore.hpp>
#include <windowSystem/SDLWindow.hpp>

class App {
public:
    App();
    ~App() = default;

    void Run();
private:
    bool            isRunning_;
    Logger          logger_;
    SDLWindow       window_;
    VulkanContext   vulkanContext_;
    Vulkan2dContext vulkan2dContext_;
    Vulkan3dContext vulkan3dContext_;
    InputSystem     inputSystem_;
    UI              ui_;

    void Init();
    void Update(float deltaTime) const;
};
