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
#include <Resources/GeometryGenerator.h>
#include <Systems/MaterialSystem.h>
#include <Systems/MeshSystem.h>
#include <Systems/ShaderSystem.h>
#include <Systems/TextureSystem.h>

#include "Gizmo.h"
#include "LevelEditorTypes.h"

struct EditorPropertyEntryControl_t
{
	AppUI::UIControlHandle_t hLabel{ kInvalidID_U32 };
	AppUI::UIControlHandle_t hEntryPanel{ kInvalidID_U32 };
	AppUI::UIControlHandle_t hTextEntry{ kInvalidID_U32 };
};

class CLevelEditorApp : public IToolApp
{
public:
	virtual bool	Tool_Init(struct ApplicationConfig_t* pAppConfig) override;
	virtual bool	Tool_Update(f32 deltaTime) override;
	virtual void	Tool_Shutdown() override;

	virtual void	FramePrepare(struct RenderPacket_t* pRenderPacket) override;
	virtual void	Render(struct RenderPacket_t* pRenderPacket) override;

	virtual void	SetCamera(struct Camera_t* pCamera) override { m_pCamera = pCamera; }

private:
	void			HandleCameraMovement(f32 deltaTime);

	void			DisplayEntityInspectorPanel(bool bEnable);

private:
	Utl::CVector<i32>	m_GridLines{};
	GeometryInstance_t	m_GridGeo{};
	i32					m_OriginCross[3]{};
	Gizmo				m_Gizmo{};

	// Current map data.
	MapEditorDocument_t	m_CurrentDoc{};
	u32					m_SelectedMeshIdx{ kInvalidID_U32 };
	Camera_t*			m_pCamera{ nullptr };

	// Current tool mode.
	u8					m_iToolMode{ EDITOR_TOOL_MODE_NONE };
	// Editor hacks.
	char*				m_pszToolModelName{ nullptr };	// The name of the model to place.

	// UI system context.
	AppUI::IAppUI*		m_pUIContext{ nullptr };

	// Default point model.
	MeshHandle_t		m_hDefaultMesh{ kInvalidID_U32 };
	GeometryInstance_t	m_DefaultPointGeometry{};

	// Glyph data.
	Utl::CVector<FontLoader::GlyphLayoutData_t>	glyphData{};

	// Editor UI controls.
	// Main tools.
	AppUI::UIControlHandle_t	hMouseToolPanel{ kInvalidID_U32 };
	AppUI::UIControlHandle_t	hSelectionToolImage{ kInvalidID_U32 };
	AppUI::UIControlHandle_t	hAddModelImage{ kInvalidID_U32 };

	AppUI::UIControlHandle_t	hSelectionToolButton{ kInvalidID_U32 };
	AppUI::UIControlHandle_t	hAddModelButton{ kInvalidID_U32 };

	// Entity inspector panel.
	AppUI::UIControlHandle_t	hEntityInspectorPanel{ kInvalidID_U32 };
	AppUI::UIControlHandle_t	hEntityInspectorLabel{ kInvalidID_U32 };

	EditorPropertyEntryControl_t translationEntry{};
	EditorPropertyEntryControl_t rotationEntry{};
	EditorPropertyEntryControl_t scaleEntry{};

	AppUI::UIControlHandle_t	hEntityModelNameLabel{ kInvalidID_U32 };
	AppUI::UIControlHandle_t	hEntityModelNameEntry{ kInvalidID_U32 };

	// These need access to our members.
	friend static bool Event_OnKeyPressed(void* pListener, EventData_t eventData);
	friend static bool Event_OnMouseButtonPressed(void* pListener, EventData_t eventData);
	friend static bool Event_OnMouseButtonReleased(void* pListener, EventData_t eventData);
	friend static bool Event_OnMouseMove(void* pListener, EventData_t eventData);
	friend static bool Event_OnMouseDragged(void* pListener, EventData_t eventData);

	friend static void ButtonEvent_ChangeTool_Selection(void* pListenerInst);
	friend static void ButtonEvent_ChangeTool_AddModel(void* pListenerInst);

	friend static u32 Editor_Raycast(CLevelEditorApp* pLevelEditor, Ray_t* pRay, bool bTestAgainstGrid);
	friend static void SetSelectedGeometry(CLevelEditorApp* pLevelEditor, u32 geometryIdx);
};

// TODO: Remove and allocate this state?
static CLevelEditorApp s_LevelEditorApp{};

// Editor utilities.
static void CreateEditorGrid(GeometryInstance_t* pGeoInstance);
static u32 Editor_Raycast(CLevelEditorApp* pLevelEditor, Ray_t* pRay, bool bTestAgainstGrid);
static void SetSelectedGeometry(CLevelEditorApp* pLevelEditor, u32 geometryIdx);
static void RegenerateGeometryTransformMatrix(MeshInstance_t* pInstance);

