//=============================================================================
// Author: Jared Simons
// Date Created: 12/17/2024
// 
// Purpose: The entrypoint to the level-editor.
//=============================================================================

// AppUI
#include <AppUI.h>

// Core
#include <Containers/CVector.h>
#include <Core/ConVar.h>
#include <Core/Logging.h>
#include <Math/Ray.h>
#include <Resources/Loaders/FontLoader.h>

// EditorHub
#include <Renderer/Editor_View.h>
#include <ToolsTypes.h>

// Engine
#include <Application.h>
#include <Core/Engine.h>
#include <Core/Event.h>
#include <Renderer/Renderer.h>
#include <Systems/DebugGeometrySystem.h>
#include <Systems/MeshSystem.h>
#include <Systems/ShaderSystem.h>
#include <Systems/TextureSystem.h>

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

	// Current tool mode.
	u8					m_iToolMode{ EDITOR_TOOL_MODE_NONE };
	// Editor hacks.
	bool				m_bToolChangeHack{ false };	// HACK: Pressing a tool change button will send events on the same mouse release. This can be a problem with, for example, clicking the 'add model' button and adding a model in the same click.
	char*				m_pszToolModelName{ nullptr };	// The name of the model to place.

	// UI system context.
	AppUI::IAppUI*		m_pUIContext{ nullptr };

	// Glyph data.
	Utl::CVector<FontLoader::GlyphLayoutData_t>	glyphData{};

	// Editor UI controls.
	// Main tools.
	AppUI::UIControlHandle_t	hMouseToolPanel{ kInvalidID_U32 };
	AppUI::UIControlHandle_t	hSelectionToolImage{ kInvalidID_U32 };
	AppUI::UIControlHandle_t	hAddModelImage{ kInvalidID_U32 };

	AppUI::UIControlHandle_t	hSelectionToolButton{ kInvalidID_U32 };
	AppUI::UIControlHandle_t	hAddModelButton{ kInvalidID_U32 };

	// These need access to our members.
	friend static bool Event_OnMouseButtonPressed(void* pListener, EventData_t eventData);
	friend static bool Event_OnMouseButtonReleased(void* pListener, EventData_t eventData);
	friend static bool Event_OnMouseMove(void* pListener, EventData_t eventData);
	friend static bool Event_OnMouseDragged(void* pListener, EventData_t eventData);

	friend static void ConsoleCommand_AddModel(struct ConVar_t* pCommand, void* pValue);
	friend static void ConsoleCommand_SetModelName(struct ConVar_t* pCommand, void* pValue);

	friend static void ButtonEvent_ChangeTool_Selection(void* pListenerInst);
	friend static void ButtonEvent_ChangeTool_AddModel(void* pListenerInst);

	friend static void Editor_Raycast(CLevelEditorApp* pLevelEditor, Ray_t* pRay);
};

// FIXME: Remove and allocate this state?
static CLevelEditorApp s_LevelEditorApp{};

// Editor utilities.
static void CreateEditorGrid(GeometryInstance_t* pGeoInstance);
static void Editor_Raycast(CLevelEditorApp* pLevelEditor, Ray_t* pRay);

// Event handlers.
static bool Event_OnMouseButtonPressed(void* pListener, EventData_t eventData);
static bool Event_OnMouseButtonReleased(void* pListener, EventData_t eventData);
static bool Event_OnMouseMove(void* pListener, EventData_t eventData);
static bool Event_OnMouseDragged(void* pListener, EventData_t eventData);

// Console Command callbacks.
static void ConsoleCommand_AddModel(struct ConVar_t* pCommand, void* pValue);

// Button callbacks.
static void ButtonEvent_ChangeTool_Selection(void* pListenerInst);
static void ButtonEvent_ChangeTool_AddModel(void* pListenerInst);

LEVEL_EDITOR_API IToolApp* GetToolApplicationState();

