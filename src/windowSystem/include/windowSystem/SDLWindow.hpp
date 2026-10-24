//
// Created by divmone on 10/16/2026.
//

#pragma once

#include <SDL3/SDL.h>

#include <functional>
#include <platformSystem/IExtensions.hpp>
#include <platformSystem/IExtentProvider.hpp>
#include <platformSystem/NativeWindowHandler.hpp>

class SDLWindow : public INativeHandler,
                  public IExtensionNamesReader,
                  public IExtentProvider {
public:
    SDLWindow(const char* title,
              int width,
              int height,
              bool isMaximize = false);
    ~SDLWindow();

    void SetOnEvent(std::function<void(SDL_Event&)> handler);
    bool ProcessEvent() const;

    SDL_Window* GetHandle() const;

    int GetWidth() const;
    int GetHeight() const;

    NativeWindowHandler GetNativeHandler() const override;
    [[deprecated]] std::vector<const char*> GetExtensionNames() const override;
    ExtentProvider GetExtentProvider() override;
private:
    SDL_Window* window_;
    std::function<void(SDL_Event&)> onEvent_;
    const char* title_;
    int width_;
    int height_;
};
