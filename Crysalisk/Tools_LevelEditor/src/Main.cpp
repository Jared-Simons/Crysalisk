//=============================================================================
// Author: Jared Simons
// Date Created: 12/17/2024
// 
// Purpose: The entrypoint to the level-editor.
//=============================================================================

// Core
#include <Containers/CVector.h>
#include <Core/Logging.h>

// EditorHub
#include <ToolsTypes.h>

// Engine
#include <Application.h>
#include <Core/Engine.h>
#include <Systems/DebugGeometrySystem.h>

class CLevelEditorApp : public IToolApp
{
public:
	virtual bool	Tool_Init(struct ApplicationConfig_t* pAppConfig) override;
	virtual bool	Tool_Run() override;
	virtual void	Tool_Shutdown() override;

private:
	Utl::CVector<i32>	m_GridLines{};
	i32					m_OriginCross[3]{};
};

static CLevelEditorApp s_LevelEditorApp{};

extern "C"
{
	__declspec(dllexport) IToolApp* GetToolApplicationState();
}

bool CLevelEditorApp::Tool_Init(struct ApplicationConfig_t* pAppConfig)
{
	LOG_INFO("Welcome to Crysalisk Level Editor!");

	// Setup world grid.
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

	return true;
}

bool CLevelEditorApp::Tool_Run()
{
	//LOG_VERBOSE("Running Level Editor Loop!");
	return true;
}

void CLevelEditorApp::Tool_Shutdown()
{
	// Destroy tool geometry.
	IDebugGeometrySystem* pDebugGeo = g_pEngine->GetEngineInterfaces()->pDebugGeometry;
}

IToolApp* GetToolApplicationState()
{
	return &s_LevelEditorApp;
}