bool CLevelEditorApp::Tool_Init(struct ApplicationConfig_t* pAppConfig)
{
	// Should've been set up by the editor.
	CAssert(pAppConfig->pUIContext);
	m_pUIContext = (AppUI::IAppUI*)pAppConfig->pUIContext;

	LOG_INFO("Welcome to Crysalisk Level Editor!");

	IRenderer* pRenderer = pAppConfig->pEngine->GetEngineInterfaces()->pRenderer;

	// Setup world grid.
	// FIXME: Lines should be optionally depth-tested.
	// We want these grids to respect the depth buffer.
	CreateEditorGrid(&m_GridGeo);

	// Create the gizmo.
	if (!Gizmo_Create(pRenderer, &m_Gizmo))
	{
		LOG_ERROR("Failed to create editor gizmo!");
		return false;
	}
	
	// Register for events.
	IEventSystem* pEventSystem = pAppConfig->pEngine->GetEngineInterfaces()->pEngineEventHandler;
	pEventSystem->EventRegister(eEventCategory::INPUT_MOUSE_BPRESSED, this, Event_OnMouseButtonPressed);
	pEventSystem->EventRegister(eEventCategory::INPUT_MOUSE_BRELEASED, this, Event_OnMouseButtonReleased);
	pEventSystem->EventRegister(eEventCategory::INPUT_MOUSE_MOVED, this, Event_OnMouseMove);
	pEventSystem->EventRegister(eEventCategory::INPUT_MOUSE_DRAGGED, this, Event_OnMouseDragged);

	// Editor console commands.
	ConVarSystemState* pConVar = pAppConfig->pEngine->GetEngineInterfaces()->pConVar;
	ConsoleCommandRegister(pConVar, "add_model", CONVAR_VALUETYPE_PASSTHROUGH, ConsoleCommand_AddModel, this);
	ConsoleCommandRegister(pConVar, "set_model_name", CONVAR_VALUETYPE_STRING, ConsoleCommand_SetModelName, this);

	// Default to fullbright for the editor.
	ConsoleCommandExecute(pConVar, "mat_debugvismode 1");

	// Default the tool mode to selection.
	m_iToolMode = EDITOR_TOOL_MODE_SELECTION;

	// Place the camera in a good initial position.
	pAppConfig->pApplication->GetMainCamera()->vecPosition = { 0.0f, 50.0f, 0.0f };

	// Setup UI controls.
	// FIXME: Would be nice to have the developer console render ontop of everything else when active.

	// Load level editor image layout data.
	if (!FontLoader::LoadFontAtlasLayoutFile("LevelEditor_UIAtlas_Layout", glyphData))
	{
		LOG_FATAL("Failed to load level editor glyph data!");
		return false;
	}

	TextureHandle_t hUIAtlas = g_pEngine->GetEngineInterfaces()->pTextureSystem->AcquireTexture("Assets/Textures/LevelEditor_UIAtlas.png", true);
	CAssert(hUIAtlas != kInvalidID_U32);
	
	// Side panel for main tools.
	hMouseToolPanel = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_PANEL);
	m_pUIContext->SetControlPosition(hMouseToolPanel, 0, 50);
	m_pUIContext->SetControlDimensions(hMouseToolPanel, 100, 670);
	m_pUIContext->SetControlColor(hMouseToolPanel, 102, 102, 102, 255);

	// Selection tool image panel.
	hSelectionToolImage = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_IMAGE_PANEL);
	m_pUIContext->SetControlParent(hSelectionToolImage, hMouseToolPanel);
	m_pUIContext->SetControlPosition(hSelectionToolImage, 10, 10);
	m_pUIContext->SetControlDimensions(hSelectionToolImage, 80, 80);
	m_pUIContext->SetControlImage(hSelectionToolImage, hUIAtlas, &glyphData[0]);

	// Add model image panel.
	hAddModelImage = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_IMAGE_PANEL);
	m_pUIContext->SetControlParent(hAddModelImage, hMouseToolPanel);
	m_pUIContext->SetControlPosition(hAddModelImage, 10, 100);
	m_pUIContext->SetControlDimensions(hAddModelImage, 80, 80);
	m_pUIContext->SetControlImage(hAddModelImage, hUIAtlas, &glyphData[1]);

	// Buttons for the above.
	// TODO: Would be good to have buttons with image panels be a single control.
	hSelectionToolButton = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_BUTTON);
	m_pUIContext->SetControlParent(hSelectionToolButton, hMouseToolPanel);
	m_pUIContext->SetControlPosition(hSelectionToolButton, 10, 10);
	m_pUIContext->SetControlDimensions(hSelectionToolButton, 80, 80);
	m_pUIContext->SetControlVisible(hSelectionToolButton, false);
	m_pUIContext->SetControlPressedCallback(hSelectionToolButton, ButtonEvent_ChangeTool_Selection, this);

	hAddModelButton = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_BUTTON);
	m_pUIContext->SetControlParent(hAddModelButton, hMouseToolPanel);
	m_pUIContext->SetControlPosition(hAddModelButton, 10, 100);
	m_pUIContext->SetControlDimensions(hAddModelButton, 80, 80);
	m_pUIContext->SetControlVisible(hAddModelButton, false);
	m_pUIContext->SetControlPressedCallback(hAddModelButton, ButtonEvent_ChangeTool_AddModel, this);

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

				// The selected mesh should share the same translation as the gizmo.
				// FIXME: The gizmo should probably control the geo.
				// FIXME: This shouldn't be hardcoded.
				Matrix4x4 matTranslation(1.0f);
				matTranslation = Matrix4x4::Scale(matTranslation, Vector{ 100.0f, 100.0f, 100.0f });
				matTranslation = Matrix4x4::Translate(matTranslation, m_Gizmo.origin);
				pViewData->mapMeshes.Back()->matLocalToWorldTransform = matTranslation;
			}
		}

		if (m_iToolMode == EDITOR_TOOL_MODE_SELECTION)
		{
			pViewData->gizmoGeo = &m_Gizmo.geometry;
		}
		else
		{
			// If not in selection mode, don't render the gizmo.
			pViewData->gizmoGeo = nullptr;
		}
	}
}

