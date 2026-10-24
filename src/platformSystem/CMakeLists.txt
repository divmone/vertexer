
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT QUIET wayland-client)
find_package(X11 QUIET)


add_engine_module(platformSystem
        include/platformSystem/NativeWindowHandler.hpp
        include/platformSystem/IExtensions.hpp
        include/platformSystem/IExtentProvider.hpp
        include/platformSystem/PlatformDefs.hpp
        include/platformSystem/NativeVkSurfaceKHR.hpp
        include/platformSystem/NativeSDLExtensions.hpp
)


target_include_directories(platformSystem INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(platformSystem INTERFACE SDL3::SDL3)

if(WAYLAND_CLIENT_FOUND)
    target_include_directories(platformSystem INTERFACE ${WAYLAND_CLIENT_INCLUDE_DIRS})
    target_link_libraries(platformSystem INTERFACE ${WAYLAND_CLIENT_LINK_LIBRARIES})
endif()

if (X11_FOUND)

endif()