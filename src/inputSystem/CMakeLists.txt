find_package(SDL3)

add_engine_module(inputSystem
        include/inputSystem/InputSystem.hpp
        src/InputSystem.cpp
)

target_link_libraries(inputSystem SDL3::SDL3)