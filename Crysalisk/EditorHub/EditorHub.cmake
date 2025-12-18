cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# Editor hub root
# TODO: This variable isn't properly expanding
#set(EDITOR_HUB_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/EditorHub/src)
set(EDITOR_HUB_ROOT Crysalisk/EditorHub/src/)

project(Crysalisk VERSION 1.0)

set(CRYSALISK_EDITOR_HUB_SRC_FILES
	${EDITOR_HUB_ROOT}/EditorHubApp.cpp
	${EDITOR_HUB_ROOT}/MemOverride.cpp
	${EDITOR_HUB_ROOT}/ToolsTypes.h
)

# Group source files
source_group(TREE ${EDITOR_HUB_ROOT} FILES ${CRYSALISK_EDITOR_HUB_SRC_FILES})

# Create Crysalisk Editor executable
add_library(EditorHub SHARED ${CRYSALISK_EDITOR_HUB_SRC_FILES})

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(EditorHub PUBLIC ${EDITOR_HUB_ROOT})
target_include_directories(EditorHub PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Core/src/)
target_include_directories(EditorHub PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Engine/src/)
target_include_directories(EditorHub PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Interfaces/src/)
target_include_directories(EditorHub PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Public/)

# Force multi-byte character set.
target_compile_definitions(EditorHub PRIVATE _MBCS)

target_compile_definitions(EditorHub PRIVATE DLLNAME=\"EditorHub.dll\")
target_compile_definitions(EditorHub PRIVATE EDITOR_HUB_DLL)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(EditorHub PRIVATE PLAT_WINDOWS)
endif()

# Linker input.
target_link_libraries(EditorHub PRIVATE Core)
target_link_libraries(EditorHub PRIVATE Engine)

# Current Working Directory.
set_property(TARGET EditorHub PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CRYSALISK_REPO_ROOT}/Crysalisk")
