//
// Created by divmone on 10/16/2026.
//

#pragma once

#include <SDL3/SDL.h>

#include <functional>

class Window {
public:
    Window(const char* title, int width, int height);
    ~Window();

    void SetOnEvent(std::function<void(SDL_Event&)> handler);
    bool ProcessEvent() const;

    SDL_Window* GetHandle() const;

    int GetWidth() const;
    int GetHeight() const;
private:
    SDL_Window*                     window_;
    std::function<void(SDL_Event&)> onEvent_;
    const char*                     title_;
    int                             width_;
    int                             height_;
};
