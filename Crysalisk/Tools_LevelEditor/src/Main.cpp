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
#include <Core/Event.h>
#include <Renderer/Renderer.h>
#include <Resources/Map.h>
#include <Systems/DebugGeometrySystem.h>
#include <Systems/MeshSystem.h>
#include <Systems/ShaderSystem.h>

#include "Gizmo.h"
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
	friend static bool Event_OnMouseMove(void* pListener, EventData_t eventData);

	friend static void ConsoleCommand_AddModel(struct ConVar_t* pCommand, void* pValue);
};

static CLevelEditorApp s_LevelEditorApp{};

// Event handlers.
static bool Event_OnMouseMove(void* pListener, EventData_t eventData);

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
		m_GridGeo.matLocalToWorldTransform = Matrix4x4::Matrix4x4(1.0f);
	}

	// Create the gizmo.
	if (!Gizmo_Create(pRenderer, &m_Gizmo))
	{
		LOG_ERROR("Failed to create editor gizmo!");
		return false;
	}
	
	// Register for events.
	IEventSystem* pEventSystem = pAppConfig->pEngine->GetEngineInterfaces()->pEngineEventHandler;
	pEventSystem->EventRegister(eEventCategory::INPUT_MOUSE_MOVED, this, Event_OnMouseMove);

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
	LOG_INFO("Shutting down level editor.");

	IRenderer* pRenderer = g_pEngine->GetEngineInterfaces()->pRenderer;

	// Gizmo
	Gizmo_Destroy(pRenderer, &m_Gizmo);
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

		// Update the gizmo.
		Gizmo_Update(&m_Gizmo);
	}
	return true;
}

void CLevelEditorApp::FramePrepare(struct RenderPacket_t* pRenderPacket)
{
	if (!pRenderPacket->pEditorViewExtendedData)
	{
		pRenderPacket->pEditorViewExtendedData = new EditorView_ExtendedData_t;
		EditorView_ExtendedData_t* pViewData = (EditorView_ExtendedData_t*)pRenderPacket->pEditorViewExtendedData;
		pViewData->editorGeo.SetAllocator(pRenderPacket->pFrameAllocator);
		pViewData->mapMeshes.SetAllocator(pRenderPacket->pFrameAllocator);
	}
	else
	{
		EditorView_ExtendedData_t* pViewData = (EditorView_ExtendedData_t*)pRenderPacket->pEditorViewExtendedData;

		pViewData->editorGeo.Resize(12);
		pViewData->editorGeo.PushBack(&m_GridGeo);

		if (m_CurrentDoc.meshes.Count() > 0)
		{
			IMeshSystem* pMeshSystem = g_pEngine->GetEngineInterfaces()->pMeshSystem;
			Mesh* pMesh = pMeshSystem->AcquireMeshDataByHandle(m_CurrentDoc.meshes[0]);
			if (pMesh->m_Geometries.Count() > 0)
			{
				// Just render the first one for now.
				GeometryInstance_t& geo = pMesh->m_Geometries[0];
				geo.matLocalToWorldTransform = pMesh->m_matLocalToWorldTransform;
				pViewData->mapMeshes.PushBack(&geo);

				IDebugGeometrySystem* pDebugGeo = g_pEngine->GetEngineInterfaces()->pDebugGeometry;

				if (m_hSelectedGeometry == kInvalidID_U32)
				{
					Mesh* pMesh = pMeshSystem->AcquireMeshDataByHandle(m_CurrentDoc.meshes[0]);
					m_hSelectedGeometry = pDebugGeo->AddBBox(pMesh->m_Geometries[0], Vector4D{ 1.0f, 1.0f, 0.0f, 1.0f }, -1.0f);
				}
			}
		}

		if (m_Gizmo.geometry.vertexCount && m_hSelectedGeometry != kInvalidID_U32)
		{
			pViewData->gizmoGeo = &m_Gizmo.geometry;
		}
	}
}

