find_package(SDL3 REQUIRED)

add_library(src App.hpp App.cpp)

add_subdirectory(windowSystem)
add_subdirectory(logSystem)
add_subdirectory(renderSystem)
add_subdirectory(sceneSystem)
add_subdirectory(resourceSystem)
add_subdirectory(platformSystem)
add_subdirectory(inputSystem)
add_subdirectory(typesSystem)

target_link_libraries(src windowSystem logSystem renderSystem sceneSystem)

target_include_directories(src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
