#pragma once

#include <variant>
#include <cstdint>

#include <SDL3/SDL.h>

#if defined(__linux)
struct X11WindowHandler {
    void*    display = nullptr;
    uint32_t window  = 0;
};

struct WaylandWindowHandler {
    void* display = nullptr;
    void* surface = nullptr;
};
#endif

#if defined(_WIN64)
struct Win32WindowHandler {
    void* hWnd      = nullptr;
    void* hInstance = nullptr;
};
#endif

#if defined(__APPLE__)
struct MacOSWindowHandler {
    void* nsWindow      = nullptr;
    void* nsApplication = nullptr;
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

NativeWindowHandler GetNativeWindowHandler(SDL_Window* window);

struct INativeHandler {
    virtual ~INativeHandler() = default;
    virtual NativeWindowHandler GetNativeHandler() const = 0;
};