cmake_minimum_required(VERSION 3.10)

# Repository root
set(CRYSALISK_REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Setup engine project.
project(Crysalisk VERSION 0.0.1 LANGUAGES CXX)

# Set the output directory.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CRYSALISK_REPO_ROOT}/Crysalisk/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CRYSALISK_REPO_ROOT}/Crysalisk/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CRYSALISK_REPO_ROOT}/Crysalisk/bin)

# Setup all the projects.
include (Crysalisk/Core/Core.cmake)
include (Crysalisk/Editor/Editor.cmake)
include (Crysalisk/Engine/Engine.cmake)
include (Crysalisk/Interfaces/Interfaces.cmake)
include (Crysalisk/Renderer_Vulkan/Renderer_Vulkan.cmake)
include (Crysalisk/Testapp/Testapp.cmake)
include (Crysalisk/UnitTest/UnitTest.cmake)
