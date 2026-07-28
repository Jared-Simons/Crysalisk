//=============================================================================
// Author: Jared Simons
// Date Created: 6/13/2026
// 
// Purpose: Render view for the AppUI plugin.
//=============================================================================
#include "AppUI_View.h"

#include "UI_Types.h"

// Engine
#include <Core/Engine.h>
#include <Renderer/Renderer.h>
#include <Systems/ShaderSystem.h>
#include <Systems/TextureSystem.h>

namespace AppUI
{
	class CAppUI_RenderView : public IRenderView
	{
	public:
		virtual bool	Initialize(const EngineInterfaces_t* pEngineSystems) override;
		virtual void	Shutdown(IRenderer* pRenderer) override;

		virtual void	BuildRenderPacket(RenderPacket_t* pRenderData) override;
		virtual void	Render(RenderPacket_t* pRenderData) override;

		virtual const char* GetViewName() const override { return "App UI View"; }
		virtual const Renderpass* GetRenderpass() const override { return &m_Renderpass; }

	private:
		IRenderer*		m_pRenderer{ nullptr };
		Renderpass		m_Renderpass{};
		RenderBuffer*	m_pVertexBuffer{ nullptr };
		RenderBuffer*	m_pIndexBuffer{ nullptr };

		IShaderSystem*	m_pShaderSystem{ nullptr };
		ShaderHandle_t	m_hUIShader{ kInvalidID_U32 };
		ShaderHandle_t	m_hTextShader{ kInvalidID_U32 };
		ShaderHandle_t	m_hImagePanelShader{ kInvalidID_U32 };
	};

	bool CAppUI_RenderView::Initialize(const EngineInterfaces_t* pEngineSystems)
	{
		// Store off subsystem pointers.
		m_pRenderer = pEngineSystems->pRenderer;
		m_pShaderSystem = pEngineSystems->pShaderSystem;

		// Setup the renderpass.
		RenderpassConfig_t appUIPassConfig{};
		appUIPassConfig.pszName = "App UI View";

		// This view renders to 2 attachments: color and depth.
		RendererViewAttachmentConfig_t colorAttachment{};
		colorAttachment.eAttachmentType = eRendererViewAttachmentType::COLOR;

		RendererViewAttachmentConfig_t depthAttachment{};
		depthAttachment.eAttachmentType = eRendererViewAttachmentType::DEPTH;

		appUIPassConfig.attachmentConfigs.PushBack(colorAttachment);
		appUIPassConfig.attachmentConfigs.PushBack(depthAttachment);
		appUIPassConfig.clearFlags = RENDERPASS_CLEAR_NONE;
		appUIPassConfig.bPresentsAfter = true;

		pEngineSystems->pRenderer->CreateRenderpass(&appUIPassConfig, &m_Renderpass);

		// Setup renderbuffers.
		m_pVertexBuffer = pEngineSystems->pRenderer->CreateRenderBuffer(MEBIBYTES(5) * sizeof(Vector2D), eRenderBufferUsage::VERTEX, true, "AppUI_Vertex_Buffer");
		if (m_pVertexBuffer == nullptr)
		{
			LOG_ERROR("AppUI View failed to initialize!");
			return false;
		}

		m_pIndexBuffer = pEngineSystems->pRenderer->CreateRenderBuffer(MEBIBYTES(5), eRenderBufferUsage::INDEX, true, "AppUI_Index_Buffer");
		if (m_pIndexBuffer == nullptr)
		{
			LOG_ERROR("AppUI View failed to initialize!");
			return false;
		}

		// Register with the renderer.
		pEngineSystems->pRenderer->RegisterRenderView(this);

		// Shaders
		// Cache these off for quick lookups later.
		m_hUIShader = m_pShaderSystem->AcquireShader("App_UI");
		m_hTextShader = m_pShaderSystem->AcquireShader("Core.UI");
		m_hImagePanelShader = m_pShaderSystem->AcquireShader("AppUI_ImagePanel");

		return true;
	}

	void CAppUI_RenderView::Shutdown(IRenderer* pRenderer)
	{
		pRenderer->DestroyRenderpass(&m_Renderpass);
	}

	void CAppUI_RenderView::BuildRenderPacket(RenderPacket_t* pRenderData)
	{
	}

	void CAppUI_RenderView::Render(RenderPacket_t* pRenderData)
	{
		IFontSystem* pFontSystem = g_pEngine->GetEngineInterfaces()->pFontSystem;
		if (m_pRenderer->BeginRenderpass(&m_Renderpass))
		{
			AppUIView_ExtendedData_t* pViewData = (AppUIView_ExtendedData_t*)pRenderData->pRenderViewExtendedData[RENDER_VIEW_TYPE_APPUI];

			for (u32 iRenderable = 0; iRenderable < pViewData->renderables.Count(); iRenderable++)
			{
				UIControlRenderData_t* pRenderable = pViewData->renderables[iRenderable];

				if (pRenderable->geo.vertexCount == kInvalidID_U64)
				{
					continue;
				}
				else if (!pRenderable->visible)
				{
					continue;
				}

				m_pShaderSystem->SetShaderUniform(pRenderable->hShader, 0, 0, pRenderable->iShaderInstance, &pViewData->projectionMatrix);
				m_pShaderSystem->BindShader(pRenderable->hShader);

				if (pRenderable->hShader == m_hUIShader)
				{
					// For default control geometry.
					m_pShaderSystem->BindPerInstanceUniforms(pRenderable->iShaderInstance);
				}
				else if (pRenderable->hShader == m_hTextShader)
				{
					// For text rendering.
					pFontSystem->BindFontInstance(pRenderable->iShaderInstance);
				}
				else if (pRenderable->hShader == m_hImagePanelShader)
				{
					// FIXME: Pass the image handle in from somewhere.
					static u8 swapDone = 0;
					if (g_pEngine->GetEngineInterfaces()->pTextureSystem->IsTextureLoaded(7) && swapDone < 2)
					{
						swapDone++;
						TextureHandle_t hImage = 7;
						m_pShaderSystem->SetShaderUniform(pRenderable->hShader, m_pShaderSystem->GetShaderUniformLocation(pRenderable->hShader, "image"), 0, pRenderable->iShaderInstance, &hImage);
					}

					// For rendering images.
					m_pShaderSystem->BindPerInstanceUniforms(pRenderable->iShaderInstance);
				}
				else
				{
					// Unknown shader.
					CAssert(0);
				}
				m_pRenderer->DrawGeometry(pRenderable->geo, pViewData->pVertexBuffer, pViewData->pIndexBuffer);
			}

			m_pRenderer->EndRenderpass();
		}
	}

	IRenderView* RenderViewCreate_AppUIView()
	{
		return new AppUI::CAppUI_RenderView;
	}

} // namespace AppUI
