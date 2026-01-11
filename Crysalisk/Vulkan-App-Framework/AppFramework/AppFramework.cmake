cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# App Framework root
# TODO: This variable isn't properly expanding
#set(CORE_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/Core/src)
set(APPFRAMEWORK_ROOT AppFramework/src/)

project(AppFramework VERSION 1.0)

set(APPFRAMEWORK_SRC_FILES
	${APPFRAMEWORK_ROOT}/AppFramework.cpp
	${APPFRAMEWORK_ROOT}/AppFramework.h
)

# Group source files
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${APPFRAMEWORK_SRC_FILES})

# Create AppFramework library.
add_library(AppFramework SHARED ${APPFRAMEWORK_SRC_FILES})

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(AppFramework PUBLIC ${APPFRAMEWORK_ROOT})
target_include_directories(AppFramework PUBLIC ${APPFRAMEWORK_ROOT}../Engine/src)

# Force multi-byte character set.
target_compile_definitions(AppFramework PRIVATE _MBCS)

# Compile defines
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(AppFramework PRIVATE PLAT_WINDOWS)
	target_compile_definitions(AppFramework PRIVATE VK_USE_PLATFORM_WIN32_KHR)
	target_compile_definitions(AppFramework PRIVATE VK_PROTOTYPES)	
endif()

# Linker directories

# Linker input.
target_link_libraries(AppFramework PUBLIC Core)
target_link_libraries(AppFramework PUBLIC Engine)

# Linker input.

# Post-Build copy.
add_custom_command(
	TARGET AppFramework
	POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
	${CMAKE_CURRENT_SOURCE_DIR}/bin
	${CMAKE_CURRENT_SOURCE_DIR}/../bin
)
