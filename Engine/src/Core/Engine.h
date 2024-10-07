//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Provides the Engine Interface to be used throughout the codebase.
//=============================================================================
#pragma once

#include "EngineGlobals.h"

// Forward declarations
// TODO: These should not be here.
class Texture;
class StaticMesh;
class DynamicAllocator;
class LinearAllocator;
class Memory;
class IInputSystem;

struct EngineConfig_t
{
	// Set this to true to run the engine without rendering or platform faculties.
	// This is useful for unit tests and dedicated servers that do not require input.
	bool isHeadless = false;
};

class ENGINE_API Engine
{
public:
	static bool	Engine_Initialize(EngineConfig_t engineConfig, void* pEngineState);
	bool	Engine_Run();
	void	Engine_Shutdown();

	// TODO: TEMP
	void	CreateSceneGeometry();

private:
	EngineConfig_t	m_engineConfig;
	DynamicAllocator* m_EngineHeap{ nullptr };
	LinearAllocator* m_SystemsAllocator{ nullptr };
	LinearAllocator* m_FrameAllocator{ nullptr };

	Memory* m_MemorySystem{ nullptr };
	IInputSystem* m_InputSystem{ nullptr };

public:
	// TODO: TEMP
	Texture* testTexture{ nullptr };
	StaticMesh* m_TestGeometry{ nullptr };
	// TODO: END TEMP
};

