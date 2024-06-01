cmake_minimum_required(VERSION 3.28)
project(LangulusModVulkan
    VERSION         1.0.0
    DESCRIPTION     "Langulus plug-in module for rendering real-time graphics using the Vulkan API"
    HOMEPAGE_URL    https://langulus.com
)

# Retrieve Vulkan installation, requires CMake 3.24+                            
find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

file(GLOB_RECURSE
	LANGULUS_MOD_VULKAN_SOURCES 
	LIST_DIRECTORIES FALSE CONFIGURE_DEPENDS
	source/*.cpp
)

# Build the module                                                              
add_library(LangulusModVulkan SHARED ${LANGULUS_MOD_VULKAN_SOURCES})

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