file(GLOB_RECURSE
	LANGULUS_MOD_VULKAN_TEST_SOURCES 
	LIST_DIRECTORIES FALSE CONFIGURE_DEPENDS
	*.cpp
)

add_executable(LangulusModVulkanTest ${LANGULUS_MOD_VULKAN_TEST_SOURCES})

target_link_libraries(LangulusModVulkanTest
	PRIVATE		Langulus
				Catch2
)

add_dependencies(LangulusModVulkanTest
	LangulusModVulkan
	LangulusModGLFW
	LangulusModFileSystem
	LangulusModAssetsImages
	LangulusModAssetsGeometry
	LangulusModAssetsMaterials
	LangulusModPhysics
)

add_test(
	NAME		LangulusModVulkanTest
	COMMAND		LangulusModVulkanTest
	WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
)

# Make the write and read data dir for PhysFS, because it doesn't have access   
add_custom_command(
    TARGET LangulusModVulkanTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
		"$<TARGET_FILE_DIR:LangulusModVulkanTest>/data/assets/images"
    COMMAND ${CMAKE_COMMAND} -E copy
		"${CMAKE_CURRENT_SOURCE_DIR}/polygons.png"
		"$<TARGET_FILE_DIR:LangulusModVulkanTest>/data/assets/images/polygons.png"
)