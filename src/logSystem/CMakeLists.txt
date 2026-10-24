find_package(spdlog REQUIRED)

add_engine_module(logSystem include/logSystem/Logger.hpp src/Logger.cpp)

target_link_libraries(logSystem PUBLIC spdlog::spdlog)