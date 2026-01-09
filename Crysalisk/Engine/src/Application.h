//=============================================================================
// Author: Jared Simons
// Date Created: 12/17/2025
// 
// Purpose: The interface for an application that uses the Crysalisk Engine.
//=============================================================================
#pragma once

#include <Platform/Platform.h>
#include <Renderer/RendererTypes.h>

struct ApplicationConfig_t
{
	class ICrysApplication* pApplication{ nullptr };

	// Renderer resources.
	Platform::ISysModule*				pRendererLibrary{ nullptr };
	Utl::CVector<RendererViewConfig_t>	renderViewConfigs{};
};

class ICrysApplication
{
public:
	virtual bool Application_Init(ApplicationConfig_t* pAppConfig) = 0;
	virtual bool Application_Run() = 0;
	virtual void Application_Shutdown() = 0;

	virtual void FramePrepare(struct RenderPacket_t* pRenderPacket) = 0;
};
