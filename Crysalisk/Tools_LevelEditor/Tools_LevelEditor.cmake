cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# Editor root
# TODO: This variable isn't properly expanding
#set(LEVEL_EDITOR_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/Editor/src)
set(LEVEL_EDITOR_ROOT Crysalisk/Tools_LevelEditor/src/)

project(Crysalisk VERSION 1.0)

set(CRYSALISK_LEVEL_EDITOR_SRC_FILES
	${LEVEL_EDITOR_ROOT}/Main.cpp
	${LEVEL_EDITOR_ROOT}/MemOverride.cpp
)

# Group source files
source_group(TREE ${LEVEL_EDITOR_ROOT} FILES ${CRYSALISK_LEVEL_EDITOR_SRC_FILES})

# Create Crysalisk Editor executable
add_library(Tools_LevelEditor SHARED ${CRYSALISK_LEVEL_EDITOR_SRC_FILES})

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(Tools_LevelEditor PUBLIC ${EDITOR_ROOT})
target_include_directories(Tools_LevelEditor PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Core/src/)
target_include_directories(Tools_LevelEditor PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Engine/src/)
target_include_directories(Tools_LevelEditor PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Interfaces/src/)
target_include_directories(Tools_LevelEditor PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Public/)

# Force multi-byte character set.
target_compile_definitions(Tools_LevelEditor PRIVATE _MBCS)

target_compile_definitions(Tools_LevelEditor PRIVATE DLLNAME=\"Tools_LevelEditor.dll\")
target_compile_definitions(Tools_LevelEditor PRIVATE EDITOR_EXE)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(Tools_LevelEditor PRIVATE PLAT_WINDOWS)
endif()

# Linker input.
target_link_libraries(Tools_LevelEditor PRIVATE Core)
target_link_libraries(Tools_LevelEditor PUBLIC Engine)

# Current Working Directory.
set_property(TARGET Tools_LevelEditor PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CRYSALISK_REPO_ROOT}/Crysalisk")
