cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# UnitTest root
# TODO: This variable isn't properly expanding
#set(UNITTEST_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/UnitTest/src)
set(UNITTEST_ROOT Crysalisk/UnitTest/src/)

project(Crysalisk VERSION 1.0)

set(CRYSALISK_UNITTEST_SRC_FILES
	${UNITTEST_ROOT}/main.cpp
	${UNITTEST_ROOT}/UnitTestType.h
	${UNITTEST_ROOT}/Containers/CVectorTests.cpp
	${UNITTEST_ROOT}/Containers/CVectorTests.h
)

# Group source files
source_group(TREE ${UNITTEST_ROOT} FILES ${CRYSALISK_UNITTEST_SRC_FILES})

# Create Crysalisk Unit Test executable.
add_executable(UnitTest ${CRYSALISK_UNITTEST_SRC_FILES})

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(UnitTest PUBLIC ${UNITTEST_ROOT})
target_include_directories(UnitTest PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Engine/src/)
target_include_directories(UnitTest PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Public)

# Force multi-byte character set.
target_compile_definitions(UnitTest PRIVATE _MBCS)

target_compile_definitions(UnitTest PRIVATE DLLNAME=\"UnitTest.exe\")

# Linker input.
target_link_libraries(UnitTest PRIVATE Core)

# Current Working Directory.
set_property(TARGET Testapp PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CRYSALISK_REPO_ROOT}/Crysalisk")
