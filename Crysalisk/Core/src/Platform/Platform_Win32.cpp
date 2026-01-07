//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: The Windows implementation of the platform layer.
//=============================================================================

#include "Defines.h"
#ifdef PLAT_WINDOWS

#include "Platform.h"

#include "Core/InputTypes.h"
#include "Core/Logging.h"
//#include "Interfaces.h"
#include "InterfaceRegistry.h"
#include "Strings/String_Utils.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <malloc.h>

namespace Platform 
{
	//-------------------------------------------------------------------------
	// Win32-Specific Window Implementation.
	//-------------------------------------------------------------------------
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

		virtual void GetWindowSize(unsigned int& pOutWidth, unsigned int& pOutHeight) override
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

		virtual Viewport_t GetViewport() override { return m_Viewport; }

		inline virtual void SetKeyboardEventCallback(Platform_KeyboardEventCallback callback) override { m_KeyEventCallback = callback; }
		inline virtual void SetMouseButtonEventCallback(Platform_MouseButtonEventCallback callback) override { m_MouseButtonEventCallback = callback; }
		inline virtual void	SetMouseMoveEventCallback(Platform_MouseMoveEventCallback callback) override { m_MouseMoveEventCallback = callback; }
		inline virtual void SetWindowResizeCallback(Platform_WindowResizedCallback callback) override { m_WindowResizeEventCallback = callback; }
		inline virtual void SetWindowCloseCallback(Platform_WindowCloseCallback callback) override { m_WindowOnClosedCallback = callback; }

		Viewport_t		m_Viewport;
	private:
		WindowParams_t	m_WindowParams;
		HWND			m_HWND;

		Platform_KeyboardEventCallback		m_KeyEventCallback;
		Platform_MouseButtonEventCallback	m_MouseButtonEventCallback;
		Platform_MouseMoveEventCallback		m_MouseMoveEventCallback;
		Platform_WindowResizedCallback		m_WindowResizeEventCallback;
		Platform_WindowCloseCallback		m_WindowOnClosedCallback;