// Event handlers.
static bool Event_OnKeyPressed(void* pListener, EventData_t eventData);
static bool Event_OnMouseButtonPressed(void* pListener, EventData_t eventData);
static bool Event_OnMouseButtonReleased(void* pListener, EventData_t eventData);
static bool Event_OnMouseMove(void* pListener, EventData_t eventData);
static bool Event_OnMouseDragged(void* pListener, EventData_t eventData);

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

	// Create the default point model: A 16x16x16 white cube.
	// TODO: Make this its own function.
	{
		RenderBuffer* pVertexBuffer = pRenderer->GetRenderBufferByName("World_Vertex_Buffer");
		RenderBuffer* pIndexBuffer = pRenderer->GetRenderBufferByName("World_Index_Buffer");

		GeometryConfig_t defaultConfig = MeshGenerator::CreateDefaultCubeConfiguration(Vector{ 0.0f, 0.0f, 0.0f }, 16.0f, 16.0f, 16.0);

		// Just need the positions.
		Utl::CVector<f32> vertData(defaultConfig.Vertices.Count() * 3);
		for (u32 iVert = 0; iVert < defaultConfig.Vertices.Count(); iVert++)
		{
			vertData.PushBack(defaultConfig.Vertices[iVert].Pos.x);
			vertData.PushBack(defaultConfig.Vertices[iVert].Pos.y);
			vertData.PushBack(defaultConfig.Vertices[iVert].Pos.z);
		}

		pAppConfig->pEngine->GetEngineInterfaces()->pRenderer->AcquireGeometryResources(*pVertexBuffer, *pIndexBuffer, defaultConfig.Vertices.Count(), sizeof(Vector), 0, vertData.Data(), defaultConfig.Indices.Count(), defaultConfig.Indices.Data(), &m_DefaultPointGeometry);
		m_DefaultPointGeometry.matLocalToWorldTransform = Matrix4x4(1.0f);
		m_DefaultPointGeometry.vecMinExtents = Vector{ -16.0f, -16.0f, -16.0f };
		m_DefaultPointGeometry.vecMaxExtents = Vector{ 16.0f, 16.0f, 16.0f };

		IMaterialSystem* pMaterialSystem = pAppConfig->pEngine->GetEngineInterfaces()->pMaterialSystem;
		IMeshSystem* pMeshSystem = pAppConfig->pEngine->GetEngineInterfaces()->pMeshSystem;
		m_hDefaultMesh = pMeshSystem->WrapGeometryInstanceInMesh(&m_DefaultPointGeometry, "LevelEditor_DefaultPointEntityGeo", pMaterialSystem->GetDefaultUnlitMaterial());
	}

	// Register for events.
	IEventSystem* pEventSystem = pAppConfig->pEngine->GetEngineInterfaces()->pEngineEventHandler;
	pEventSystem->EventRegister(eEventCategory::INPUT_KEY_PRESSED, this, Event_OnKeyPressed);
	pEventSystem->EventRegister(eEventCategory::INPUT_MOUSE_BPRESSED, this, Event_OnMouseButtonPressed);
	pEventSystem->EventRegister(eEventCategory::INPUT_MOUSE_BRELEASED, this, Event_OnMouseButtonReleased);
	pEventSystem->EventRegister(eEventCategory::INPUT_MOUSE_MOVED, this, Event_OnMouseMove);
	pEventSystem->EventRegister(eEventCategory::INPUT_MOUSE_DRAGGED, this, Event_OnMouseDragged);

	ConVarSystemState* pConVar = pAppConfig->pEngine->GetEngineInterfaces()->pConVar;

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

	// LEFTOFF: Need to pass key messages into here for controls

	TextureHandle_t hUIAtlas = g_pEngine->GetEngineInterfaces()->pTextureSystem->AcquireTexture("LevelEditor_UIAtlas.png", true);
	CAssert(hUIAtlas != kInvalidID_U32);

	// Side panel for main tools.
	{
		hMouseToolPanel = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_PANEL);
		m_pUIContext->SetControlPosition(hMouseToolPanel, 0, 150);
		m_pUIContext->SetControlDimensions(hMouseToolPanel, 100, 570);
		m_pUIContext->SetControlColor(hMouseToolPanel, 102, 102, 102, 255);
	}

	// Selection tool image panel.
	{
		hSelectionToolImage = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_IMAGE_PANEL);
		m_pUIContext->SetControlParent(hSelectionToolImage, hMouseToolPanel);
		m_pUIContext->SetControlPosition(hSelectionToolImage, 10, 10);
		m_pUIContext->SetControlDimensions(hSelectionToolImage, 80, 80);
		m_pUIContext->SetControlImage(hSelectionToolImage, hUIAtlas, &glyphData[0]);
	}

	// Add model image panel.
	{
		hAddModelImage = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_IMAGE_PANEL);
		m_pUIContext->SetControlParent(hAddModelImage, hMouseToolPanel);
		m_pUIContext->SetControlPosition(hAddModelImage, 10, 100);
		m_pUIContext->SetControlDimensions(hAddModelImage, 80, 80);
		m_pUIContext->SetControlImage(hAddModelImage, hUIAtlas, &glyphData[1]);
	}

	// Buttons for the above.
	// TODO: Would be good to have buttons with image panels be a single control.
	{
		hSelectionToolButton = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_BUTTON);
		m_pUIContext->SetControlParent(hSelectionToolButton, hMouseToolPanel);
		m_pUIContext->SetControlPosition(hSelectionToolButton, 10, 10);
		m_pUIContext->SetControlDimensions(hSelectionToolButton, 80, 80);
		m_pUIContext->SetControlVisible(hSelectionToolButton, false);
		m_pUIContext->SetControlPressedCallback(hSelectionToolButton, ButtonEvent_ChangeTool_Selection, this);
	}

	{
		hAddModelButton = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_BUTTON);
		m_pUIContext->SetControlParent(hAddModelButton, hMouseToolPanel);
		m_pUIContext->SetControlPosition(hAddModelButton, 10, 100);
		m_pUIContext->SetControlDimensions(hAddModelButton, 80, 80);
		m_pUIContext->SetControlVisible(hAddModelButton, false);
		m_pUIContext->SetControlPressedCallback(hAddModelButton, ButtonEvent_ChangeTool_AddModel, this);
	}

	// Entity inspector panel.
	{
		hEntityInspectorPanel = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_PANEL);
		m_pUIContext->SetControlPosition(hEntityInspectorPanel, 830, 200);
		m_pUIContext->SetControlDimensions(hEntityInspectorPanel, 250, 520);
	}

	// Entity inspector panel label.
	{
		hEntityInspectorLabel = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
		m_pUIContext->SetControlParent(hEntityInspectorLabel, hEntityInspectorPanel);
		m_pUIContext->SetControlPosition(hEntityInspectorLabel, 0, 30);
		m_pUIContext->SetControlText(hEntityInspectorLabel, "Entity Inspector");
	}

	// Translation control.
	{
		// Text label.
		translationEntry.hLabel = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
		m_pUIContext->SetControlParent(translationEntry.hLabel, hEntityInspectorPanel);
		m_pUIContext->SetControlPosition(translationEntry.hLabel, 0, 100);
		m_pUIContext->SetControlFont(translationEntry.hLabel, "UbuntuMono-R.ttf", 20.0f);
		m_pUIContext->SetControlText(translationEntry.hLabel, "Position");

		// Text panel.
		translationEntry.hEntryPanel = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_PANEL);
		m_pUIContext->SetControlParent(translationEntry.hEntryPanel, hEntityInspectorPanel);
		m_pUIContext->SetControlPosition(translationEntry.hEntryPanel, 25, 105);
		m_pUIContext->SetControlDimensions(translationEntry.hEntryPanel, 190, 25);
		m_pUIContext->SetControlColor(translationEntry.hEntryPanel, 50, 50, 50, 255);

		// Text entry.
		translationEntry.hTextEntry = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
		m_pUIContext->SetControlParent(translationEntry.hTextEntry, hEntityInspectorPanel);
		m_pUIContext->SetControlPosition(translationEntry.hTextEntry, 30, 123);
		m_pUIContext->SetControlFlag(translationEntry.hTextEntry, AppUI::UI_CONTROL_FLAG_TEXT_AUTO_CLEAR, true);
		m_pUIContext->SetControlFont(translationEntry.hTextEntry, "UbuntuMono-R.ttf", 18.0f);
		m_pUIContext->SetControlText(translationEntry.hTextEntry, "0.0 0.0 0.0");
	}

	// Rotation control.
	{
		// Text label
		rotationEntry.hLabel = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
		m_pUIContext->SetControlParent(rotationEntry.hLabel, hEntityInspectorPanel);
		m_pUIContext->SetControlPosition(rotationEntry.hLabel, 0, 153);
		m_pUIContext->SetControlFont(rotationEntry.hLabel, "UbuntuMono-R.ttf", 20.0f);
		m_pUIContext->SetControlText(rotationEntry.hLabel, "Rotation");

		// Text panel.
		rotationEntry.hEntryPanel = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_PANEL);
		m_pUIContext->SetControlParent(rotationEntry.hEntryPanel, hEntityInspectorPanel);
		m_pUIContext->SetControlPosition(rotationEntry.hEntryPanel, 25, 158);
		m_pUIContext->SetControlDimensions(rotationEntry.hEntryPanel, 190, 25);
		m_pUIContext->SetControlColor(rotationEntry.hEntryPanel, 50, 50, 50, 255);

		// Text entry.
		rotationEntry.hTextEntry = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
		m_pUIContext->SetControlParent(rotationEntry.hTextEntry, hEntityInspectorPanel);
		m_pUIContext->SetControlPosition(rotationEntry.hTextEntry, 30, 176);
		m_pUIContext->SetControlFlag(rotationEntry.hTextEntry, AppUI::UI_CONTROL_FLAG_TEXT_AUTO_CLEAR, true);
		m_pUIContext->SetControlFont(rotationEntry.hTextEntry, "UbuntuMono-R.ttf", 18.0f);
		m_pUIContext->SetControlText(rotationEntry.hTextEntry, "0.0 0.0 0.0");
	}

	// Scale control.
	{
		// Text label
		scaleEntry.hLabel = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
		m_pUIContext->SetControlParent(scaleEntry.hLabel, hEntityInspectorPanel);
		m_pUIContext->SetControlPosition(scaleEntry.hLabel, 0, 206);
		m_pUIContext->SetControlFont(scaleEntry.hLabel, "UbuntuMono-R.ttf", 20.0f);
		m_pUIContext->SetControlText(scaleEntry.hLabel, "Scale");

		// Text panel.
		scaleEntry.hEntryPanel = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_PANEL);
		m_pUIContext->SetControlParent(scaleEntry.hEntryPanel, hEntityInspectorPanel);
		m_pUIContext->SetControlPosition(scaleEntry.hEntryPanel, 25, 211);
		m_pUIContext->SetControlDimensions(scaleEntry.hEntryPanel, 190, 25);
		m_pUIContext->SetControlColor(scaleEntry.hEntryPanel, 50, 50, 50, 255);

		// Text entry.
		scaleEntry.hTextEntry = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
		m_pUIContext->SetControlParent(scaleEntry.hTextEntry, hEntityInspectorPanel);
		m_pUIContext->SetControlPosition(scaleEntry.hTextEntry, 30, 229);
		m_pUIContext->SetControlFlag(scaleEntry.hTextEntry, AppUI::UI_CONTROL_FLAG_TEXT_AUTO_CLEAR, true);
		m_pUIContext->SetControlFont(scaleEntry.hTextEntry, "UbuntuMono-R.ttf", 18.0f);
		m_pUIContext->SetControlText(scaleEntry.hTextEntry, "1.0 1.0 1.0");
	}

	// Entity inspector model name label.
	{
		hEntityModelNameLabel = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
		m_pUIContext->SetControlParent(hEntityModelNameLabel, hEntityInspectorPanel);
		m_pUIContext->SetControlPosition(hEntityModelNameLabel, 0, 300);
		m_pUIContext->SetControlFont(hEntityModelNameLabel, "UbuntuMono-R.ttf", 20.0f);
		m_pUIContext->SetControlText(hEntityModelNameLabel, "Model Name");
	}

	// Entity inspector model name entry.
	{
		hEntityModelNameEntry = m_pUIContext->CreateUIControl(AppUI::CONTROL_TYPE_LABEL);
		m_pUIContext->SetControlParent(hEntityModelNameEntry, hEntityInspectorPanel);
		m_pUIContext->SetControlPosition(hEntityModelNameEntry, 30, 320);
		m_pUIContext->SetControlFont(hEntityModelNameEntry, "UbuntuMono-R.ttf", 18.0f);
		m_pUIContext->SetControlText(hEntityModelNameEntry, "<Your model name here>");
		m_pUIContext->SetControlFlag(hEntityModelNameEntry, AppUI::UI_CONTROL_FLAG_TEXT_AUTO_CLEAR, true);
		m_pUIContext->SetControlInteractable(hEntityModelNameEntry, true);
	}

	// Regenerate the UI control layout.
	m_pUIContext->RegenerateRenderList();

	DisplayEntityInspectorPanel(false);

	// Give the UI layer focus by default.
	m_pUIContext->AcquireInputFocus();

	return true;
}

