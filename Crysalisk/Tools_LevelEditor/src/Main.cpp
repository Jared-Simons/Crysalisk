//=============================================================================
// Author: Jared Simons
// Date Created: 12/17/2024
// 
// Purpose: The entrypoint to the level-editor.
//=============================================================================

// Core
#include <Containers/CVector.h>
#include <Core/ConVar.h>
#include <Core/Logging.h>

// EditorHub
#include <ToolsTypes.h>

// Engine
#include <Application.h>
#include <Core/Engine.h>
#include <Renderer/Views/3D_Scene_View.h>
#include <Systems/DebugGeometrySystem.h>
#include <Systems/MeshSystem.h>

#include "LevelEditorTypes.h"

class CLevelEditorApp : public IToolApp
{
public:
	virtual bool	Tool_Init(struct ApplicationConfig_t* pAppConfig) override;
	virtual bool	Tool_Update() override;
	virtual void	Tool_Shutdown() override;

	virtual void	FramePrepare(struct RenderPacket_t* pRenderPacket) override;
	virtual void	Render(struct RenderPacket_t* pRenderPacket) override;

private:
	Utl::CVector<i32>	m_GridLines{};
	i32					m_OriginCross[3]{};

	// Current map data.
	MapEditorDocument_t	m_CurrentDoc{};
	u32					m_SelectedMeshIdx{ kInvalidID_U32 };
	u32					m_hSelectedGeometry{ kInvalidID_U32 };

	// These need access to our members.
	friend static void ConsoleCommand_AddModel(struct ConVar_t* pCommand, void* pValue);
};

static CLevelEditorApp s_LevelEditorApp{};

// Console Command callbacks.
static void ConsoleCommand_AddModel(struct ConVar_t* pCommand, void* pValue);

extern "C"
{
	__declspec(dllexport) IToolApp* GetToolApplicationState();
}

bool CLevelEditorApp::Tool_Init(struct ApplicationConfig_t* pAppConfig)
{
	LOG_INFO("Welcome to Crysalisk Level Editor!");

	// Setup world grid.
	// FIXME: Lines should be optionally depth-tested.
	// We want these grids to respect the depth buffer.
	IDebugGeometrySystem* pDebugGeo = pAppConfig->pEngine->GetEngineInterfaces()->pDebugGeometry;
	m_GridLines.Resize(50);
	{
		// X-axis.
		f32 zAxis = 0.0f;
		while (zAxis <= 1024.0f)
		{
			i32 newLine = pDebugGeo->AddLine(Vector{ -1024.0f, 0.0f, zAxis }, Vector{ 1024.0f, 0.0f, zAxis }, Vector4D{ 1.0f, 1.0f, 1.0f, 1.0f }, -1.0f);
			m_GridLines.PushBack(newLine);
			zAxis += 100.0f;
		}

		zAxis = -100.0f;	
		while (zAxis >= -1024.0f)
		{
			i32 newLine = pDebugGeo->AddLine(Vector{ -1024.0f, 0.0f, zAxis }, Vector{ 1024.0f, 0.0f, zAxis }, Vector4D{ 1.0f, 1.0f, 1.0f, 1.0f }, -1.0f);
			m_GridLines.PushBack(newLine);
			zAxis -= 100.0f;
		}

		// Z-axis.
		f32 xAxis = 0.0f;
		while (xAxis <= 1024.0f)
		{
			i32 newLine = pDebugGeo->AddLine(Vector{ xAxis, 0.0f, -1024.0f }, Vector{ xAxis, 0.0f, 1024.0f }, Vector4D{ 1.0f, 1.0f, 1.0f, 1.0f }, -1.0f);
			m_GridLines.PushBack(newLine);
			xAxis += 100.0f;
		}

		xAxis = -100.0f;
		while (xAxis >= -1024.0f)
		{
			i32 newLine = pDebugGeo->AddLine(Vector{ xAxis, 0.0f, -1024.0f }, Vector{ xAxis, 0.0f, 1024.0f }, Vector4D{ 1.0f, 1.0f, 1.0f, 1.0f }, -1.0f);
			m_GridLines.PushBack(newLine);
			xAxis -= 100.0f;
		}
	}

	// Create a cross at the world origin.
	{
		f32 flRadius = 25.0f;
		m_OriginCross[0] = pDebugGeo->AddLine(Vector{ 0.0f, 0.0f, 0.0f }, Vector{ flRadius, 0.0f, 0.0f }, Vector4D{ 1.0f, 0.0f, 0.0f, 1.0f }, -1.0f);
		m_OriginCross[1] = pDebugGeo->AddLine(Vector{ 0.0f, 0.0f, 0.0f }, Vector{ 0.0f, flRadius , 0.0f }, Vector4D{ 0.0f, 1.0f, 0.0f, 1.0f }, -1.0f);
		m_OriginCross[2] = pDebugGeo->AddLine(Vector{ 0.0f, 0.0f, 0.0f }, Vector{ 0.0f, 0.0f, flRadius }, Vector4D{ 0.0f, 0.0f, 1.0f, 1.0f }, -1.0f);
	}

	// LEFTOFF: Gizmo and per-vertex color.

	// Editor console commands.
	ConVarSystemState* pConVar = pAppConfig->pEngine->GetEngineInterfaces()->pConVar;
	ConsoleCommandRegister(pConVar, "add_model", CONVAR_VALUETYPE_PASSTHROUGH, ConsoleCommand_AddModel, this);

	// Default to fullbright for the editor.
	ConsoleCommandExecute(pConVar, "mat_debugvismode 1");

	return true;
}

