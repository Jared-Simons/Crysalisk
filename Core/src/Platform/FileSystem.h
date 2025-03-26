//=============================================================================
// Author: Jared Simons
// Date Created: 5/12/2024
// 
// Purpose: The interface for Filesystems.
//=============================================================================
#pragma once

#include "Defines.h"

// String buffers should be this wide when copying into them.
const int kScratchStringBufferLength = 10000;

enum eFileMode
{
	FILEMODE_UNKNOWN = 0,
	FILEMODE_READ,
	FILEMODE_WRITE,

	FILEMODE_MAXMODES
};

struct CORE_API FileHandle_t
{
	char* path{};
	bool isValid{ false };
	eFileMode fileAccessMode{ FILEMODE_UNKNOWN };
	void* internalData{ nullptr };

	~FileHandle_t();
};

class CORE_API FileSystem
{
public:
	static bool			OpenFile(const char* filePath, eFileMode fileMode, bool bIsBinary, FileHandle_t* pOutFileHandle);
	static bool			CloseFile(FileHandle_t* fileHandle);
	static bool			FileExists(const char* pszFilePath);
	static u64			FileSize(FileHandle_t* fileHandle);

	static bool 		ReadLine(char* pBuffer, i32 bufferSize, u64* pOutBytesRead, FileHandle_t* file);
	static bool			ReadWholeFile(char* pBuffer, u64 bufferSize, u64* pOutBytesRead, FileHandle_t* file);
	static bool			ReadWholeBinaryFile(byte* pBuffer, u64 bufferSize, u64* pOutBytesRead, FileHandle_t* file);
	static bool			IsEOF(FileHandle_t* file);

	static void			WriteLine(char* pBuffer, FileHandle_t* file);
	/*
	static u64			FileSize(const char* filePath) = 0;

	static u64			ReadWholeFile(char* buffer, u64 bufferSize, FileHandle_t* file) = 0;

	static bool			WriteLine(FileHandle_t* fileHandle, char* text) = 0;
	*/
};
