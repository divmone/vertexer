cmake_minimum_required(VERSION 4.2)
project(vertexer)

set(CMAKE_CXX_STANDARD 23)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif ()

function(add_engine_module name)
    add_library(${name} ${ARGN})
    target_include_directories(${name}
            PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    )
endfunction()

add_subdirectory(app)
add_subdirectory(src)
add_subdirectory(tests)