cmake_minimum_required(VERSION 3.24)
project(LangulusModVulkan
    VERSION         1.0.0
    DESCRIPTION     "Langulus plug-in module for rendering real-time graphics using the Vulkan API"
    HOMEPAGE_URL    https://langulus.com
)

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

if(LANGULUS_TESTING)
    enable_testing()
	add_subdirectory(test)
endif()