set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/build/Debug/generators/conan_toolchain.cmake" CACHE FILEPATH "Conan toolchain")


find_package(SDL3 REQUIRED)

add_library(src render.hpp render.cpp)

target_include_directories(src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})


