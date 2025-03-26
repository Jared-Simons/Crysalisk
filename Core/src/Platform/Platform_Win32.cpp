//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: The Windows implementation of the platform layer.
//=============================================================================

#include "Defines.h"
#ifdef PLAT_WINDOWS

#include "Platform.h"

#include <Core/Event.h>
#include "Core/InputTypes.h"
#include "Core/Logging.h"
#include "Interfaces.h"
#include "InterfaceRegistry.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <malloc.h>

namespace Platform {

	struct Platform_Win32_InternalData
	{
		HINSTANCE hInstance = 0;
		IPlatformWindow* window = nullptr; // TODO: Make this an array where the windows are owned.

		// Clock info
		inline static double m_ClockFreq = -1.0;
		inline static LARGE_INTEGER m_ClockStartTime;
	};

	static Platform_Win32_InternalData* s_pWin32PlatformData{ nullptr };
	static Utl::CVector<class CSysModule> s_ModuleRegistry{};

	class CWindowsWindow : public IPlatformWindow
	{
	public:
		CWindowsWindow() : m_WindowParams({ 0, 0, 800, 600, "Unnamed Window!" }), m_HWND{ 0 } {}
		CWindowsWindow(WindowParams_t params) : m_WindowParams{ params }, m_HWND{ 0 } {}

		virtual ~CWindowsWindow()
		{
			LOG_DEBUG("Deleting Window!");
			if (m_HWND)
			{
				DestroyWindow(m_HWND);
			}
		}

		virtual void	GetWindowSize(unsigned int& pOutWidth, unsigned int& pOutHeight) override
		{
			RECT rect;
			if (GetClientRect(m_HWND, &rect))
			{
				int width = rect.right - rect.left;
				int height = rect.bottom - rect.top;

				pOutWidth = width;
				pOutHeight = height;
			}
		}

		virtual void*	GetInternalHandle() override { return m_HWND; }

		inline WindowParams_t GetParams() const { return m_WindowParams; }
		inline void SetInternalHandle(HWND hWnd) { m_HWND = hWnd; }
		inline virtual void SetKeyboardEventCallback(Platform_KeyboardEventCallback callback) override { m_KeyEventCallback = callback; }
		inline virtual void SetMouseButtonEventCallback(Platform_MouseButtonEventCallback callback) override { m_MouseButtonEventCallback = callback; }
		inline virtual void	SetMouseMoveEventCallback(Platform_MouseMoveEventCallback callback) override { m_MouseMoveEventCallback = callback; }
		inline virtual void SetWindowResizeCallback(Platform_WindowResizedCallback callback) override { m_WindowResizeEventCallback = callback; }

	private:
		WindowParams_t m_WindowParams;
		HWND m_HWND;

	// TODO: Make these private (and handle these events per window).
	public:
		Platform_KeyboardEventCallback m_KeyEventCallback; // TODO: Make this an array so multiple contexts can register as listeners.
		Platform_MouseButtonEventCallback m_MouseButtonEventCallback;
		Platform_MouseMoveEventCallback m_MouseMoveEventCallback;
		Platform_WindowResizedCallback m_WindowResizeEventCallback;
	};

	CWindowsWindow* s_pMainWindow; // FIXME: Get rid of this once we have a better solution, this should live nor be owned here.

	// Default Window Procedure
	static LRESULT WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static void CreateWin32Clock();
	static int ProcessWin32Input_Key(WPARAM wParam);
	static int ProcessWin32Input_MouseButton(UINT uMsg);
	static int ProcessWin32Input_MouseMove(LPARAM lParam);

	bool InitializePlatformLayer()
	{
		if (s_pWin32PlatformData != nullptr)
		{
			LOG_WARN("PlatformInitialize() - Platform State Already Exists!");
			return true;
		}

		s_pWin32PlatformData = new Platform_Win32_InternalData;
		s_pWin32PlatformData->hInstance = GetModuleHandleA(0);
		
		LOG_INFO("Platform Layer Initialized!");
		return true;
	}

	void ShutdownPlatformLayer()
	{
		// clear out allocated platform state.
		delete s_pWin32PlatformData;
		s_pWin32PlatformData = nullptr;

		// Hitting this means our entire process is shutting down,
		// so clear out our interface registry now.
		CleanupInterfaceRegistry();
	}

