//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Provides the Platform interface for the application.
//=============================================================================
#pragma once

#include "Defines.h"
#include "Math/MathLib.h"

namespace Platform 
{
	//-------------------------------------------------------------------------
	// Configuration for a new window.
	//-------------------------------------------------------------------------
	struct WindowParams_t
	{
		// The starting X position of the window.
		int X;
		// The starting Y position of the window.
		int Y;
		// The starting width of the window.
		int Width;
		// The starting height of the window.
		int Height;
		// The name of the window.
		const char* pszName;
	};

	//-------------------------------------------------------------------------
	// Platform Layer Message Callbacks.
	//-------------------------------------------------------------------------
	typedef void (*Platform_KeyboardEventCallback)(const int processedKeyInput, bool pressed);
	typedef void (*Platform_MouseButtonEventCallback)(u32 mouseButtonFlags, bool bPressed);
	typedef void (*Platform_MouseMoveEventCallback)(const int mouseXPos, const int mouseYPos);
	typedef bool (*Platform_WindowResizedCallback)(u32 width, u32 height);
	typedef void (*Platform_WindowCloseCallback)();

	//-------------------------------------------------------------------------
	// A viewport that can be associated with a window.
	//-------------------------------------------------------------------------
	struct Viewport_t
	{
		int			X{};
		int			Y{};
		int			Width{};
		int			Height{};
		f32		flFOVDegrees{};
		Matrix4x4	matProjection{};
	};

	//-------------------------------------------------------------------------
	// Interface for a platform-specific window.
	//-------------------------------------------------------------------------
	class IPlatformWindow
	{
	public:
		virtual ~IPlatformWindow() {}

		virtual void	GetWindowSize(u32& pOutWidth, u32& pOutHeight) = 0;
		virtual void*	GetInternalHandle() = 0;

		virtual Viewport_t GetViewport() = 0;

		virtual void	SetKeyboardEventCallback(Platform_KeyboardEventCallback callback) = 0;
		virtual void	SetMouseButtonEventCallback(Platform_MouseButtonEventCallback callback) = 0;
		virtual void	SetMouseMoveEventCallback(Platform_MouseMoveEventCallback callback) = 0;
		virtual void	SetWindowResizeCallback(Platform_WindowResizedCallback callback) = 0;
		virtual void	SetWindowCloseCallback(Platform_WindowCloseCallback callback) = 0;

		// Renderer-specific state needed for window rendering.
		void*			m_pRendererState{ nullptr };
	};

	//-------------------------------------------------------------------------
	// Interface and factory methods for code modules (.dlls)
	//-------------------------------------------------------------------------
	class ISysModule
	{
	public:
		virtual void* GetFuncAddr(const char* pFuncName) = 0;
	};

	CORE_API ISysModule*	LoadSysModule(const char* pModulePath);

	//-------------------------------------------------------------------------
	// Threads
	//-------------------------------------------------------------------------
	CORE_API int		GetCurrentThreadID();
	CORE_API u64		GetNumberOfSystemProcessors();

	class ISystemThread
	{
	public:
		u8 m_iThreadID{ kInvalidID_U8 };
	};

	// Function pointer signature for thread entry points.
	typedef int (*PFN_Thread_Entry_Point)(void* pParams);

	CORE_API ISystemThread* CreateSystemThread(PFN_Thread_Entry_Point pEntryPoint, void* pParams);
	CORE_API void			DestroySystemThread(ISystemThread* pThread);

	class ISystemMutex
	{
	public:
		virtual bool Lock() const = 0;
		virtual void Unlock() const = 0;
	};

	CORE_API ISystemMutex*	CreateSystemMutex();
	CORE_API void			DestroySystemMutex(ISystemMutex* pMutex);

	CORE_API bool	DisplayMessageBox(IPlatformWindow* pWindow, const char* pszMessageText, const char* pszMessageBoxTitle, bool bIsErrorRecoverable);

	CORE_API bool	InitializePlatformLayer();
	CORE_API void	ShutdownPlatformLayer();
	CORE_API void	ProcessMessages();

	CORE_API void	ObtainLibraryInterfaces();
	CORE_API void	ConnectLibraryInterfaces();

	CORE_API void	LogMessage(int level, const char* message);

	CORE_API f64	GetAbsoluteTime();
	CORE_API void	ThreadSleep(f64 msToSleep);

	//-----------------------------------------------------------------------------
	// Platform memory functions
	// 
	// Do NOT call these directly, instead call through the memory manager.
	//-----------------------------------------------------------------------------
	CORE_API void* MemoryAllocate(u64 size);
	CORE_API void  MemoryFree(void* pMemoryBlock);
	CORE_API void* MemoryReallocate(void* pMemBlock, u64 newSize);
	CORE_API void* MemoryCopy(void* pDest, const void* pSource, u64 size);
	CORE_API void* MemorySet(void* pMemBlock, int value, u64 size);

	CORE_API IPlatformWindow* CreatePlatformWindow(WindowParams_t params, bool bShow);
}