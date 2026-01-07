cmake_minimum_required(VERSION 3.10)

# Repository root
set(APPFRAMEWORK_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

# Setup app framework project.
project(AppFramework VERSION 0.0.1 LANGUAGES CXX)

# Set the output directory.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${APPFRAMEWORK_ROOT}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${APPFRAMEWORK_ROOT}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${APPFRAMEWORK_ROOT}/bin)

# Setup all the projects.
include (AppFramework/AppFramework.cmake)
include (Testapp/Testapp.cmake)

# We need the Crys-STD lib as well.
add_subdirectory(Crysalisk-Stdlib/)
