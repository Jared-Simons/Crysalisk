//=============================================================================
// Author: Jared Simons
// Date Created: 5/12/2024
// 
// Purpose: The Interface for Filesystems.
//=============================================================================
#pragma once


enum eFileMode
{
	FILEMODE_UNKNOWN = 0,
	FILEMODE_READ,
	FILEMODE_WRITE,

	FILEMODE_MAXMODES
};

struct FileHandle_t
{
	const char* path{};
	bool isValid{ false };
	eFileMode fileAccessMode{ FILEMODE_UNKNOWN };
	void* internalData{ nullptr };
};

class FileSystem
{
public:
	static bool			OpenFile(const char* filePath, eFileMode fileMode, bool bIsBinary, FileHandle_t* pOutFileHandle);
	static bool			CloseFile(FileHandle_t* fileHandle);
	static size_t		FileSize(FileHandle_t* fileHandle);

	static bool 		ReadLine(char* pBuffer, int bufferSize, size_t* pOutBytesRead, FileHandle_t* file);
	static bool			ReadWholeFile(char* pBuffer, size_t bufferSize, size_t* pOutBytesRead, FileHandle_t* file);
	static bool			ReadWholeBinaryFile(char* pBuffer, size_t bufferSize, size_t* pOutBytesRead, FileHandle_t* file);
	/*
	static size_t			FileSize(const char* filePath) = 0;

	static size_t			ReadWholeFile(char* buffer, size_t bufferSize, FileHandle_t* file) = 0;

	static bool			WriteLine(FileHandle_t* fileHandle, char* text) = 0;
	*/
};