bool CLevelEditorApp::Tool_Update()
{
	//LOG_VERBOSE("Running Level Editor Loop!");
	return true;
}

void CLevelEditorApp::Tool_Shutdown()
{
}

void CLevelEditorApp::FramePrepare(struct RenderPacket_t* pRenderPacket)
{
	if (m_CurrentDoc.meshes.Count() > 0)
	{
		MainSceneView_ExtendedData_t* pViewData = (MainSceneView_ExtendedData_t*)pRenderPacket->pRenderViewExtendedData[RENDER_VIEW_TYPE_3D_SCENE];
		IMeshSystem* pMeshSystem = g_pEngine->GetEngineInterfaces()->pMeshSystem;
		Mesh* pMesh = pMeshSystem->AcquireMeshDataByHandle(m_CurrentDoc.meshes[0]);
		if (pMesh->m_Geometries.Count() > 0)
		{
			GeometryInstance_t& geo = pMesh->m_Geometries[0];
			geo.matLocalToWorldTransform = pMesh->m_matLocalToWorldTransform;
			pViewData->pOpaqueGeometries.PushBack(&geo);

			IDebugGeometrySystem* pDebugGeo = g_pEngine->GetEngineInterfaces()->pDebugGeometry;

			if (m_hSelectedGeometry == kInvalidID_U32)
			{
				Mesh* pMesh = pMeshSystem->AcquireMeshDataByHandle(m_CurrentDoc.meshes[0]);
				m_hSelectedGeometry = pDebugGeo->AddBBox(pMesh->m_Geometries[0], Vector4D{ 1.0f, 1.0f, 0.0f, 1.0f }, -1.0f);
			}
		}
	}
}

void CLevelEditorApp::Render(struct RenderPacket_t* pRenderPacket)
{

}

static void ConsoleCommand_AddModel(struct ConVar_t* pCommand, void* pValue)
{
	LOG_DEBUG("Add model here (%s)", (char*)pValue);
	CLevelEditorApp* pApp = (CLevelEditorApp*)pCommand->listenerInst;
	IMeshSystem* pMeshSystem = g_pEngine->GetEngineInterfaces()->pMeshSystem;

	pApp->m_CurrentDoc.meshes.PushBack(kInvalidID_U32);
	pApp->m_CurrentDoc.meshes.Back() = pMeshSystem->AcquireMeshInstance((const char*)pValue, Vector{0.0f, 0.0f, 0.0f}, Vector{0.0f, 0.0f, 0.0f}, Vector{100.0f, 100.0f, 100.0f}, true);

	// Set as the selected mesh.
	pApp->m_SelectedMeshIdx = pApp->m_CurrentDoc.meshes.Count() - 1;
}

IToolApp* GetToolApplicationState()
{
	return &s_LevelEditorApp;
}