void CLevelEditorApp::Render(struct RenderPacket_t* pRenderPacket)
{
	// TODO: Get rid of this and move all rendering to views?
}

static bool Event_OnMouseMove(void* pListener, EventData_t eventData)
{
	CLevelEditorApp* pEditorAppState = (CLevelEditorApp*)pListener;

	// Early out if no geometry is selected.
	if (pEditorAppState->m_SelectedMeshIdx == kInvalidID_U32)
	{
		return true;
	}

	Camera_t* pMainCamera = g_pEngine->GetApplication()->GetMainCamera();
	u32 iWindowWidth{ 0 }, iWindowHeight{ 0 };
	Vector4D vecPosClip{};

	// Cast a ray and see if we intersect with the gizmo.
	i32 iMouseX = eventData.i32[0];
	i32 iMouseY = eventData.i32[1];

	// First, get the mouse coordinates in viewport-space.
	g_pEngine->GetEngineWindow()->GetWindowSize(iWindowWidth, iWindowHeight);

	// Next, bring our coordinates to NDC.
	vecPosClip.x = (2.0f * iMouseX) / iWindowWidth - 1.0f;
	vecPosClip.y = 1.0f - (2.0f * iMouseY) / iWindowHeight;
	vecPosClip.z = -1.0f;
	vecPosClip.w = 1.0f;

	// Clip space -> Camera space:
	Vector4D vecPosCamera{};
	// Get the inverse projection from our window's viewport.
	Matrix4x4 matInverseProj = g_pEngine->GetEngineWindow()->GetViewport().matProjection.Inverse();
	vecPosCamera = matInverseProj * vecPosClip;
	vecPosCamera.z = -1.0f;
	vecPosCamera.w = 0.0f;

	// Camera space -> World space:
	Vector vecRayDir = (pMainCamera->GetViewMatrix().Inverse() * vecPosCamera).AsVector3D();
	vecRayDir.NormalizeInPlace();

	Ray_t ray{};
	ray.vecOrigin = pMainCamera->vecPosition;
	ray.vecDirection = vecRayDir;

	// Now that we have a ray, check it against the gizmo's extents.
	ray.bDidHit = false;
	ray.flDistance = kFLT_MAX;

	Vector4D vecLocalDirection = ray.vecDirection.ToVector4D();
	vecLocalDirection.w = 0.0f;

	u8 prevHitAxis = pEditorAppState->m_Gizmo.selectedAxis;

	// Check if the ray intersects the gizmo.
	if (Gizmo_Raycast(&pEditorAppState->m_Gizmo, &ray, &pEditorAppState->m_Gizmo.selectedAxis))
	{
		//LOG_WARN("HIT AXIS: %c!", (char)'X' + pEditorAppState->m_Gizmo.selectedAxis);
	}

	// Mark the gizmo as needing it's geometry to be updated if the axis has changed.
	if (prevHitAxis != pEditorAppState->m_Gizmo.selectedAxis)
	{
		pEditorAppState->m_Gizmo.geometryDirty = true;
	}

	return true;
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

	// FIXME: If the mesh loading fails (e.g. the mesh doesn't exist), then this will take up a slot in the map mesh array.
	MeshHandle_t hNewMesh = pMeshSystem->AcquireMeshInstance((const char*)pValue, Vector{ 0.0f, 0.0f, 0.0f }, Vector{ 0.0f, 0.0f, 0.0f }, Vector{ 100.0f, 100.0f, 100.0f }, true);
	if (hNewMesh != kInvalidID_U32)
	{
		pApp->m_CurrentDoc.meshes.PushBack(hNewMesh);
	}
	else
	{
		return;
	}

	// Set as the selected mesh.
	pApp->m_SelectedMeshIdx = (u32)(pApp->m_CurrentDoc.meshes.Count() - 1);

	// Move the gizmo to this model.
	pApp->m_Gizmo.origin = Vector{ 0.0f, 0.0f, 0.0f };
}

IToolApp* GetToolApplicationState()
{
	return &s_LevelEditorApp;
}