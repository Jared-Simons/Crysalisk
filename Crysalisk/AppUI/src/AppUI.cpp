//=============================================================================
// Author: Jared Simons
// Date Created: 6/13/2026
// 
// Purpose: Entry point to the AppUI plugin.
//=============================================================================
#include "AppUI.h"

#include "Renderer/AppUI_View.h"

// Controls
#include "Controls/Panel.h"
#include "Controls/Button.h"

// Core
#include <Core/Logging.h>

// Engine
#include <Application.h>
#include <Core/Engine.h>
#include <Core/Input.h>
#include <Renderer/Renderer.h>
#include <Renderer/RendererTypes.h>
#include <Systems/ShaderSystem.h>

namespace AppUI
{
	class CAppUI : public IAppUI
	{
	public:
		virtual bool	PreInitialize(struct ApplicationConfig_t* pAppConfig) override;
		virtual bool	Initialize(struct ApplicationConfig_t* pConfig) override;

		virtual void	FramePrepare(struct RenderPacket_t* pRenderPacket) override;
		virtual void	Update() override;

		virtual UIControlHandle_t	CreateUIControl(eControlType controlType) override;
		virtual void				DestroyUIControl(UIControlHandle_t hControl) override;

		virtual void	SetControlPosition(UIControlHandle_t hControl, u16 x, u16 y) override;
		virtual void	SetControlDimensions(UIControlHandle_t hControl, u16 width, u16 height) override;
		virtual void	SetControlColor(UIControlHandle_t hControl, u8 r, u8 g, u8 b, u8 a) override;

		AppUIRenderContext_t	renderContext{};

		Utl::CVector<UIControlRenderData_t>	renderables{};

		// Controls
		Utl::CVector<BaseUIControl>			baseControls{};
		Utl::CVector<ButtonControlData_t>	buttonControls{};
	};

	static UIControlHandle_t GetAvailableBaseControl(CAppUI* pUIContext);
	static u32				 GetAvailableRenderableIndex(CAppUI* pUIContext);


	bool CAppUI::PreInitialize(struct ApplicationConfig_t* pAppConfig)
	{
		CAssert(pAppConfig);

		pAppConfig->pViews.PushBack(RenderViewCreate_AppUIView());

		return true;
	}

	bool CAppUI::Initialize(struct ApplicationConfig_t* pAppConfig)
	{
		CAssert(pAppConfig);

		LOG_INFO("Initializing AppUI framework");

		renderContext.m_pRenderer = pAppConfig->pEngine->GetEngineInterfaces()->pRenderer;
		renderContext.m_pShaderSystem = pAppConfig->pEngine->GetEngineInterfaces()->pShaderSystem;

		// Get renderbuffer pointers.
		// FIXME: We need a better way creating renderbuffers.
		renderContext.m_pVertexBuffer = renderContext.m_pRenderer->GetRenderBufferByName("AppUI_Vertex_Buffer");
		renderContext.m_pIndexBuffer = renderContext.m_pRenderer->GetRenderBufferByName("AppUI_Index_Buffer");

		renderables.Resize(32);
		renderables.SetCount(32);	// FIXME: We need to get rid of this pattern.

		baseControls.Resize(32);
		baseControls.SetCount(32);

		buttonControls.Resize(32);
		buttonControls.SetCount(32);

		return true;
	}

	void CAppUI::FramePrepare(struct RenderPacket_t* pRenderPacket)
	{
		pRenderPacket->pEditorExtendedData = &renderables[0];
	}