	IPlatformWindow* CreatePlatformWindow(WindowParams_t params, bool bShow)
	{
		CWindowsWindow* newWindow = new CWindowsWindow(params);

		if (!newWindow)
		{
			LOG_ERROR("Failed to allocate a new window!");
			return nullptr;
		}

		// Create the Win32 window.
		//LPCSTR windowClassName = params.Name;
		LPCSTR windowClassName = "MyEngine";

		if (s_pWin32PlatformData->window == nullptr)
		{
			// Register the window with the platform state.
			s_pWin32PlatformData->window = newWindow;
		}

		u32 windowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME;
		u32 windowExStyle = WS_EX_APPWINDOW;

		// Do some fixup on the window dimensions to account for the window border.
		int windowX = newWindow->GetParams().X;
		int windowY = newWindow->GetParams().Y;
		int windowWidth = newWindow->GetParams().Width;
		int windowHeight = newWindow->GetParams().Height;

		RECT borderRect = { 0 };
		AdjustWindowRectEx(&borderRect, windowStyle, 0, windowExStyle);

		windowX += borderRect.left;
		windowY += borderRect.top;

		windowWidth += borderRect.right - borderRect.left;
		windowHeight += borderRect.bottom - borderRect.top;

		WNDCLASSA wc{};
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wc.lpfnWndProc = WinProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = s_pWin32PlatformData->hInstance;
		wc.hIcon = LoadIconA(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
		wc.lpszMenuName = NULL;
		wc.lpszClassName = windowClassName;

		// Register the window class.
		RegisterClassA(&wc);

		HWND hWndInit = CreateWindowExA(
			0,
			windowClassName,
			newWindow->GetParams().Name,
			windowStyle,
			windowX,
			windowY,
			windowWidth,
			windowHeight,
			NULL,
			NULL,
			s_pWin32PlatformData->hInstance,
			NULL);

		if (!hWndInit)
		{
			DWORD error = GetLastError();
			LOG_ERROR("Failed to create Win32 Window! Error code: %ul", error);
			return nullptr;
		}

		if (bShow)
		{
			ShowWindow(hWndInit, SW_SHOW);
		}
		newWindow->SetInternalHandle(hWndInit);
		s_pMainWindow = newWindow; // FIXME: This shouldn't be here.

		LOG_INFO("Window created!");
		return newWindow;
	}

	// ============================================================================
	//  WIN32 Event Callbacks
	// ============================================================================
	void ProcessMessages()
	{
		MSG msg = {};
		while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}


	static LRESULT WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_CLOSE:
			g_pEngineEvents->EventFire(eEventCategory::APPLICATION_QUIT, nullptr, EventData_t{});
			break;

		case WM_SIZE:
			{
				UINT width = LOWORD(lParam);
				UINT height = HIWORD(lParam);

				if (s_pMainWindow)
					s_pMainWindow->m_WindowResizeEventCallback(width, height);
			}
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
			{
				// Prepare message data for input processing
				int processedKeyCode = ProcessWin32Input_Key(wParam);
				bool pressed = (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN);

				// Send it to our listeners.
				if (s_pMainWindow && s_pMainWindow->m_KeyEventCallback)
				{
					s_pMainWindow->m_KeyEventCallback(processedKeyCode, pressed);
				}
			}
			break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
			{
				int processedMouseCode = ProcessWin32Input_MouseButton(uMsg);
				bool pressed = (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_MBUTTONDOWN);

				if (s_pMainWindow && s_pMainWindow->m_MouseButtonEventCallback)
				{
					s_pMainWindow->m_MouseButtonEventCallback(processedMouseCode, pressed);
				}
			}
			break;

		case WM_MOUSEMOVE:
			ProcessWin32Input_MouseMove(lParam);
			break;
		}


		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	void CreateWin32Clock()
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		s_pWin32PlatformData->m_ClockFreq = 1.0 / frequency.QuadPart;
		QueryPerformanceCounter(&s_pWin32PlatformData->m_ClockStartTime);
	}

	// ============================================================================
	//  WIN32 Input Processing
	// ============================================================================
	int ProcessWin32Input_Key(WPARAM wParam)
	{
		switch (wParam)
		{
		case 0x11:
			return KEY_CTRL;
			break;

		case 0x1B:
			return KEY_ESCAPE;
			break;

		case 0x20:
			return KEY_SPACE;
			break;

		case 0x25:
			return KEY_LEFT;
			break;
		case 0x26:
			return KEY_UP;
			break;
		case 0x27:
			return KEY_RIGHT;
			break;
		case 0x28:
			return KEY_DOWN;
			break;

		case 0x30:
			return KEY_0;
			break;
		case 0x31:
			return KEY_1;
			break;
		case 0x32:
			return KEY_2;
			break;
		case 0x33:
			return KEY_3;
			break;
		case 0x34:
			return KEY_4;
			break;
		case 0x35:
			return KEY_5;
			break;
		case 0x36:
			return KEY_6;
			break;
		case 0x37:
			return KEY_7;
			break;
		case 0x38:
			return KEY_8;
			break;
		case 0x39:
			return KEY_9;
			break;

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
		case 514:
			return MOUSE_BUTTON_LEFT;
			break;
		case 516:
		case 517:
			return MOUSE_BUTTON_RIGHT;
			break;
		case 519:
		case 520:
			return MOUSE_BUTTON_MIDDLE;
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

		if (s_pMainWindow && s_pMainWindow->m_MouseButtonEventCallback)
		{
			s_pMainWindow->m_MouseMoveEventCallback(mouseXPos, mouseYPos);
		}

		return true;
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

	void LogMessage(int level, const char* message)
	{
		HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

		static char levelColor[6] = { 64, 4, 6, 3, 5, 8 };
		SetConsoleTextAttribute(consoleHandle, levelColor[level]);
		size_t length = strlen(message);
		WriteConsoleA(consoleHandle, message, (DWORD)length, 0, 0);

		// Clear the console back to light gray after printing the message.
		SetConsoleTextAttribute(consoleHandle, 15);
	}

	double GetAbsoluteTime()
	{
		double elapsedTime = 0.0;

		// If the clock has not yet been initialized.
		if (s_pWin32PlatformData->m_ClockFreq == -1)
		{
			CreateWin32Clock();
		}

		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);

		elapsedTime = currentTime.QuadPart * s_pWin32PlatformData->m_ClockFreq;
		return elapsedTime;
	}

	//-------------------------------------------------------------------------
	// Interface and factory methods for code modules (.dlls)
	//-------------------------------------------------------------------------
	typedef Utl::CVector<SystemInterface_t>(*PFN_ObtainInterfaces)();
	typedef void* (*PFN_ConnectInterface)(Utl::CVector<SystemInterface_t>& pInterfaceState);
	class CSysModule : public ISysModule
	{
	public:
		CSysModule() {}

		~CSysModule()
		{
			if (m_szModuleName)
				free(m_szModuleName);
		}

		virtual void* ConnectInterface(void* pInterfaceState) override
		{
			if (!m_ConnectInterfaceFunc)
				return false;

			return m_ConnectInterfaceFunc(GetRegisteredInterfaces());
		}

		void* GetFuncAddr(const char* pFuncName) override
		{
			return GetProcAddress(m_Module, pFuncName);
		}

	private:
		CSysModule(CSysModule& pOther) {}; // No copying!

	public:
		// These can be public, they can't be touched by another file w/o the windows headers anyways ¯\_(ツ)_/¯
		HMODULE	m_Module;
		PFN_ObtainInterfaces m_ObtainInterfacesFunc;
		PFN_ConnectInterface m_ConnectInterfaceFunc;
		char* m_szModuleName;
	};

	ISysModule* LoadSysModule(const char* pModulePath)
	{
		if (!pModulePath)
		{
			LOG_ERROR("Platform::LoadSysModule requires a path to the module to load! Returning nullptr.");
			return nullptr;
		}

		CSysModule* pNewModule = new CSysModule;
		pNewModule->m_Module = LoadLibraryA(pModulePath);

		if (!pNewModule->m_Module)
		{
			// Try to append the .dll extension to the path and load again.
			char formattedPath[512];
			StringFormat("%s.%s", pModulePath, "dll", formattedPath);

			pNewModule->m_Module = LoadLibraryA(formattedPath);

			// If we still have nothing then there's probably no module here.
			if (!pNewModule->m_Module)
			{
				LOG_ERROR("Failed to load '%s.dll'. LoadSysModule returning nullptr.", pModulePath);
				delete pNewModule;
				return nullptr;
			}
		}

		// Attempt to get the module's 'ObtainInterfaces' function to get shared interfaces
		pNewModule->m_ObtainInterfacesFunc = (PFN_ObtainInterfaces)GetProcAddress(pNewModule->m_Module, "ObtainInterfaces");

		if (!pNewModule->m_ObtainInterfacesFunc)
		{
			LOG_WARN("Platform::LoadSysModule could not find required function 'ObtainInterfaces()' from library '%s.dll' the module may not work as intended!", pModulePath);
		}

		// Attempt to get at the module's 'ConnectInterface' function to bridge the gap!
		pNewModule->m_ConnectInterfaceFunc = (PFN_ConnectInterface)GetProcAddress(pNewModule->m_Module, "ConnectInterface");

		if (!pNewModule->m_ConnectInterfaceFunc)
		{
			LOG_WARN("Platform::LoadSysModule could not find required function 'ConnectInterface()' from library '%s.dll' the module may not work as intended!", pModulePath);
		}

		pNewModule->m_szModuleName = StringDuplicate(pModulePath);

		// Register this module.
		s_ModuleRegistry.PushBack(pNewModule);

		return pNewModule;
	}

	void SetupExternalInterfaces()
	{
		::AssignInterfaces(GetRegisteredInterfaces());
	}

	bool DisplayMessageBox(IPlatformWindow* pWindow, const char* pszMessageText, const char* pszMessageBoxTitle, bool bIsErrorRecoverable)
	{
		int messageBoxResult = 0;
		if (pWindow)
		{
			messageBoxResult = MessageBoxExA((HWND)pWindow->GetInternalHandle(), pszMessageText, pszMessageBoxTitle, (bIsErrorRecoverable ? MB_CANCELTRYCONTINUE : MB_OK), 0);
		}
		else
		{
			messageBoxResult = MessageBoxExA(0, pszMessageText, pszMessageBoxTitle, (bIsErrorRecoverable ? MB_CANCELTRYCONTINUE : MB_OK), 0);
		}
		return (messageBoxResult == IDCONTINUE);
	}

	void ObtainLibraryInterfaces()
	{
		for (int idx = 0; idx < s_ModuleRegistry.Count(); idx++)
		{
			if (s_ModuleRegistry[idx].m_ObtainInterfacesFunc)
			{
				Utl::CVector<SystemInterface_t> incomingInterfaces = s_ModuleRegistry[idx].m_ObtainInterfacesFunc();
				for (int iface = 0; iface < incomingInterfaces.Count(); iface++)
				{
					RegisterInterface(incomingInterfaces[iface]);
				}
			}
		}
	}

	void ConnectLibraryInterfaces()
	{
		for (int idx = 0; idx < s_ModuleRegistry.Count(); idx++)
		{
			if (s_ModuleRegistry[idx].m_ConnectInterfaceFunc)
			{
				s_ModuleRegistry[idx].m_ConnectInterfaceFunc(GetRegisteredInterfaces());
			}
		}
	}

	//-----------------------------------------------------------------------------
	// Platform memory functions
	//-----------------------------------------------------------------------------
	void* MemoryAllocate(size_t size)
	{
		void* pMemBlock = HeapAlloc(GetProcessHeap(), 0, size);

		return pMemBlock;
	}

	void MemoryFree(void* pMemBlock)
	{
		if (pMemBlock)
		{
			HeapFree(GetProcessHeap(), 0, pMemBlock);
		}
	}

	void* MemoryReallocate(void* memory, size_t newSize)
	{
		if (memory && newSize)
			return realloc(memory, newSize);

		//LOG_ERROR("Platform_MemoryRealloc() - Invalid memory pointer passed, returning nullptr.");
		return nullptr;
	}

	void* MemoryCopy(void* dest, const void* source, size_t size)
	{
		if (dest && source && size)
			return memcpy(dest, source, size);

		//LOG_ERROR("Platform_MemoryCopy() - Invalid memory pointer(s) passed, returning nullptr.");
		return nullptr;
	}

	void* MemorySet(void* memory, i32 value, size_t size)
	{
		if (memory)
			return memset(memory, value, size);

		//LOG_ERROR("Platform_MemorySet() - Invalid memory pointer(s) passed, returning nullptr.");
		return nullptr;
	}
}

#endif