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
#include "Controls/ImagePanel.h"

// Core
#include <Core/InputTypes.h>
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
		~CAppUI() {}

		virtual bool	PreInitialize(struct ApplicationConfig_t* pAppConfig) override;
		virtual bool	Initialize(struct ApplicationConfig_t* pConfig) override;
		virtual void	Shutdown() override;

		virtual void	FramePrepare(struct RenderPacket_t* pRenderPacket) override;
		virtual void	Update() override;

		virtual void	SetRenderPriorityControl(UIControlHandle_t hControl) override;

		virtual void	AcquireInputFocus() override;
		virtual void	LoseInputFocus() override;

		virtual void	SetActiveControl(UIControlHandle_t hControl, bool bRegenerateControlLayout) override;

		// Call this to recalculate the order in which UI elements should be rendered.
		virtual void	RegenerateRenderList() override;

		virtual UIControlHandle_t	CreateUIControl(eControlType controlType) override;
		virtual void				DestroyUIControl(UIControlHandle_t hControl) override;

		virtual void				SetControlFlag(UIControlHandle_t hControl, eUIControlFlagBits flag, bool bSet) override;

		virtual void	SetControlVisible(UIControlHandle_t hControl, bool visible) override;

		virtual void	SetControlParent(UIControlHandle_t hChild, UIControlHandle_t hParent) override;
		virtual void	SetControlPosition(UIControlHandle_t hControl, u16 x, u16 y) override;
		virtual void	SetControlDimensions(UIControlHandle_t hControl, u16 width, u16 height) override;
		virtual void	SetControlColor(UIControlHandle_t hControl, u8 r, u8 g, u8 b, u8 a) override;

		virtual char*	GetControlText(UIControlHandle_t hControl) override;
		virtual void	SetControlText(UIControlHandle_t hControl, const char* pszText) override;

		virtual void	SetControlFont(UIControlHandle_t hControl, const char* pszFontName, f32 flFontSize) override;

		virtual void	SetControlImage(UIControlHandle_t hControl, TextureHandle_t hImage, FontLoader::GlyphLayoutData_t* pGlyphData) override;

		virtual void	SetControlPressedCallback(UIControlHandle_t hControl, PFN_UI_Button_Control_Clicked callback, void* pListener) override;
		virtual void	SetControlInteractable(UIControlHandle_t hControl, bool bInteractable) override;

		virtual UIControlHandle_t GetActiveControl() const override { return hActiveControl; }

		void			RegenerateControlGeometry(u32 newWidth, u32 newHeight);

		enum ePluginInitializeState{ PLUGIN_STATE_NOT_INITIALIZED = 0, PLUGIN_STATE_PRE_INITIALIZED, PLUGIN_STATE_INITIALIZED };
		u8 pluginState{ PLUGIN_STATE_NOT_INITIALIZED };

		AppUIRenderContext_t	m_RenderContext{};
		IInputSystem*			m_pInputSystem{ nullptr };

		InputMap_t m_InputHandler{};

		Utl::CVector<u32>					orderedRenderList{};

		Utl::CVector<UIControlRenderData_t>	renderables{};
		Utl::CVector<UIImageRenderData_t>	imageRenderData{};

		// Controls
		Utl::CVector<UIBaseControl_t>		baseControls{};
		Utl::CVector<UIControlPanel_t>		panels{};
		Utl::CVector<UIControlFrame_t>		frames{};
		Utl::CVector<UIControlLabel_t>		labels{};
		Utl::CVector<UIControlButton_t>		buttons{};
		Utl::CVector<UIControlImagePanel_t>	imagePanels{};

		// Default font.
		FontHandle_t						hDefaultFont{ kInvalidID_U32 };

		UIControlHandle_t					hActiveControl{ kInvalidID_U32 };
	};

	static UIControlHandle_t	GetAvailableBaseControl(CAppUI* pUIContext);
	static u32					GetAvailableRenderableIndex(CAppUI* pUIContext);
	static const char*			GetStringForControlType(u16 controlType);

	// Input handlers.
	static bool Input_KeyEvent_Handler(void* pListener, u16 keyCode, bool pressed, keyMessageFlagBits flags);
	static bool Input_MouseButtonEvent_Handler(void* pListener, u16 mouseButtonFlags);

	bool CAppUI::PreInitialize(struct ApplicationConfig_t* pAppConfig)
	{
		CASSERTMSG(!pAppConfig->pUIContext, "CAppUI attempting initialize multiple times!");

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
		m_RenderContext.currentSurfaceWidth = 1080;
		m_RenderContext.currentSurfaceHeight = 720;

		m_RenderContext.m_pRenderer = pAppConfig->pEngine->GetEngineInterfaces()->pRenderer;
		m_RenderContext.m_pShaderSystem = pAppConfig->pEngine->GetEngineInterfaces()->pShaderSystem;
		m_pInputSystem = pAppConfig->pEngine->GetEngineInterfaces()->pInputSystem;

		// Get renderbuffer pointers.
		// FIXME: We need a better way creating renderbuffers.
		m_RenderContext.m_pVertexBuffer = m_RenderContext.m_pRenderer->GetRenderBufferByName("AppUI_Vertex_Buffer");
		m_RenderContext.m_pIndexBuffer = m_RenderContext.m_pRenderer->GetRenderBufferByName("AppUI_Index_Buffer");

		// Setup event handlers.
		m_InputHandler.PFN_KeyEvent_Callback = Input_KeyEvent_Handler;
		m_InputHandler.PFN_MouseButtonEvent_Callback = Input_MouseButtonEvent_Handler;
		m_InputHandler.pListenerInst = this;

		// Default font.
		hDefaultFont = pAppConfig->pEngine->GetEngineInterfaces()->pFontSystem->AcquireFontResources("UbuntuMono-R.ttf", 30.0f);

		pluginState = PLUGIN_STATE_INITIALIZED;

		// FIXME: Letting this start from 0 and resize doesn't seem to work.
		// It appears to be trying to free the same block of memory multiple times.
		// This hack should settle things for now.
		baseControls.Resize(40);

		return true;
	}

	void CAppUI::Shutdown()
	{
		// Destroy all controls.
		imagePanels.Purge();
		buttons.Purge();
		labels.Purge();
		frames.Purge();
		panels.Purge();
		baseControls.Purge();
	}

	void CAppUI::FramePrepare(RenderPacket_t* pRenderPacket)
	{
		// If the ordered renderable list has not yet been generated, do that now.
		if (orderedRenderList.Count() < 1)
		{
			RegenerateRenderList();
		}

		if (pRenderPacket->pRenderViewExtendedData[RENDER_VIEW_TYPE_APPUI] == nullptr)
		{
			pRenderPacket->pRenderViewExtendedData[RENDER_VIEW_TYPE_APPUI] = new AppUIView_ExtendedData_t;
			AppUIView_ExtendedData_t* pViewData = (AppUIView_ExtendedData_t*)pRenderPacket->pRenderViewExtendedData[RENDER_VIEW_TYPE_APPUI];

			pViewData->pVertexBuffer = m_RenderContext.m_pRenderer->GetRenderBufferByName("AppUI_Vertex_Buffer");
			pViewData->pIndexBuffer = m_RenderContext.m_pRenderer->GetRenderBufferByName("AppUI_Index_Buffer");

			// Allocate geometry data using the frame allocator.
			pViewData->renderables.SetAllocator(pRenderPacket->pFrameAllocator);
			pViewData->imageRenderData.SetAllocator(pRenderPacket->pFrameAllocator);
		}

		AppUIView_ExtendedData_t* pViewData = (AppUIView_ExtendedData_t*)pRenderPacket->pRenderViewExtendedData[RENDER_VIEW_TYPE_APPUI];

		// FIXME: Put this in a post render method?
		// Clear out data from previous frame.
		pViewData->renderables.Purge();
		pViewData->imageRenderData.Purge();

		pViewData->renderables.Resize(renderables.Count());
		pViewData->imageRenderData.Resize(imagePanels.Count());
		pViewData->projectionMatrix = Matrix4x4::Orthographic(0.0f, (f32)m_RenderContext.currentSurfaceWidth, 0.0f, (f32)m_RenderContext.currentSurfaceHeight, 0.1f, 1000.0f);

		for (u32 iControl = 0; iControl < orderedRenderList.Count(); iControl++)
		{
			UIControlRenderData_t* pRenderable = &renderables[baseControls[orderedRenderList[iControl]].m_iRenderableIdx];

			if (pRenderable->vertexCount)
			{
				pViewData->renderables.PushBack(pRenderable);
			}
		}

		for (u32 iImage = 0; iImage < imageRenderData.Count(); iImage++)
		{
			pViewData->imageRenderData.PushBack(&imageRenderData[iImage]);
		}
	}

	void CAppUI::Update()
	{
		// Update our controls.
		for (u32 iControl = 0; iControl < baseControls.Count(); iControl++)
		{
			UIBaseControl_t* pCurrentBase = &baseControls[iControl];

			if (pCurrentBase->m_eControlType != CONTROL_TYPE_UNDEFINED)
			{
				i32 xPos{ 0 }, yPos{ 0 };
				g_pEngine->GetEngineInterfaces()->pInputSystem->GetCurrentMousePos(xPos, yPos);

				switch (pCurrentBase->m_eControlType)
				{
				case CONTROL_TYPE_BUTTON:
					{
						UIControlButton_t* pCurrentControl = &buttons[pCurrentBase->m_iControlDataIdx];

						// Is the mouse hovering over the button?
						if ((xPos >= pCurrentControl->panel.x && xPos <= pCurrentControl->panel.x + pCurrentControl->panel.width) &&
						   (yPos >= pCurrentControl->panel.y && yPos <= pCurrentControl->panel.y + pCurrentControl->panel.height))
						{
							// Button is hovered over, make sure we update our state.
							Vector4D hoveredColor{ 0.0f, 1.0f, 0.0f, 1.0f };

							UIControlRenderData_t* pRenderable = &renderables[pCurrentBase->m_iRenderableIdx];
							m_RenderContext.m_pShaderSystem->SetShaderUniform(pRenderable->hShader,
								m_RenderContext.m_pShaderSystem->GetShaderUniformLocation(pRenderable->hShader, "color"), 0, pRenderable->iShaderInstance, &hoveredColor);

							// If the left mouse button is pressed, do the callback.
							// FIXME: Only if it's a fresh click! No dragging a pressed cursor over the button.
							if (g_pEngine->GetEngineInterfaces()->pInputSystem->IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && pCurrentBase->m_bInteractable)
							{
								if (pCurrentControl->onPressedCallback)
								{
									pCurrentControl->onPressedCallback(pCurrentControl->callbackListenerInst);
								}
							}
						}
						else
						{
							// Button is not hovered over, reset state.
							// FIXME: Cache off shader uniform locations.
							UIControlPanel_t* pPanel = &buttons[pCurrentBase->m_iControlDataIdx].panel;
							UIControlRenderData_t* pRenderable = &renderables[pCurrentBase->m_iRenderableIdx];
							m_RenderContext.m_pShaderSystem->SetShaderUniform(pRenderable->hShader,
								m_RenderContext.m_pShaderSystem->GetShaderUniformLocation(pRenderable->hShader, "color"), 0, pRenderable->iShaderInstance, &pPanel->color);
						}
					}
					break;

				case CONTROL_TYPE_LABEL:
					{
						UIControlLabel_t* pCurrentControl = &labels[pCurrentBase->m_iControlDataIdx];

						// Determine if the mouse is hovering over the text area.
						// FIXME: This doesn't work with large width/height.
						if ((xPos >= pCurrentControl->x && xPos <= pCurrentControl->x + pCurrentControl->width) &&
							(yPos >= pCurrentControl->y && yPos <= pCurrentControl->y + pCurrentControl->height))
						{
							// If the left mouse button is pressed, we can select the text.
							if (g_pEngine->GetEngineInterfaces()->pInputSystem->IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && pCurrentBase->m_bInteractable)
							{
								// FIXME: Need a better way to set input focus.
								AcquireInputFocus();
								SetActiveControl(iControl, true);
							}
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

	void CAppUI::SetRenderPriorityControl(UIControlHandle_t hControl)
	{
		CAssert(hControl != kInvalidID_U32);
	}

	void CAppUI::AcquireInputFocus()
	{
		m_pInputSystem->PushInputMap(&m_InputHandler);
	}

	void CAppUI::LoseInputFocus()
	{
		SetActiveControl(kInvalidID_U32, false);
		m_pInputSystem->PopInputMap();
	}

	void CAppUI::SetActiveControl(UIControlHandle_t hControl, bool bRegenerateControlLayout)
	{
		hActiveControl = hControl;

		if (bRegenerateControlLayout)
		{
			RegenerateRenderList();
		}
	}

	static void GetChildControls_r(Utl::CVector<UIBaseControl_t>* pBaseControls, Utl::CVector<UIControlHandle_t>* pOutChildren, UIControlHandle_t hParent)
	{
		CAssert(pBaseControls && pOutChildren && (hParent != kInvalidID_U32));

		UIBaseControl_t* pBaseParent = &pBaseControls->Data()[hParent];

		// Base case: no child controls.
		if (pBaseParent->children.Count() == 0)
		{
			return;
		}

		// Add the controls first, then move to the children.
		for (u64 iControl = 0; iControl < pBaseParent->children.Count(); iControl++)
		{
			pOutChildren->PushBack(pBaseParent->children[iControl]);
		}

		for (u64 iControl = 0; iControl < pBaseParent->children.Count(); iControl++)
		{
			GetChildControls_r(pBaseControls, pOutChildren, pBaseParent->children[iControl]);
		}
	}

	void CAppUI::RegenerateRenderList()
	{
		orderedRenderList.Clear();

		// Get a list of the selected control and any possible children.
		Utl::CVector<UIControlHandle_t> activeControls(32);
		
		// Include the parent as it is the selected control.
		activeControls.PushBack(hActiveControl);

		if (hActiveControl != kInvalidID_U32)
		{
			GetChildControls_r(&baseControls, &activeControls, hActiveControl);
		}

		// For now, add all of the controls except the focused ones in order.
		for (u64 iControl = 0; iControl < baseControls.Count(); iControl++)
		{
			bool bFocusedChild = false;
			for (u64 iFocusedControl = 0; iFocusedControl < activeControls.Count(); iFocusedControl++)
			{
				if (iControl == activeControls[iFocusedControl])
				{
					bFocusedChild = true;
					break;
				}
			}

			if (!bFocusedChild)
			{
				orderedRenderList.PushBack((u32)iControl);
			}
		}

		// Now place any focused controls and their childern last.
		if (hActiveControl != kInvalidID_U32)
		{
			for (u64 iControl = 0; iControl < activeControls.Count(); iControl++)
			{
				UIBaseControl_t* pBase = &baseControls[activeControls[iControl]];

				orderedRenderList.PushBack((u32)pBase->m_iRenderableIdx);

				// Also set the stencil reference value so this control renders on top of everything else.
				renderables[pBase->m_iRenderableIdx].iRenderPriority = kFocusedControlStencilReference;
			}
		}
	}

	UIControlHandle_t CAppUI::CreateUIControl(eControlType controlType)
	{
		UIControlHandle_t	hNewControl = GetAvailableBaseControl(this);
		UIBaseControl_t*	pNewBase = &baseControls[hNewControl];

		pNewBase->m_iRenderableIdx = GetAvailableRenderableIndex(this);
		UIControlRenderData_t* pRenderable = &renderables[pNewBase->m_iRenderableIdx];

		pNewBase->m_eControlType = controlType;

		switch (controlType)
		{
		case CONTROL_TYPE_PANEL:
			{
				panels.PushBack(UIControlPanel_t{});
				UIControlPanel_t* pPanelData = &panels.Back();
				pNewBase->m_iControlDataIdx = (u32)panels.Count() - 1;

				CreatePanel(pNewBase, pRenderable, pPanelData, &m_RenderContext);
				// These calls will generate the base control geometry.
				SetPanelControlPosition(pPanelData, &renderables[pNewBase->m_iRenderableIdx], &m_RenderContext, pPanelData->x, pPanelData->y, true);
				SetPanelControlDimensions(pPanelData, &renderables[pNewBase->m_iRenderableIdx], &m_RenderContext, pPanelData->width, pPanelData->height, true);
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
				pLabelData->hFont = hDefaultFont;

				pLabelData->x = 0;
				pLabelData->y = 0;
				pLabelData->width = 100;
				pLabelData->height = 100;

				CreateLabelControl(pNewBase, pRenderable, pLabelData, &m_RenderContext);
			}
			break;

		case CONTROL_TYPE_BUTTON:
			{
				buttons.PushBack(UIControlButton_t{});
				UIControlButton_t* pButtonData = &buttons.Back();
				pNewBase->m_iControlDataIdx = (u32)buttons.Count() - 1;

				CreateButton(pNewBase, pRenderable, pButtonData, &m_RenderContext);
				// These calls will generate the base control geometry.
				SetPanelControlPosition(&pButtonData->panel, &renderables[pNewBase->m_iRenderableIdx], &m_RenderContext, pButtonData->panel.x, pButtonData->panel.y, true);
				SetPanelControlDimensions(&pButtonData->panel, &renderables[pNewBase->m_iRenderableIdx], &m_RenderContext, pButtonData->panel.width, pButtonData->panel.height, true);

				// Buttons are interactable by default.
				pNewBase->m_bInteractable = true;
			}
			break;

		case CONTROL_TYPE_IMAGE_PANEL:
			{
				imagePanels.PushBack(UIControlImagePanel_t{});
				UIControlImagePanel_t* pImagePanelData = &imagePanels.Back();
				pNewBase->m_iControlDataIdx = (u32)imagePanels.Count() - 1;

				imageRenderData.PushBack(UIImageRenderData_t{});
				UIImageRenderData_t* pImageRenderData = &imageRenderData.Back();
				pRenderable->extendedRenderDataIdx = (u16)imageRenderData.Count() - 1;

				CreateImagePanel(pNewBase, pRenderable, pImageRenderData, pImagePanelData, &m_RenderContext);
				SetImagePanelImage(&renderables[pNewBase->m_iRenderableIdx], pImageRenderData, pImagePanelData, &m_RenderContext, 0, pImagePanelData->pCurGlyph);
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
		UIControlPanel_t* panel = &panels[pBase->m_iControlDataIdx];
		switch (pBase->m_eControlType)
		{
		case CONTROL_TYPE_PANEL:
			DestroyPanel(panel, &renderables[pBase->m_iRenderableIdx], &m_RenderContext);
			break;

		default:
			//CAssert(0);
			break;
		}
	}

	void CAppUI::SetControlFlag(UIControlHandle_t hControl, eUIControlFlagBits flag, bool bSet)
	{
		CAssert(hControl != kInvalidID_U32);
		UIBaseControl_t* pBase = &baseControls[hControl];

		if (bSet)
		{
			pBase->flags |= flag;
		}
		else
		{
			pBase->flags = (pBase->flags &= ~flag);
		}
	}

	void CAppUI::SetControlPosition(UIControlHandle_t hControl, u16 x, u16 y)
	{
		CAssert(hControl != kInvalidID_U32);
		UIBaseControl_t* pBase = &baseControls[hControl];
		UIControlPanel_t* panel = nullptr;

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
				panel = &panels[pParentBase->m_iControlDataIdx];
				break;
			case CONTROL_TYPE_FRAME:
				panel = frames[pParentBase->m_iControlDataIdx].panel;
				break;
			case CONTROL_TYPE_LABEL:
				// FIXME: Implement this correctly.
				break;
			case CONTROL_TYPE_BUTTON:
				panel = &buttons[pParentBase->m_iControlDataIdx].panel;
				break;
			default:
				CAssert(0);
				break;
			}

			// HACK until the above is fixed.
			if (pParentBase->m_eControlType != CONTROL_TYPE_LABEL)
			{
				parentedX += panel->x;
				parentedY += panel->y;
			}
		}

		switch (pBase->m_eControlType)
		{
		case CONTROL_TYPE_PANEL:
			panel = &panels[pBase->m_iControlDataIdx];
			SetPanelControlPosition(panel, &renderables[pBase->m_iRenderableIdx], &m_RenderContext, parentedX, parentedY, true);
			break;
		case CONTROL_TYPE_FRAME:
			panel = frames[pBase->m_iControlDataIdx].panel;
			SetPanelControlPosition(panel, &renderables[pBase->m_iRenderableIdx], &m_RenderContext, parentedX, parentedY, true);
			break;
		case CONTROL_TYPE_LABEL:
			{
				UIControlLabel_t* pLabelData = &labels[pBase->m_iControlDataIdx];
				SetLabelPosition(pLabelData, parentedX, parentedY);
				RegenerateLabelGeometry(pLabelData, &renderables[pBase->m_iRenderableIdx], &m_RenderContext);
			}
			break;
		case CONTROL_TYPE_BUTTON:
			{
				// Get our own panel.
				UIControlButton_t* pButtonData = &buttons[pBase->m_iControlDataIdx];
				panel = &pButtonData->panel;
				SetPanelControlPosition(panel, &renderables[pBase->m_iRenderableIdx], &m_RenderContext, parentedX, parentedY, true);
			}
			break;
		case CONTROL_TYPE_IMAGE_PANEL:
			{
				UIControlImagePanel_t* pImagePanelData = &imagePanels[pBase->m_iControlDataIdx];
				pImagePanelData->panel.x = parentedX;
				pImagePanelData->panel.y = parentedY;
				RegenerateImagePanelGeometry(&renderables[pBase->m_iRenderableIdx], pImagePanelData, &m_RenderContext);
			}
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
		UIControlPanel_t* panel = nullptr;
		switch (pBase->m_eControlType)
		{
		case CONTROL_TYPE_PANEL:
			panel = &panels[pBase->m_iControlDataIdx];
			SetPanelControlDimensions(panel, &renderables[pBase->m_iRenderableIdx], &m_RenderContext, width, height, true);
			break;
		case CONTROL_TYPE_FRAME:
			panel = frames[pBase->m_iControlDataIdx].panel;
			SetPanelControlDimensions(panel, &renderables[pBase->m_iRenderableIdx], &m_RenderContext, width, height, true);
			break;
		case CONTROL_TYPE_BUTTON:
			panel = &buttons[pBase->m_iControlDataIdx].panel;
			SetPanelControlDimensions(panel, &renderables[pBase->m_iRenderableIdx], &m_RenderContext, width, height, true);
			break;
		case CONTROL_TYPE_IMAGE_PANEL:
		{
			UIControlImagePanel_t* pImagePanelData = &imagePanels[pBase->m_iControlDataIdx];
			pImagePanelData->panel.width = width;
			pImagePanelData->panel.height = height;
			RegenerateImagePanelGeometry(&renderables[pBase->m_iRenderableIdx], pImagePanelData, &m_RenderContext);
		}
		break;

		default:
			CAssert(0);
			break;
		}
	}

	void CAppUI::SetControlVisible(UIControlHandle_t hControl, bool visible)
	{
		CAssert(hControl != kInvalidID_U32);
		UIBaseControl_t* pBase = &baseControls[hControl];
		renderables[pBase->m_iRenderableIdx].visible = visible;
	}

	void CAppUI::SetControlParent(UIControlHandle_t hChild, UIControlHandle_t hParent)
	{
		CAssert(hChild != kInvalidID_U32 && hParent != kInvalidID_U32);
		UIBaseControl_t* pChildBase = &baseControls[hChild];
		pChildBase->m_hParentControl = hParent;

		// Go to the parent and assign the child.
		UIBaseControl_t* pParentBase = &baseControls[hParent];
		pParentBase->children.PushBack(hChild);
	}

	void CAppUI::SetControlColor(UIControlHandle_t hControl, u8 r, u8 g, u8 b, u8 a)
	{
		CAssert(hControl != kInvalidID_U32);
		UIBaseControl_t* pBase = &baseControls[hControl];
		UIControlPanel_t* panel = nullptr;
		UIControlRenderData_t* pRenderable = &renderables[pBase->m_iRenderableIdx];

		switch (pBase->m_eControlType)
		{
		case CONTROL_TYPE_PANEL:
			panel = &panels[pBase->m_iControlDataIdx];
			break;
		case CONTROL_TYPE_BUTTON:
			panel = &buttons[pBase->m_iControlDataIdx].panel;
			break;
		}

		// FIXME: Set label color?

		CAssert(panel);

		// Convert the color range from [0-256] to [0-1].
		float flR = (float)(r / 256.0f);
		float flG = (float)(g / 256.0f);
		float flB = (float)(b / 256.0f);
		float flA = (float)(a / 256.0f);
		
		panel->color.x = flR;
		panel->color.y = flG;
		panel->color.z = flB;
		panel->color.w = flA;
		m_RenderContext.m_pShaderSystem->SetShaderUniform(pRenderable->hShader, 
			m_RenderContext.m_pShaderSystem->GetShaderUniformLocation(pRenderable->hShader, "color"), 0, pRenderable->iShaderInstance, &panel->color);
	}

	char* CAppUI::GetControlText(UIControlHandle_t hControl)
	{
		UIBaseControl_t* pBase = &baseControls[hControl];

		switch (pBase->m_eControlType)
		{
		case CONTROL_TYPE_LABEL:
			return labels[pBase->m_iControlDataIdx].pszText;
		default:
			const char* typeStr = GetStringForControlType(pBase->m_eControlType);
			LOG_ERROR("GetControlText() cannot get text for control type '%s'!", typeStr);
			return nullptr;
		}
	}

	void CAppUI::SetControlText(UIControlHandle_t hControl, const char* pszText)
	{
		CAssert(hControl != kInvalidID_U32 && pszText);
		UIBaseControl_t* pBase = &baseControls[hControl];
		CASSERTMSG(pBase->m_eControlType == CONTROL_TYPE_LABEL, "SetControlText - This function must only be called for UI elements that support text!");

		UIControlLabel_t* pLabel = &labels[pBase->m_iControlDataIdx];
		UIControlRenderData_t* pRenderable = &renderables[pBase->m_iRenderableIdx];

		SetLabelText(pLabel, pRenderable, &m_RenderContext, pszText);
	}

	void CAppUI::SetControlFont(UIControlHandle_t hControl, const char* pszFontName, f32 flFontSize)
	{
		CAssert(hControl != kInvalidID_U32 && pszFontName);
		UIBaseControl_t* pBase = &baseControls[hControl];
		CASSERTMSG(pBase->m_eControlType == CONTROL_TYPE_LABEL, "SetControlFont - This function must only be called for UI elements that support text!");

		UIControlLabel_t* pLabel = &labels[pBase->m_iControlDataIdx];
		SetLabelFont(pLabel, &renderables[pBase->m_iRenderableIdx], &m_RenderContext, pszFontName, flFontSize);
	}

	void CAppUI::SetControlImage(UIControlHandle_t hControl, TextureHandle_t hImage, FontLoader::GlyphLayoutData_t* pGlyphData)
	{
		CAssert(hControl != kInvalidID_U32);
		UIBaseControl_t* pBase = &baseControls[hControl];
		CASSERTMSG(pBase->m_eControlType == CONTROL_TYPE_IMAGE_PANEL, "SetControlImage - This function must only be called for UI elements that support images!");

		UIControlImagePanel_t* pImagePanel = &imagePanels[pBase->m_iControlDataIdx];
		UIImageRenderData_t* pImageExtendedData = &imageRenderData[renderables[pBase->m_iRenderableIdx].extendedRenderDataIdx];

		SetImagePanelImage(&renderables[pBase->m_iRenderableIdx], pImageExtendedData, pImagePanel, &m_RenderContext, hImage, pGlyphData);
	}

	void CAppUI::SetControlPressedCallback(UIControlHandle_t hControl, PFN_UI_Button_Control_Clicked callback, void* pListener)
	{
		CAssert(hControl != kInvalidID_U32 && callback);
		UIBaseControl_t* pBase = &baseControls[hControl];
		CASSERTMSG(pBase->m_eControlType == CONTROL_TYPE_BUTTON, "SetControlPressedCallback - This function must only be called for UI elements that can be pressed!");

		buttons[pBase->m_iControlDataIdx].onPressedCallback = callback;
		buttons[pBase->m_iControlDataIdx].callbackListenerInst = pListener;
	}

	void CAppUI::SetControlInteractable(UIControlHandle_t hControl, bool bInteractable)
	{
		CAssert(hControl != kInvalidID_U32);
		UIBaseControl_t* pBase = &baseControls[hControl];
		pBase->m_bInteractable = bInteractable;
	}

	void CAppUI::RegenerateControlGeometry(u32 newWidth, u32 newHeight)
	{
		u32 oldSurfaceWidth = m_RenderContext.currentSurfaceWidth;
		u32 oldSurfaceHeight = m_RenderContext.currentSurfaceHeight;

		for (u32 iControl = 0; iControl < baseControls.Count(); ++iControl)
		{
			UIBaseControl_t* pCurrentBase = &baseControls[iControl];
			if (pCurrentBase->m_eControlType != CONTROL_TYPE_UNDEFINED)
			{
				if (pCurrentBase->m_eControlType != CONTROL_TYPE_LABEL)
				{
					UIControlPanel_t* pCurrentPanel = nullptr;
					switch (pCurrentBase->m_eControlType)
					{
					case CONTROL_TYPE_PANEL:
						pCurrentPanel = &panels[pCurrentBase->m_iControlDataIdx];
						break;
					case CONTROL_TYPE_BUTTON:
						pCurrentPanel = &buttons[pCurrentBase->m_iControlDataIdx].panel;
						break;
					case CONTROL_TYPE_IMAGE_PANEL:
						pCurrentPanel = &imagePanels[pCurrentBase->m_iControlDataIdx].panel;
						return;	// TODO: Handle resizing separately here since this geometry has different requirements.
						break;
					default:
						CAssert(0);
						break;
					}
					CAssert(pCurrentPanel);

					// These will regenerate control geometry. FIXME: Just regenerate without setting fields?
					SetPanelControlPosition(pCurrentPanel, &renderables[pCurrentBase->m_iRenderableIdx], &m_RenderContext, pCurrentPanel->x, pCurrentPanel->y, true);
					SetPanelControlDimensions(pCurrentPanel, &renderables[pCurrentBase->m_iRenderableIdx], &m_RenderContext, pCurrentPanel->width, pCurrentPanel->height, true);
				}
				else
				{
					// Label case.
					UIControlLabel_t* pLabel = &labels[pCurrentBase->m_iControlDataIdx];
					UIControlRenderData_t* pRenderable = &renderables[pCurrentBase->m_iRenderableIdx];

					RegenerateLabelGeometry(pLabel, pRenderable, &m_RenderContext);
				}
			}
		}

		// Lastly, store off these new values.
		m_RenderContext.currentSurfaceWidth = newWidth;
		m_RenderContext.currentSurfaceHeight = newHeight;
	}

	static UIControlHandle_t GetAvailableBaseControl(CAppUI* pUIContext)
	{
		for (u32 iBase = 0; iBase < pUIContext->baseControls.Capacity(); iBase++)
		{
			if (pUIContext->baseControls[iBase].m_eControlType == CONTROL_TYPE_UNDEFINED)
			{
				// Found an empty slot!
				pUIContext->baseControls.PushBack(UIBaseControl_t{});
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

	static const char* GetStringForControlType(u16 controlType)
	{
		switch (controlType)
		{
		default:
		case CONTROL_TYPE_PANEL:
			return "Panel";
			break;
		case CONTROL_TYPE_FRAME:
			return "Frame";
			break;
		case CONTROL_TYPE_LABEL:
			return "Label";
			break;
		case CONTROL_TYPE_BUTTON:
			return "Button";
			break;
		case CONTROL_TYPE_IMAGE_PANEL:
			return "Image Panel";
			break;
		case CONTROL_TYPE_UNDEFINED:
			CAssert(0);
			break;
		}

		return nullptr;
	}

	static bool Input_KeyEvent_Handler(void* pListener, u16 keyCode, bool pressed, keyMessageFlagBits flags)
	{
		CAppUI* pUIContext = (CAppUI*)pListener;
		char c = (char)keyCode;

		if (c == KEY_ESCAPE || c == KEY_TILDE)
		{
			// Let this fall through to the engine.
			return false;
		}

		if (pUIContext->hActiveControl == kInvalidID_U32)
		{
			// Let this fall through.
			return false;
		}

		UIBaseControl_t* pActiveBase = &pUIContext->baseControls[pUIContext->hActiveControl];

		if (pActiveBase->m_eControlType == CONTROL_TYPE_LABEL && pressed)
		{
			UIControlLabel_t* pLabelData = &pUIContext->labels[pActiveBase->m_iControlDataIdx];

			if (c == KEY_SHIFT)
			{
				return true;
			}

			// Get the current text in the control.
			char* pCurrentEntryText = pLabelData->pszText;
			u64 length = Utl::StringLength(pCurrentEntryText);

			if (length < 1)
			{
				return true;
			}

			if (c == KEY_ENTER)
			{
				// Pass this down the chain. Console will use it to execute a command.
				// FIXME: Labels currently don't support typing newline codes.
				return false;
			}
			else if (c == KEY_UP || c == KEY_DOWN)
			{
				// Dev console can use these.
				return false;
			}

			char szOutBuffer[10000]{};

			// Handle special cases
			if (c == KEY_BACKSPACE)
			{
				// Take the current text, and remove one character from it.
				if (length < 1)
				{
					return true;
				}

				Platform::MemoryCopy(szOutBuffer, pCurrentEntryText, length - 1);
				szOutBuffer[length] = 0;
			}
			else
			{
				// Default case.

				// Check shift key state.
				// FIXME: This translation should be handled by the input system.
				bool bShiftPressed = g_pEngine->GetEngineInterfaces()->pInputSystem->IsKeyPressed(KEY_SHIFT);

				if (!bShiftPressed)
				{
					if (c >= KEY_A && c <= KEY_Z)
					{
						// Lowercase letters if necessary.
						c += ('a' - 'A');
					}
				}
				else
				{
					if (c >= KEY_0 && c <= KEY_9)
					{
						// Numbers/symbol case.
						// Map the key to the correct symbol.
						switch (c)
						{
						case KEY_0:
							c = 41;
							break;
						case KEY_1:
							c = 33;
							break;
						case KEY_2:
							c = 64;
							break;
						case KEY_3:
							c = 35;
							break;
						case KEY_4:
							c = 36;
							break;
						case KEY_5:
							c = 37;
							break;
						case KEY_6:
							c = 94;
							break;
						case KEY_7:
							c = 38;
							break;
						case KEY_8:
							c = 42;
							break;
						case KEY_9:
							c = 40;
							break;
						default:
							CAssert(0);
						}
					}
					else if (c == KEY_HYPHEN)
					{
						c = 95;	// to underscore
					}
				}

				if (pActiveBase->flags & UI_CONTROL_FLAG_TEXT_AUTO_CLEAR)
				{
					// Only use the first character and clear the rest.
					szOutBuffer[0] = c;
					szOutBuffer[1] = '\0';
					
					pUIContext->SetControlFlag(pUIContext->hActiveControl, UI_CONTROL_FLAG_TEXT_AUTO_CLEAR, false);
				}
				else
				{
					// Append the new char to it.
					Utl::StringFormat_V("%s%c", szOutBuffer, pCurrentEntryText, c);
				}
			}

			// Apply to the control.
			pUIContext->SetControlText(pUIContext->hActiveControl, szOutBuffer);
		}
		else
		{
			// These cases not yet supported.
			CAssert(pActiveBase->m_eControlType == CONTROL_TYPE_LABEL);
		}

		// Don't let other listeners have this event.
		return true;
	}

	// FIXME: Move all mouse handling to this handler.
	static bool Input_MouseButtonEvent_Handler(void* pListener, u16 mouseButtonFlags)
	{
		CAppUI* pUIContext = (CAppUI*)pListener;

		// Stop the event if the cursor is over a control.
		for (u32 iControl = 0; iControl < pUIContext->baseControls.Count(); iControl++)
		{
			UIBaseControl_t* pCurrentBase = &pUIContext->baseControls[iControl];

			if (pCurrentBase->m_eControlType != CONTROL_TYPE_UNDEFINED)
			{
				i32 xPos{ 0 }, yPos{ 0 };
				g_pEngine->GetEngineInterfaces()->pInputSystem->GetCurrentMousePos(xPos, yPos);

				switch (pCurrentBase->m_eControlType)
				{
				case CONTROL_TYPE_PANEL:
				{
					UIControlPanel_t* pCurrentPanel = &pUIContext->panels[pCurrentBase->m_iControlDataIdx];

					if ((xPos >= pCurrentPanel->x && xPos <= pCurrentPanel->x + pCurrentPanel->width) &&
						(yPos >= pCurrentPanel->y && yPos <= pCurrentPanel->y + pCurrentPanel->height))
					{
						// Swallow the event.
						return true;
					}
				}
				break;

				case CONTROL_TYPE_BUTTON:
				{
					UIControlButton_t* pCurrentControl = &pUIContext->buttons[pCurrentBase->m_iControlDataIdx];

					// Is the mouse hovering over the button?
					if ((xPos >= pCurrentControl->panel.x && xPos <= pCurrentControl->panel.x + pCurrentControl->panel.width) &&
						(yPos >= pCurrentControl->panel.y && yPos <= pCurrentControl->panel.y + pCurrentControl->panel.height))
					{
						// Swallow the event.
						return true;
					}
				}
				break;

				case CONTROL_TYPE_LABEL:
				{
					UIControlLabel_t* pCurrentControl = &pUIContext->labels[pCurrentBase->m_iControlDataIdx];

					// Determine if the mouse is hovering over the text area.
					// FIXME: This doesn't work with large width/height.
					if ((xPos >= pCurrentControl->x && xPos <= pCurrentControl->x + pCurrentControl->width) &&
						(yPos >= pCurrentControl->y && yPos <= pCurrentControl->y + pCurrentControl->height))
					{
						return true;
					}
				}
				break;

				default:
					// Just keep going.
					continue;
				}
			}
		}

		// Let other listeners get this.
		return false;
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