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
        src/vulkanContext/VulkanContextFrame.cpp
        include/renderSystem/2d/Vulkan2dContext.hpp
        src/2d/Vulkan2dContext.cpp
        src/2d/Vulkan2dUi.cpp
        src/2d/Vulkan2dFrame.cpp
        include/renderSystem/3d/Vulkan3dContext.hpp
        src/3d/Vulkan3dContext.cpp
        src/3d/Vulkan3dScene.cpp
        include/renderSystem/detail/PipelineBuilder.hpp src/PipelineBuilder.cpp
        include/renderSystem/PSO.hpp src/PSO.cpp
        include/renderSystem/detail/DescriptorSetLayoutBuilder.hpp
        src/DescriptorSetLayoutBuilder.cpp
        include/renderSystem/detail/DescriptorPool.hpp
        src/DescriptorPool.cpp
        # common types
        include/renderSystem/types/common/Metadata.hpp
        include/renderSystem/types/common/Providers.hpp
        include/renderSystem/types/common/VulkanTexture.hpp
        # 2d types
        include/renderSystem/types/2d/DrawCall2d.hpp
        include/renderSystem/types/2d/PushConstantObject2d.hpp
        include/renderSystem/types/2d/VulkanSprite.hpp
        include/renderSystem/types/2d/VulkanVertex2d.hpp
        # 3d types
        include/renderSystem/types/3d/DrawCall3d.hpp
        include/renderSystem/types/3d/PushConstantObject3d.hpp
        include/renderSystem/types/3d/PushConstantObjectBlend.hpp
        include/renderSystem/types/3d/UBObject.hpp
        include/renderSystem/types/3d/VulkanModel.hpp
        include/renderSystem/types/3d/VulkanVertex3d.hpp
)

target_link_libraries(renderSystem fileSystem logSystem sceneSystem resourceSystem platformSystem typesSystem Vulkan::Vulkan glm)
target_compile_definitions(renderSystem PUBLIC VULKAN_HPP_NO_STRUCT_CONSTRUCTORS GLM_FORCE_RADIANS GLM_FORCE_DEPTH_ZERO_TO_ONE)

target_precompile_headers(renderSystem PRIVATE <vulkan/vulkan_raii.hpp>)