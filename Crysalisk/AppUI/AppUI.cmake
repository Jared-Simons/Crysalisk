cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# Editor hub root
# TODO: This variable isn't properly expanding
#set(APPUI_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/AppUI/src)
set(APPUI_ROOT Crysalisk/AppUI/)

project(Crysalisk VERSION 1.0)

set(CRYSALISK_APPUI_SRC_FILES
	# Controls
	${APPUI_ROOT}/src/Controls/Button.cpp
	${APPUI_ROOT}/src/Controls/Button.h
	${APPUI_ROOT}/src/Controls/Frame.cpp
	${APPUI_ROOT}/src/Controls/Frame.h
	${APPUI_ROOT}/src/Controls/ImagePanel.cpp
	${APPUI_ROOT}/src/Controls/ImagePanel.h
	${APPUI_ROOT}/src/Controls/Label.cpp
	${APPUI_ROOT}/src/Controls/Label.h
	${APPUI_ROOT}/src/Controls/Panel.cpp
	${APPUI_ROOT}/src/Controls/Panel.h

	${APPUI_ROOT}/src/Renderer/AppUI_View.cpp
	${APPUI_ROOT}/src/Renderer/AppUI_View.h
	${APPUI_ROOT}/src/AppUI.cpp
	${APPUI_ROOT}/src/AppUI.h
	${APPUI_ROOT}/src/DeveloperConsole.cpp
	${APPUI_ROOT}/src/DeveloperConsole.h
	${APPUI_ROOT}/src/MemOverride.cpp
	${APPUI_ROOT}/src/UI_Types.h
)

# Group source files
source_group(TREE ${APPUI_ROOT} FILES ${CRYSALISK_APPUI_SRC_FILES})

# Create Crysalisk Editor executable
add_library(AppUI SHARED ${CRYSALISK_APPUI_SRC_FILES})

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(AppUI PUBLIC ${APPUI_ROOT}/src)
target_include_directories(AppUI PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Core/src/)
target_include_directories(AppUI PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Engine/src/)
target_include_directories(AppUI PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Public/)

# Force multi-byte character set.
target_compile_definitions(AppUI PRIVATE _MBCS)

target_compile_definitions(AppUI PRIVATE DLLNAME=\"AppUI.dll\")
target_compile_definitions(AppUI PRIVATE APPUI_DLL)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(AppUI PRIVATE PLAT_WINDOWS)
endif()

# Linker input.
target_link_libraries(AppUI PRIVATE Core)
target_link_libraries(AppUI PRIVATE Engine)

# Current Working Directory.
set_property(TARGET AppUI PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CRYSALISK_REPO_ROOT}/Crysalisk")
