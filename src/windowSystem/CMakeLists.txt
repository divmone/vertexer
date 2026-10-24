add_engine_module(windowSystem include/windowSystem/SDLWindow.hpp src/SDLWindow.cpp)

target_link_libraries(windowSystem logSystem platformSystem SDL3::SDL3)
