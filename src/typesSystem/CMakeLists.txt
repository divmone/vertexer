find_package(glm REQUIRED)

add_engine_module(typesSystem
        include/typesSystem/MeshData.hpp
        include/typesSystem/EngineStats.hpp
        include/typesSystem/Vertex.hpp)

target_link_libraries(typesSystem INTERFACE glm)