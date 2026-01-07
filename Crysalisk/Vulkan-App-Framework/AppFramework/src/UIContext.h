#pragma once

#include "Platform/Platform.h"

namespace AppFramework
{
	struct UI_App_Config_t
	{
		// TODO: Multiple windows?
		Platform::WindowParams_t	pWindow{};
	};

	bool CreateUIContext(AppFramework::UI_App_Config_t& pConfig);
	void ShutdownUIContext();
	void RunUIContext();

	// TODO: Make this more elegant.
	bool ShouldRunUIContext();
	void SetUIContextQuitting(bool bQuitting);
	void SetMousePosition(int xPos, int yPos);
	void GetMousePosition(int& pXPos, int& pYPos);
	void SetLeftMousePressed(unsigned int mouseButtonFlags, bool bPressed);
}
