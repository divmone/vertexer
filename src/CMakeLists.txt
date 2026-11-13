find_package(SDL3 REQUIRED)

add_compile_definitions(
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_RADIANS
        GLM_FORCE_DEPTH_ZERO_TO_ONE
)
add_library(src App.hpp App.cpp)

add_subdirectory(windowSystem)
add_subdirectory(logSystem)
add_subdirectory(renderSystem)
add_subdirectory(sceneSystem)
add_subdirectory(resourceSystem)
add_subdirectory(platformSystem)
add_subdirectory(inputSystem)
add_subdirectory(typesSystem)
add_subdirectory(uiSystem)
add_subdirectory(fileSystem)

target_link_libraries(src windowSystem logSystem renderSystem sceneSystem inputSystem resourceSystem typesSystem uiSystem)

target_include_directories(src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
