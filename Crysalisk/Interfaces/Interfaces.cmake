cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# Interface root
# TODO: This variable isn't properly expanding
#set(INTERFACE_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/Interfaces/src)
set(INTERFACE_ROOT Crysalisk/Interfaces/src/)

project(Crysalisk VERSION 1.0)

set(CRYSALISK_INTERFACES_SRC_FILES
	${INTERFACE_ROOT}/Interfaces.cpp
	${INTERFACE_ROOT}/Interfaces.h
)

# Group source files
source_group(TREE ${INTERFACE_ROOT} FILES ${CRYSALISK_INTERFACES_SRC_FILES})

# Create Crysalisk Interface library.
add_library(Interfaces STATIC ${CRYSALISK_INTERFACES_SRC_FILES})

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(Interfaces PUBLIC ${INTERFACE_ROOT})
target_include_directories(Interfaces PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Core/src/)
target_include_directories(Interfaces PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Engine/src/)
target_include_directories(Interfaces PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Public)

# Force multi-byte character set.
target_compile_definitions(Interfaces PRIVATE _MBCS)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(Engine PRIVATE PLAT_WINDOWS)
endif()
