find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
#find_package(fastgltf REQUIRED)
#find_package(stb REQUIRED)
#find_package(imgui REQUIRED)
#find_package(nlohmann_json REQUIRED)
#find_package(spdlog REQUIRED)

add_library(
        renderSystem STATIC
        VulkanContext.hpp VulkanContext.cpp
        PipelineBuilder.hpp PipelineBuilder.cpp
        PSO.hpp PSO.cpp
        types/EngineStats.hpp
        types/MeshData.hpp
        types/Providers.hpp
)

target_link_libraries(renderSystem logSystem sceneSystem Vulkan::Vulkan glm)
target_include_directories(renderSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(renderSystem PUBLIC VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)