#pragma once

#include "Platform/Platform.h"

namespace AppFramework
{
	struct UIAppConfig_t
	{
		// TODO: Multiple windows?
		Platform::WindowParams_t	pWindow{};
		Platform::ISysModule*		pRendererLibrary{ nullptr };
	};

	bool CreateUIContext(UIAppConfig_t& pConfig);
	void ShutdownUIContext();
	void RunUIContext();

	// TODO: Make this more elegant.
	bool ShouldRunUIContext();
	void SetUIContextQuitting(bool bQuitting);
	void SetMousePosition(int xPos, int yPos);
	void GetMousePosition(int& pXPos, int& pYPos);
	void SetLeftMousePressed(unsigned int mouseButtonFlags, bool bPressed);
}
