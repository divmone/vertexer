
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT QUIET wayland-client)
find_package(X11 QUIET)
find_package(Vulkan REQUIRED)

add_engine_module(platformSystem
        include/platformSystem/NativeWindowHandler.hpp
        include/platformSystem/IExtensions.hpp
        include/platformSystem/IExtentProvider.hpp
        include/platformSystem/Platform.hpp
        include/platformSystem/NativeVkSurfaceKHR.hpp
        include/platformSystem/NativeSDLExtensions.hpp
        src/NativeVkSurfaceKHR.cpp
        src/NativeWindowHandler.cpp
)

target_link_libraries(platformSystem PUBLIC SDL3::SDL3 Vulkan::Vulkan)

if (WAYLAND_CLIENT_FOUND)
    target_include_directories(platformSystem PRIVATE ${WAYLAND_CLIENT_INCLUDE_DIRS})
    target_link_libraries(platformSystem PRIVATE ${WAYLAND_CLIENT_LINK_LIBRARIES})
endif ()

if (X11_FOUND)
    target_include_directories(platformSystem PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(platformSystem PRIVATE ${X11_LIBRARIES})
endif ()