//
// Created by divmone on 10/16/2026.
//

#pragma once

#include "logSystem/Logger.hpp"
#include "windowSystem/Window.hpp"

class App {
public:
    App();
    ~App() = default;

    void Run();
private:
    bool   isRunning_;
    Logger logger_;
    Window window_;
};