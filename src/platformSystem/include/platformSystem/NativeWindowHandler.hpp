//
// Created by divmone on 10/21/2026.
//

#pragma once

#include <variant>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <SDL3/SDL.h>

#if defined(__linux)

struct X11WindowHandler {
    void*   display = nullptr;
    uint32_t window  = 0; // xcb_window_t / Window (XID)
};

struct WaylandWindowHandler {
    void* display = nullptr;
    void* surface = nullptr;
};
#endif

#if defined(_WIN64)
struct Win32WindowHandler {
    void* hWnd      = nullptr;   // HWND
    void* hInstance = nullptr;   // HINSTANCE
};
#endif

#if defined(__APPLE__)
struct MacOSWindowHandler {
    void* nsWindows      = nullptr;   // NSWindow*
    void* nsApplication= nullptr;   // NSApplication* (заглушка)
};
#endif


using NativeWindowHandler = std::variant<
#if defined(_WIN64)
    Win32WindowHandler,
#endif
#if defined(__linux)
    X11WindowHandler,
    WaylandWindowHandler,
#endif
#if defined(__APPLE__)
    MacOSWindowHandler,
#endif
    std::monostate
>;


inline NativeWindowHandler GetNativeWindowHandler(SDL_Window* window) {
    const SDL_PropertiesID props = SDL_GetWindowProperties(window);
#if defined(_WIN64)
    Win32WindowHandler win32;
    win32.hWnd = SDL_GetPointerProperty(
        props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    win32.hInstance = SDL_GetPointerProperty(
        props, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr);
    return win32;
#endif

#if defined(__linux)
    const char* driver = SDL_GetCurrentVideoDriver();

    if (std::strcmp(driver, "wayland") == 0) {
        WaylandWindowHandler wl;
        wl.display = SDL_GetPointerProperty(
            props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
        wl.surface = SDL_GetPointerProperty(
            props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
        return wl;
    }

    if (std::strcmp(driver, "x11") == 0) {
        X11WindowHandler x11;
        x11.display = SDL_GetPointerProperty(
            props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
        // xcb_window_t / Window (XID) (число)
        x11.window = static_cast<uint32_t>(SDL_GetNumberProperty(
            props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
        return x11;
    }
#endif

#if defined(__APPLE__)
    // ченибудь придумаем
#endif

    return std::monostate{};
}


struct INativeHandler {
    virtual ~INativeHandler() = default;
    virtual NativeWindowHandler GetNativeHandler() const = 0;
};