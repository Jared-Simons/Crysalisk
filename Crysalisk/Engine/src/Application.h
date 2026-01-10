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

	~ApplicationConfig_t()
	{
		renderViewConfigs.Clear();
	}
};

class ICrysApplication
{
public:
	virtual bool	PreInitialize(ApplicationConfig_t* pAppConfig) = 0;
	virtual bool	Initialize(ApplicationConfig_t* pAppConfig) = 0;
	virtual bool	Run() = 0;
	virtual void	Shutdown() = 0;

	virtual void	FramePrepare(struct RenderPacket_t* pRenderPacket) = 0;
};
