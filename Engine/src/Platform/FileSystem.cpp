//=============================================================================
// Author: Jared Simons
// Date Created: 5/12/2024
// 
// Purpose: The Interface for Filesystems.
//=============================================================================
#include "FileSystem.h"

#include "Core/Logging.h"

#include "String_Utils.h"

#include <stdio.h>	// Needed for FILE type
#include <string>

bool FileSystem::OpenFile(const char* filePath, eFileMode fileMode, bool bIsBinary, FileHandle_t* pOutFileHandle)
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

bool FileSystem::CloseFile(FileHandle_t* fileHandle)
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

size_t FileSystem::FileSize(FileHandle_t* fileHandle)
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

bool FileSystem::ReadLine(char* pBuffer, int bufferSize, size_t* pOutBytesRead, FileHandle_t* pFileHandle)
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

bool FileSystem::ReadWholeFile(char* pBuffer, size_t bufferSize, size_t* pOutBytesRead, FileHandle_t* pFileHandle)
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

bool FileSystem::ReadWholeBinaryFile(char* pBuffer, size_t bufferSize, size_t* pOutBytesRead, FileHandle_t* pFileHandle)
{
	if (!pFileHandle || !pFileHandle->isValid || !pBuffer || !bufferSize || !pOutBytesRead)
	{
		LOG_ERROR("FileSystem->ReadWholeBinaryFile() requires a valid pointer to a file handle, buffer, Buffer Size, and Out Bytes Read");
		return false;
	}

	*pOutBytesRead = fread_s(pBuffer, bufferSize, 1, bufferSize, (FILE*)pFileHandle->internalData);

	return true;
}
