find_package(RmlUi REQUIRED)
find_package(Vulkan REQUIRED)

add_engine_module(uiSystem 
    include/uiSystem/UICore.hpp 
    include/uiSystem/AppBaseUiLayout.hpp

    src/AppBaseUiLayout.cpp
    src/Core.cpp
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

