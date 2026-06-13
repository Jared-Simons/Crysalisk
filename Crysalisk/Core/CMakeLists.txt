cmake_minimum_required(VERSION 3.10)

# Repository root
set(CRYS_STD_REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Setup engine project.
project(Crys_StdLib VERSION 0.0.1 LANGUAGES CXX)

# Set the output directory.
# TODO: These need to work regardless of the project this lib is being used with.
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CRYS_STD_REPO_ROOT}/bin)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CRYS_STD_REPO_ROOT}/bin)
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CRYS_STD_REPO_ROOT}/bin)

# Setup all the projects.
if (NOT TARGET Core)
#include (Core.cmake)
endif()
include (Core.cmake)