void CLevelEditorApp::Render(struct RenderPacket_t* pRenderPacket)
{
	// TODO: Get rid of this and move all rendering to views?
}

static void CreateEditorGrid(GeometryInstance_t* pOutGeoInstance)
{
	IRenderer* pRenderer = g_pEngine->GetEngineInterfaces()->pRenderer;
	IShaderSystem* pShaderSystem = g_pEngine->GetEngineInterfaces()->pShaderSystem;
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

	pRenderer->AcquireGeometryResources(*pVertexBuffer, *pIndexBuffer, gridVertCount, sizeof(Vector), 0, gridVerts.Data(), 0, nullptr, pOutGeoInstance);

	// Setup grid extents.
	pOutGeoInstance->vecMinExtents = { -2048.0f, -0.001f, -2048.0f };
	pOutGeoInstance->vecMaxExtents = { 2048.0f, 0.001f, 2048.0f };

	pShaderSystem->AcquireShaderInstanceResources(hDebugLineShader, &pOutGeoInstance->hShaderInstance, "Editor_Grid");
	pShaderSystem->SetShaderUniform(hDebugLineShader, pShaderSystem->GetShaderUniformLocation(hDebugLineShader, "color"), 0, pOutGeoInstance->hShaderInstance, &Vector4D{ 1.0f, 1.0f, 1.0f, 1.0f });
	pOutGeoInstance->hShader = hDebugLineShader;
	pOutGeoInstance->matLocalToWorldTransform = Matrix4x4::Matrix4x4(1.0f);
}

