find_package(glm REQUIRED)
find_package(fastgltf REQUIRED)
find_package(stb REQUIRED)
#find_package(imgui REQUIRED)
find_package(nlohmann_json REQUIRED)
#find_package(spdlog REQUIRED)

add_library(
        sceneSystem STATIC
        Scene.hpp
        Scene.cpp
)

target_link_libraries(sceneSystem logSystem glm fastgltf::fastgltf nlohmann_json::nlohmann_json stb::stb)
target_include_directories(sceneSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})