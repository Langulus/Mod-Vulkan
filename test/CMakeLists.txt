add_executable(LangulusModVulkanTest
	Main.cpp
	TestRenderer.cpp
)

target_link_libraries(LangulusModVulkanTest
	PRIVATE		Langulus
				Catch2
)

add_test(
	NAME		LangulusModVulkanTest
	COMMAND		LangulusModVulkanTest
)

langulus_copy_dlls(LangulusModVulkanTest ON LangulusModVulkanTest)
langulus_copy_dlls_advanced(LangulusModVulkanTest
	TO			LangulusModVulkanTest
	FROM		LangulusModGLFW
				LangulusModVulkan
				LangulusModAssetsImages
				LangulusModFileSystem
)

# Make the write and read data dir for PhysFS, because it doesn't have access   
add_custom_command(
    TARGET LangulusModVulkanTest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:LangulusModVulkanTest>/data/assets/images"
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/polygons.png" "$<TARGET_FILE_DIR:LangulusModVulkanTest>/data/assets/images/polygons.png"
)