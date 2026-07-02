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
		appUIPassConfig.bPresentsAfter = false;

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
		m_hUIShader = pEngineSystems->pShaderSystem->AcquireShader("App_UI");

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
			// Render hack!
			// FIXME: Make this generic/create render packet structures.
			if (pRenderData->pEditorExtendedData)
			{
				UIControlRenderData_t* pRenderable = (UIControlRenderData_t*)pRenderData->pEditorExtendedData;
				m_pShaderSystem->BindShader(pRenderable->hShader);
				m_pShaderSystem->SetShaderUniform(pRenderable->hShader, 0, 0, 1, &pRenderData->UIProjectionMatrix);
				m_pShaderSystem->BindPerInstanceUniforms(pRenderable->iShaderInstance);
				m_pRenderer->DrawGeometry(pRenderable->geo, m_pVertexBuffer, m_pIndexBuffer);
			}

			if (pRenderData->pEditorTextTest)
			{
				UIControlRenderData_t* pRenderable = (UIControlRenderData_t*)pRenderData->pEditorTextTest;
				m_pShaderSystem->BindShader(pRenderable->hShader);
				m_pShaderSystem->SetShaderUniform(pRenderable->hShader, 0, 0, 1, &pRenderData->UIProjectionMatrix);
				pFontSystem->BindFontInstance(pRenderable->iShaderInstance);
				m_pRenderer->DrawGeometry(pRenderable->geo, m_pVertexBuffer, m_pIndexBuffer);
			}

			if (pRenderData->pEditorTextTest2)
			{
				UIControlRenderData_t* pRenderable = (UIControlRenderData_t*)pRenderData->pEditorTextTest2;
				m_pShaderSystem->BindShader(pRenderable->hShader);
				m_pShaderSystem->SetShaderUniform(pRenderable->hShader, 0, 0, 1, &pRenderData->UIProjectionMatrix);
				pFontSystem->BindFontInstance(pRenderable->iShaderInstance);
				m_pRenderer->DrawGeometry(pRenderable->geo, m_pVertexBuffer, m_pIndexBuffer);
			}

			m_pRenderer->EndRenderpass();
		}
	}

	IRenderView* RenderViewCreate_AppUIView()
	{
		return new AppUI::CAppUI_RenderView;
	}

} // namespace AppUI
