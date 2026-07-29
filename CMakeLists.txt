cmake_minimum_required(VERSION 3.10)

# Repository root
set(CRYSALISK_REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Setup engine project.
project(Crysalisk VERSION 0.0.1 LANGUAGES CXX)

# Set the output directory.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CRYSALISK_REPO_ROOT}/Crysalisk/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CRYSALISK_REPO_ROOT}/Crysalisk/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CRYSALISK_REPO_ROOT}/Crysalisk/bin)

set(OUTPUT_BIN_GAME_DIRECTORY ${CRYSALISK_REPO_ROOT}/Game/game/bin)
set(OUTPUT_BIN_CORE_DIRECTORY ${CRYSALISK_REPO_ROOT}/Game/bin)

# This path is the same as the game binaries directory so that only one copy of assets need to 
# exist for the application to work from both the debugger and stand-alone.
set(OUTPUT_BIN_GAME_ASSET_DIRECTORY ${CRYSALISK_REPO_ROOT}/Game/bin)

# Setup all the projects.
include (Crysalisk/Core/Core.cmake)
include (Crysalisk/Engine/Engine.cmake)
include (Crysalisk/Renderer_Vulkan/Renderer_Vulkan.cmake)
include (Crysalisk/AppUI/AppUI.cmake)
include (Crysalisk/EditorHub/EditorHub.cmake)
include (Crysalisk/Tools_LevelEditor/Tools_LevelEditor.cmake)

include (Crysalisk/Testapp/Testapp.cmake)
include (Crysalisk/Editor/Editor.cmake)
include (Crysalisk/UnitTest/UnitTest.cmake)
