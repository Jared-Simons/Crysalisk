cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# Vulkan Renderer root
# TODO: This variable isn't properly expanding
#set(RENDERER_VULKAN_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/Renderer_Vulkan/src)
set(RENDERER_VULKAN_ROOT Crysalisk/Renderer_Vulkan/src/)

project(Crysalisk VERSION 1.0)

set(CRYSALISK_RENDERER_VULKAN_SRC_FILES
	${RENDERER_VULKAN_ROOT}/MemOverride.cpp
	${RENDERER_VULKAN_ROOT}/VulkanBuffer.cpp
	${RENDERER_VULKAN_ROOT}/VulkanDevice.cpp
	${RENDERER_VULKAN_ROOT}/VulkanImage.cpp
	${RENDERER_VULKAN_ROOT}/VulkanRenderer.cpp
	${RENDERER_VULKAN_ROOT}/VulkanRenderer.h
	${RENDERER_VULKAN_ROOT}/VulkanSwapchain.cpp
	${RENDERER_VULKAN_ROOT}/VulkanTypes.h
	${RENDERER_VULKAN_ROOT}/VulkanUtils.h
	${RENDERER_VULKAN_ROOT}/Platform/VulkanPlatform_Win32.cpp
)

# Group source files
source_group(TREE ${RENDERER_VULKAN_ROOT} FILES ${CRYSALISK_RENDERER_VULKAN_SRC_FILES})

# Create Crysalisk Vulkan Renderer library.
add_library(Renderer_Vulkan SHARED ${CRYSALISK_RENDERER_VULKAN_SRC_FILES})

# Setup a post build command to copy the bins to a game build.
add_custom_command(TARGET Renderer_Vulkan POST_BUILD 
					COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:Renderer_Vulkan>" "${OUTPUT_BIN_CORE_DIRECTORY}/")

# Working directory for debugger
set_target_properties(Renderer_Vulkan PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${OUTPUT_BIN_GAME_ASSET_DIRECTORY}/")

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(Renderer_Vulkan PUBLIC ${RENDERER_VULKAN_ROOT})
target_include_directories(Renderer_Vulkan PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Core/src/)
target_include_directories(Renderer_Vulkan PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Engine/src/)
target_include_directories(Renderer_Vulkan PUBLIC $ENV{VULKAN_SDK}/Include/)
target_include_directories(Renderer_Vulkan PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Public)

# Compile defines
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(Renderer_Vulkan PRIVATE PLAT_WINDOWS)
	target_compile_definitions(Renderer_Vulkan PRIVATE VK_USE_PLATFORM_WIN32_KHR)
	target_compile_definitions(Renderer_Vulkan PRIVATE VK_PROTOTYPES)	
endif()

# Force multi-byte character set.
target_compile_definitions(Renderer_Vulkan PRIVATE _MBCS)
target_compile_definitions(Renderer_Vulkan PRIVATE DLLNAME=\"Renderer_Vulkan.dll\")

# Linker directories
target_link_directories(Renderer_Vulkan PRIVATE $ENV{VULKAN_SDK}/Lib/)

# Linker input.
target_link_libraries(Renderer_Vulkan PRIVATE Core)
target_link_libraries(Renderer_Vulkan PRIVATE Engine)
target_link_libraries(Renderer_Vulkan PRIVATE Vulkan-1) # Vulkan symbol lib
