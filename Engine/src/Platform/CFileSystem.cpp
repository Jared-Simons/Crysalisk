//=============================================================================
// Author: Jared Simons
// Date Created: 5/12/2024
// 
// Purpose: The Interface for Filesystems.
//=============================================================================
#include "Systems/IFileSystem.h"

#include "Core/Logging.h"

#include "String_Utils.h"

#include <stdio.h>	// Needed for FILE type


class CFileSystem : public IFileSystem
{
public:
	FileHandle_t*	OpenFile(const char* filePath, eFileMode fileMode) override;
	void			CloseFile(FileHandle_t* fileHandle) override;
	size_t			FileSize(FileHandle_t* fileHandle) override;


	bool 			ReadLine(char* buffer, size_t bufferSize, FileHandle_t* file) override;
	size_t			ReadWholeFile(char* buffer, size_t bufferSize, FileHandle_t* file) override;

	bool			WriteLine(FileHandle_t* fileHandle, char* text) override;
};


FileHandle_t* CFileSystem::OpenFile(const char* filePath, eFileMode fileMode)
{
	if (!filePath || fileMode == FILEMODE_UNKNOWN)
	{
		LOG_ERROR("FileSystem - OpenFile() requires valid a file path and file mode parameters");
		return nullptr;
	}

	FILE* fileHandle = new FILE;

	// any code that is not 0 indicates a failure to open the file.
	if (fopen_s(&fileHandle, filePath, fileModeStrings[fileMode]) != 0)
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

	delete fileData;

	fileHandle->path = nullptr;
	delete fileHandle;
}

size_t CFileSystem::FileSize(FileHandle_t* fileHandle)
{
	if (!fileHandle->isValid)
	{
		LOG_ERROR("FileSystem->FileSize() - Requires a pointer to a valid file handle.");
		return 0;
	}

	size_t fileSize{ 0 };

	fseek((FILE*)fileHandle->internalData, 0, SEEK_END);
	fileSize = ftell((FILE*)fileHandle->internalData);

	return fileSize;
}

bool CFileSystem::ReadLine(char* buffer, size_t bufferSize, FileHandle_t* file)
{
	if (!file)
	{
		LOG_ERROR("FileSystem->Read() requires a valid pointer to a file handle!");
		return size_t(0);
	}

	//size_t numBytesRead = fread_s(buffer, bufferSize, 1, bufferSize, (FILE*)file->internalData);
	char* retString = fgets(buffer, 8192, (FILE*)file->internalData);
	
	// NOTE: fgets() handles null-termination for us!

	// If fgets returns NULL - this is a possible indicator for EOF Condition
	// Return false to indicate this to the caller.
	return retString != NULL;
}

size_t CFileSystem::ReadWholeFile(char* buffer, size_t bufferSize, FileHandle_t* file)
{
	if (!file)
	{
		LOG_ERROR("FileSystem->ReadWholeLine() requires a valid pointer to a file handle!");
		return size_t(0);
	}

	size_t numBytesRead = fread_s(buffer, bufferSize, 1, bufferSize, (FILE*)file->internalData);

	// Append a null terminator at the end of the buffer.
	buffer[numBytesRead] = '\0';

	return numBytesRead;
}

bool CFileSystem::WriteLine(FileHandle_t* fileHandle, char* text)
{
	if ((fileHandle->fileAccessMode != FILEMODE_WRITE) && (fileHandle->fileAccessMode != FILEMODE_WRITEBINARY))
	{
		LOG_ERROR("FileSystem->WriteLine() - Cannot write to a file that is read-only.");
		return false;
	}

	fputs(text, (FILE*)fileHandle->internalData);

	return true;
}


static CFileSystem sFileSystem;
IFileSystem* gpFileSystem = &sFileSystem;

