//=============================================================================
// Author: Jared Simons
// Date Created: 5/21/2025
// 
// Purpose: Shader resource loading utilities.
//=============================================================================
#include "ShaderLoader.h"

#include "Core/Logging.h"
#include "Platform/FileSystem.h"
#include "Platform/Platform.h"

namespace ShaderLoader
{
#define VERIFY_EQUAL_TOKEN_FOUND(currentLine, equalIndex)\
	if (equalIndex == kInvalidID_U32)																					\
	{																													\
		LOG_WARN("Error in parsing 'name' value in shader configuration. Line: '%s'. Skipping line.", currentLine);		\
		continue;																										\
	}

	bool LoadShaderResource(const char* pszShaderPath, ShaderResource_t& pOutResource)
	{
		FileHandle_t hShaderCfgFile{};
		if (!FileSystem::FileExists(pszShaderPath))
		{
			LOG_ERROR("Failed to find shader config file '%s'.", pszShaderPath);
			return false;
		}

		if (!FileSystem::OpenFile(pszShaderPath, eFileMode::FILEMODE_READ, false, &hShaderCfgFile))
		{
			return false;
		}

		// Parse the file's contents.
		while (!FileSystem::IsEOF(&hShaderCfgFile))
		{
			char	szCurrentLine[14000];
			size_t	currentLineLength{ 0 };
			size_t	bytesRead{ 0 };
			size_t	equalIndex{ 0 };
			char	szKey[500];
			char	szValue[500];

			FileSystem::ReadLine(szCurrentLine, 14000, &bytesRead, &hShaderCfgFile);
			currentLineLength = StringLength(szCurrentLine);

			szKey[0] = 0;
			szValue[0] = 0;

			if (currentLineLength <= 0 || szCurrentLine[0] == '#') // Blank line or comment
				continue;

			equalIndex = StringIndexOf(szCurrentLine, '=');

			if (equalIndex != kInvalidID_U32)
			{
				Platform::MemoryCopy(szKey, szCurrentLine, equalIndex);
				szKey[equalIndex] = 0;
				Platform::MemoryCopy(szValue, &szCurrentLine[equalIndex + 1], bytesRead - equalIndex);
				szValue[bytesRead - 1] = 0;
			}

			// Parse the configuration data.
			if (equalIndex != kInvalidID_U32)
			{
				// If equalIndex is set, then this line is a key-value pair that can be processed individually.
				if (StringCompare(szKey, "name"))
				{
					// The name of the shader.
					VERIFY_EQUAL_TOKEN_FOUND(szCurrentLine, equalIndex);
					pOutResource.pszShaderName = StringStrip(szValue, '\"');	// Strip the quotes out of the string.
					continue;
				}
				else if (StringCompare(szKey, "num_shader_stages"))
				{
					// Number of stages this shader will have.
					VERIFY_EQUAL_TOKEN_FOUND(szCurrentLine, equalIndex);
					int iNumStages = String_ATOI(szValue);
					pOutResource.ShaderStages.Resize(iNumStages);
					continue;
				}
			}
			else
			{
				// If no equal token was found, then this line is the start of a larger, multi-line data block to be processed.
				if (StringCompare(szCurrentLine, "stages"))
				{
					int		nIterations = 0;	// Just a failsafe so a malformed file doesn't get us stuck in an infinite loop.
					char	pszDataBlockLine[14000];
					size_t	subBytesRead{ 0 };

					while (pszDataBlockLine && pszDataBlockLine[0] != '}' && nIterations < 100)	// Hopefully we never have more than 100 shader stages...
					{
						FileSystem::ReadLine(pszDataBlockLine, 14000, &subBytesRead, &hShaderCfgFile);

						if (!pszDataBlockLine[0] || pszDataBlockLine[0] == '{' || pszDataBlockLine[0] == '#') // Skip blank lines, opening braces, and comments.
							continue;

						// We're assuming that all these values are names of shader module files and their associated shader types.
						size_t commaIndex = StringIndexOf(pszDataBlockLine, ',');

						nIterations++;
					}

					continue;
				}
			}
			LOG_WARN("Failed to parse shader config data: '%s'.", szCurrentLine);

			//LOG_DEBUG(szCurrentLine);
		}

		// Remember to close the config file.
		FileSystem::CloseFile(&hShaderCfgFile);

		return true;
	}
}