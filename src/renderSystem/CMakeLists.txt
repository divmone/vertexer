find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)
#find_package(fastgltf REQUIRED)
#find_package(stb REQUIRED)
#find_package(imgui REQUIRED)
#find_package(nlohmann_json REQUIRED)
#find_package(spdlog REQUIRED)

add_engine_module(
        renderSystem
        include/renderSystem/VulkanContext.hpp
        src/vulkanContext/VulkanContext.cpp
        src/vulkanContext/VulkanContextDevice.cpp
        src/vulkanContext/VulkanContextPipelines.cpp
        src/vulkanContext/VulkanContextResources.cpp
        src/vulkanContext/VulkanContextScene3D.cpp
        src/vulkanContext/VulkanContextSprites2D.cpp
        src/vulkanContext/VulkanContextUi.cpp
        include/renderSystem/detail/PipelineBuilder.hpp src/PipelineBuilder.cpp
        include/renderSystem/PSO.hpp src/PSO.cpp
        include/renderSystem/types/Providers.hpp
        src/types/VulkanVertex3d.hpp
        src/types/VulkanVertex2d.hpp
        include/renderSystem/types/DrawCall.hpp
        include/renderSystem/types/VulkanModel.hpp
        include/renderSystem/types/VulkanTexture.hpp
        include/renderSystem/detail/DescriptorSetLayoutBuilder.hpp
        src/DescriptorSetLayoutBuilder.cpp
        include/renderSystem/detail/DescriptorPool.hpp
        src/DescriptorPool.cpp
        include/renderSystem/types/VulkanSprite.hpp
)

target_link_libraries(renderSystem fileSystem logSystem sceneSystem resourceSystem platformSystem typesSystem Vulkan::Vulkan glm)
target_compile_definitions(renderSystem PUBLIC VULKAN_HPP_NO_STRUCT_CONSTRUCTORS GLM_FORCE_RADIANS GLM_FORCE_DEPTH_ZERO_TO_ONE)

target_precompile_headers(renderSystem PRIVATE <vulkan/vulkan_raii.hpp>)