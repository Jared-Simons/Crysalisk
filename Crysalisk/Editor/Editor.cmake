cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# Editor root
# TODO: This variable isn't properly expanding
#set(EDITOR_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/Editor/src)
set(EDITOR_ROOT Crysalisk/Editor/src/)

project(Crysalisk VERSION 1.0)

set(CRYSALISK_EDITOR_EXE_SRC_FILES
	${EDITOR_ROOT}/Main.cpp
	${EDITOR_ROOT}/MemOverride.cpp
)

# Group source files
source_group(TREE ${EDITOR_ROOT} FILES ${CRYSALISK_EDITOR_EXE_SRC_FILES})

# Create Crysalisk Editor executable
add_executable(Editor ${CRYSALISK_EDITOR_EXE_SRC_FILES})

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(Editor PUBLIC ${EDITOR_ROOT})
target_include_directories(Editor PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Core/src/)
target_include_directories(Editor PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Engine/src/)
target_include_directories(Editor PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Interfaces/src/)
target_include_directories(Editor PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Public/)

# Force multi-byte character set.
target_compile_definitions(Editor PRIVATE _MBCS)

target_compile_definitions(Editor PRIVATE DLLNAME=\"Editor.exe\")
target_compile_definitions(Editor PRIVATE EDITOR_EXE)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(Editor PRIVATE PLAT_WINDOWS)
endif()

# Linker input.
target_link_libraries(Editor PRIVATE Core)
target_link_libraries(Editor PUBLIC Engine)

# Current Working Directory.
set_property(TARGET Editor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CRYSALISK_REPO_ROOT}/Crysalisk")
