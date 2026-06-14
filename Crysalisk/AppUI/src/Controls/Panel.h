//=============================================================================
// Author: Jared Simons
// Date Created: 6/14/2026
// 
// Purpose: A base panel, simply used for parenting other controls to.
//=============================================================================
#pragma once

#include <Defines.h>
#include <UI_Types.h>

// Engine
// FIXME: These really shouldn't be here.
#include <Renderer/Renderer.h>
#include <Systems/ShaderSystem.h>

namespace AppUI
{
	BaseUIControl*	CreatePanel(u16 x, u16 y, u16 width, u16 height, IRenderer* pRenderer, IShaderSystem* pShaderSystem, RenderBuffer* pVertexBuffer, RenderBuffer* pIndexBuffer);
	void			DestroyPanel(BaseUIControl* pPanel);

} // namespace AppUI
