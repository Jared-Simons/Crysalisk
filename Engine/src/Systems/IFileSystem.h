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
	FILEMODE_READONLY,
	FILEMODE_WRITE,
	FILEMODE_READBINARY,
	FILEMODE_WRITEBINARY,

	FILEMODE_MAXMODES
};

static const char* fileModeStrings[FILEMODE_MAXMODES] =
{
	"0",
	"r",
	"w",
	"rb",
	"wb"
};

struct FileHandle_t
{
	const char* path;
	bool isValid;
	eFileMode fileAccessMode;
	void* internalData;
};

class IFileSystem
{
public:
	virtual FileHandle_t* OpenFile(const char* filePath, eFileMode fileMode) = 0;
	virtual void CloseFile(FileHandle_t* fileHandle) = 0;
};

extern IFileSystem* gpFileSystem;

