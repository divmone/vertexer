configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/src/fileConfig.hpp.in"
        "${CMAKE_CURRENT_BINARY_DIR}/fileConfig.hpp"
)

add_engine_module(fileSystem
        include/FileSystem.hpp
        src/FileSystem.cpp
)

target_include_directories(fileSystem PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")