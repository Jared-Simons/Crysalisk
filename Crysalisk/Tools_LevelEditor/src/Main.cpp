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
#include <Renderer/Editor_View.h>
#include <ToolsTypes.h>

// Engine
#include <Application.h>
#include <Core/Engine.h>
#include <Renderer/Renderer.h>
#include <Renderer/Views/3D_Scene_View.h>
#include <Systems/DebugGeometrySystem.h>
#include <Systems/MeshSystem.h>
#include <Systems/ShaderSystem.h>

#include "LevelEditorTypes.h"

class CLevelEditorApp : public IToolApp
{
public:
	virtual bool	Tool_Init(struct ApplicationConfig_t* pAppConfig) override;
	virtual bool	Tool_Update() override;
	virtual void	Tool_Shutdown() override;

	virtual void	FramePrepare(struct RenderPacket_t* pRenderPacket) override;
	virtual void	Render(struct RenderPacket_t* pRenderPacket) override;

	virtual void	SetCamera(struct Camera_t* pCamera) override { m_pCamera = pCamera; }

private:
	Utl::CVector<i32>	m_GridLines{};
	GeometryInstance_t	m_GridGeo{};
	i32					m_OriginCross[3]{};
	Gizmo				m_Gizmo{};

	// Current map data.
	MapEditorDocument_t	m_CurrentDoc{};
	u32					m_SelectedMeshIdx{ kInvalidID_U32 };
	u32					m_hSelectedGeometry{ kInvalidID_U32 };
	Camera_t*			m_pCamera{ nullptr };

	// These need access to our members.
	friend static void ConsoleCommand_AddModel(struct ConVar_t* pCommand, void* pValue);
};

static CLevelEditorApp s_LevelEditorApp{};

// Console Command callbacks.
static void ConsoleCommand_AddModel(struct ConVar_t* pCommand, void* pValue);

LEVEL_EDITOR_API IToolApp* GetToolApplicationState();

