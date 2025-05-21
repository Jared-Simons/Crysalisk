//=============================================================================
// Author: Jared Simons
// Date Created: 5/21/2025
// 
// Purpose: Shader resource loading utilities.
//=============================================================================
#include "ShaderLoader.h"

#include "Core/Logging.h"
#include "Memory/MemoryManager.h"
#include "Platform/FileSystem.h"
#include "Platform/Platform.h"

namespace ShaderLoader
{
	// Helper functions.
	static bool ProcessShaderModule(char* pszConfigData, ShaderResource_t& pOutResource);
	

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
			char	szCurrentLine[1000];
			size_t	currentLineLength{ 0 };
			size_t	bytesRead{ 0 };
			size_t	equalIndex{ 0 };
			char	szKey[500];
			char	szValue[500];

			FileSystem::ReadLine(szCurrentLine, 1000, &bytesRead, &hShaderCfgFile);
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
				else if (StringCompare(szKey, "topology_type"))
				{
					// The type(s) of topology that the shader can support.
					// NOTE: Currently shaders can only use one topology type.
					VERIFY_EQUAL_TOKEN_FOUND(szCurrentLine, equalIndex);
					pOutResource.pszTopologyType = StringStrip(szValue, '\"');
					continue;
				}
			}
			else
			{
				// If no equal token was found, then this line is the start of a larger, multi-line data block to be processed.
				if (StringCompare(szCurrentLine, "stages"))
				{
					// Shader stage modules.
					int		nIterations = 0;	// Just a failsafe so a malformed file doesn't get us stuck in an infinite loop.
					char	pszDataBlockLine[1000];
					size_t	subBlockBytesRead{ 0 };

					while (pszDataBlockLine && pszDataBlockLine[0] != '}' && nIterations <= 9)	// Hopefully we never have more than 8 shader stages...
					{
						if (nIterations == 9)
						{
							LOG_WARN("Shaders currently only support a max of 8 stages. Shader '%s' exceeds this limit. Only 8 stages we're processed.", pOutResource.pszShaderName);
							break;
						}

						FileSystem::ReadLine(pszDataBlockLine, 1000, &subBlockBytesRead, &hShaderCfgFile);

						if (!pszDataBlockLine[0] || pszDataBlockLine[0] == '{' || pszDataBlockLine[0] == '}' || pszDataBlockLine[0] == '#') // Skip blank lines, braces, and comments.
							continue;

						if (!ProcessShaderModule(pszDataBlockLine, pOutResource))
						{
							LOG_ERROR("Failed to process shader module config '%s'. Skipping module.", pszDataBlockLine);
							continue;
						}

						nIterations++;
					}

					continue;
				}
				else if (StringCompare(szCurrentLine, "attributes"))
				{
					// Vertex attributes.

				}
			}
			LOG_WARN("Failed to parse shader config data: '%s'.", szCurrentLine);
		}

		// Remember to close the config file.
		FileSystem::CloseFile(&hShaderCfgFile);

		return true;
	}

	static bool ProcessShaderModule(char* pszConfigData, ShaderResource_t& pOutResource)
	{
		size_t									commaIndex{ 0 };
		char									szFormattedPathBuffer[512];
		char*									pszTrimmedModulePath{ nullptr };
		char*									pszTypeString{ nullptr };
		ShaderResource_t::ShaderStageResource_t newShaderModule{};

		// We're assuming that all the config data passed here are the names of shader module files and their associated shader types.
		// Start by putting together the path to the module.
		pszTrimmedModulePath = StringTrim(pszConfigData);
		
		// While we're here, we can copy over the shader type string as well.
		commaIndex = StringIndexOf(pszTrimmedModulePath, ',');
		pszTypeString = StringDuplicate(&pszTrimmedModulePath[commaIndex + 1]);
		newShaderModule.pszStageString = StringStrip(pszTypeString, '\"');
		 
		// Chop off the type string from the path.
		pszTrimmedModulePath[commaIndex] = 0;

		// Append the shader asset directory.
		StringFormat_V("%s/%s", szFormattedPathBuffer, "Assets/Shaders", pszTrimmedModulePath);
		// Finally, strip the quotes out of the path.
		StringStripInPlace(szFormattedPathBuffer, '\"');

		// Now actually get the shader module data.
		// Make sure the file actually exists.
		if (!FileSystem::FileExists(szFormattedPathBuffer))
		{
			LOG_ERROR("Failed to find shader module with the name '%s'. Skipping this module.", szFormattedPathBuffer);

			// TODO: Release all dynamically-allocated resources.
			return false;
		}

		// Read the contents of the file into a buffer.
		FileHandle_t hShaderModule{};
		if (!FileSystem::OpenFile(szFormattedPathBuffer, eFileMode::FILEMODE_READ, true, &hShaderModule))
		{
			// TODO: Release all dynamically-allocated resources.
			return false;
		}

		newShaderModule.CodeDataSize = FileSystem::FileSize(&hShaderModule);
		newShaderModule.pShaderModuleBytecode.Resize(newShaderModule.CodeDataSize);
		size_t outBytesRead{ 0 };

		if (!FileSystem::ReadWholeBinaryFile(newShaderModule.pShaderModuleBytecode.Data(), newShaderModule.CodeDataSize, &outBytesRead, &hShaderModule))
		{
			LOG_ERROR("Failed to binary read shader module '%s'. Resource acquisition failed.", szFormattedPathBuffer);

			// TODO: Release all dynamically-allocated resources.
			return false;
		}

		if (outBytesRead != newShaderModule.CodeDataSize)
		{
			LOG_WARN("Data mismatch on shader module read! Expected %d bytes, read %d bytes.", newShaderModule.CodeDataSize, outBytesRead);
		}
		FileSystem::CloseFile(&hShaderModule);

		// Release temp string data.
		StringFree(pszTypeString);
		StringFree(pszTrimmedModulePath);

		// Add the new module to the resource.
		pOutResource.ShaderStages.PushBack(newShaderModule);

		return true;
	}
}