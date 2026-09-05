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

	// UI Library.
	// FIXME: Pass the plugin around some other way.
	void*								pUIContext{ nullptr };

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
	virtual bool	Run(f32 deltaTime) = 0;
	virtual void	FramePrepare(struct RenderPacket_t* pRenderPacket) = 0;
	virtual bool	Render(struct RenderPacket_t* pRenderPacket) = 0;
	virtual void	Shutdown() = 0;

	virtual void	OnWindowResized(u16 width, u16 height) = 0;

	virtual Camera_t*	GetMainCamera() = 0;
};
