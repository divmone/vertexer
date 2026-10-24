find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
#find_package(fastgltf REQUIRED)
#find_package(stb REQUIRED)
#find_package(imgui REQUIRED)
#find_package(nlohmann_json REQUIRED)
#find_package(spdlog REQUIRED)

add_engine_module(
        renderSystem
        include/renderSystem/VulkanContext.hpp src/VulkanContext.cpp
        include/renderSystem/PipelineBuilder.hpp src/PipelineBuilder.cpp
        include/renderSystem/PSO.hpp src/PSO.cpp
        include/renderSystem/types/Providers.hpp
        src/types/VulkanVertex.hpp
        include/renderSystem/types/DrawCall.hpp
        include/renderSystem/types/VulkanModel.hpp
        include/renderSystem/types/VulkanTexture.hpp
)

target_link_libraries(renderSystem logSystem sceneSystem resourceSystem platformSystem typesSystem Vulkan::Vulkan glm)
target_compile_definitions(renderSystem PUBLIC VULKAN_HPP_NO_STRUCT_CONSTRUCTORS)