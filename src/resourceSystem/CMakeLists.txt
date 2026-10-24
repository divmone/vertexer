find_package(stb REQUIRED)
find_package(glm REQUIRED)
find_package(fastgltf REQUIRED)
find_package(nlohmann_json REQUIRED)

add_engine_module(resourceSystem
        include/resourceSystem/types/ImageData.hpp
        include/resourceSystem/AssetLoader.hpp
        src/AssetLoader.cpp
        include/resourceSystem/SceneLoader.hpp
        src/SceneLoader.cpp
        include/resourceSystem/types/Model.hpp
)

target_link_libraries(resourceSystem logSystem typesSystem sceneSystem renderSystem fastgltf::fastgltf stb::stb glm nlohmann_json::nlohmann_json)
