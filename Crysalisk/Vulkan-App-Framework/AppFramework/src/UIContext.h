#pragma once

#include <Defines.h>
#include "Platform/Platform.h"

namespace AppFramework
{
	struct UIAppConfig_t
	{
		// TODO: Multiple windows?
		Platform::WindowParams_t	pWindow{};
		Platform::ISysModule*		pRendererLibrary{ nullptr };
	};

	DLL_EXPORT bool CreateUIContext(UIAppConfig_t& pConfig);
	DLL_EXPORT void ShutdownUIContext();
	DLL_EXPORT void RunUIContext();

	DLL_EXPORT Platform::IPlatformWindow* GetActiveWindow();

	// TODO: Make this more elegant.
	DLL_EXPORT bool ShouldRunUIContext();
	DLL_EXPORT void SetUIContextQuitting(bool bQuitting);
	void SetMousePosition(int xPos, int yPos);
	void GetMousePosition(int& pXPos, int& pYPos);
	void SetLeftMousePressed(unsigned int mouseButtonFlags, bool bPressed);
}
