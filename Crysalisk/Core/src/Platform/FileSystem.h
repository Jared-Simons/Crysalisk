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

//-------------------------------------------------------------------------
// File access modes.
//-------------------------------------------------------------------------
enum eFileMode
{
	// Undefined, considered an error.
	FILEMODE_UNKNOWN = 0,
	// The file should be opened for reading.
	FILEMODE_READ,
	// The file should be opened for writing.
	FILEMODE_WRITE,

	FILEMODE_MAXMODES
};

//-------------------------------------------------------------------------
// An opaque file handle.
//-------------------------------------------------------------------------
struct CORE_API FileHandle_t
{
	// The path to this file on disk.
	char* pszPath{};
	// Indicates if this file handle is valid for reading or writing.
	bool bIsValid{ false };
	// The mode this handle was opened with.
	eFileMode eFileAccessMode{ FILEMODE_UNKNOWN };
	// A pointer to the internal state of the file handle.
	void* pInternalData{ nullptr };

	~FileHandle_t();
};

//-------------------------------------------------------------------------
// The file system.
//-------------------------------------------------------------------------
class CORE_API FileSystem
{
public:
	// Attempts to open a file, if the file does not exist, then it creates a new file.
	static bool			OpenFile(const char* filePath, eFileMode fileMode, bool bIsBinary, FileHandle_t* pOutFileHandle);
	// Attempts to close a file handle.
	static void			CloseFile(FileHandle_t* fileHandle);
	// Checks to see if the file exists on disk.
	static bool			FileExists(const char* pszFilePath);
	// Returns the size of the provided file.
	static u64		FileSize(FileHandle_t* fileHandle);

	// Reads a line of text from the provided file and places it into pBuffer.
	static bool 		ReadLine(char* pBuffer, u64 bufferSize, u64* pOutBytesRead, FileHandle_t* pFileHandle);
	// Reads the entire file and places it into pBuffer.
	static bool			ReadWholeFile(char* pBuffer, u64 bufferSize, u64* pOutBytesRead, FileHandle_t* pFileHandle);
	// Reads the entire file in binary mode, and places the bytes into pBuffer.
	static bool			ReadWholeBinaryFile(byte* pBuffer, u64 bufferSize, u64* pOutBytesRead, FileHandle_t* pFileHandle);
	// Returns whether or not the internal pointer for this handle has reached the end of the file.
	static bool			IsEOF(FileHandle_t* pFileHandle);
	// Returns the file's read pointer back to the head of the file.
	// Allows the file to be read multiple times.
	static void			ReturnFilePointerToHead(FileHandle_t* pFileHandle);

	// Writes the text in pBuffer to the provided file.
	static void			WriteLine(char* pBuffer, FileHandle_t* pFileHandle);
	// Writes the bytes in pBytes to the provided file.
	static void			WriteBytes(const void* pBytes, u64 dataSize, FileHandle_t* pFileHandle);
};