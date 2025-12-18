cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# Testapp root
# TODO: This variable isn't properly expanding
#set(TESTAPP_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/Testapp/src)
set(TESTAPP_ROOT Crysalisk/Testapp/src/)

project(Crysalisk VERSION 1.0)

set(CRYSALISK_TESTAPP_SRC_FILES
	${TESTAPP_ROOT}/Main.cpp
)

# Group source files
source_group(TREE ${TESTAPP_ROOT} FILES ${CRYSALISK_TESTAPP_SRC_FILES})

# Create Crysalisk Testapp executable
add_executable(Testapp ${CRYSALISK_TESTAPP_SRC_FILES})

# Proper startup project
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT Testapp)

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(Testapp PUBLIC ${TESTAPP_ROOT})
target_include_directories(Testapp PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Core/src/)
target_include_directories(Testapp PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Engine/src/)
target_include_directories(Testapp PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Public)

# Force multi-byte character set.
target_compile_definitions(Testapp PRIVATE _MBCS)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(Testapp PRIVATE PLAT_WINDOWS)
endif()

target_compile_definitions(Testapp PRIVATE DLLNAME=\"Testapp.exe\")

# Linker input.
target_link_libraries(Testapp PRIVATE Core)
target_link_libraries(Testapp PRIVATE Engine)

# Current Working Directory.
set_property(TARGET Testapp PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CRYSALISK_REPO_ROOT}/Crysalisk")
