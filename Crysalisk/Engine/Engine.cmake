cmake_minimum_required(VERSION 3.10)

# Engine root
# TODO: This variable isn't properly expanding
#set(ENGINE_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/Engine/src)
set(ENGINE_ROOT ../Crysalisk/Engine/src/)

project(Engine VERSION 1.0)

set(CRYSALISK_ENGINE_SRC_FILES
	${ENGINE_ROOT}/Core/Engine.cpp
	${ENGINE_ROOT}/Core/Engine.h
	${ENGINE_ROOT}/Core/Event.cpp
	${ENGINE_ROOT}/Core/Event.h
	${ENGINE_ROOT}/Core/Input.cpp
	${ENGINE_ROOT}/Core/Input.h
)

# Create Crysalisk Engine library
add_library(Engine ${CRYSALISK_ENGINE_SRC_FILES})
