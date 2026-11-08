add_executable(app main.cpp)
target_link_options(app PRIVATE -static -static-libgcc -static-libstdc++)
target_link_libraries(app src)