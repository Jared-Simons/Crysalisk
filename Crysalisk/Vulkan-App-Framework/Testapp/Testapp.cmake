cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# Testapp root
# TODO: This variable isn't properly expanding
#set(CORE_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/Core/src)
set(TESTAPP_ROOT Testapp/src/)

project(AppFramework VERSION 1.0)

set(TESTAPP_SRC_FILES
	${TESTAPP_ROOT}/main.cpp
)

# Group source files
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${TESTAPP_SRC_FILES})

# Create Testapp executable.
add_executable(AppFramework_Example ${TESTAPP_SRC_FILES})

# Proper startup project
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT AppFramework_Example)

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(AppFramework_Example PUBLIC ${TESTAPP_ROOT})

# Force multi-byte character set.
target_compile_definitions(AppFramework_Example PRIVATE _MBCS)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(AppFramework_Example PRIVATE PLAT_WINDOWS)
endif()

# Linker input.
target_link_libraries(AppFramework_Example PRIVATE AppFramework)