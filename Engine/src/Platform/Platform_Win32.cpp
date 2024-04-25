//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: The Windows implementation of the platform layer.
//=============================================================================

#include "EngineGlobals.h"
#ifdef PLAT_WINDOWS

#include "Platform.h"

#include "Core/Logging.h"

#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

struct Platform_Win32_InternalData
{
	HWND hWnd = 0;
	HINSTANCE hInstance = 0;
};

static Platform_Win32_InternalData win32PlatformData;

// Windows Procedure
static LRESULT WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
const char* engineWindowClass{ "Engine_Window_Class" };

static bool	CreateWin32Window();

bool Platform_Initialize()
{
	if (win32PlatformData.hInstance != 0 && win32PlatformData.hWnd != 0)
	{
		LOG_ERROR("PlatformInitialize() - Platform State Already Exists!\n");
		return true;
	}

	if (!CreateWin32Window())
	{
		LOG_FATAL("CreateWin32Window failed! - Aborting application!");
		return false;
	}

	LOG_DEBUG("Platform Layer Initialized!");

	return true;
}

bool CreateWin32Window()
{
	// Create the window class.
	LPCSTR windowClassName = engineWindowClass;

	win32PlatformData.hInstance = GetModuleHandle(NULL);

	WNDCLASSA wc{};
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wc.lpfnWndProc = WinProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = win32PlatformData.hInstance;
	wc.hIcon = LoadIconA(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	wc.lpszMenuName = NULL;
	wc.lpszClassName = windowClassName;

	// Register the window class.
	RegisterClassA(&wc);

	// TODO: Make this configurable from the frontend.
	HWND hWndInit = CreateWindowExA(
		0,
		windowClassName,
		"Engine Testbed",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		1080,
		720,
		NULL,
		NULL,
		win32PlatformData.hInstance,
		NULL);

	if (!hWndInit)
	{
		return false;
	}

	win32PlatformData.hWnd = hWndInit;

	// Show the window
	ShowWindow(win32PlatformData.hWnd, 1);

	return true;
}

bool Platform_Update()
{
	Platform_ProcessMessages();

	return true;
}

void Platform_Shutdown()
{
}

bool Platform_ProcessMessages()
{
	MSG msg = {};
	while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
	return true;
}

LRESULT WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}
}

#endif // PLAT_WINDOWS