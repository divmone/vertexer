add_library(logSystem Logger.hpp Logger.cpp)

find_package(spdlog REQUIRED)

target_link_libraries(logSystem PUBLIC spdlog::spdlog)
target_include_directories(logSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
