cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# Testapp root
# TODO: This variable isn't properly expanding
#set(TESTAPP_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/Testapp/src)
set(TESTAPP_ROOT ../Crysalisk/Testapp/src/)

project(Engine VERSION 1.0)

set(CRYSALISK_TESTAPP_SRC_FILES
	${TESTAPP_ROOT}/Main.cpp
)

# Create Crysalisk Engine library
add_library(Testapp ${CRYSALISK_TESTAPP_SRC_FILES})

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(Testapp PUBLIC ${TESTAPP_ROOT})
target_include_directories(Testapp PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Core/src/)
target_include_directories(Testapp PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Engine/src/)
target_include_directories(Testapp PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Public)
