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
#include <string>


class CFileSystem : public IFileSystem
{
public:
	bool			OpenFile(const char* filePath, eFileMode fileMode, bool bIsBinary, FileHandle_t* pOutFileHandle) override;
	bool			CloseFile(FileHandle_t* fileHandle) override;
	size_t			FileSize(FileHandle_t* fileHandle) override;

	bool 			ReadLine(char* pBuffer, size_t bufferSize, size_t* pOutBytesRead, FileHandle_t* file) override;
	bool			ReadWholeFile(char* pBuffer, size_t bufferSize, size_t* pOutBytesRead, FileHandle_t* file) override;

	bool			ReadWholeBinaryFile(char* pBuffer, size_t bufferSize, size_t* pOutBytesRead, FileHandle_t* file) override;
	/*
	size_t			FileSize(const char* filePath) override;


	size_t			ReadWholeFile(char* buffer, size_t bufferSize, FileHandle_t* file) override;
	size_t			ReadWholeBinaryFile(char* buffer, size_t bufferSize, const char* filePath) override;

	bool			WriteLine(FileHandle_t* fileHandle, char* text) override;
	*/
};

bool CFileSystem::OpenFile(const char* filePath, eFileMode fileMode, bool bIsBinary, FileHandle_t* pOutFileHandle)
{
	char* modeString;

	if (!pOutFileHandle || !filePath)
	{
		LOG_ERROR("FileSystem->OpenFile() requires a file path and a valid pointer to a file handle");
		return false;
	}

	if (fileMode == eFileMode::FILEMODE_READ)
	{
		if (bIsBinary)
			modeString = "rb";
		else
			modeString = "r";
	}
	else if (fileMode == eFileMode::FILEMODE_WRITE)
	{
		if (bIsBinary)
			modeString = "wb";
		else
			modeString = "w";
	}
	else
	{
		LOG_ERROR("FileSystem->OpenFile() cannot open a file using UNKNOWN File Mode!");
		return false;
	}

	FILE* tempHandle = (FILE*)pOutFileHandle->internalData;

	if (fopen_s(&tempHandle, filePath, modeString) != 0)
	{
		LOG_ERROR("FileSystem->OpenFile() cannot find/open file '%s'.", filePath);
		return false;
	}

	pOutFileHandle->internalData = tempHandle;

	pOutFileHandle->fileAccessMode = fileMode;
	pOutFileHandle->isValid = true;
	pOutFileHandle->path = filePath;

	return true;
}

bool CFileSystem::CloseFile(FileHandle_t* fileHandle)
{
	if (!fileHandle)
	{
		LOG_ERROR("FileSystem->CloseFile() requires a valid pointer to a file handle.");
		return false;
	}

	if (fclose((FILE*)fileHandle->internalData) != 0)
	{
		LOG_ERROR("FileSystem->CloseFile() failed to close file '%s'", fileHandle->path);
		return false;
	}

	fileHandle->isValid = false;

	return true;
}

size_t CFileSystem::FileSize(FileHandle_t* fileHandle)
{
	if (!fileHandle || !fileHandle->isValid)
	{
		LOG_ERROR("FileSystem->FileSize(fileHandle_t*) Requires a pointer to a valid file handle. Returning 0.");
		return 0;
	}

	fseek((FILE*)fileHandle->internalData, 0, SEEK_END);

	size_t fileSize = ftell((FILE*)fileHandle->internalData);

	// Return the file pointer back to the top of the file... else all further file reads will fail
	rewind((FILE*)fileHandle->internalData);

	return fileSize;
}

bool CFileSystem::ReadLine(char* pBuffer, size_t bufferSize, size_t* pOutBytesRead, FileHandle_t* pFileHandle)
{
	if (!pFileHandle || !pFileHandle->isValid || !pBuffer || !bufferSize || !pOutBytesRead)
	{
		LOG_ERROR("FileSystem->ReadLine() requires a valid pointer to a file handle, buffer, Buffer Size, and Out Bytes Read");
		return false;
	}

	// Zero out the buffer
	memset(pBuffer, 0, bufferSize);

	// Read the contents of the line into the buffer
	if (fgets(pBuffer, bufferSize, (FILE*)pFileHandle->internalData) == 0)
	{
		LOG_ERROR("FileSystem->ReadLine() Failed to read line to buffer");
		return false;
	}

	return true;
}

bool CFileSystem::ReadWholeFile(char* pBuffer, size_t bufferSize, size_t* pOutBytesRead, FileHandle_t* pFileHandle)
{
	if (!pFileHandle || !pFileHandle->isValid || !pBuffer || !bufferSize || !pOutBytesRead)
	{
		LOG_ERROR("FileSystem->ReadWholeFile() requires a valid pointer to a file handle, buffer, Buffer Size, and Out Bytes Read");
		return false;
	}

	// Zero out the buffer
	memset(pBuffer, 0, bufferSize);

	// Read the contents into the buffer
	*pOutBytesRead = fread(pBuffer, bufferSize, 1, (FILE*)pFileHandle->internalData);

	return true;
}

bool CFileSystem::ReadWholeBinaryFile(char* pBuffer, size_t bufferSize, size_t* pOutBytesRead, FileHandle_t* pFileHandle)
{
	if (!pFileHandle || !pFileHandle->isValid || !pBuffer || !bufferSize || !pOutBytesRead)
	{
		LOG_ERROR("FileSystem->ReadWholeBinaryFile() requires a valid pointer to a file handle, buffer, Buffer Size, and Out Bytes Read");
		return false;
	}

	*pOutBytesRead = fread_s(pBuffer, bufferSize, 1, bufferSize, (FILE*)pFileHandle->internalData);

	return true;
}



static CFileSystem sFileSystem;
IFileSystem* gpFileSystem = &sFileSystem;

