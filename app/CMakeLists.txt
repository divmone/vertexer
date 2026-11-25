add_executable(app main.cpp)
if(${WIN32})
    target_link_options(app PRIVATE -static -static-libgcc -static-libstdc++)
endif()

target_link_libraries(app src)