static void Editor_Raycast(CLevelEditorApp* pLevelEditor, Ray_t* pRay)
{
	CAssert(pLevelEditor && pRay);

	// We're going to test against the editor grid and all existing geometry in the scene.

	u32 iGeometryCount = (u32)pLevelEditor->m_CurrentDoc.meshes.Count() + 1;	// + 1 to include the editor grid.

	for (u32 iGeo = 0; iGeo < iGeometryCount; iGeo++)
	{
		GeometryInstance_t* pCurGeo = nullptr;

		if (iGeo == 0)
		{
			// Test against the grid first.
			pCurGeo = &pLevelEditor->m_GridGeo;
		}
		else
		{
			// Now test against the other geometries.
			CAssert(0);	// FIXME: Not implemented yet!
		}

		CAssert(pCurGeo);

		// Convert the ray to the geometry's local space.
		Ray_t localRay;
		Matrix4x4 geoWorldToLocal = pCurGeo->matLocalToWorldTransform.Inverse();
		Vector4D vecDirection = pRay->vecDirection.ToVector4D();
		vecDirection.w = 0.0f;

		localRay.vecOrigin = geoWorldToLocal * pRay->vecOrigin;
		localRay.vecDirection = (geoWorldToLocal * vecDirection).AsVector3D();

		// These properties are already in local space.
		Vector vecCenter = pCurGeo->vecCenter;
		Vector vecMins = pCurGeo->vecMinExtents;
		Vector vecMaxs = pCurGeo->vecMaxExtents;

		f32 flTMin = kFLT_MAX;
		f32 flTMax = kFLT_MIN;

		Vector tMin = (vecMins - localRay.vecOrigin) / localRay.vecDirection;
		Vector tMax = (vecMaxs - localRay.vecOrigin) / localRay.vecDirection;

		Vector tNear = { FloatMin(tMin.x, tMax.x), FloatMin(tMin.y, tMax.y), FloatMin(tMin.z, tMax.z) };
		Vector tFar = { FloatMax(tMin.x, tMax.x), FloatMax(tMin.y, tMax.y), FloatMax(tMin.z, tMax.z) };

		f32 flTNearMax = FloatMax(FloatMax(tNear.x, tNear.y), tNear.z);
		f32 flTFarMin = FloatMin(FloatMin(tFar.x, tFar.y), tFar.z);

		if ((flTNearMax <= flTFarMin) && (flTNearMax < pRay->flDistance) && (flTNearMax > 0.0f))	// NOTE: In this particular test, the distance should always be > 0, as the objects in the view frustum should be along the front of the ray
		{																							// (the transforms involved in calculating the ray direction already accounts for Camera Forward = -Z axis).
			pRay->bDidHit = true;
			pRay->flDistance = flTNearMax;
			pRay->vecEnd = pRay->vecOrigin + (pRay->vecDirection * pRay->flDistance);
			pRay->pHitGeometry = pCurGeo;
		}
	}
}

static bool Event_OnMouseButtonPressed(void* pListener, EventData_t eventData)
{
	CLevelEditorApp* pEditorAppState = (CLevelEditorApp*)pListener;
	Gizmo* pGizmo = &pEditorAppState->m_Gizmo;

	if (pGizmo->selectedAxis != kInvalidID_U8)
	{
		Gizmo_BeginInteraction(pGizmo);
	}

	return true;
}

static bool Event_OnMouseButtonReleased(void* pListener, EventData_t eventData)
{
	CLevelEditorApp* pEditorAppState = (CLevelEditorApp*)pListener;
	Gizmo* pGizmo = &pEditorAppState->m_Gizmo;

	if (pGizmo->selectedAxis != kInvalidID_U8)
	{
		Gizmo_EndInteraction(pGizmo);
	}

	if (pEditorAppState->m_iToolMode == EDITOR_TOOL_MODE_ADD_MODEL)
	{
		if (pEditorAppState->m_bToolChangeHack)
		{
			pEditorAppState->m_bToolChangeHack = false;
			return true;
		}

		// If no model name present, don't create.
		if (!pEditorAppState->m_pszToolModelName)
		{
			LOG_ERROR("No model name entered, exiting");
			return true;
		}

		// TODO ALGORITHM:
		// 1.) On mouse release, cast a ray into the scene that intersects with the grid AND other geometry (the grid needs an AABB).
		// 2.) Wherever it intersects, place the model there.
		//
		// FUTURE WORK:
		// 3.) Configurable models.
		// 4.) Add some padding so that the model doesn't get stuck in the wall regardless of its extents.
		i32 mouseX{ 0 }, mouseY{ 0 };
		g_pEngine->GetEngineInterfaces()->pInputSystem->GetCurrentMousePos(mouseX, mouseY);

		Ray_t ray = GetRayFromScreen(g_pEngine->GetEngineWindow(), pEditorAppState->m_pCamera, mouseX, mouseY);
		Editor_Raycast(pEditorAppState, &ray);

		IMeshSystem* pMeshSystem = g_pEngine->GetEngineInterfaces()->pMeshSystem;

		// FIXME: If the mesh loading fails (e.g. the mesh doesn't exist), then this will take up a slot in the map mesh array.
		MeshHandle_t hNewMesh = pMeshSystem->AcquireMeshInstance(pEditorAppState->m_pszToolModelName, Vector{ 0.0f, 0.0f, 0.0f }, Vector{ 0.0f, 0.0f, 0.0f }, Vector{ 100.0f, 100.0f, 100.0f }, true);
		if (hNewMesh != kInvalidID_U32)
		{
			pEditorAppState->m_CurrentDoc.meshes.PushBack(hNewMesh);
		}
		else
		{
			return true;
		}
		
		IDebugGeometrySystem* pDebug = g_pEngine->GetEngineInterfaces()->pDebugGeometry;
		pDebug->AddLine(ray.vecOrigin, ray.vecEnd, Vector4D{ 1.0f, 1.0f, 1.0f, 1.0f }, -1.0f);
	}

	return true;
}

