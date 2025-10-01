cmake_minimum_required(VERSION 3.10)

# C++ 17 minimum
set(CMAKE_CXX_STANDARD 17)

# Core root
# TODO: This variable isn't properly expanding
#set(CORE_ROOT %{CRYSALISK_REPO_ROOT}/Crysalisk/Core/src)
set(CORE_ROOT ../Crysalisk/Core/src/)

project(Engine VERSION 1.0)

set(CRYSALISK_CORE_SRC_FILES
	${CORE_ROOT}/MemOverride.cpp
	${CORE_ROOT}/Containers/CFreeList.h
	${CORE_ROOT}/Containers/CHashMap.h
	${CORE_ROOT}/Containers/CVector.h
	${CORE_ROOT}/Core/Assertions.h
	${CORE_ROOT}/Core/Clock.cpp
	${CORE_ROOT}/Core/Clock.h
	${CORE_ROOT}/Core/InputTypes.h
	${CORE_ROOT}/Core/Logging.cpp
	${CORE_ROOT}/Core/Logging.h
	${CORE_ROOT}/Math/MathLib.cpp
	${CORE_ROOT}/Math/MathLib.h
	${CORE_ROOT}/Math/MatrixTypes.h
	${CORE_ROOT}/Math/VectorTypes.h
	${CORE_ROOT}/Math/VertexTypes.h
	${CORE_ROOT}/Memory/DynamicAllocator.cpp
	${CORE_ROOT}/Memory/DynamicAllocator.h
	${CORE_ROOT}/Memory/IMemoryAllocator.h
	${CORE_ROOT}/Memory/MemoryAllocatorBase.cpp
	${CORE_ROOT}/Memory/MemoryAllocatorBase.h
	${CORE_ROOT}/Memory/MemoryManager.cpp
	${CORE_ROOT}/Memory/MemoryManager.h
	${CORE_ROOT}/Platform/FileSystem.cpp
	${CORE_ROOT}/Platform/FileSystem.h
	${CORE_ROOT}/Platform/InterfaceRegistry.cpp
	${CORE_ROOT}/Platform/InterfaceRegistry.h
	${CORE_ROOT}/Platform/Platform.h
	${CORE_ROOT}/Platform/Platform_Win32.cpp
	${CORE_ROOT}/Resources/Loaders/FontLoader.cpp
	${CORE_ROOT}/Resources/Loaders/FontLoader.h
	${CORE_ROOT}/Resources/Loaders/ImageLoader.cpp
	${CORE_ROOT}/Resources/Loaders/ImageLoader.h
	${CORE_ROOT}/Resources/Loaders/MapLoader.cpp
	${CORE_ROOT}/Resources/Loaders/MapLoader.h
	${CORE_ROOT}/Resources/Loaders/MaterialLoader.cpp
	${CORE_ROOT}/Resources/Loaders/MaterialLoader.h
	${CORE_ROOT}/Resources/Loaders/MeshLoader.cpp
	${CORE_ROOT}/Resources/Loaders/MeshLoader.h
	${CORE_ROOT}/Resources/Loaders/ShaderLoader.cpp
	${CORE_ROOT}/Resources/Loaders/ShaderLoader.h
	${CORE_ROOT}/Resources/Serializers/MaterialSerializer.cpp
	${CORE_ROOT}/Resources/Serializers/MaterialSerializer.h
	${CORE_ROOT}/Resources/Serializers/MeshSerializer.cpp
	${CORE_ROOT}/Resources/Serializers/MeshSerializer.h
	${CORE_ROOT}/Strings/String_Utils.cpp
	${CORE_ROOT}/Strings/String_Utils.h
	${CORE_ROOT}/Utils/Utils.cpp
	${CORE_ROOT}/Utils/Utils.h
	${CORE_ROOT}/Vendor/stb_image.h
	${CORE_ROOT}/Vendor/stb_truetype.h
)

# Create Crysalisk Core Utilities library
add_library(Core ${CRYSALISK_CORE_SRC_FILES})

# Include directories
# TODO: Get these directories from somewhere else.
target_include_directories(Core PUBLIC ${CORE_ROOT})
target_include_directories(Core PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Engine/src/)
target_include_directories(Core PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Interfaces/src/)
target_include_directories(Core PUBLIC ${CRYSALISK_REPO_ROOT}/Crysalisk/Public/)
