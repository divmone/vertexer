#pragma once

#if defined(_WIN64)
#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if defined(__linux__)
#define VK_USE_PLATFORM_WAYLAND_KHR
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#if defined(__APPLE__)
// ченибудь придумаем
#endif