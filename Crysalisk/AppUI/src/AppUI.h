//=============================================================================
// Author: Jared Simons
// Date Created: 6/13/2026
// 
// Purpose: Entry point to the AppUI plugin.
//=============================================================================
#pragma once

#include <Defines.h>

#include <UI_Types.h>

// Core
#include <Containers/CVector.h>

// Forward declarations
struct ApplicationConfig_t;
struct RenderPacket_t;
class IRenderer;
class IShaderSystem;
class RenderBuffer;

namespace AppUI
{
	struct AppUIRenderContext_t
	{
		IRenderer* m_pRenderer{ nullptr };
		IShaderSystem* m_pShaderSystem{ nullptr };
		RenderBuffer* m_pVertexBuffer{ nullptr };
		RenderBuffer* m_pIndexBuffer{ nullptr };

		u32			currentSurfaceWidth{ 0 };
		u32			currentSurfaceHeight{ 0 };
	};

	class IAppUI
	{
	public:
		virtual bool	PreInitialize(struct ApplicationConfig_t* pAppConfig) = 0; 
		virtual bool	Initialize(struct ApplicationConfig_t* pConfig) = 0;

		virtual void	FramePrepare(struct RenderPacket_t* pRenderPacket) = 0;
		virtual void	Update() = 0;

		virtual UIControlHandle_t	CreateUIControl(eControlType controlType) = 0;
		virtual void				DestroyUIControl(UIControlHandle_t hControl) = 0;

		virtual void	SetControlParent(UIControlHandle_t hChild, UIControlHandle_t hParent) = 0;
		virtual void	SetControlPosition(UIControlHandle_t hControl, u16 x, u16 y) = 0;
		virtual void	SetControlDimensions(UIControlHandle_t hControl, u16 width, u16 height) = 0;
		virtual void	SetControlColor(UIControlHandle_t hControl, u8 r, u8 g, u8 b, u8 a) = 0;

		virtual void	SetControlText(UIControlHandle_t hControl, const char* pszText) = 0;
		virtual void	SetControlFont(UIControlHandle_t hControl, const char* pszFontName, f32 flFontSize) = 0;
	};

} // namespace AppUI

extern AppUI::IAppUI* g_pUISystem;

extern "C" APPUI_API AppUI::IAppUI* GetUIPluginInterface();
