//
// Created by divmone on 10/16/2026.
//

#include "App.hpp"

App::App() : isRunning_(true), logger_("APP", true), window_("APP", 500, 500) {
    window_.SetOnEvent([&](SDL_Event& e) {

    });
}

void App::Run() {
    while (isRunning_) {
        isRunning_ = window_.ProcessEvent();
    }
}