cmake_minimum_required(VERSION 3.10)

# Repository root
set(CRYSALISK_REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../)

# Setup engine project
project(Crysalisk VERSION 0.0.1 LANGUAGES CXX)

include (../Crysalisk/Engine/Engine.cmake)
