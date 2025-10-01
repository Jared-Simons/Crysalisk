cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# Interface root
# TODO: This variable isn't properly expanding
#set(INTERFACE_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/Interfaces/src)
set(INTERFACE_ROOT ../Crysalisk/Interfaces/src/)

project(Engine VERSION 1.0)

set(CRYSALISK_INTERFACES_SRC_FILES
	${INTERFACE_ROOT}/Interfaces.cpp
	${INTERFACE_ROOT}/Interfaces.h
)

# Create Crysalisk Interface library
add_library(Interfaces ${CRYSALISK_INTERFACES_SRC_FILES})

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(Interfaces PUBLIC ${INTERFACE_ROOT})
target_include_directories(Interfaces PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Core/src/)
target_include_directories(Interfaces PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Engine/src/)
target_include_directories(Interfaces PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Public)
