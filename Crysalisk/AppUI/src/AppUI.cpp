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
#include "Controls/Frame.h"
#include "Controls/Label.h"
#include "Controls/Button.h"

// Core
#include <Core/Logging.h>

// Engine
#include <Application.h>
#include <Core/Engine.h>
#include <Core/Event.h>
#include <Core/Input.h>
#include <Renderer/Renderer.h>
#include <Renderer/RendererTypes.h>
#include <Systems/ShaderSystem.h>

// Engine event handlers.
static bool Event_WindowResized(void* pEventListener, EventData_t data);

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

		virtual void	SetControlParent(UIControlHandle_t hChild, UIControlHandle_t hParent) override;
		virtual void	SetControlPosition(UIControlHandle_t hControl, u16 x, u16 y) override;
		virtual void	SetControlDimensions(UIControlHandle_t hControl, u16 width, u16 height) override;
		virtual void	SetControlColor(UIControlHandle_t hControl, u8 r, u8 g, u8 b, u8 a) override;

		virtual void	SetControlText(UIControlHandle_t hControl, const char* pszText) override;
		virtual void	SetControlFont(UIControlHandle_t hControl, const char* pszFontName, f32 flFontSize) override;

		void			RegenerateControlGeometry(u32 newWidth, u32 newHeight);

		enum ePluginInitializeState{ PLUGIN_STATE_NOT_INITIALIZED = 0, PLUGIN_STATE_PRE_INITIALIZED, PLUGIN_STATE_INITIALIZED };
		u8 pluginState{ PLUGIN_STATE_NOT_INITIALIZED };

		AppUIRenderContext_t	renderContext{};

		Utl::CVector<UIControlRenderData_t>	renderables{};

		// Controls
		Utl::CVector<UIBaseControl_t>		baseControls{};
		Utl::CVector<UIControlPanel_t>		panels{};
		Utl::CVector<UIControlFrame_t>		frames{};
		Utl::CVector<UIControlLabel_t>		labels{};
		Utl::CVector<UIControlButton_t>		buttons{};
	};

	static UIControlHandle_t GetAvailableBaseControl(CAppUI* pUIContext);
	static u32				 GetAvailableRenderableIndex(CAppUI* pUIContext);

	bool CAppUI::PreInitialize(struct ApplicationConfig_t* pAppConfig)
	{
		// This plugin can be accessed from numerous different users at once. Make sure we only initialize once.
		if (pluginState > PLUGIN_STATE_NOT_INITIALIZED)
		{
			return true;
		}

		CAssert(pAppConfig);

		pAppConfig->pViews.PushBack(RenderViewCreate_AppUIView());

		pluginState = PLUGIN_STATE_PRE_INITIALIZED;

		return true;
	}

	bool CAppUI::Initialize(struct ApplicationConfig_t* pAppConfig)
	{
		CAssert(pAppConfig);

		// This plugin can be accessed from numerous different users at once. Make sure we only initialize once.
		if (pluginState > PLUGIN_STATE_PRE_INITIALIZED)
		{
			return true;
		}

		LOG_INFO("Initializing AppUI framework");

		// FIXME: Unregister this on plugin shutdown.
		pAppConfig->pEngine->GetEngineInterfaces()->pEngineEventHandler->EventRegister(eEventCategory::WINDOW_RESIZE, this, Event_WindowResized);

		// FIXME: Don't hardcode these.
		renderContext.currentSurfaceWidth = 1080;
		renderContext.currentSurfaceHeight = 720;

		renderContext.m_pRenderer = pAppConfig->pEngine->GetEngineInterfaces()->pRenderer;
		renderContext.m_pShaderSystem = pAppConfig->pEngine->GetEngineInterfaces()->pShaderSystem;

		// Get renderbuffer pointers.
		// FIXME: We need a better way creating renderbuffers.
		renderContext.m_pVertexBuffer = renderContext.m_pRenderer->GetRenderBufferByName("AppUI_Vertex_Buffer");
		renderContext.m_pIndexBuffer = renderContext.m_pRenderer->GetRenderBufferByName("AppUI_Index_Buffer");

		pluginState = PLUGIN_STATE_INITIALIZED;

		return true;
	}

	void CAppUI::FramePrepare(RenderPacket_t* pRenderPacket)
	{
		if (pRenderPacket->pRenderViewExtendedData[RENDER_VIEW_TYPE_APPUI] == nullptr)
		{
			pRenderPacket->pRenderViewExtendedData[RENDER_VIEW_TYPE_APPUI] = new AppUIView_ExtendedData_t;
			AppUIView_ExtendedData_t* pViewData = (AppUIView_ExtendedData_t*)pRenderPacket->pRenderViewExtendedData[RENDER_VIEW_TYPE_APPUI];

			pViewData->pVertexBuffer = renderContext.m_pRenderer->GetRenderBufferByName("AppUI_Vertex_Buffer");
			pViewData->pIndexBuffer = renderContext.m_pRenderer->GetRenderBufferByName("AppUI_Index_Buffer");

			// Allocate geometry data using the frame allocator.
			pViewData->renderables.SetAllocator(pRenderPacket->pFrameAllocator);
		}

		AppUIView_ExtendedData_t* pViewData = (AppUIView_ExtendedData_t*)pRenderPacket->pRenderViewExtendedData[RENDER_VIEW_TYPE_APPUI];

		// FIXME: Put this in a post render method?
		// Clear out data from previous frame.
		pViewData->renderables.Clear();

		pViewData->renderables.Resize(renderables.Count());
		pViewData->projectionMatrix = Matrix4x4::Orthographic(0.0f, (f32)renderContext.currentSurfaceWidth, 0.0f, (f32)renderContext.currentSurfaceHeight, 0.1f, 1000.0f);

		for (u32 iRenderable = 0; iRenderable < renderables.Count(); iRenderable++)
		{
			if (renderables[iRenderable].vertexCount)
			{
				pViewData->renderables.PushBack(&renderables[iRenderable]);
			}
		}
	}

	void CAppUI::Update()
	{
		UIBaseControl_t* pCurrentControl = nullptr;
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
						// FIXME: Make this work.
						/*
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
								buttons[0].PFN_UI_Button_Control_Clicked();
							}
						}
						else
						{
							// Button is not hovered over, reset state.
							Vector4D notHoveredColor{ 1.0f, 0.0f, 0.0f, 1.0f };

							UIControlRenderData_t* pRenderable = &renderables[pCurrentControl->m_iRenderableIdx];
							renderContext.m_pShaderSystem->SetShaderUniform(pRenderable->hShader,
								renderContext.m_pShaderSystem->GetShaderUniformLocation(pRenderable->hShader, "color"), 0, pRenderable->iShaderInstance, &notHoveredColor);
						}*/
					}
					break;

				case CONTROL_TYPE_LABEL:
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
		UIBaseControl_t*	pNewBase = &baseControls[hNewControl];

		pNewBase->m_iRenderableIdx = GetAvailableRenderableIndex(this);
		UIControlRenderData_t* pRenderable = &renderables[pNewBase->m_iRenderableIdx];

		switch (controlType)
		{
		case CONTROL_TYPE_PANEL:
			{
				panels.PushBack(UIControlPanel_t{});
				UIControlPanel_t* pPanelData = &panels.Back();
				pNewBase->m_iControlDataIdx = (u32)panels.Count() - 1;

				CreatePanel(pNewBase, pRenderable, pPanelData, &renderContext);
				// These calls will generate the base control geometry.
				SetPanelControlPosition(pPanelData, &renderables[pNewBase->m_iRenderableIdx], &renderContext, pPanelData->x, pPanelData->y);
				SetPanelControlDimensions(pPanelData, &renderables[pNewBase->m_iRenderableIdx], &renderContext, pPanelData->width, pPanelData->height);
			}
			break;

		case CONTROL_TYPE_FRAME:
			{
				/*frames.PushBack(UIControlFrame_t{});
				UIControlFrame_t* pFrameData = &frames.Back();

				CreateFrameControl(pBasePanel, pRenderable, pFrameData, &renderContext);
				// These calls will generate the base control geometry.
				SetPanelControlPosition(pBasePanel, &renderables[pBasePanel->m_iRenderableIdx], &renderContext, pBasePanel->x, pBasePanel->y);
				SetPanelControlDimensions(pBasePanel, &renderables[pBasePanel->m_iRenderableIdx], &renderContext, pBasePanel->width, pBasePanel->height);*/
			}
			break;

		case CONTROL_TYPE_LABEL:
			{
				labels.PushBack(UIControlLabel_t{});
				UIControlLabel_t* pLabelData = &labels.Back();
				pNewBase->m_iControlDataIdx = (u32)labels.Count() - 1;

				CreateLabelControl(pNewBase, pRenderable, pLabelData, &renderContext);
			}
			break;

		case CONTROL_TYPE_BUTTON:
			{	
				UIControlHandle_t hButtonPanel = CreateUIControl(CONTROL_TYPE_PANEL);
				CAssert(hButtonPanel != kInvalidID_U32);

				buttons.PushBack(UIControlButton_t{});
				UIControlButton_t* pButtonData = &buttons.Back();

				pButtonData->pPanel = &panels[hButtonPanel];

				CreateButton(pButtonData->pPanel, pRenderable, pButtonData, &renderContext);
				// These calls will generate the base control geometry.
				SetPanelControlPosition(pButtonData->pPanel, &renderables[pNewBase->m_iRenderableIdx], &renderContext, pButtonData->pPanel->x, pButtonData->pPanel->y);
				SetPanelControlDimensions(pButtonData->pPanel, &renderables[pNewBase->m_iRenderableIdx], &renderContext, pButtonData->pPanel->width, pButtonData->pPanel->height);
				
			}
			break;

		default:
			CAssert(0);
			break;
		}

		return hNewControl;
	}

	void CAppUI::DestroyUIControl(UIControlHandle_t hControl)
	{
		CAssert(hControl != kInvalidID_U32);
		UIBaseControl_t* pBase = &baseControls[hControl];
		UIControlPanel_t* pPanel = &panels[pBase->m_iControlDataIdx];
		switch (pBase->m_eControlType)
		{
		case CONTROL_TYPE_PANEL:
			DestroyPanel(pPanel, &renderables[pBase->m_iRenderableIdx], &renderContext);
			break;

		default:
			//CAssert(0);
			break;
		}
	}

	void CAppUI::SetControlPosition(UIControlHandle_t hControl, u16 x, u16 y)
	{
		CAssert(hControl != kInvalidID_U32);
		UIBaseControl_t* pBase = &baseControls[hControl];
		UIControlPanel_t* pPanel = nullptr;

		// A control's position is defined relative to its parent, if it has one.
		UIBaseControl_t* pParentBase = nullptr;
		u16 parentedX = x;
		u16 parentedY = y;

		if (pBase->m_hParentControl != kInvalidID_U32)
		{
			pParentBase = &baseControls[pBase->m_hParentControl];
			
			switch (pParentBase->m_eControlType)
			{
			case CONTROL_TYPE_PANEL:	
				pPanel = &panels[pParentBase->m_iControlDataIdx];
				break;
			case CONTROL_TYPE_FRAME:
				pPanel = frames[pParentBase->m_iControlDataIdx].pPanel;
				break;
			default:
				CAssert(0);
				break;
			}

			parentedX += pPanel->x;
			parentedY += pPanel->y;
		}

		switch (pBase->m_eControlType)
		{
		case CONTROL_TYPE_PANEL:
			pPanel = &panels[pBase->m_iControlDataIdx];
			SetPanelControlPosition(pPanel, &renderables[pBase->m_iRenderableIdx], &renderContext, parentedX, parentedY);
			break;
		case CONTROL_TYPE_FRAME:
			pPanel = frames[pBase->m_iControlDataIdx].pPanel;
			SetPanelControlPosition(pPanel, &renderables[pBase->m_iRenderableIdx], &renderContext, parentedX, parentedY);
			break;
		case CONTROL_TYPE_LABEL:
			{
				UIControlLabel_t* pLabelData = &labels[pBase->m_iControlDataIdx];
				SetLabelPosition(pLabelData, parentedX, parentedY);
			}
			break;
		case CONTROL_TYPE_BUTTON:
			// FIXME: Make this work.
			SetPanelControlPosition(pPanel, &renderables[pBase->m_iRenderableIdx], &renderContext, parentedX, parentedY);
			break;

		default:
			CAssert(0);
			break;
		}
	}

	void CAppUI::SetControlDimensions(UIControlHandle_t hControl, u16 width, u16 height)
	{
		CAssert(hControl != kInvalidID_U32);
		UIBaseControl_t* pBase = &baseControls[hControl];
		UIControlPanel_t* pPanel = nullptr;
		switch (pBase->m_eControlType)
		{
		case CONTROL_TYPE_PANEL:
			pPanel = &panels[pBase->m_iControlDataIdx];
			SetPanelControlDimensions(pPanel, &renderables[pBase->m_iRenderableIdx], &renderContext, width, height);
			break;
		case CONTROL_TYPE_FRAME:
			pPanel = frames[pBase->m_iControlDataIdx].pPanel;
			SetPanelControlDimensions(pPanel, &renderables[pBase->m_iRenderableIdx], &renderContext, width, height);
			break;
		case CONTROL_TYPE_BUTTON:
			// FIXME: Make this work.
			SetPanelControlDimensions(pPanel, &renderables[pBase->m_iRenderableIdx], &renderContext, width, height);
			break;

		default:
			CAssert(0);
			break;
		}
	}

	void CAppUI::SetControlParent(UIControlHandle_t hChild, UIControlHandle_t hParent)
	{
		CAssert(hChild != kInvalidID_U32 && hParent != kInvalidID_U32);
		UIBaseControl_t* pChildBase = &baseControls[hChild];
		pChildBase->m_hParentControl = hParent;
	}

	void CAppUI::SetControlColor(UIControlHandle_t hControl, u8 r, u8 g, u8 b, u8 a)
	{
		CAssert(hControl != kInvalidID_U32);
		UIBaseControl_t* pBase = &baseControls[hControl];
		UIControlPanel_t* pPanel = &panels[hControl];
		UIControlRenderData_t* pRenderable = &renderables[pBase->m_iRenderableIdx];

		// Convert the color range from [0-256] to [0-1].
		float flR = (float)(r / 256.0f);
		float flG = (float)(g / 256.0f);
		float flB = (float)(b / 256.0f);
		float flA = (float)(a / 256.0f);
		
		pPanel->color.x = flR;
		pPanel->color.y = flG;
		pPanel->color.z = flB;
		pPanel->color.w = flA;
		renderContext.m_pShaderSystem->SetShaderUniform(pRenderable->hShader, 
			renderContext.m_pShaderSystem->GetShaderUniformLocation(pRenderable->hShader, "color"), 0, pRenderable->iShaderInstance, &pPanel->color);
	}

	void CAppUI::SetControlText(UIControlHandle_t hControl, const char* pszText)
	{
		CAssert(hControl != kInvalidID_U32 && pszText);
		UIBaseControl_t* pBase = &baseControls[hControl];
		CASSERTMSG(pBase->m_eControlType == CONTROL_TYPE_LABEL, "SetControlText - This function must only be called for UI elements that support text!");

		UIControlLabel_t* pLabel = &labels[pBase->m_iControlDataIdx];
		UIControlRenderData_t* pRenderable = &renderables[pBase->m_iRenderableIdx];

		SetLabelText(pLabel, pRenderable, &renderContext, pszText);
	}

	void CAppUI::SetControlFont(UIControlHandle_t hControl, const char* pszFontName, f32 flFontSize)
	{
		CAssert(hControl != kInvalidID_U32 && pszFontName);
		UIBaseControl_t* pBase = &baseControls[hControl];
		CASSERTMSG(pBase->m_eControlType == CONTROL_TYPE_LABEL, "SetControlFont - This function must only be called for UI elements that support text!");

		UIControlLabel_t* pLabel = &labels[pBase->m_iControlDataIdx];
		SetLabelFont(pLabel, &renderables[pBase->m_iRenderableIdx], &renderContext, pszFontName, flFontSize);
	}

	void CAppUI::RegenerateControlGeometry(u32 newWidth, u32 newHeight)
	{
		u32 oldSurfaceWidth = renderContext.currentSurfaceWidth;
		u32 oldSurfaceHeight = renderContext.currentSurfaceHeight;

		// We need to stretch/squish the values for the new controls to that they are correct proportionally at the new resolution.
		for (u32 iControl = 0; iControl < baseControls.Count(); ++iControl)
		{
			UIBaseControl_t* pCurrentBase = &baseControls[iControl];
			if (pCurrentBase->m_eControlType != CONTROL_TYPE_UNDEFINED)
			{
				if (pCurrentBase->m_eControlType != CONTROL_TYPE_LABEL)
				{
					UIControlPanel_t* pCurrentControl = &panels[pCurrentBase->m_iControlDataIdx];

					// These will regenerate control geometry. FIXME: Just regenerate without setting fields?
					SetPanelControlPosition(pCurrentControl, &renderables[pCurrentBase->m_iRenderableIdx], &renderContext, pCurrentControl->x, pCurrentControl->y);
					SetPanelControlDimensions(pCurrentControl, &renderables[pCurrentBase->m_iRenderableIdx], &renderContext, pCurrentControl->width, pCurrentControl->height);
				}
				else
				{
					// Label case.
					UIControlLabel_t* pLabel = &labels[pCurrentBase->m_iControlDataIdx];
					UIControlRenderData_t* pRenderable = &renderables[pCurrentBase->m_iRenderableIdx];

					RegenerateLabelGeometry(pLabel, pRenderable, &renderContext);
				}
			}
		}

		// Lastly, store off these new values.
		renderContext.currentSurfaceWidth = newWidth;
		renderContext.currentSurfaceHeight = newHeight;
	}

	static UIControlHandle_t GetAvailableBaseControl(CAppUI* pUIContext)
	{
		for (u32 iBase = 0; iBase < pUIContext->baseControls.Capacity(); iBase++)
		{
			if (pUIContext->baseControls[iBase].m_eControlType == CONTROL_TYPE_UNDEFINED)
			{
				// Found an empty slot!
				return iBase;
			}
		}

		// Failed to find a control, push a new one.
		return (u32)pUIContext->baseControls.PushBack(UIBaseControl_t{});
	}

	static u32 GetAvailableRenderableIndex(CAppUI* pUIContext)
	{
		for (u32 iRenderable = 0; iRenderable < pUIContext->renderables.Count(); iRenderable++)
		{
			if (pUIContext->renderables[iRenderable].vertexCount == kInvalidID_U64 && pUIContext->renderables[iRenderable].geo.vertexCount == kInvalidID_U64)
			{
				// Found an empty slot!
				return iRenderable;
			}
		}

		// Failed to find a renderable, push a new one.
		return (u32)pUIContext->renderables.PushBack(UIControlRenderData_t{});
	}

} // namespace AppUI

// FIXME: Store this somewhere, doesn't need to be global.
AppUI::CAppUI s_UISystem;
AppUI::IAppUI* g_pUISystem = &s_UISystem;

AppUI::IAppUI* GetUIPluginInterface()
{
	return g_pUISystem;
}

static bool Event_WindowResized(void* pEventListener, EventData_t data)
{
	LOG_INFO("AppUI detected window resize. Regenerating control geometry.");

	AppUI::CAppUI* pUIContext = (AppUI::CAppUI*)pEventListener;

	if (pUIContext)
	{
		pUIContext->RegenerateControlGeometry(data.u32[0], data.u32[1]);
	}

	// Let other listeners handle this.
	return false;
}
