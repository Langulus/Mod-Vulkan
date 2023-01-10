project(Langulus.Mod.Vulkan)

find_package(Vulkan REQUIRED)

# Build the module                                                          
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/source LANGULUS_MOD_VULKAN_SOURCES)
add_library(Mod.Vulkan SHARED ${LANGULUS_MOD_VULKAN_SOURCES})

target_link_libraries(Mod.Vulkan
    PRIVATE     Langulus.Framework
                Vulkan
)

target_include_directories (Mod.Vulkan 
    PUBLIC      ${Vulkan_INCLUDE_DIRS}
)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()