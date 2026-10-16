find_package(SDL3)

add_library(src render.hpp render.cpp)

target_link_libraries(src SDL3::SDL3)

target_include_directories(src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


