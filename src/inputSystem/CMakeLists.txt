find_package(SDL3)

add_library(inputSystem STATIC
        InputSystem.hpp
        InputSystem.cpp
)

target_link_libraries(inputSystem SDL3::SDL3)

target_include_directories(inputSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})