cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# Engine root
# TODO: This variable isn't properly expanding
#set(ENGINE_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/Engine/src)
set(ENGINE_ROOT Crysalisk/Engine/src/)

project(Crysalisk VERSION 1.0)

set(CRYSALISK_ENGINE_SRC_FILES
	${ENGINE_ROOT}/EntryPoint.cpp
	${ENGINE_ROOT}/MemOverride.cpp
	${ENGINE_ROOT}/Core/Engine.cpp
	${ENGINE_ROOT}/Core/Engine.h
	${ENGINE_ROOT}/Core/Event.cpp
	${ENGINE_ROOT}/Core/Event.h
	${ENGINE_ROOT}/Core/Input.cpp
	${ENGINE_ROOT}/Core/Input.h
	${ENGINE_ROOT}/Renderer/Camera.cpp
	${ENGINE_ROOT}/Renderer/Camera.h
	${ENGINE_ROOT}/Renderer/IRendererBackend.h
	${ENGINE_ROOT}/Renderer/Renderer.cpp
	${ENGINE_ROOT}/Renderer/Renderer.h
	${ENGINE_ROOT}/Renderer/RendererTypes.h
	${ENGINE_ROOT}/Resources/GeometryGenerator.cpp
	${ENGINE_ROOT}/Resources/GeometryGenerator.h
	${ENGINE_ROOT}/Resources/Map.cpp
	${ENGINE_ROOT}/Resources/Map.h
	${ENGINE_ROOT}/Resources/ResourceTypes.h
	${ENGINE_ROOT}/Systems/FontSystem.cpp
	${ENGINE_ROOT}/Systems/FontSystem.h
	${ENGINE_ROOT}/Systems/IGame.h
	${ENGINE_ROOT}/Systems/MaterialSystem.cpp
	${ENGINE_ROOT}/Systems/MaterialSystem.h
	${ENGINE_ROOT}/Systems/ShaderSystem.cpp
	${ENGINE_ROOT}/Systems/ShaderSystem.h
	${ENGINE_ROOT}/Systems/TextureSystem.cpp
	${ENGINE_ROOT}/Systems/TextureSystem.h
)

# Group source files
source_group(TREE ${ENGINE_ROOT} FILES ${CRYSALISK_ENGINE_SRC_FILES})

# Create Crysalisk Engine library.
add_library(Engine SHARED ${CRYSALISK_ENGINE_SRC_FILES})

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(Engine PUBLIC ${ENGINE_ROOT})
target_include_directories(Engine PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Core/src/)
target_include_directories(Engine PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Interfaces/src/)
target_include_directories(Engine PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Public)

# Force multi-byte character set.
target_compile_definitions(Engine PRIVATE _MBCS)

target_compile_definitions(Engine PRIVATE DLLNAME=\"Engine.dll\")
target_compile_definitions(Engine PRIVATE ENGINE_DLL)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(Engine PRIVATE PLAT_WINDOWS)
endif()

# Linker input.
target_link_libraries(Engine PRIVATE Core)
target_link_libraries(Engine PRIVATE Interfaces)
