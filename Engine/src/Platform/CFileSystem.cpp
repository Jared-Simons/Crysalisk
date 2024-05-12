//=============================================================================
// Author: Jared Simons
// Date Created: 5/12/2024
// 
// Purpose: The Interface for Filesystems.
//=============================================================================
#include "Systems/IFileSystem.h"

#include "Core/Logging.h"

#include <stdio.h>	// Needed for FILE type


class CFileSystem : public IFileSystem
{
public:
	FileHandle_t* OpenFile(const char* filePath, eFileMode fileMode) override;
	void CloseFile(FileHandle_t* fileHandle) override;
};


FileHandle_t* CFileSystem::OpenFile(const char* filePath, eFileMode fileMode)
{
	if (!filePath || fileMode == FILEMODE_UNKNOWN)
	{
		LOG_ERROR("FileSystem - OpenFile() requires valid a file path and file mode parameters");
		return nullptr;
	}

	FILE* fileHandle = fopen(filePath, fileModeStrings[fileMode]);

	if (fileHandle == nullptr)
	{
		LOG_ERROR("FileSystem - OpenFile() failed to open file: '%s'", filePath);
		return nullptr;
	}

	FileHandle_t* openedFile = new FileHandle_t;

	openedFile->path = filePath;
	openedFile->isValid = true;
	openedFile->fileAccessMode = fileMode;
	openedFile->internalData = (void*)fileHandle;

	return openedFile;
}

void CFileSystem::CloseFile(FileHandle_t* fileHandle)
{
	if (!fileHandle)
	{
		LOG_ERROR("FileSystem - CloseFile() failed to close file. No handle provided");
		return;
	}

	fileHandle->isValid = false;
	fileHandle->fileAccessMode = FILEMODE_UNKNOWN;
	
	FILE* fileData = (FILE*)fileHandle->internalData;

	int result = fclose(fileData);

	if (result != 0)
	{
		LOG_ERROR("FileSystem - CloseFile() failed to close file: '%s'.", fileHandle->path);
		return;
	}

	fileHandle->path = nullptr;
	delete fileHandle;
}


static CFileSystem sFileSystem;
IFileSystem* gpFileSystem = &sFileSystem;