	void CAppUI::Update()
	{
		BaseUIControl* pCurrentControl = nullptr;
		// Update our controls.
		for (u32 iControl = 0; iControl < baseControls.Count(); iControl++)
		{
			pCurrentControl = &baseControls[iControl];

			if (pCurrentControl->m_eControlType != CONTROL_TYPE_UNDEFINED)
			{
				switch (pCurrentControl->m_eControlType)
				{
				case CONTROL_TYPE_BUTTON:
					{
						// Is the mouse hovering over the button?
						i32 xPos, yPos;
						g_pEngine->GetEngineInterfaces()->pInputSystem->GetCurrentMousePos(xPos, yPos);
						if ((xPos >= pCurrentControl->x && xPos <= pCurrentControl->x + pCurrentControl->width) &&
						   (yPos >= pCurrentControl->y && yPos <= pCurrentControl->y + pCurrentControl->height))
						{
							// Button is hovered over, make sure we update our state.
							Vector4D hoveredColor{ 0.0f, 1.0f, 0.0f, 1.0f };

							UIControlRenderData_t* pRenderable = &renderables[pCurrentControl->m_iRenderableIdx];
							renderContext.m_pShaderSystem->SetShaderUniform(pRenderable->hShader,
								renderContext.m_pShaderSystem->GetShaderUniformLocation(pRenderable->hShader, "color"), 0, pRenderable->iShaderInstance, &hoveredColor);

							// If the left mouse button is pressed, do the callback.
							if (g_pEngine->GetEngineInterfaces()->pInputSystem->IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
							{
								// FIXME: Don't hardcode the control index.
								buttonControls[0].PFN_UI_Button_Control_Clicked();
							}
						}
						else
						{
							// Button is not hovered over, reset state.
							Vector4D notHoveredColor{ 1.0f, 0.0f, 0.0f, 1.0f };

							UIControlRenderData_t* pRenderable = &renderables[pCurrentControl->m_iRenderableIdx];
							renderContext.m_pShaderSystem->SetShaderUniform(pRenderable->hShader,
								renderContext.m_pShaderSystem->GetShaderUniformLocation(pRenderable->hShader, "color"), 0, pRenderable->iShaderInstance, &notHoveredColor);
						}
					}
					break;

				default:
					// Just keep going.
					continue;
				}
			}
		}
	}

	UIControlHandle_t CAppUI::CreateUIControl(eControlType controlType)
	{
		UIControlHandle_t	hNewControl = GetAvailableBaseControl(this);
		u32					renderableIdx = GetAvailableRenderableIndex(this);

		BaseUIControl* pBase = &baseControls[hNewControl];
		UIControlRenderData_t* pRenderable = &renderables[renderableIdx];
		pBase->m_iRenderableIdx = renderableIdx;

		switch (controlType)
		{
		case CONTROL_TYPE_PANEL:
			CreatePanel(pBase, pRenderable, &renderContext);
			break;

		case CONTROL_TYPE_BUTTON:
			{	
				// LEFTOFF: Make this work properly.
				ButtonControlData_t* pButtonData = &buttonControls[0];
				CreateButton(pBase, pRenderable, pButtonData, &renderContext);
				break;
			}

		default:
			CAssert(0);
			break;
		}

		return hNewControl;
	}

	void CAppUI::DestroyUIControl(UIControlHandle_t hControl)
	{
		CAssert(hControl != kInvalidID_U32);
		BaseUIControl* pBase = &baseControls[hControl];
		switch (pBase->m_eControlType)
		{
		case CONTROL_TYPE_PANEL:
			DestroyPanel(pBase, &renderables[pBase->m_iRenderableIdx], &renderContext);
			break;

		default:
			//CAssert(0);
			break;
		}
	}

	void CAppUI::SetControlPosition(UIControlHandle_t hControl, u16 x, u16 y)
	{
		CAssert(hControl != kInvalidID_U32);
		BaseUIControl* pBase = &baseControls[hControl];
		switch (pBase->m_eControlType)
		{
		case CONTROL_TYPE_PANEL:
		case CONTROL_TYPE_BUTTON:
			SetPanelControlPosition(pBase, &renderables[pBase->m_iRenderableIdx], &renderContext, x, y);
			break;

		default:
			CAssert(0);
			break;
		}
	}

	void CAppUI::SetControlDimensions(UIControlHandle_t hControl, u16 width, u16 height)
	{
		CAssert(hControl != kInvalidID_U32);
		BaseUIControl* pBase = &baseControls[hControl];
		switch (pBase->m_eControlType)
		{
		case CONTROL_TYPE_PANEL:
		case CONTROL_TYPE_BUTTON:
			SetPanelControlDimensions(pBase, &renderables[pBase->m_iRenderableIdx], &renderContext, width, height);
			break;

		default:
			CAssert(0);
			break;
		}
	}

	void CAppUI::SetControlColor(UIControlHandle_t hControl, u8 r, u8 g, u8 b, u8 a)
	{
		CAssert(hControl != kInvalidID_U32);
		BaseUIControl* pBase = &baseControls[hControl];
		UIControlRenderData_t* pRenderable = &renderables[pBase->m_iRenderableIdx];

		// Convert the color range from [0-256] to [0-1].
		float flR = (float)(r / 256.0f);
		float flG = (float)(g / 256.0f);
		float flB = (float)(b / 256.0f);
		float flA = (float)(a / 256.0f);
		
		pBase->color.x = flR;
		pBase->color.y = flG;
		pBase->color.z = flB;
		pBase->color.w = flA;
		renderContext.m_pShaderSystem->SetShaderUniform(pRenderable->hShader, 
			renderContext.m_pShaderSystem->GetShaderUniformLocation(pRenderable->hShader, "color"), 0, pRenderable->iShaderInstance, &pBase->color);
	}

	static UIControlHandle_t GetAvailableBaseControl(CAppUI* pUIContext)
	{
		for (u32 iBase = 0; iBase < pUIContext->baseControls.Count(); iBase++)
		{
			if (pUIContext->baseControls[iBase].m_eControlType == CONTROL_TYPE_UNDEFINED)
			{
				// Found an empty slot!
				return iBase;
			}
		}
		CASSERTMSG(0, "Failed to find an available base UI control!");
		return kInvalidID_U32;
	}

	static u32 GetAvailableRenderableIndex(CAppUI* pUIContext)
	{
		for (u32 iRenderable = 0; iRenderable < pUIContext->renderables.Count(); iRenderable++)
		{
			if (pUIContext->renderables[iRenderable].vertexCount == kInvalidID_U64)
			{
				// Found an empty slot!
				return iRenderable;
			}
		}
		CASSERTMSG(0, "Failed to find an available UI renderable data slot!");
		return kInvalidID_U32;
	}

} // namespace AppUI

// FIXME: Store this somewhere, doesn't need to be global.
AppUI::CAppUI s_UISystem;
AppUI::IAppUI* g_pUISystem = &s_UISystem;

AppUI::IAppUI* GetUIPluginInterface()
{
	return g_pUISystem;
}
