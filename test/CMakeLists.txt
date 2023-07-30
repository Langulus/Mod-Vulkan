project(LangulusModVulkanTest)

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
)