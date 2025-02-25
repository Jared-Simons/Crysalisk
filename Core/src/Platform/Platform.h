//=============================================================================
// Author: Jared Simons
// Date Created: 4/25/2024
// 
// Purpose: Provides the Platform interface for the application.
//=============================================================================
#pragma once

#include "Defines.h"

namespace Platform {

	struct WindowParams_t
	{
		int X;
		int Y;
		int Width;
		int Height;
		const char* Name;
	};

	//-------------------------------------------------------------------------
	// Platform Layer Message Callbacks.
	//-------------------------------------------------------------------------
	typedef void (*Platform_KeyboardEventCallback)(const int processedKeyInput, bool pressed);
	typedef void (*Platform_MouseEventCallback)(const float mouseXPos, const float mouseYPos, unsigned int mouseButtonFlags);
	typedef bool (*Platform_WindowResizedCallback)(unsigned int width, unsigned int height);

	class IPlatformWindow
	{
	public:
		virtual ~IPlatformWindow() {}

		virtual void	GetWindowSize(unsigned int& pOutWidth, unsigned int& pOutHeight) = 0;
		virtual void*	GetInternalHandle() = 0;

		virtual void	SetKeyboardEventCallback(Platform_KeyboardEventCallback callback) = 0;
		virtual void	SetMouseEventCallback(Platform_MouseEventCallback callback) = 0;
		virtual void	SetWindowResizeCallback(Platform_WindowResizedCallback callback) = 0;
	};

	//-------------------------------------------------------------------------
	// Interface and factory methods for code modules (.dlls)
	//-------------------------------------------------------------------------
	class ISysModule
	{
	public:
		virtual void* ConnectInterface(void* pInterfaceState) = 0;
		virtual void* GetFuncAddr(const char* pFuncName) = 0;
	};

	CORE_API ISysModule*	LoadSysModule(const char* pModulePath);
	CORE_API void			SetupExternalInterfaces();

	CORE_API bool			DisplayMessageBox(IPlatformWindow* pWindow, const char* pszMessageText, const char* pszMessageBoxTitle, bool bIsErrorRecoverable);

	CORE_API bool	InitializePlatformLayer();
	CORE_API void	ShutdownPlatformLayer();
	CORE_API void	ProcessMessages();

	CORE_API void	ObtainLibraryInterfaces();
	CORE_API void	ConnectLibraryInterfaces();

	CORE_API void	LogMessage(int level, const char* message);

	CORE_API double	GetAbsoluteTime();

	//-----------------------------------------------------------------------------
	// Platform memory functions
	// 
	// Do NOT call these directly, instead call through the memory manager.
	//-----------------------------------------------------------------------------
	CORE_API void* MemoryAllocate(size_t size);
	CORE_API void  MemoryFree(void* pMemoryBlock);
	CORE_API void* MemoryReallocate(void* pMemBlock, size_t newSize);
	CORE_API void* MemoryCopy(void* pDest, const void* pSource, size_t size);
	CORE_API void* MemorySet(void* pMemBlock, int value, size_t size);

	CORE_API IPlatformWindow* CreatePlatformWindow(WindowParams_t params, bool bShow);
}