void CLevelEditorApp::Tool_Shutdown()
{
	LOG_INFO("Shutting down level editor.");

	IRenderer* pRenderer = g_pEngine->GetEngineInterfaces()->pRenderer;

	// Gizmo
	Gizmo_Destroy(pRenderer, &m_Gizmo);
}

bool CLevelEditorApp::Tool_Update(f32 deltaTime)
{
	if (m_SelectedMeshIdx != kInvalidID_U32)
	{
		DisplayEntityInspectorPanel(true);
	}
	else
	{
		DisplayEntityInspectorPanel(false);
	}

	// Update camera.
	HandleCameraMovement(deltaTime);

	if (m_Gizmo.geometry.vertexCount)
	{
		IMeshSystem* pMeshSystem = g_pEngine->GetEngineInterfaces()->pMeshSystem;

		// Move the gizmo over to the selected geometry.
		if (m_SelectedMeshIdx != kInvalidID_U32)
		{
			MeshInstance_t* pInstance = pMeshSystem->AcquireMeshInstanceDataFromHandle(m_CurrentDoc.meshInstances[m_SelectedMeshIdx]);
			Vector translation = pInstance->matLocalToWorld.GetTranslation();
			m_Gizmo.origin = translation;
		}

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
		pViewData->mapInstances.SetAllocator(pRenderPacket->pFrameAllocator);
	}
	else
	{
		IMeshSystem* pMeshSystem = g_pEngine->GetEngineInterfaces()->pMeshSystem;
		EditorView_ExtendedData_t* pViewData = (EditorView_ExtendedData_t*)pRenderPacket->pEditorViewExtendedData;

		pViewData->editorGeo.Resize(12);
		pViewData->editorGeo.PushBack(&m_GridGeo);

		pViewData->mapMeshes.PushBack(&m_DefaultPointGeometry);

		pViewData->mapInstances.Resize(m_CurrentDoc.meshInstances.Count());

		for (u32 iGeo = 0; iGeo < m_CurrentDoc.meshInstances.Count(); iGeo++)
		{
			pViewData->mapInstances.PushBack(m_CurrentDoc.meshInstances[iGeo]);
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

		pViewData->selectedMeshIndex = m_SelectedMeshIdx;
	}
}

void CLevelEditorApp::Render(struct RenderPacket_t* pRenderPacket)
{
	// TODO: Get rid of this and move all rendering to views?
}

void CLevelEditorApp::HandleCameraMovement(f32 deltaTime)
{

}

void CLevelEditorApp::DisplayEntityInspectorPanel(bool bEnable)
{
	m_pUIContext->SetControlVisible(hEntityInspectorLabel, bEnable);

	m_pUIContext->SetControlVisible(translationEntry.hLabel, bEnable);
	m_pUIContext->SetControlVisible(translationEntry.hEntryPanel, bEnable);
	m_pUIContext->SetControlVisible(translationEntry.hTextEntry, bEnable);
	m_pUIContext->SetControlInteractable(translationEntry.hTextEntry, bEnable);

	m_pUIContext->SetControlVisible(rotationEntry.hLabel, bEnable);
	m_pUIContext->SetControlVisible(rotationEntry.hEntryPanel, bEnable);
	m_pUIContext->SetControlVisible(rotationEntry.hTextEntry, bEnable);
	m_pUIContext->SetControlInteractable(rotationEntry.hTextEntry, bEnable);

	m_pUIContext->SetControlVisible(scaleEntry.hLabel, bEnable);
	m_pUIContext->SetControlVisible(scaleEntry.hEntryPanel, bEnable);
	m_pUIContext->SetControlVisible(scaleEntry.hTextEntry, bEnable);
	m_pUIContext->SetControlInteractable(scaleEntry.hTextEntry, bEnable);

	m_pUIContext->SetControlVisible(hEntityModelNameLabel, bEnable);
	m_pUIContext->SetControlVisible(hEntityModelNameEntry, bEnable);
	m_pUIContext->SetControlInteractable(hEntityModelNameEntry, bEnable);
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

static u32 Editor_Raycast(CLevelEditorApp* pLevelEditor, Ray_t* pRay, bool bTestAgainstGrid)
{
	CAssert(pLevelEditor && pRay);

	// We're going to test against the editor grid and all existing geometry in the scene.

	u32 iGeometryCount = (u32)pLevelEditor->m_CurrentDoc.meshInstances.Count() + 1;	// + 1 to include the editor grid.
	IMeshSystem* pMeshSystem = g_pEngine->GetEngineInterfaces()->pMeshSystem;

	u32 hitMeshIdx = kInvalidID_U32;

	for (u32 iGeo = 0; iGeo < iGeometryCount; iGeo++)
	{
		GeometryInstance_t* pCurGeo = nullptr;
		MeshInstance_t* pCurInstance = nullptr;
		MeshBase_t* pCurBase = nullptr;

		if (iGeo == 0)
		{
			if (bTestAgainstGrid)
			{
				// Test against the grid first.
				pCurGeo = &pLevelEditor->m_GridGeo;
			}
			else
			{
				continue;
			}
		}
		else
		{
			// Now test against the other geometries.
			pCurInstance = pMeshSystem->AcquireMeshInstanceDataFromHandle(pLevelEditor->m_CurrentDoc.meshInstances[iGeo - 1]);
			pCurBase = pMeshSystem->AcquireMeshBaseDataFromHandle(pCurInstance->hBaseMesh);
			pCurGeo = &pCurBase->geometry;
		}

		CAssert(pCurGeo);

		// Convert the ray to the geometry's local space.

		Ray_t localRay;
		Matrix4x4 geoWorldToLocal = pCurGeo->matLocalToWorldTransform.Inverse();

		// HACK until all geometry uses the new base/instance system.
		if (iGeo > 0)
		{
			geoWorldToLocal = pCurInstance->matLocalToWorld.Inverse();
		}

		Vector4D vecDirection = pRay->vecDirection.ToVector4D();
		vecDirection.w = 0.0f;

		localRay.vecOrigin = geoWorldToLocal * pRay->vecOrigin;
		localRay.vecDirection = (geoWorldToLocal * vecDirection).AsVector3D();

		// These properties are already in local space.
		Vector vecCenter = pCurGeo->vecCenter;
		Vector vecMins = pCurGeo->vecMinExtents;
		Vector vecMaxs = pCurGeo->vecMaxExtents;

		// HACK until all geometry uses the new base/instance system.
		if (iGeo > 0)
		{
			vecCenter = pCurBase->submeshCenters[0];
			vecMins = pCurBase->submeshMinExtents[0];
			vecMaxs = pCurBase->submeshMaxExtents[0];
		}

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
			hitMeshIdx = iGeo - 1;
		}
	}

	return hitMeshIdx;
}

static void SetSelectedGeometry(CLevelEditorApp* pLevelEditor, u32 geometryIdx)
{
	CAssert(pLevelEditor);

	IMeshSystem* pMeshSystem = g_pEngine->GetEngineInterfaces()->pMeshSystem;

	pLevelEditor->m_SelectedMeshIdx = geometryIdx;

	if (geometryIdx == kInvalidID_U32)
	{
		pLevelEditor->DisplayEntityInspectorPanel(false);
		return;
	}

	MeshInstance_t* pSelectedInstance = pMeshSystem->AcquireMeshInstanceDataFromHandle(pLevelEditor->m_CurrentDoc.meshInstances[geometryIdx]);
	MeshBase_t* pBase = pMeshSystem->AcquireMeshBaseDataFromHandle(pSelectedInstance->hBaseMesh);
	Vector vecTranslation = pSelectedInstance->matLocalToWorld.GetTranslation();

	// Update inspector controls.
	AppUI::IAppUI* pUIContext = pLevelEditor->m_pUIContext;
	char szTextBuffer[128]{};
	Utl::StringFormat_V("%.2f %.2f %.2f", szTextBuffer, vecTranslation.x, vecTranslation.y, vecTranslation.z);

	pUIContext->SetControlText(pLevelEditor->translationEntry.hTextEntry, szTextBuffer);

	// Model name control.
	Platform::MemorySet(szTextBuffer, 0, sizeof(szTextBuffer));
	Utl::StringFormat_V("%s", szTextBuffer, pBase->pszName);
	pUIContext->SetControlText(pLevelEditor->hEntityModelNameEntry, szTextBuffer);
}

static void RegenerateGeometryTransformMatrix(MeshInstance_t* pInstance)
{
	CAssert(pInstance);

	Matrix4x4 transform{ 1.0f };

	// Scale
	transform = Matrix4x4::Scale(transform, pInstance->vecScale);

	// Rotation
	// FIXME: This doesn't seem to work?
	//Quaternion quatRotation = QuaternionFromEulerAngles(pInstance->vecEulerAngles);
	//Matrix4x4 matRotation = Matrix4x4::Mat4FromQuaternion(quatRotation);

	//transform = matRotation * transform;

	Matrix4x4 rotateX = Matrix4x4::RotateAboutAxisAngles(transform, Vector{ 1.0f, 0.0f, 0.0f }, M_DEG2RAD_F(pInstance->vecEulerAngles.x));
	transform = transform * rotateX;
	Matrix4x4 rotateY = Matrix4x4::RotateAboutAxisAngles(transform, Vector{ 0.0f, 1.0f, 0.0f }, M_DEG2RAD_F(pInstance->vecEulerAngles.y));
	transform = transform * rotateY;
	Matrix4x4 rotateZ = Matrix4x4::RotateAboutAxisAngles(transform, Vector{ 0.0f, 0.0f, 1.0f }, M_DEG2RAD_F(pInstance->vecEulerAngles.z));
	transform = transform * rotateZ;

	// Translation
	transform = Matrix4x4::Translate(transform, pInstance->vecTranslation);
	pInstance->matLocalToWorld = transform;
}

// FIXME: This function needs to be cleaned up.
static bool Event_OnKeyPressed(void* pListener, EventData_t eventData)
{
	CLevelEditorApp* pEditorAppState = (CLevelEditorApp*)pListener;
	IMeshSystem* pMeshSystem = g_pEngine->GetEngineInterfaces()->pMeshSystem;

	AppUI::UIControlHandle_t hActiveControl = pEditorAppState->m_pUIContext->GetActiveControl();

	if (eventData.c[0] == KEY_ENTER && hActiveControl != kInvalidID_U32)
	{
		char* pszEnteredTextName = pEditorAppState->m_pUIContext->GetControlText(hActiveControl);
		char* pszTrimmedText = Utl::StringTrim(pszEnteredTextName);

		// Check the active control.
		if (hActiveControl == pEditorAppState->hEntityModelNameEntry)
		{
			// If we pressed enter on the model name control, we might need to set the model.
			if (Utl::StringLength(pszEnteredTextName) > 0)
			{
				// Try to load the model.
				MeshHandle_t hNewMesh = pMeshSystem->AcquireMeshBase(pszTrimmedText);

				Vector translation = pMeshSystem->AcquireMeshInstanceDataFromHandle(pEditorAppState->m_CurrentDoc.meshInstances[pEditorAppState->m_SelectedMeshIdx])->matLocalToWorld.GetTranslation();

				pEditorAppState->m_CurrentDoc.meshInstances[pEditorAppState->m_SelectedMeshIdx] = pMeshSystem->AcquireMeshInstance(hNewMesh, translation, Vector{ 0.0f, 0.0f, 0.0f }, Vector{ 1.0f, 1.0f, 1.0f }, true);

				// Store the transform.
				MeshInstance_t* pMesh = pMeshSystem->AcquireMeshInstanceDataFromHandle(pEditorAppState->m_CurrentDoc.meshInstances[pEditorAppState->m_SelectedMeshIdx]);
				pMesh->vecTranslation = translation;
				pMesh->vecScale = Vector{ 1.0f, 1.0f, 1.0f };
			}
			// Deactivate the text control.
			pEditorAppState->m_pUIContext->SetActiveControl(kInvalidID_U32);
		}
		else if (hActiveControl == pEditorAppState->translationEntry.hTextEntry)
		{
			Vector newTranslation{};
			bool parseResult = Utl::String_ToVector3(pszTrimmedText, &newTranslation);

			if (!parseResult)
			{
				// Couldn't parse translation, don't do anything else.
				goto cleanup_and_return;
			}

			// Set translation and regenerate the transform matrix.
			u32 selectedInstance = pEditorAppState->m_CurrentDoc.meshInstances[pEditorAppState->m_SelectedMeshIdx];
			MeshInstance_t* pSelectedInstanceData = pMeshSystem->AcquireMeshInstanceDataFromHandle(selectedInstance);

			pSelectedInstanceData->vecTranslation = newTranslation;

			RegenerateGeometryTransformMatrix(pSelectedInstanceData);

			// Deactivate the text control.
			pEditorAppState->m_pUIContext->SetActiveControl(kInvalidID_U32);
		}
		else if (hActiveControl == pEditorAppState->rotationEntry.hTextEntry)
		{
			Vector newAngles{};
			bool parseResult = Utl::String_ToVector3(pszTrimmedText, &newAngles);

			if (!parseResult)
			{
				// Couldn't parse rotation, don't do anything else.
				goto cleanup_and_return;
			}

			// Set angles and regenerate the transform matrix.
			u32 selectedInstance = pEditorAppState->m_CurrentDoc.meshInstances[pEditorAppState->m_SelectedMeshIdx];
			MeshInstance_t* pSelectedInstanceData = pMeshSystem->AcquireMeshInstanceDataFromHandle(selectedInstance);

			pSelectedInstanceData->vecEulerAngles = newAngles;

			RegenerateGeometryTransformMatrix(pSelectedInstanceData);

			// Deactivate the text control.
			pEditorAppState->m_pUIContext->SetActiveControl(kInvalidID_U32);
		}
		else if (hActiveControl == pEditorAppState->scaleEntry.hTextEntry)
		{
			Vector newScale{};
			bool parseResult = Utl::String_ToVector3(pszTrimmedText, &newScale);

			if (!parseResult)
			{
				// Couldn't parse scale, don't do anything else.
				goto cleanup_and_return;
			}

			// Set scale and regenerate the transform matrix.
			u32 selectedInstance = pEditorAppState->m_CurrentDoc.meshInstances[pEditorAppState->m_SelectedMeshIdx];
			MeshInstance_t* pSelectedInstanceData = pMeshSystem->AcquireMeshInstanceDataFromHandle(selectedInstance);

			pSelectedInstanceData->vecScale = newScale;

			RegenerateGeometryTransformMatrix(pSelectedInstanceData);

			// Deactivate the text control.
			pEditorAppState->m_pUIContext->SetActiveControl(kInvalidID_U32);
		}

cleanup_and_return:
		Utl::StringFree(pszTrimmedText);
		pszTrimmedText = nullptr;
	}

	return true;
}

static bool Event_OnMouseButtonPressed(void* pListener, EventData_t eventData)
{
	CLevelEditorApp* pEditorAppState = (CLevelEditorApp*)pListener;
	Gizmo* pGizmo = &pEditorAppState->m_Gizmo;

	if (pGizmo->selectedAxis != kInvalidID_U8)
	{
		// Create a ray to get the starting position.i32 iMouseX = 0;
		i32 iMouseX = 0;
		i32 iMouseY = 0;
		IInputSystem* pInputSystem = g_pEngine->GetEngineInterfaces()->pInputSystem;
		pInputSystem->GetCurrentMousePos(iMouseX, iMouseY);

		Ray_t ray = GetRayFromScreen(g_pEngine->GetEngineWindow(), pEditorAppState->m_pCamera, iMouseX, iMouseY);

		u8 unused{ 0 };
		Gizmo_Raycast(pGizmo, &ray, &unused);

		Gizmo_BeginInteraction(pGizmo, &ray);
	}

	return true;
}

static bool Event_OnMouseButtonReleased(void* pListener, EventData_t eventData)
{
	CLevelEditorApp* pEditorAppState = (CLevelEditorApp*)pListener;
	IMeshSystem* pMeshSystem = g_pEngine->GetEngineInterfaces()->pMeshSystem;
	Gizmo* pGizmo = &pEditorAppState->m_Gizmo;

	Gizmo_EndInteraction(pGizmo);

	if (pGizmo->selectedAxis != kInvalidID_U8 && pEditorAppState->m_SelectedMeshIdx != kInvalidID_U32)
	{
		// Update the translation entry if a piece of geo was selected.
		// NOTE: We currently only do this on mouse release because updaing strings is VERY slow and can make dragging stutter.
		// Might change this later.
		u32 selectedInstance = pEditorAppState->m_CurrentDoc.meshInstances[pEditorAppState->m_SelectedMeshIdx];
		MeshInstance_t* pSelectedInstanceData = pMeshSystem->AcquireMeshInstanceDataFromHandle(selectedInstance);
		Matrix4x4 transformCopy = pSelectedInstanceData->matLocalToWorld;
		char szTranslation[128]{};
		Utl::StringFormat_V("%.2f %.2f %.2f", szTranslation, transformCopy.flData[12], transformCopy.flData[13], transformCopy.flData[14]);
		pEditorAppState->m_pUIContext->SetControlText(pEditorAppState->translationEntry.hTextEntry, szTranslation);

		// Early return to keep the selection on the current entity.
		return true;
	}

	switch (pEditorAppState->m_iToolMode)
	{
	case EDITOR_TOOL_MODE_SELECTION:
		{
			if (pEditorAppState->m_CurrentDoc.meshInstances.Count() > 0 && !pEditorAppState->m_Gizmo.dragging)
			{
				i32 mouseX{ 0 }, mouseY{ 0 };
				g_pEngine->GetEngineInterfaces()->pInputSystem->GetCurrentMousePos(mouseX, mouseY);

				Ray_t ray = GetRayFromScreen(g_pEngine->GetEngineWindow(), pEditorAppState->m_pCamera, mouseX, mouseY);
				u32 idx = Editor_Raycast(pEditorAppState, &ray, false);

				SetSelectedGeometry(pEditorAppState, idx);
			}
		}
		break;

	case EDITOR_TOOL_MODE_ADD_MODEL:
		{
			i32 mouseX{ 0 }, mouseY{ 0 };
			g_pEngine->GetEngineInterfaces()->pInputSystem->GetCurrentMousePos(mouseX, mouseY);

			Ray_t ray = GetRayFromScreen(g_pEngine->GetEngineWindow(), pEditorAppState->m_pCamera, mouseX, mouseY);
			Editor_Raycast(pEditorAppState, &ray, true);

			Matrix4x4 matTransform = Matrix4x4(1.0f);
			matTransform = Matrix4x4::Translate(matTransform, ray.vecEnd);

			pEditorAppState->m_DefaultPointGeometry.matLocalToWorldTransform = matTransform;

			// Add a new mesh here.
			IMeshSystem* pMeshSystem = g_pEngine->GetEngineInterfaces()->pMeshSystem;
			pEditorAppState->m_CurrentDoc.meshInstances.PushBack(pMeshSystem->AcquireMeshInstance(pEditorAppState->m_hDefaultMesh, ray.vecEnd, Vector{ 0.0f, 0.0f, 0.0f }, Vector{ 1.0f, 1.0f, 1.0f }, false));

			// Move the gizmo to the mesh.
			pEditorAppState->m_SelectedMeshIdx = pEditorAppState->m_CurrentDoc.meshInstances.Count() - 1;

			// Setup the inspector panel.
			AppUI::IAppUI* pUIContext = pEditorAppState->m_pUIContext;
			char szTextBuffer[128]{};
			Utl::StringFormat_V("%.2f %.2f %.2f", szTextBuffer, ray.vecEnd.x, ray.vecEnd.y, ray.vecEnd.z);

			pUIContext->SetControlText(pEditorAppState->translationEntry.hTextEntry, szTextBuffer);
		}
		break;
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

		if (pEditorAppState->m_SelectedMeshIdx != kInvalidID_U32)
		{
			IMeshSystem* pMeshSystem = g_pEngine->GetEngineInterfaces()->pMeshSystem;

			MeshInstance_t* pSelectedInstance = pMeshSystem->AcquireMeshInstanceDataFromHandle(pEditorAppState->m_CurrentDoc.meshInstances[pEditorAppState->m_SelectedMeshIdx]);
			Matrix4x4* transformCopy = &pSelectedInstance->matLocalToWorld;

			// Overwrite the transform with the new translation.
			transformCopy->flData[12] = pEditorAppState->m_Gizmo.origin.x;
			transformCopy->flData[13] = pEditorAppState->m_Gizmo.origin.y;
			transformCopy->flData[14] = pEditorAppState->m_Gizmo.origin.z;
		}
	}

	return true;
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
}

IToolApp* GetToolApplicationState()
{
	return &s_LevelEditorApp;
}