cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# Vulkan Renderer root
# TODO: This variable isn't properly expanding
#set(RENDERER_VULKAN_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/Renderer_Vulkan/src)
set(RENDERER_VULKAN_ROOT ../Crysalisk/Renderer_Vulkan/src/)

project(Engine VERSION 1.0)

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

# Create Crysalisk Vulkan Renderer library
add_library(Renderer_Vulkan ${CRYSALISK_RENDERER_VULKAN_SRC_FILES})

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(Renderer_Vulkan PUBLIC ${RENDERER_VULKAN_ROOT})
target_include_directories(Renderer_Vulkan PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Core/src/)
target_include_directories(Renderer_Vulkan PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Engine/src/)
target_include_directories(Renderer_Vulkan PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Interfaces/src/)
target_include_directories(Renderer_Vulkan PUBLIC $ENV{VULKAN_SDK}/Include/)
target_include_directories(Renderer_Vulkan PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Public)

# Compile defines
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(Renderer_Vulkan PRIVATE VK_USE_PLATFORM_WIN32_KHR)
	target_compile_definitions(Renderer_Vulkan PRIVATE VK_PROTOTYPES)	
endif()
