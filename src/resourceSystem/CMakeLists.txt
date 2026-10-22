find_package(stb REQUIRED)
find_package(glm REQUIRED)
find_package(fastgltf REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(resourceSystem STATIC
        types/ImageData.hpp
        AssetLoader.hpp
        AssetLoader.cpp
        SceneLoader.hpp
        SceneLoader.cpp
        types/Model.hpp
)

target_link_libraries(resourceSystem logSystem typesSystem sceneSystem renderSystem fastgltf::fastgltf stb::stb glm nlohmann_json::nlohmann_json)

target_include_directories(resourceSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})