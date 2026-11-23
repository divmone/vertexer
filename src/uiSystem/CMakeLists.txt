find_package(RmlUi REQUIRED)
find_package(Vulkan REQUIRED)

add_engine_module(uiSystem 
    include/uiSystem/UICore.hpp
    include/uiSystem/AppBaseUiLayout.hpp
        include/uiSystem/RmlUi_Platform_SDL.h
        include/RmlUi_Include_Xlib.h

    src/AppBaseUiLayout.cpp
    src/Core.cpp
    src/RmlUi_Platform_SDL.cpp
)

target_link_libraries(uiSystem
    RmlUi::RmlUi
    Vulkan::Vulkan
    renderSystem
    resourceSystem
    windowSystem
    fileSystem
    logSystem
)

