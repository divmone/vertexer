#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>

#include "NativeWindowHandler.hpp"

#if defined(_WIN64)
#  if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable: 4005)
#  endif
#  include <windows.h>
#  if defined(_MSC_VER)
#    pragma warning(pop)
#  endif
#  include <vulkan/vulkan_win32.h>
#endif

#if defined(__linux__)
#include <vulkan/vulkan_wayland.h>

#include <X11/Xlib.h>

// это как вообще понимать???????
#ifdef True
#    undef True
#endif
#  ifdef False
#    undef False
#endif

#include <vulkan/vulkan_xlib.h>
#include <wayland-client.h>
#endif

#if defined(__APPLE__)
#  include <vulkan/vulkan_metal.h>
#endif

namespace detail {

#if defined(_WIN64)
inline VkSurfaceKHR CreateWin32Surface(const vk::raii::Instance& instance,
                                       const Win32WindowHandler& h) {
    VkWin32SurfaceCreateInfoKHR ci{};
    ci.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    ci.hinstance = static_cast<HINSTANCE>(h.hInstance);
    ci.hwnd      = static_cast<HWND>(h.hWnd);

    VkSurfaceKHR s = VK_NULL_HANDLE;
    if (vkCreateWin32SurfaceKHR(*instance, &ci, nullptr, &s) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateWin32SurfaceKHR failed");
    }
    return s;
}
#endif

#if defined(__linux__)
inline VkSurfaceKHR CreateWaylandSurface(const vk::raii::Instance& instance,
                                         const WaylandWindowHandler& h) {
    VkWaylandSurfaceCreateInfoKHR ci{};
    ci.sType   = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    ci.display = static_cast<wl_display*>(h.display);
    ci.surface = static_cast<wl_surface*>(h.surface);

    VkSurfaceKHR s = VK_NULL_HANDLE;
    if (vkCreateWaylandSurfaceKHR(*instance, &ci, nullptr, &s) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateWaylandSurfaceKHR failed");
    }
    return s;
}

inline VkSurfaceKHR CreateX11Surface(const vk::raii::Instance& instance,
                                     const X11WindowHandler& h) {
    VkXlibSurfaceCreateInfoKHR ci{};
    ci.sType  = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    ci.dpy    = static_cast<Display*>(h.display);
    ci.window = static_cast<Window>(h.window);

    VkSurfaceKHR s = VK_NULL_HANDLE;
    if (vkCreateXlibSurfaceKHR(*instance, &ci, nullptr, &s) != VK_SUCCESS) {
        throw std::runtime_error("vkCreateXlibSurfaceKHR failed");
    }
    return s;
}
#endif

#if defined(__APPLE__)
inline VkSurfaceKHR CreateMacOSSurface(const vk::raii::Instance&,
                                       const MacOSWindowHandler&) {
    throw std::runtime_error("macOS surface creation is not implemented");
}
#endif

} // namespace detail


inline VkSurfaceKHR GetNativeVkSurfaceKHR(const vk::raii::Instance& instance,
                                          const NativeWindowHandler& h) {
    return std::visit(
        [&](const auto& handler) -> VkSurfaceKHR {
            using T = std::decay_t<decltype(handler)>;

            if constexpr (std::is_same_v<T, std::monostate>) {
                throw std::runtime_error(
                    "GetNativeVkSurfaceKHR: empty NativeWindowHandler");
            }
#if defined(_WIN64)
            else if constexpr (std::is_same_v<T, Win32WindowHandler>) {
                return detail::CreateWin32Surface(instance, handler);
            }
#endif
#if defined(__linux__)
            else if constexpr (std::is_same_v<T, WaylandWindowHandler>) {
                return detail::CreateWaylandSurface(instance, handler);
            } else if constexpr (std::is_same_v<T, X11WindowHandler>) {
                return detail::CreateX11Surface(instance, handler);
            }
#endif
#if defined(__APPLE__)
            else if constexpr (std::is_same_v<T, MacOSWindowHandler>) {
                return detail::CreateMacOSSurface(instance, handler);
            }
#endif
            else {
                static_assert(sizeof(T) == 0,
                              "Unhandled NativeWindowHandler alternative");
            }
        },
        h);
}