static bool Event_OnMouseMove(void* pListener, EventData_t eventData)
{
	CLevelEditorApp* pEditorAppState = (CLevelEditorApp*)pListener;

	// Early out if no geometry is selected.
	/*if (pEditorAppState->m_SelectedMeshIdx == kInvalidID_U32)
	{
		return true;
	}*/

	// Don't update if dragging.
	if (pEditorAppState->m_Gizmo.dragging)
	{
		return true;
	}

	// Cast a ray and see if we intersect with the gizmo.
	i32 iMouseX = eventData.i32[0];
	i32 iMouseY = eventData.i32[1];

	Ray_t ray = GetRayFromScreen(g_pEngine->GetEngineWindow(), g_pEngine->GetApplication()->GetMainCamera(), iMouseX, iMouseY);

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

static bool Event_OnMouseDragged(void* pListener, EventData_t eventData)
{
	CLevelEditorApp* pEditorAppState = (CLevelEditorApp*)pListener;

	if (pEditorAppState->m_Gizmo.selectedAxis != kInvalidID_U8)
	{
		Gizmo_HandleInteraction(&pEditorAppState->m_Gizmo, pEditorAppState->m_pCamera);
	}

	return true;
}

static void ConsoleCommand_SetModelName(struct ConVar_t* pCommand, void* pValue)
{
	if (!pValue)
	{
		return;
	}

	CLevelEditorApp* pApp = (CLevelEditorApp*)pCommand->listenerInst;
	if (pApp->m_pszToolModelName)
	{
		Utl::StringFree(pApp->m_pszToolModelName);
	}

	pApp->m_pszToolModelName = Utl::StringDuplicate((char*)pValue);
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

	// LEFTOFF: Re-enable this when the gizmo is working.
	// Set as the selected mesh.
	//pApp->m_SelectedMeshIdx = (u32)(pApp->m_CurrentDoc.meshes.Count() - 1);

	// Move the gizmo to this model.
	//pApp->m_Gizmo.origin = Vector{ 0.0f, 0.0f, 0.0f };
}

static void ButtonEvent_ChangeTool_Selection(void* pListenerInst)
{
	CLevelEditorApp* pEditor = (CLevelEditorApp*)pListenerInst;

	pEditor->m_iToolMode = EDITOR_TOOL_MODE_SELECTION;
}

static void ButtonEvent_ChangeTool_AddModel(void* pListenerInst)
{
	CLevelEditorApp* pEditor = (CLevelEditorApp*)pListenerInst;

	pEditor->m_iToolMode = EDITOR_TOOL_MODE_ADD_MODEL;
	pEditor->m_bToolChangeHack = true;
}

IToolApp* GetToolApplicationState()
{
	return &s_LevelEditorApp;
}