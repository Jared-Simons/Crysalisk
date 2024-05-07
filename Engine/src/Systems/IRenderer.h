//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: The Rendering Front-end Interface
//=============================================================================
#pragma once

class IRenderer
{
public:
	virtual bool	Renderer_Initialize() = 0;
	virtual void	Renderer_Shutdown() = 0;
	virtual bool	Renderer_Update() = 0;
};

extern IRenderer* gpRenderer;
