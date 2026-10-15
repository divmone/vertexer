cmake_minimum_required(VERSION 4.2)
project(vertexer)

set(CMAKE_CXX_STANDARD 23)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif()

add_subdirectory(app)
add_subdirectory(src)
add_subdirectory(tests)