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
#include "Systems/IEvent.h"

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

	LOG_INFO("Platform Layer Initialized!");

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

void Platform_LogMessage(int level, const char* message)
{
	HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	// TODO: make these colors selectable per log channel (this will probably turn into a larger array/move elsewhere in the future).
	static char levelColor[6] = { 2, 3, 1, 6, 4, 64 };	// Borrowed from Kohi
	SetConsoleTextAttribute(outputHandle, levelColor[level]);
	size_t length = strlen(message);
	WriteConsoleA(outputHandle, message, length, 0, 0);
}

// ============================================================================
//  WIN32 Event Callbacks
// ============================================================================

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
		break;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		if (GetKeyState(VK_ESCAPE) & 0x8000)
		{
			// Escape pressed... exit application TODO!!! MOVE ELSEWHERE
			gpEngineEvents->EventFire(eEventCategory::EVENT_TYPE_APPLICATION_QUIT, 0);
			break;
		}
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}
}

//-----------------------------------------------------------------------------
// Platform memory functions
// 
// Do NOT call these directly, instead call through the memory manager
//-----------------------------------------------------------------------------
void* Platform_MemoryAllocate(size_t size)
{
	return malloc(size);
}

void Platform_MemoryFree(void* memory)
{
	if(memory)
		free(memory);
}

void* Platform_MemoryReallocate(void* memory, size_t newSize)
{
	if (memory && newSize)
		return realloc(memory, newSize);

	LOG_ERROR("Platform_MemoryRealloc() - Invalid memory pointer passed, returning nullptr.");
	return nullptr;
}

void* Platform_MemoryCopy(void* dest, const void* source, size_t size)
{
	if (dest && source && size)
		return memcpy(dest, source, size);

	LOG_ERROR("Platform_MemoryCopy() - Invalid memory pointer(s) passed, returning nullptr.");
	return nullptr;
}

void* Platform_MemorySet(void* memory, int value, size_t size)
{
	if (memory && value && size)
		return memset(memory, value, size);

	LOG_ERROR("Platform_MemorySet() - Invalid memory pointer(s) passed, returning nullptr.");
	return nullptr;
}

#endif // PLAT_WINDOWS