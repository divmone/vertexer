//
// Created by divmone on 10/16/2026.
//

#include "Window.hpp"

#include <Logger.hpp>
#include <utility>

Window::Window(const char* title, int width, int height)
    : title_(title), width_(width), height_(height) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LOG_ERROR("SDL_Init failed: %s", SDL_GetError());
        throw std::runtime_error("Failed to initialize SDL");
    }

    window_ = SDL_CreateWindow(title_, width_, height_,
                               SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

    if (!window_) {
        LOG_ERROR("Failed to create window: %s", SDL_GetError());
        throw std::runtime_error("Failed to create window");
    }
}

Window::~Window() {
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
}

void Window::SetOnEvent(std::function<void(SDL_Event&)> handler) {
    onEvent_ = std::move(handler);
}

bool Window::ProcessEvent() const {
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
            return false;
        }
        // SDL_Log(std::to_string(e.type).c_str());
        onEvent_(e);
    }

    return true;
}

SDL_Window* Window::GetHandle() const {
    return window_;
}

int Window::GetWidth() const {
    return width_;
}

int Window::GetHeight() const {
    return height_;
}