add_library(typesSystem
        INTERFACE
        MeshData.hpp
        EngineStats.hpp
        Vertex.hpp
        Vertex.hpp
)

target_include_directories(typesSystem INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})