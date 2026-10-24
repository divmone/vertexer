//
// Created by divmone on 10/16/2026.
//

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#endif

#include <logSystem/Logger.hpp>
#include <utility>

#include <windowSystem/SDLWindow.hpp>
#include <platformSystem/NativeWindowHandler.hpp>

SDLWindow::SDLWindow(const char* title, const int width, const int height)
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

SDLWindow::~SDLWindow() {
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
}

void SDLWindow::SetOnEvent(std::function<void(SDL_Event&)> handler) {
    onEvent_ = std::move(handler);
}

bool SDLWindow::ProcessEvent() const {
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

SDL_Window* SDLWindow::GetHandle() const {
    return window_;
}

int SDLWindow::GetWidth() const {
    return width_;
}

int SDLWindow::GetHeight() const {
    return height_;
}

NativeWindowHandler SDLWindow::GetNativeHandler() const {
    return GetNativeWindowHandler(window_);
}

std::vector<const char*> SDLWindow::GetExtensionNames() const {
    // uint32_t sdlExtensionCount = 0;
    // auto sdlExtensions =
    // SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount); auto
    // extensionProperties = m_context.enumerateInstanceExtensionProperties();
    // for (uint32_t i = 0; i < sdlExtensionCount; ++i) {
    //     if (std::ranges::none_of(
    //             extensionProperties, [sdlExtension = sdlExtensions[i]](
    //                                      const auto& extensionProperty) {
    //                 return strcmp(extensionProperty.extensionName,
    //                               sdlExtension) == 0;
    //             })) {
    //         throw std::runtime_error("Required SDL extension not supported: "
    //         +
    //                                  std::string(sdlExtensions[i]));
    //     }
    // }
}

ExtentProvider SDLWindow::GetExtentProvider() const {
    ExtentProvider provider;
    provider.GetExtentSizes = [&]() -> std::pair<std::uint32_t, std::uint32_t> {
        int width;
        int height;
        SDL_GetWindowSizeInPixels(window_, &width, &height);
        return {width, height};
    };

    return provider;
}
