find_package(stb REQUIRED)
find_package(fastgltf REQUIRED)
find_package(nlohmann_json REQUIRED)

add_library(resourceSystem STATIC
        types/ImageData.hpp
        AssetLoader.hpp
        AssetLoader.cpp
        SceneLoader.hpp
        SceneLoader.cpp
)

target_link_libraries(resourceSystem logSystem)

target_include_directories(resourceSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})