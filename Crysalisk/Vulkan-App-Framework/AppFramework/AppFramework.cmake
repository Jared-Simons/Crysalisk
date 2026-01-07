cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# App Framework root
# TODO: This variable isn't properly expanding
#set(CORE_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/Core/src)
set(APPFRAMEWORK_ROOT AppFramework/src/)

project(AppFramework VERSION 1.0)

set(APPFRAMEWORK_SRC_FILES
	${APPFRAMEWORK_ROOT}/pch.h
	${APPFRAMEWORK_ROOT}/ControlFactory.cpp
	${APPFRAMEWORK_ROOT}/ControlFactory.h
	${APPFRAMEWORK_ROOT}/Test.cpp
	${APPFRAMEWORK_ROOT}/Test.h
	${APPFRAMEWORK_ROOT}/UIContext.cpp
	${APPFRAMEWORK_ROOT}/UIContext.h
	${APPFRAMEWORK_ROOT}/Controls/BasePanel.cpp
	${APPFRAMEWORK_ROOT}/Controls/BasePanel.h
	${APPFRAMEWORK_ROOT}/Controls/Button.cpp
	${APPFRAMEWORK_ROOT}/Controls/Button.h
	${APPFRAMEWORK_ROOT}/Renderer/IRendererBackend.h
	${APPFRAMEWORK_ROOT}/Renderer/RendererTypes.h
	${APPFRAMEWORK_ROOT}/Renderer/VulkanPlatform.h
	${APPFRAMEWORK_ROOT}/Renderer/VulkanPlatform_Win32.cpp
	${APPFRAMEWORK_ROOT}/Renderer/VulkanRenderer.cpp
	${APPFRAMEWORK_ROOT}/Renderer/VulkanRenderer.h
	${APPFRAMEWORK_ROOT}/Renderer/VulkanUtils.h
)

# Group source files
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${APPFRAMEWORK_SRC_FILES})

# Create AppFramework library.
add_library(AppFramework ${APPFRAMEWORK_SRC_FILES})

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(AppFramework PUBLIC ${APPFRAMEWORK_ROOT})
target_include_directories(AppFramework PUBLIC ${APPFRAMEWORK_ROOT}../../Crysalisk-Stdlib/src)
target_include_directories(AppFramework PUBLIC $ENV{VULKAN_SDK}/Include/)

# Force multi-byte character set.
target_compile_definitions(AppFramework PRIVATE _MBCS)

# Compile defines
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(AppFramework PRIVATE PLAT_WINDOWS)
	target_compile_definitions(AppFramework PRIVATE VK_USE_PLATFORM_WIN32_KHR)
	target_compile_definitions(AppFramework PRIVATE VK_PROTOTYPES)	
endif()

# Linker directories
target_link_directories(AppFramework PUBLIC $ENV{VULKAN_SDK}/Lib/)

# Linker input.
target_link_libraries(AppFramework PUBLIC Core)

# Linker input.
target_link_libraries(AppFramework PUBLIC Vulkan-1) # Vulkan symbol lib