bool CLevelEditorApp::Tool_Init(struct ApplicationConfig_t* pAppConfig)
{
	LOG_INFO("Welcome to Crysalisk Level Editor!");

	IRenderer* pRenderer = pAppConfig->pEngine->GetEngineInterfaces()->pRenderer;

	// Setup world grid.
	// FIXME: Lines should be optionally depth-tested.
	// We want these grids to respect the depth buffer.
	{
		IRenderer* pRenderer = pAppConfig->pEngine->GetEngineInterfaces()->pRenderer;
		IShaderSystem* pShaderSystem = pAppConfig->pEngine->GetEngineInterfaces()->pShaderSystem;
		ShaderHandle_t hDebugLineShader = pShaderSystem->AcquireShader("Debug.Line");

		RenderBuffer* pVertexBuffer = pRenderer->GetRenderBufferByName("Debug_Renderer_Line_Vertex_Buffer");
		RenderBuffer* pIndexBuffer = pRenderer->GetRenderBufferByName("Debug_Renderer_Line_Index_Buffer");

		u64 gridVertCount = 400;	// 2 verts per line, 100 lines per axis, 2 axes.
		Utl::CVector<f32> gridVerts(gridVertCount * 3);	// 3 floats per point.

		// X-axis.
 		f32 zPos = 0.0f;
		for (u32 iLine = 0; iLine < 50; iLine++)
		{
			Vector vecPos = { -2048.0f, 0.0f, zPos };
			gridVerts.PushBack(vecPos.x);
			gridVerts.PushBack(vecPos.y);
			gridVerts.PushBack(vecPos.z);

			vecPos = { 2048.0f, 0.0f, zPos };
			gridVerts.PushBack(vecPos.x);
			gridVerts.PushBack(vecPos.y);
			gridVerts.PushBack(vecPos.z);

			zPos += 64.0f;
		}

		// X-axis, back the other way.
		zPos = -64.0f;
		for (u32 iLine = 0; iLine < 50; iLine++)
		{
			Vector vecPos = { -2048.0f, 0.0f, zPos };
			gridVerts.PushBack(vecPos.x);
			gridVerts.PushBack(vecPos.y);
			gridVerts.PushBack(vecPos.z);

			vecPos = { 2048.0f, 0.0f, zPos };
			gridVerts.PushBack(vecPos.x);
			gridVerts.PushBack(vecPos.y);
			gridVerts.PushBack(vecPos.z);

			zPos -= 64.0f;
		}

		// Z-axis.
		f32 xPos = 0.0f;
		for (u32 iLine = 0; iLine < 50; iLine++)
		{
			Vector vecPos = { xPos, 0.0f, -2048.0f };
			gridVerts.PushBack(vecPos.x);
			gridVerts.PushBack(vecPos.y);
			gridVerts.PushBack(vecPos.z);

			vecPos = { xPos, 0.0f, 2048.0f };
			gridVerts.PushBack(vecPos.x);
			gridVerts.PushBack(vecPos.y);
			gridVerts.PushBack(vecPos.z);

			xPos += 64.0f;
		}

		// Z-axis, back the other way.
		xPos = -64.0f;
		for (u32 iLine = 0; iLine < 50; iLine++)
		{
			Vector vecPos = { xPos, 0.0f, -2048.0f };
			gridVerts.PushBack(vecPos.x);
			gridVerts.PushBack(vecPos.y);
			gridVerts.PushBack(vecPos.z);

			vecPos = { xPos, 0.0f, 2048.0f };
			gridVerts.PushBack(vecPos.x);
			gridVerts.PushBack(vecPos.y);
			gridVerts.PushBack(vecPos.z);

			xPos -= 64.0f;
		}

		pRenderer->AcquireGeometryResources(*pVertexBuffer, *pIndexBuffer, gridVertCount, sizeof(Vector), 0, gridVerts.Data(), 0, nullptr, &m_GridGeo);

		pShaderSystem->AcquireShaderInstanceResources(hDebugLineShader, &m_GridGeo.hShaderInstance, "Editor_Grid");
		pShaderSystem->SetShaderUniform(hDebugLineShader, pShaderSystem->GetShaderUniformLocation(hDebugLineShader, "color"), 0, m_GridGeo.hShaderInstance, &Vector4D{ 1.0f, 1.0f, 1.0f, 1.0f });
		m_GridGeo.hShader = hDebugLineShader;
	}
	/*IDebugGeometrySystem* pDebugGeo = pAppConfig->pEngine->GetEngineInterfaces()->pDebugGeometry;
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
	}*/

	// Create the gizmo.
	{
		Vector verts[6]{};
		verts[0] = { 0.0f, 0.0f, 0.0f };
		verts[1] = { 0.25f, 0.0f, 0.0f };
		verts[2] = { 0.0f, 0.0f, 0.0f };
		verts[3] = { 0.0f, 0.25f, 0.0f };
		verts[4] = { 0.0f, 0.0f, 0.0f };
		verts[5] = { 0.0f, 0.0f, 0.25f };

		Vector4D colors[6]{};
		colors[0] = { 1.0f, 0.0f, 0.0f, 1.0f };
		colors[1] = { 1.0f, 0.0f, 0.0f, 1.0f };
		colors[2] = { 0.0f, 1.0f, 0.0f, 1.0f };
		colors[3] = { 0.0f, 1.0f, 0.0f, 1.0f };
		colors[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
		colors[5] = { 0.0f, 0.0f, 1.0f, 1.0f };

		// Create vertex stream.
		u64 numFloats = (3 * 6) + (4 * 6);	// 3 floats per position vector * 6 verts + 4 floats per color * 6 verts.
		Utl::CVector<f32> vertexData(numFloats);

		for (u8 iVert = 0; iVert < 6; iVert++)
		{
			vertexData.PushBack(verts[iVert].x);
			vertexData.PushBack(verts[iVert].y);
			vertexData.PushBack(verts[iVert].z);
			vertexData.PushBack(colors[iVert].x);
			vertexData.PushBack(colors[iVert].y);
			vertexData.PushBack(colors[iVert].z);
			vertexData.PushBack(colors[iVert].w);
		}

		RenderBuffer* pVertexBuffer = pRenderer->GetRenderBufferByName("Debug_Renderer_Line_Vertex_Buffer");
		RenderBuffer* pIndexBuffer = pRenderer->GetRenderBufferByName("Debug_Renderer_Line_Index_Buffer");

		IShaderSystem* pShaderSystem = g_pEngine->GetEngineInterfaces()->pShaderSystem;

		pRenderer->AcquireGeometryResources(*pVertexBuffer, *pIndexBuffer, 6, sizeof(Vector) + sizeof(Vector4D), 0, vertexData.Data(), 0, nullptr, &m_Gizmo.geometry);

		// Acquire new shader instance resources.
		ShaderHandle_t hShader = pShaderSystem->AcquireShader("Debug.Line.VertColor");
		Vector4D color{ 0.3f, 0.0f, 0.3f, 1.0f };
		pShaderSystem->AcquireShaderInstanceResources(hShader, &m_Gizmo.geometry.hShaderInstance, "DebugLineInstance");
		pShaderSystem->SetShaderUniform(hShader, pShaderSystem->GetShaderUniformLocation(hShader, "color"), 0, m_Gizmo.geometry.hShaderInstance, &color);
		m_Gizmo.geometry.hShader = hShader;
	}

	// Editor console commands.
	ConVarSystemState* pConVar = pAppConfig->pEngine->GetEngineInterfaces()->pConVar;
	ConsoleCommandRegister(pConVar, "add_model", CONVAR_VALUETYPE_PASSTHROUGH, ConsoleCommand_AddModel, this);

	// Default to fullbright for the editor.
	ConsoleCommandExecute(pConVar, "mat_debugvismode 1");

	// Place the camera in a good initial position.
	pAppConfig->pApplication->GetMainCamera()->vecPosition = { 0.0f, 50.0f, 0.0f };

	return true;
}

void CLevelEditorApp::Tool_Shutdown()
{
}

bool CLevelEditorApp::Tool_Update()
{
	if (m_Gizmo.geometry.vertexCount)
	{
		Matrix4x4 matGizmoTransform{ 1.0f };

		// Scale the gizmo by its position from the camera.
		float gizmoToCameraDist = (m_pCamera->vecPosition - m_Gizmo.origin).Length();
		matGizmoTransform = Matrix4x4::Scale(matGizmoTransform, Vector{ gizmoToCameraDist, gizmoToCameraDist, gizmoToCameraDist });

		// Translation.
		matGizmoTransform = Matrix4x4::Translate(matGizmoTransform, m_Gizmo.origin);

		m_Gizmo.geometry.matLocalToWorldTransform = matGizmoTransform;
	}
	return true;
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

	if (!pRenderPacket->pEditorViewExtendedData)
	{
		pRenderPacket->pEditorViewExtendedData = new EditorViewExtendedData_t;
		EditorViewExtendedData_t* pViewData = (EditorViewExtendedData_t*)pRenderPacket->pEditorViewExtendedData;
		pViewData->geo.SetAllocator(pRenderPacket->pFrameAllocator);
	}
	else
	{
		EditorViewExtendedData_t* pViewData = (EditorViewExtendedData_t*)pRenderPacket->pEditorViewExtendedData;
		pViewData->geo.Resize(12);

		pViewData->geo.PushBack(&m_GridGeo);
	}

	if (m_Gizmo.geometry.vertexCount && m_hSelectedGeometry != kInvalidID_U32)
	{
		//pRenderPacket->pEditorViewExtendedData = &m_Gizmo.geometry;
	}
}

void CLevelEditorApp::Render(struct RenderPacket_t* pRenderPacket)
{
	// TODO: Get rid of this and move all rendering to views?
}

static void ConsoleCommand_AddModel(struct ConVar_t* pCommand, void* pValue)
{
	if (!pValue)
	{
		return;
	}

	LOG_DEBUG("Add model here (%s)", (char*)pValue);
	CLevelEditorApp* pApp = (CLevelEditorApp*)pCommand->listenerInst;
	IMeshSystem* pMeshSystem = g_pEngine->GetEngineInterfaces()->pMeshSystem;

	pApp->m_CurrentDoc.meshes.PushBack(kInvalidID_U32);
	pApp->m_CurrentDoc.meshes.Back() = pMeshSystem->AcquireMeshInstance((const char*)pValue, Vector{0.0f, 0.0f, 0.0f}, Vector{0.0f, 0.0f, 0.0f}, Vector{100.0f, 100.0f, 100.0f}, true);

	// Set as the selected mesh.
	pApp->m_SelectedMeshIdx = (u32)(pApp->m_CurrentDoc.meshes.Count() - 1);

	// Move the gizmo to this model.
	pApp->m_Gizmo.origin = Vector{ 0.0f, 0.0f, 0.0f };
}

IToolApp* GetToolApplicationState()
{
	return &s_LevelEditorApp;
}