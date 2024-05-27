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
#include "Systems/IInput.h"

#include <Windows.h>
#define WIN32_LEAN_AND_MEAN

// For Vulkan Surface Creation
#include "Renderer/Vulkan/Platform/VulkanPlatform.h"

struct Platform_Win32_InternalData
{
	HWND hWnd = 0;
	HINSTANCE hInstance = 0;

	inline static double m_ClockFreq = -1;
	inline static LARGE_INTEGER m_ClockStartTime;
};

static Platform_Win32_InternalData win32PlatformData;

// Windows Procedure
static LRESULT WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
const char* engineWindowClass{ "Engine_Window_Class" };

static bool	CreateWin32Window();
static bool CreateWin32Clock();
static int ProcessWin32Input_Key(WPARAM wParam);
static int ProcessWin32Input_MouseButton(UINT uMsg);
static int ProcessWin32Input_MouseMove(LPARAM lParam);

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

	// Initialize clock frequency
	if (!CreateWin32Clock())
	{
		LOG_FATAL("CreateWin32Clock failed! - Aborting application!");
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

bool CreateWin32Clock()
{
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	win32PlatformData.m_ClockFreq = 1.0 / frequency.QuadPart;
	QueryPerformanceCounter(&win32PlatformData.m_ClockStartTime);

	return true;
}

void Platform_Shutdown()
{
	LOG_INFO("Platform_Shutdown called");

	if (win32PlatformData.hWnd)
	{
		DestroyWindow(win32PlatformData.hWnd);

		win32PlatformData.hWnd = 0;
		win32PlatformData.hInstance = 0;
	}
}

bool Platform_Update()
{
	Platform_ProcessMessages();

	return true;
}

double Platform_GetAbsoluteTime()
{
	// If the clock has not yet been initialized (possibly due to headless mode)
	// initialize it here
	if (win32PlatformData.m_ClockFreq == -1)
	{
		// 2 = Debug level message
		Platform_LogMessage(1, "Platform Clock not yet initialized - Initializing now\n");
		CreateWin32Clock();
	}

	LARGE_INTEGER currentTime;
	QueryPerformanceCounter(&currentTime);

	double elapsedTime = (double)currentTime.QuadPart * win32PlatformData.m_ClockFreq;
	return elapsedTime;
}

// These are just here for reference until we have customizable logging channels (they will probably then be moved elsewhere)
/*
#define BLACK			0
#define BLUE			1
#define GREEN			2
#define CYAN			3
#define RED				4
#define MAGENTA			5
#define BROWN			6
#define LIGHTGRAY		7
#define DARKGRAY		8
#define LIGHTBLUE		9
#define LIGHTGREEN		10
#define LIGHTCYAN		11
#define LIGHTRED		12
#define LIGHTMAGENTA	13
#define YELLOW			14
#define WHITE			15
*/

void Platform_LogMessage(int level, const char* message)
{
	HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	// TODO: make these colors selectable per log channel (this will probably turn into a larger array/move elsewhere in the future).
	static char levelColor[6] = { 8, 3, 5, 6, 4, 64 };	// Borrowed from Kohi
	SetConsoleTextAttribute(outputHandle, levelColor[level]);
	size_t length = strlen(message);
	WriteConsoleA(outputHandle, message, length, 0, 0);
	// Clear the console back to light gray after printing the message
	SetConsoleTextAttribute(outputHandle, 15);
}

VkWin32SurfaceCreateInfoKHR Platform_GetVulkanSurface()
{
	VkWin32SurfaceCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
	createInfo.hwnd = win32PlatformData.hWnd;
	createInfo.hinstance = win32PlatformData.hInstance;
	return createInfo;
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
		else
		{
			// Prepare message data for input processing
			int processedKeyCode = ProcessWin32Input_Key(wParam);

			// Send to the input system.
			gpInputSystem->ProcessKeyInput(processedKeyCode, true);
		}
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
		{
			// Prepare message data for input processing
			int processedKeyCode = ProcessWin32Input_Key(wParam);

			// Send to the input system.
			gpInputSystem->ProcessKeyInput(processedKeyCode, false);
		}
		break;

	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	{
		int processedMouseCode = ProcessWin32Input_MouseButton(uMsg);
		gpInputSystem->ProcessMouseInput(NULL, NULL, processedMouseCode, false);
		break;
	}

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	{
		int processedMouseCode = ProcessWin32Input_MouseButton(uMsg);
		gpInputSystem->ProcessMouseInput(NULL, NULL, processedMouseCode, true);
		break;
	}

	case WM_MOUSEMOVE:
		ProcessWin32Input_MouseMove(lParam);
		break;

	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// ============================================================================
//  WIN32 Input Processing
// ============================================================================
int ProcessWin32Input_Key(WPARAM wParam)
{
	switch (wParam)
	{
	case 0x41:
		return KEY_A;
		break;
	case 0x42:
		return KEY_B;
		break;
	case 0x43:
		return KEY_C;
		break;
	case 0x44:
		return KEY_D;
		break;
	case 0x45:
		return KEY_E;
		break;
	case 0x46:
		return KEY_F;
		break;
	case 0x47:
		return KEY_G;
		break;
	case 0x48:
		return KEY_H;
		break;
	case 0x49:
		return KEY_I;
		break;
	case 0x4A:
		return KEY_J;
		break;
	case 0x4B:
		return KEY_K;
		break;
	case 0x4C:
		return KEY_L;
		break;
	case 0x4D:
		return KEY_M;
		break;
	case 0x4E:
		return KEY_N;
		break;
	case 0x4F:
		return KEY_O;
		break;
	case 0x50:
		return KEY_P;
		break;
	case 0x51:
		return KEY_Q;
		break;
	case 0x52:
		return KEY_R;
		break;
	case 0x53:
		return KEY_S;
		break;
	case 0x54:
		return KEY_T;
		break;
	case 0x55:
		return KEY_U;
		break;
	case 0x56:
		return KEY_V;
		break;
	case 0x57:
		return KEY_W;
		break;
	case 0x58:
		return KEY_X;
		break;
	case 0x59:
		return KEY_Y;
		break;
	case 0x5A:
		return KEY_Z;
		break;
	default:
		return KEY_UNDEFINED;
		break;
	}
}

int ProcessWin32Input_MouseButton(UINT uMsg)
{
	switch (uMsg)
	{
	case 513:
		return MOUSE_BUTTON_LEFTDOWN;
		break;
	case 514:
		return MOUSE_BUTTON_LEFTUP;
		break;
	case 516:
		return MOUSE_BUTTON_RIGHTDOWN;
		break;
	case 517:
		return MOUSE_BUTTON_RIGHTUP;
		break;
	case 519:
		return MOUSE_BUTTON_MIDDLEDOWN;
		break;
	case 520:
		return MOUSE_BUTTON_MIDDLEUP;
		break;
	default:
		return MOUSE_BUTTON_UNDEFINED;
		break;
	}
}

int ProcessWin32Input_MouseMove(LPARAM lParam)
{
	float mouseXPos = 0;
	float mouseYPos = 0;

	if (lParam)
	{
		mouseXPos = LOWORD(lParam);
		mouseYPos = HIWORD(lParam);
	}

	gpInputSystem->ProcessMouseInput(mouseXPos, mouseYPos, NULL, NULL);
	return true;
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
	if (memory && size)
		return memset(memory, value, size);

	LOG_ERROR("Platform_MemorySet() - Invalid memory pointer(s) passed, returning nullptr.");
	return nullptr;
}

#endif // PLAT_WINDOWS