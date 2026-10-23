cmake_minimum_required(VERSION 4.2)
project(vertexer)

set(CMAKE_CXX_STANDARD 23)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif ()

function(add_engine_module name)
    set(sources ${ARGN})
    list(FILTER sources INCLUDE REGEX "\\.(c|cc|cpp|cxx)$")

    if (sources)
        add_library(${name} STATIC ${ARGN})
        target_include_directories(${name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    else ()
        add_library(${name} INTERFACE)
        target_sources(${name} INTERFACE ${ARGN})   # optional, keeps headers visible in IDEs
        target_include_directories(${name} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    endif ()

    add_library(engine::${name} ALIAS ${name})  # if you use aliases
endfunction()

add_subdirectory(app)
add_subdirectory(src)
add_subdirectory(tests)