find_package(glm REQUIRED)
find_package(fastgltf REQUIRED)
find_package(stb REQUIRED)
find_package(nlohmann_json REQUIRED)


add_engine_module(
        sceneSystem
        include/sceneSystem/Scene.hpp
        src/Scene.cpp
        include/sceneSystem/types/Object.hpp
        include/sceneSystem/Camera.hpp
        src/Camera.cpp
)

target_link_libraries(sceneSystem logSystem glm fastgltf::fastgltf nlohmann_json::nlohmann_json stb::stb)
