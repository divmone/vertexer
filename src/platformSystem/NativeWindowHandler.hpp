//
// Created by divmone on 10/21/2026.
//

#pragma once
#include <variant>

struct Win32WindowHandler {
    void* hWnd;
    void* hInstance;
};

struct X11WindowHandler {
    void* hWnd;
    void* hInstance;
};

struct WaylandWindowHandler {
    void* hWnd;
    void* hInstance;
};

struct MacOSWindowHandler {
    void* hWnd;
    void* hInstance;
};

using NativeWindowHandler = std::variant<Win32WindowHandler,
                                         X11WindowHandler,
                                         WaylandWindowHandler,
                                         MacOSWindowHandler>;

struct INativeHandler {
    virtual ~INativeHandler() = default;
    virtual NativeWindowHandler GetNativeHandler() const = 0;
};
