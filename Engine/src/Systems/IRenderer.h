//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: The Rendering Front-end Interface
//=============================================================================
#pragma once

// Forward Declarations
class LinearAllocator;

class IRenderer
{
public:
	static bool		Renderer_Create(IRenderer* statePtr, LinearAllocator* pAllocator);
	static void		Renderer_Shutdown(IRenderer* statePtr, bool bOwnsMemory = false);

	virtual bool	Initialize() = 0;
	virtual void	Shutdown() = 0;
	virtual bool	Update() = 0;
};

extern IRenderer* gpRenderer;
