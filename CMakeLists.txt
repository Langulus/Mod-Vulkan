cmake_minimum_required(VERSION 3.24)
project(LangulusModVulkan)

# Retrieve Vulkan installation, requires CMake 3.24+                        
find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

# Build the module                                                          
add_library(LangulusModVulkan SHARED
    source/Vulkan.cpp
    source/VulkanRenderer.cpp
    source/VulkanRenderable.cpp
    source/VulkanPipeline.cpp
    source/VulkanLight.cpp
    source/VulkanCamera.cpp
    source/VulkanLayer.cpp
    source/Platform.cpp
    source/inner/UBO.cpp
    source/inner/VulkanGeometry.cpp
    source/inner/VulkanMemory.cpp
    source/inner/VulkanShader.cpp
    source/inner/VulkanSwapchain.cpp
    source/inner/VulkanTexture.cpp
)

target_link_libraries(LangulusModVulkan
    PRIVATE     Langulus
                Vulkan::Vulkan
                Vulkan::shaderc_combined
)

target_include_directories (LangulusModVulkan
    PRIVATE     ${Vulkan_INCLUDE_DIRS}
)

if(LANGULUS_ENABLE_TESTING)
    enable_testing()
	add_subdirectory(test)
endif()