		// NOTE: These are exposed to our private data because the input handlers need to
		// be able to call our callbacks into the engine's input system.
		friend static LRESULT WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		friend static int ProcessWin32Input_MouseMove(LPARAM lParam);
	};

	//-------------------------------------------------------------------------
	// Win32-Specific Platform State.
	//-------------------------------------------------------------------------
	struct Win32_InternalState
	{
		HINSTANCE hInstance = 0;
		IPlatformWindow* window = nullptr;
		Utl::CVector<CWindowsWindow*>	windows{};
		CWindowsWindow*					activeWindow{ nullptr };

		// Clock info
		inline static double m_ClockFreq = -1.0;
		inline static LARGE_INTEGER m_ClockStartTime;
	};

	static Win32_InternalState* s_pWin32PlatformData{ nullptr };
	static Utl::CVector<class CSysModule> s_ModuleRegistry{};

	// Default Window Procedure
	static LRESULT WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	static void InitializeWin32Clock();
	static int ProcessWin32Input_Key(WPARAM wParam);
	static int ProcessWin32Input_MouseButton(UINT uMsg);
	static int ProcessWin32Input_MouseMove(LPARAM lParam);

	//-------------------------------------------------------------------------
	// Purpose: Initialize the platform-specific state.
	// Input:	none.
	// Output:	bool - Returns true on successful initialization, otherwise false.
	//-------------------------------------------------------------------------
	bool InitializePlatformLayer()
	{
		if (s_pWin32PlatformData != nullptr)
		{
			LOG_WARN("PlatformInitialize() - Platform State Already Exists!");
			return true;
		}

		s_pWin32PlatformData = new Win32_InternalState;
		s_pWin32PlatformData->hInstance = GetModuleHandleA(0);
		s_pWin32PlatformData->windows.Resize(1);
		
		LOG_INFO("Platform Layer Initialized!");
		return true;
	}

	//-------------------------------------------------------------------------
	// Purpose: Shuts down platform layer.
	// Input:	none.
	// Output:	none.
	//-------------------------------------------------------------------------
	void ShutdownPlatformLayer()
	{
		// clear out allocated platform state.
		delete s_pWin32PlatformData;
		s_pWin32PlatformData = nullptr;

		// Hitting this means our entire process is shutting down,
		// so clear out our interface registry now.
		CleanupInterfaceRegistry();
		s_ModuleRegistry.Clear();
	}

	//-------------------------------------------------------------------------
	// Purpose: Factory function that creates a Win32 Window.
	// Input:	params - Parameters to configure the new window.
	//			bShow - Indicates if the window should be focused upon creation.
	// Output:	Returns a pointer to the new window
	//-------------------------------------------------------------------------
	IPlatformWindow* CreatePlatformWindow(WindowParams_t params, bool bShow)
	{
		// Add this to the windows array.
		s_pWin32PlatformData->windows.PushBack(new CWindowsWindow(params));
		CWindowsWindow* pNewWindow = s_pWin32PlatformData->windows.Back();

		if (!pNewWindow)
		{
			LOG_ERROR("Failed to allocate a new window!");
			return nullptr;
		}

		// Create the Win32 window.
		LPCSTR windowClassName = params.pszName;

		if (s_pWin32PlatformData->window == nullptr)
		{
			// Register the window with the platform state.
			//s_pWin32PlatformData->window = newWindow;
		}

		u32 windowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME;
		u32 windowExStyle = WS_EX_APPWINDOW;

		// Do some fixup on the window dimensions to account for the window border.
		int windowX = pNewWindow->GetParams().X;
		int windowY = pNewWindow->GetParams().Y;
		int windowWidth = pNewWindow->GetParams().Width;
		int windowHeight = pNewWindow->GetParams().Height;

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
			pNewWindow->GetParams().pszName,
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
		pNewWindow->SetInternalHandle(hWndInit);
		s_pWin32PlatformData->activeWindow = pNewWindow;	// TODO: Maybe this shouldn't be here.

		// Setup the window's viewport.
		pNewWindow->m_Viewport.X = params.X;
		pNewWindow->m_Viewport.Y = params.Y;
		pNewWindow->m_Viewport.Width = params.Width;
		pNewWindow->m_Viewport.Height = params.Height;
		pNewWindow->m_Viewport.flFOVDegrees = 75.0f; // TODO: Default.
		pNewWindow->m_Viewport.matProjection = Matrix4x4::Perspective(M_DEG2RAD_F(pNewWindow->m_Viewport.flFOVDegrees), ((float)pNewWindow->m_Viewport.Width / pNewWindow->m_Viewport.Height), 0.1f, 1000000.0f);

		LOG_INFO("Window created!");
		return pNewWindow;
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

	// ============================================================================
	//  Main WIN32 Event Processing.
	// ============================================================================
	static LRESULT WinProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		// CRYS_STD_LIB TODO: Fix how this is handled.
		case WM_CLOSE:
			//g_pEngineEvents->EventFire(eEventCategory::APPLICATION_QUIT, nullptr, EventData_t{});
			// NOTE: This assumes that the active window is the one being closed.
			if (s_pWin32PlatformData->activeWindow && s_pWin32PlatformData->activeWindow->m_WindowOnClosedCallback)
			{
				s_pWin32PlatformData->activeWindow->m_WindowOnClosedCallback();
			}
			break;

		case WM_SIZE:
			{
				UINT width = LOWORD(lParam);
				UINT height = HIWORD(lParam);

				if (s_pWin32PlatformData->activeWindow)
				{
					s_pWin32PlatformData->activeWindow->m_WindowResizeEventCallback(width, height);

					// This only works because we currently only have one window.
					s_pWin32PlatformData->activeWindow->m_Viewport.Width = width;
					s_pWin32PlatformData->activeWindow->m_Viewport.Height = height;
					s_pWin32PlatformData->activeWindow->m_Viewport.matProjection = Matrix4x4::Perspective(M_DEG2RAD_F(s_pWin32PlatformData->activeWindow->m_Viewport.flFOVDegrees), ((float)s_pWin32PlatformData->activeWindow->m_Viewport.Width / s_pWin32PlatformData->activeWindow->m_Viewport.Height), 0.1f, 1000000.0f);
				}
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
				if (s_pWin32PlatformData->activeWindow && s_pWin32PlatformData->activeWindow->m_KeyEventCallback)
				{
					s_pWin32PlatformData->activeWindow->m_KeyEventCallback(processedKeyCode, pressed);
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

				if (s_pWin32PlatformData->activeWindow && s_pWin32PlatformData->activeWindow->m_MouseButtonEventCallback)
				{
					s_pWin32PlatformData->activeWindow->m_MouseButtonEventCallback(processedMouseCode, pressed);
				}
			}
			break;

		case WM_MOUSEMOVE:
			ProcessWin32Input_MouseMove(lParam);
			break;

		case WM_ACTIVATE:
			if (!lParam)
				break;

			// Find and assign the active window.
			for (int iWindow = 0; iWindow < s_pWin32PlatformData->windows.Count(); iWindow++)
			{
				CWindowsWindow& pWindow = *s_pWin32PlatformData->windows[iWindow];
				bool isActive = (LOWORD(wParam) == WA_ACTIVE) || (LOWORD(wParam) == WA_CLICKACTIVE);
				HWND activeWindow = GetActiveWindow();
				if (pWindow.m_HWND && activeWindow &&
					((void*)pWindow.m_HWND == activeWindow) && isActive)
				{
					// Found the active window.
					s_pWin32PlatformData->activeWindow = &pWindow;
				}
			}
			break;
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	void InitializeWin32Clock()
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
		int mouseXPos = 0;
		int mouseYPos = 0;

		if (lParam)
		{
			mouseXPos = LOWORD(lParam);
			mouseYPos = HIWORD(lParam);
		}

		if (s_pWin32PlatformData->activeWindow && s_pWin32PlatformData->activeWindow->m_MouseMoveEventCallback)
		{
			s_pWin32PlatformData->activeWindow->m_MouseMoveEventCallback(mouseXPos, mouseYPos);
		}

		return true;
	}

	//-------------------------------------------------------------------------
	// Threads
	//-------------------------------------------------------------------------
	int GetCurrentThreadID()
	{
		return GetCurrentThreadId();
	}

	size_t GetNumberOfSystemProcessors()
	{
		SYSTEM_INFO sysInfo{};
		GetSystemInfo(&sysInfo);
		return sysInfo.dwNumberOfProcessors;
	}

	class CWin32SysThread : public ISystemThread
	{
	private:
		HANDLE	m_hThread{ 0 };

		// This needs access to our members.
		friend ISystemThread* CreateSystemThread(PFN_Thread_Entry_Point pEntryPoint, void* pParams);
		friend void DestroySystemThread(ISystemThread* pThread);
	};

	ISystemThread* CreateSystemThread(PFN_Thread_Entry_Point pEntryPoint, void* pParams)
	{
		if (!pEntryPoint)
		{
			LOG_ERROR("CreateSystemThread() was not provided with an entry point. Thread creation failed.");
			return nullptr;
		}

		// NOTE: Just heap allocating for now.
		CWin32SysThread* pThread = new CWin32SysThread;

		CAssert(pThread);

		pThread->m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)pEntryPoint, (LPVOID)pParams, 0, NULL);

		return pThread;
	}

	void DestroySystemThread(ISystemThread* pThread)
	{
		if (pThread)
		{
			CWin32SysThread* pInternalThread = reinterpret_cast<CWin32SysThread*>(pThread);
			if (pInternalThread)
			{
				CloseHandle(pInternalThread->m_hThread);
				delete pThread;
				pThread = nullptr;
			}
		}
	}

	class CWin32SysMutex : public ISystemMutex
	{
	public:
		virtual bool Lock() const override
		{
			//LOG_VERBOSE("Attempting to lock mutex");
			DWORD waitResult = WaitForSingleObject(m_hMutex, INFINITE);
			if (waitResult == WAIT_TIMEOUT)
			{
				//LOG_ERROR("Failed to obtain mutex lock!");
				return false;
			}
			//LOG_VERBOSE("Mutex lock obtained!");

			return true;
		}

		virtual void Unlock() const override
		{
			//LOG_VERBOSE("Attempting to unlock mutex");
			ReleaseMutex(m_hMutex);
		}

	private:
		HANDLE	m_hMutex{ 0 };

		// These needs access to our members.
		friend ISystemMutex* CreateSystemMutex();
		friend void DestroySystemMutex(ISystemMutex* pMutex);
	};

	ISystemMutex* CreateSystemMutex()
	{
		// NOTE: Just heap allocating for now.
		CWin32SysMutex* pMutex = new CWin32SysMutex;
		CAssert(pMutex);

		pMutex->m_hMutex = CreateMutex(NULL, FALSE, NULL);

		return pMutex;
	}

	void DestroySystemMutex(ISystemMutex* pMutex)
	{
		if (pMutex)
		{
			CWin32SysMutex* pInternalMutex = reinterpret_cast<CWin32SysMutex*>(pMutex);
			if (pInternalMutex)
			{
				CloseHandle(pInternalMutex->m_hMutex);
				delete pMutex;
				pMutex = nullptr;
			}
		}
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

	//-------------------------------------------------------------------------
	// Purpose: Logs a message to the platform-specific output.
	// Input:	iLevel - The severity of the console message.
	//			pszMessage - The message to be logged.
	// Output:	none.
	//-------------------------------------------------------------------------
	void LogMessage(int iLevel, const char* pszMessage)
	{
		HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);

		static char levelColor[6] = { 64, 4, 6, 3, 5, 8 };
		SetConsoleTextAttribute(consoleHandle, levelColor[iLevel]);
		size_t length = strlen(pszMessage);
		WriteConsoleA(consoleHandle, pszMessage, (DWORD)length, 0, 0);

		// Clear the console back to light gray after printing the message.
		SetConsoleTextAttribute(consoleHandle, 15);
	}

	//-------------------------------------------------------------------------
	// Purpose: Displays a message box, typically in response to application-crashing scenarios.
	// Input:   pWindow - A handle to the window to associate the message with.
	//			pszMessageText - The message text to display.
	//			pszMessageBoxTitle - The title for the message box.
	//			bIsErrorRecoverable - Indicates if this error should end the process.
	// Output:	bool - Returns true if the application can continue, otherwise false.
	//-------------------------------------------------------------------------
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

	//-------------------------------------------------------------------------
	// Purpose: Returns the current timestamp from platform code.
	// Input:	none.
	// Output:	double - The current timestep.
	//-------------------------------------------------------------------------
	double GetAbsoluteTime()
	{
		double elapsedTime = 0.0;

		// If the clock has not yet been initialized.
		if (s_pWin32PlatformData->m_ClockFreq == -1)
		{
			InitializeWin32Clock();
		}

		LARGE_INTEGER currentTime;
		QueryPerformanceCounter(&currentTime);

		elapsedTime = currentTime.QuadPart * s_pWin32PlatformData->m_ClockFreq;
		return elapsedTime;
	}

	//-------------------------------------------------------------------------
	// Forces the current thread to sleep for the provided milliseconds.
	//-------------------------------------------------------------------------
	void ThreadSleep(double msToSleep)
	{
		Sleep((DWORD)msToSleep);
	}

	//-------------------------------------------------------------------------
	// Interface and factory methods for code modules (.dlls)
	//-------------------------------------------------------------------------
	typedef void (*PFN_ObtainInterfaces)(Utl::CVector<SystemInterface_t>& pOutInterfaces);
	typedef void* (*PFN_ConnectInterface)(Utl::CVector<SystemInterface_t>& pInterfaceState);
	class CSysModule : public ISysModule
	{
	public:
		CSysModule() {}

		~CSysModule()
		{
			if (m_szModuleName)
				Utl::StringFree(m_szModuleName);

			m_szModuleName = nullptr;
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

	//-------------------------------------------------------------------------
	// Purpose: Attempts to load a .dll from disk.
	// Input:	pszModulePath - The path to the .dll to attempt loading.
	// Output:	A handle to the module.
	//-------------------------------------------------------------------------
	ISysModule* LoadSysModule(const char* pszModulePath)
	{
		if (!pszModulePath)
		{
			LOG_ERROR("Platform::LoadSysModule requires a pszPath to the module to load! Returning nullptr.");
			return nullptr;
		}

		CSysModule* pNewModule = new CSysModule;
		pNewModule->m_Module = LoadLibraryA(pszModulePath);

		if (!pNewModule->m_Module)
		{
			// Try to append the .dll extension to the path and load again.
			char formattedPath[10000];
			Utl::StringFormat("%s.%s", pszModulePath, "dll", formattedPath);

			pNewModule->m_Module = LoadLibraryA(formattedPath);

			// If we still have nothing then there's probably no module here.
			if (!pNewModule->m_Module)
			{
				LOG_ERROR("Failed to load '%s.dll'. LoadSysModule returning nullptr.", pszModulePath);
				delete pNewModule;
				return nullptr;
			}
		}

		// Attempt to get the module's 'ObtainInterfaces' function to get shared interfaces
		pNewModule->m_ObtainInterfacesFunc = (PFN_ObtainInterfaces)GetProcAddress(pNewModule->m_Module, "ObtainInterfaces");

		if (!pNewModule->m_ObtainInterfacesFunc)
		{
			LOG_WARN("Platform::LoadSysModule could not find required function 'ObtainInterfaces()' from library '%s.dll' the module may not work as intended!", pszModulePath);
		}

		// Attempt to get at the module's 'ConnectInterface' function to bridge the gap!
		pNewModule->m_ConnectInterfaceFunc = (PFN_ConnectInterface)GetProcAddress(pNewModule->m_Module, "ConnectInterface");

		if (!pNewModule->m_ConnectInterfaceFunc)
		{
			LOG_WARN("Platform::LoadSysModule could not find required function 'ConnectInterface()' from library '%s.dll' the module may not work as intended!", pszModulePath);
		}

		pNewModule->m_szModuleName = Utl::StringDuplicate(pszModulePath);

		// Register this module.
		s_ModuleRegistry.PushBack(pNewModule);

		return pNewModule;
	}

	//-------------------------------------------------------------------------
	// Purpose: Assigns our interfaces obtained from external libraries.
	//-------------------------------------------------------------------------
	void SetupExternalInterfaces()
	{
		// CRYS_STD_LIB TODO: Fix how this is handled.
		//::AssignInterfaces(GetRegisteredInterfaces());
	}

	//-------------------------------------------------------------------------
	// Purpose: Stores off pointers to all exposed interfaces within all loaded .dlls.
	// Input:	none.
	// Output:	none.
	//-------------------------------------------------------------------------
	void ObtainLibraryInterfaces()
	{
		for (int idx = 0; idx < s_ModuleRegistry.Count(); idx++)
		{
			if (s_ModuleRegistry[idx].m_ObtainInterfacesFunc)
			{
				Utl::CVector<SystemInterface_t> incomingInterfaces{};
				s_ModuleRegistry[idx].m_ObtainInterfacesFunc(incomingInterfaces);
				for (int iface = 0; iface < incomingInterfaces.Count(); iface++)
				{
					RegisterInterface(incomingInterfaces[iface]);
				}
			}
		}
	}

	//-------------------------------------------------------------------------
	// Purpose: Connects all previously-obtained interface pointers for all registered .dlls.
	//				After 'ObtainLibraryInterfaces()' and this, each registered .dll should have
	//				access to all registered interfaces.
	// Input:	none.
	// Output:	none.
	//-------------------------------------------------------------------------
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

	//-------------------------------------------------------------------------
	// Purpose: Allocates a block of memory from the platform allocator.
	// 
	// Input:	size - The size in bytes of the allocation to make.
	// Output:	A pointer to the allocated memory.
	//-------------------------------------------------------------------------
	void* MemoryAllocate(size_t size)
	{
		void* pMemBlock = HeapAlloc(GetProcessHeap(), 0, size);

		return pMemBlock;
	}

	//-------------------------------------------------------------------------
	// Purpose: Frees a block of memory.
	// 
	// Input:	pMemBlock - A pointer to the block of memory.
	// Output:	none.
	//-------------------------------------------------------------------------
	void MemoryFree(void* pMemBlock)
	{
		if (pMemBlock)
		{
			HeapFree(GetProcessHeap(), 0, pMemBlock);
		}
	}

	//-------------------------------------------------------------------------
	// Purpose: Attempts to re-allocate a block of memory.
	// 
	// Input:	memory - A pointer to the block of memory.
	//			newSize - The size of the new memory block.
	// Output:	none.
	//-------------------------------------------------------------------------
	void* MemoryReallocate(void* memory, size_t newSize)
	{
		if (memory && newSize)
			return realloc(memory, newSize);

		return nullptr;
	}

	//-------------------------------------------------------------------------
	// Purpose: Attempts to perform a memory copy.
	// 
	// Input:	dest - The memory location to copy to.
	//			source - The memory location to be copied.
	//			size - The size of memory in bytes to be copied.
	// Output:	A pointer to the new block of memory.
	//-------------------------------------------------------------------------
	void* MemoryCopy(void* dest, const void* source, size_t size)
	{
		if (dest && source && size)
			return memcpy(dest, source, size);

		return nullptr;
	}

	//-------------------------------------------------------------------------
	// Purpose: Sets a range of memory to a given value.
	// 
	// Input:	memory - The memory location to be set.
	//			source - The value to uniformally set to the memory block.
	//			size - The size of memory in bytes to be set.
	// Output:	A pointer to the block of memory.
	//-------------------------------------------------------------------------
	void* MemorySet(void* memory, int value, size_t size)
	{
		if (memory)
			return memset(memory, value, size);

		return nullptr;
	}
}

#endif
