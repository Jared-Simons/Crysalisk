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
	class ICrysApplication*				pApplication{ nullptr };
	class IEngine*						pEngine{ nullptr };

	// Renderer resources.
	Platform::ISysModule*				pRendererLibrary{ nullptr };
	Utl::CVector<RendererViewConfig_t>	renderViewConfigs{};

	Utl::CVector<IRenderView*>			pViews{};

	// TODO: This shouldn't happen here.
	Platform::IPlatformWindow*			pWindowToRegisterWithRenderer{ nullptr };

	~ApplicationConfig_t()
	{
	}
};

class ICrysApplication
{
public:
	virtual bool	PreInitialize(ApplicationConfig_t* pAppConfig) = 0;
	virtual bool	Initialize(ApplicationConfig_t* pAppConfig) = 0;
	virtual bool	Run() = 0;
	virtual void	FramePrepare(struct RenderPacket_t* pRenderPacket) = 0;
	virtual bool	Render() = 0;
	virtual void	Shutdown() = 0;

	virtual Camera_t*	GetMainCamera() = 0;
};
