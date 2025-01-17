//=============================================================================
// Author: Jared Simons
// Date Created: 5/7/2024
// 
// Purpose: The Rendering Front-end interface
//=============================================================================
#pragma once

#include "Defines.h"
#include "Core/Logging.h"

// Forward Declarations
class IRendererBackend;
struct RenderPacket_t;

struct RendererConfig_t
{
	const char* szApplicationName;
	void* pHWnd;
	Platform::ISysModule* pRendererBackendDLL;
};

class Renderer
{
public:
	bool	Initialize(RendererConfig_t& pConfig);
	void	Shutdown();
	bool	Update(struct RenderPacket_t& pRenderData);

private:
	IRendererBackend* m_RendererBackend{ nullptr };
};

extern Renderer* g_pRenderer;
