project(Langulus.Test.Mod.Vulkan)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} LANGULUS_MOD_VULKAN_TEST_SOURCES)
add_executable(Test.Mod.Vulkan ${LANGULUS_MOD_VULKAN_TEST_SOURCES})

target_link_libraries(Test.Mod.Vulkan
	PRIVATE		Langulus.Framework
				Catch2
)

langulus_copy_dlls(
	TO		Test.Mod.Vulkan
	FROM	Langulus.Framework
)

add_test(NAME Test.Mod.Vulkan COMMAND Test.Mod.Vulkan)
