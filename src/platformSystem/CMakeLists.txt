add_library(platformSystem INTERFACE
        NativeWindowHandler.hpp
        IExtensions.hpp
        IExtentProvider.hpp
)

target_include_directories(platformSystem INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})