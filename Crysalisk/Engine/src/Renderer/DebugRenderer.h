//=============================================================================
// Author: Jared Simons
// Date Created: 10/11/2025
// 
// Purpose: The debug renderer, used to draw simple primitives.
//=============================================================================
#pragma once

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
class IDebugRenderer
{
public:
	virtual bool	Initialize() = 0;
	virtual void	Shutdown() = 0;

	virtual void	Render() = 0;
};

extern IDebugRenderer* g_pDebugRenderer;
