//=============================================================================
// Author: Jared Simons
// Date Created: 5/12/2024
// 
// Purpose: The interface for Filesystems.
//=============================================================================
#include "FileSystem.h"

#include "Core/Assertions.h"
#include "Core/Logging.h"
#include "Memory/MemoryManager.h"
#include "Platform/Platform.h"

#include "Strings/String_Utils.h"

#include <io.h>		// _access()
#include <stdio.h>	// Needed for FILE type

//-------------------------------------------------------------------------
// Purpose: Destructor.
//-------------------------------------------------------------------------
FileHandle_t::~FileHandle_t()
{
	// If we are still open, we need to release our resources.
	if (bIsValid || pInternalData)
	{
		FileSystem::CloseFile(this);
	}

	if (pszPath != nullptr)
	{
		StringFree(pszPath);
		pszPath = nullptr;
	}
}

//-------------------------------------------------------------------------
// Purpose: Attempts to open a file at the provided path. 
//				Creates a new file if a file wasn't found.
// 
// Input:  pszFilePath - The path to the file, relative to the root directory
//							of the project.
//		   eFileMode - The mode to open the file in, usually either read or write.
//		   bIsBinary - Indicates whether or not to interpret the file's contents
//							as binary data or plain text.
//		   pOutFileHandle - A pointer to the file handle that manages this file's contents.
// Output: bool - Returns true on opening/creating the file, otherwise false.
//-------------------------------------------------------------------------
bool FileSystem::OpenFile(const char* pszFilePath, eFileMode eFileMode, bool bIsBinary, FileHandle_t* pOutFileHandle)
{
	char* pszModeString;

	if (!pOutFileHandle || !pszFilePath)
	{
		LOG_ERROR("FileSystem->OpenFile() requires a pFileHandle pszPath and a valid pointer to a pFileHandle handle");
		return false;
	}

	// Do nothing if the file is already opened.
	if (pOutFileHandle->bIsValid)
		return true;

	if (eFileMode == eFileMode::FILEMODE_READ)
	{
		if (bIsBinary)
			pszModeString = "rb";
		else
			pszModeString = "r";
	}
	else if (eFileMode == eFileMode::FILEMODE_WRITE)
	{
		if (bIsBinary)
			pszModeString = "wb";
		else
			pszModeString = "w";
	}
	else
	{
		LOG_ERROR("FileSystem->OpenFile() cannot open a pFileHandle using UNKNOWN File Mode!");
		return false;
	}

	FILE* pInternalHandle = reinterpret_cast<FILE*>(pOutFileHandle->pInternalData);

	if (fopen_s(&pInternalHandle, pszFilePath, pszModeString) != 0)
	{
		LOG_ERROR("FileSystem->OpenFile() cannot find/open pFileHandle '%s'.", pszFilePath);
		return false;
	}

	pOutFileHandle->pInternalData = pInternalHandle;

	pOutFileHandle->eFileAccessMode = eFileMode;
	pOutFileHandle->bIsValid = true;
	pOutFileHandle->pszPath = StringDuplicate(pszFilePath);

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Attempts to close the file associated with the provided handle.
// 
// Input:  pFileHandle - A pointer to the file handle to be closed.
// Output: none.
//-------------------------------------------------------------------------
void FileSystem::CloseFile(FileHandle_t* pFileHandle)
{
	if (!pFileHandle)
	{
		return;
	}

	if (fclose(reinterpret_cast<FILE*>(pFileHandle->pInternalData)) != 0)
	{
		LOG_ERROR("FileSystem->CloseFile() failed to close pFileHandle '%s'", pFileHandle->pszPath);
	}
	pFileHandle->pInternalData = nullptr;

	pFileHandle->bIsValid = false;
	if (pFileHandle->pszPath)
	{
		StringFree(pFileHandle->pszPath);
		pFileHandle->pszPath = nullptr;
	}
}

//-------------------------------------------------------------------------
// Purpose: Checks to see if a file exists at the provided path.
// Input:  pszFilePath - The path to the file, relative to the root directory
//							of the project.
// Output: bool - Returns true if the file exists, otherwise false.
//-------------------------------------------------------------------------
bool FileSystem::FileExists(const char* pszFileName)
{
	if (!pszFileName)
		return false;

	return (_access(pszFileName, 0) == 0);
}

//-------------------------------------------------------------------------
// Purpose: Checks to see if a file exists at the provided path.
// Input:  pszFilePath - The path to the file, relative to the root directory
//							of the project.
// Output: bool - Returns true if the file exists, otherwise false.
//-------------------------------------------------------------------------
size_t FileSystem::FileSize(FileHandle_t* pFileHandle)
{
	if (!pFileHandle || !pFileHandle->bIsValid)
	{
		LOG_ERROR("FileSystem->FileSize(fileHandle_t*) Requires a pointer to a valid pFileHandle handle. Returning 0.");
		return 0;
	}
	FILE* pInternalData = reinterpret_cast<FILE*>(pFileHandle->pInternalData);

	fseek(pInternalData, 0, SEEK_END);

	size_t fileSize = ftell((FILE*)pFileHandle->pInternalData);

	// Return the file pointer back to the top of the file... else all further file reads will fail.
	rewind(pInternalData);

	return fileSize;
}

//-------------------------------------------------------------------------
// Purpose: Reads the current line to the provided buffer.
// Input:  pBuffer - The buffer to read the line's contents into.
//		   bufferSize - The size in bytes of pBuffer.
//		   pOutBytesRead - Will contain the size of bytes read by this function.
//		   pFileHandle - A pointer to the file handle to read from.
// Output: bool - Returns true on successful read, otherwise false.
//-------------------------------------------------------------------------
bool FileSystem::ReadLine(char* pBuffer, size_t bufferSize, size_t* pOutBytesRead, FileHandle_t* pFileHandle)
{
	if (!pFileHandle || !pFileHandle->bIsValid || !pBuffer || !bufferSize || !pOutBytesRead)
	{
		LOG_ERROR("FileSystem->ReadLine() requires a valid pointer to a pFileHandle handle, buffer, Buffer Size, and Out Bytes Read");
		return false;
	}

	// Zero out the buffer
	Platform::MemorySet(pBuffer, 0, bufferSize);

	// Read the contents of the line into the buffer
	if (fgets(pBuffer, bufferSize, reinterpret_cast<FILE*>(pFileHandle->pInternalData)) == 0)
	{
		return false;
	}

	*pOutBytesRead = StringLength(pBuffer);
	pBuffer[*pOutBytesRead - 1] = '\0';
	return true;
}

//-------------------------------------------------------------------------
// Purpose: Reads the whole file to the provided buffer.
// Input:  pBuffer - The buffer to read the file's contents into.
//		   bufferSize - The size in bytes of pBuffer.
//		   pOutBytesRead - Will contain the size of bytes read by this function.
//		   pFileHandle - A pointer to the file handle to read from.
// Output: bool - Returns true on successful read, otherwise false.
//-------------------------------------------------------------------------
bool FileSystem::ReadWholeFile(char* pBuffer, size_t bufferSize, size_t* pOutBytesRead, FileHandle_t* pFileHandle)
{
	if (!pFileHandle || !pFileHandle->bIsValid || !pBuffer || !bufferSize || !pOutBytesRead)
	{
		LOG_ERROR("FileSystem->ReadWholeFile() requires a valid pointer to a pFileHandle handle, buffer, Buffer Size, and Out Bytes Read");
		return false;
	}

	// Zero out the buffer
	Platform::MemorySet(pBuffer, 0, bufferSize);

	// Read the contents into the buffer
	*pOutBytesRead = fread(pBuffer, bufferSize, 1, reinterpret_cast<FILE*>(pFileHandle->pInternalData));

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Reads the whole file as bytes to the provided buffer.
// Input:  pBuffer - The buffer to read the file's contents into.
//		   bufferSize - The size in bytes of pBuffer.
//		   pOutBytesRead - Will contain the size of bytes read by this function.
//		   pFileHandle - A pointer to the file handle to read from.
// Output: bool - Returns true on successful read, otherwise false.
//-------------------------------------------------------------------------
bool FileSystem::ReadWholeBinaryFile(byte* pBuffer, u64 bufferSize, u64* pOutBytesRead, FileHandle_t* pFileHandle)
{
	if (!pFileHandle || !pFileHandle->bIsValid || !pBuffer || !bufferSize || !pOutBytesRead)
	{
		LOG_ERROR("FileSystem->ReadWholeBinaryFile() requires a valid pointer to a pFileHandle handle, buffer, Buffer Size, and Out Bytes Read");
		return false;
	}

	*pOutBytesRead = fread_s(pBuffer, bufferSize, 1, bufferSize, reinterpret_cast<FILE*>(pFileHandle->pInternalData));

	return true;
}

//-------------------------------------------------------------------------
// Purpose: Returns whether or not the internal pointer for this handle has reached the end of the file.
// Input:  pFileHandle - A pointer to the file handle to check.
// Output: bool - Returns true if EOF, otherwise false.
//-------------------------------------------------------------------------
bool FileSystem::IsEOF(FileHandle_t* pFileHandle)
{
	if (!feof((FILE*)pFileHandle->pInternalData))
	{
		return false;
	}
	return true;
}

//-------------------------------------------------------------------------
// Purpose: Writes the text data in pBuffer to the provided file.
// Input:  pBuffer - The buffer to write to the file.  
//		   pFileHandle - A pointer to the file handle to write to.
// Output: none.
//-------------------------------------------------------------------------
void FileSystem::WriteLine(char* pBuffer, FileHandle_t* pFileHandle)
{
	if (pBuffer && pFileHandle)
	{
		fputs(pBuffer, (FILE*)pFileHandle->pInternalData);
	}
}

//-------------------------------------------------------------------------
// Purpose: Writes the binary data in pBytes to the provided file.
// Input:  pBytes - The buffer to write to the file.  
//		   dataSize - The size in bytes of pBytes.
//		   pFileHandle - A pointer to the file handle to write to.
// Output: none.
//-------------------------------------------------------------------------
void FileSystem::WriteBytes(const void* pBytes, size_t dataSize, FileHandle_t* pFileHandle)
{
	if (pBytes && pFileHandle)
	{
		fwrite(pBytes, 1, dataSize, (FILE*)pFileHandle->pInternalData);
	}
}