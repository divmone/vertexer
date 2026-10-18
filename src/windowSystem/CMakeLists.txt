add_library(windowSystem Window.hpp Window.cpp)

target_link_libraries(windowSystem logSystem SDL3::SDL3)
target_include_directories(windowSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
