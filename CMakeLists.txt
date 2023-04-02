project(Langulus.Mod.Vulkan)

# Retrieve Vulkan installation, requires CMake 3.24+                        
find_package(Vulkan REQUIRED COMPONENTS shaderc_combined)

# Build the module                                                          
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/source LANGULUS_MOD_VULKAN_SOURCES)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/source/inner LANGULUS_MOD_VULKAN_SOURCES)
add_library(Mod.Vulkan SHARED ${LANGULUS_MOD_VULKAN_SOURCES})

if (MSVC)
    target_compile_options(Mod.Vulkan PRIVATE /bigobj)
endif()

target_link_libraries(Mod.Vulkan
    PRIVATE     Langulus.Framework
                Vulkan::Vulkan
                Vulkan::shaderc_combined
)

target_include_directories (Mod.Vulkan 
    PUBLIC      ${Vulkan_INCLUDE_DIRS}
)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()