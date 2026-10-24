#pragma once
#include <variant>
#include <vector>

#include "NativeWindowHandler.hpp"

inline std::vector<const char*> GetNativeSDLExtensions(const NativeWindowHandler& h) {
    return std::visit(
        [](const auto& handler) -> std::vector<const char*> {
            using T = std::decay_t<decltype(handler)>;

            if constexpr (std::is_same_v<T, std::monostate>) {
                throw std::runtime_error(
                    "GetNativeSDLExtensions: empty NativeWindowHandler");
            }
#if defined(_WIN64)
            else if constexpr (std::is_same_v<T, Win32WindowHandler>) {
                return {"VK_KHR_surface", "VK_KHR_win32_surface"};
            }
#endif
#if defined(__linux__)
            else if constexpr (std::is_same_v<T, WaylandWindowHandler>) {
                return {"VK_KHR_surface", "VK_KHR_wayland_surface"};
            } else if constexpr (std::is_same_v<T, X11WindowHandler>) {
                return {"VK_KHR_surface", "VK_KHR_xlib_surface"};
            }
#endif
#if defined(__APPLE__)
            else if constexpr (std::is_same_v<T, MacOSWindowHandler>) {
                // мб оно а мб и нет
                return {"VK_KHR_surface", "VK_EXT_metal_surface"};
            }
#endif
            else {
                static_assert(sizeof(T) == 0,
                              "Unhandled NativeWindowHandler alternative");
            }
        },
        h);
}