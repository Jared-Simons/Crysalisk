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
	static bool	ProcessShaderVertexAttribute(char* pszConfigData, ShaderResource_t& pOutResource);
	static bool ProcessShaderUniforms(char* pszConfigData, ShaderResource_t& pOutResource);
	
#define VERIFY_EQUAL_TOKEN_FOUND(currentLine, equalIndex)																\
	if (equalIndex == kInvalidID_U32)																					\
	{																													\
		LOG_WARN("Error in parsing 'name' value in shader configuration. Line: '%s'. Skipping line.", currentLine);		\
		continue;																										\
	}

	//-------------------------------------------------------------------------
	// Purpose: Attempts to load an existing shader asset from disk.
	// 
	// Input:	pszShaderPath - The path to the shader configuration to be loaded.
	//			pOutResource - The loaded resource data.
	// Output:	bool - Returns true on successful resource acquisition, otherwise false.
	//-------------------------------------------------------------------------
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
				else if (StringCompare(szKey, "enable_depth_testing"))
				{
					// Should this shader be set up to perform depth testing?
					VERIFY_EQUAL_TOKEN_FOUND(szCurrentLine, equalIndex);
					pOutResource.bDepthTestingEnabled = (StringCompareInsensitive("\"true\"", szValue)) ? true : false;
					continue;
				}
				else if (StringCompare(szKey, "enable_blending"))
				{
					// Should this shader be set up to perform color blending?
					VERIFY_EQUAL_TOKEN_FOUND(szCurrentLine, equalIndex);
					pOutResource.bColorBlendingEnabled = (StringCompareInsensitive("\"true\"", szValue)) ? true : false;
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

					FileSystem::ReadLine(pszDataBlockLine, 1000, &subBlockBytesRead, &hShaderCfgFile);

					while (pszDataBlockLine && pszDataBlockLine[0] != '}' && nIterations <= 9)	// Hopefully we never have more than 8 shader stages...
					{
						// Doing a second line read here is okay because the one above should simply be a '{'.
						// If this is not the case, then the file is formatted wrong.
						FileSystem::ReadLine(pszDataBlockLine, 1000, &subBlockBytesRead, &hShaderCfgFile);
						
						if (nIterations == 9)
						{
							LOG_WARN("Shaders currently only support a max of 8 stages. Shader '%s' exceeds this limit. Only 8 stages we're processed.", pOutResource.pszShaderName);
							break;
						}

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
					int		nIterations = 0;	// Just a failsafe so a malformed file doesn't get us stuck in an infinite loop.
					char	pszDataBlockLine[1000];
					size_t	subBlockBytesRead{ 0 };

					FileSystem::ReadLine(pszDataBlockLine, 1000, &subBlockBytesRead, &hShaderCfgFile);

					while (pszDataBlockLine && pszDataBlockLine[0] != '}' && nIterations <= 101)	// We currently only support 100 vertex attributes to one shader.
					{
						// Doing a second line read here is okay because the one above should simply be a '{'.
						// If this is not the case, then the file is formatted wrong.
						FileSystem::ReadLine(pszDataBlockLine, 1000, &subBlockBytesRead, &hShaderCfgFile);

						if (nIterations == 101)
						{
							LOG_WARN("Shaders currently only support a max of 100 vertex attributes. Shader '%s' exceeds this limit. Only 100 attributes were processed.", pOutResource.pszShaderName);
							break;
						}

						if (!pszDataBlockLine[0] || pszDataBlockLine[0] == '{' || pszDataBlockLine[0] == '}' || pszDataBlockLine[1] == '#') // Skip blank lines, braces, and comments.
							continue;

						if (!ProcessShaderVertexAttribute(pszDataBlockLine, pOutResource))
						{
							LOG_ERROR("Failed to process shader vertex attribute config '%s'. Skipping attribute.", pszDataBlockLine);
							continue;
						}

						nIterations++;
					}
					continue;
				}
				else if (StringCompare(szCurrentLine, "uniforms"))
				{
					// Shader uniforms.
					int		nIterations = 0;	// Just a failsafe so a malformed file doesn't get us stuck in an infinite loop.
					char	pszDataBlockLine[1000];
					size_t	subBlockBytesRead{ 0 };
					FileSystem::ReadLine(pszDataBlockLine, 1000, &subBlockBytesRead, &hShaderCfgFile);

					while (pszDataBlockLine && pszDataBlockLine[0] != '}' && nIterations <= 101)	// We currently only support 100 vertex attributes to one shader.
					{
						// Doing a second line read here is okay because the one above should simply be a '{'.
						// If this is not the case, then the file is formatted wrong.
						FileSystem::ReadLine(pszDataBlockLine, 1000, &subBlockBytesRead, &hShaderCfgFile);

						if (nIterations == 101)
						{
							LOG_WARN("Shaders currently only support a max of 100 uniforms. Shader '%s' exceeds this limit. Only 100 uniforms were processed.", pOutResource.pszShaderName);
							break;
						}

						if (!pszDataBlockLine[0] || pszDataBlockLine[0] == '{' || pszDataBlockLine[0] == '}' || pszDataBlockLine[1] == '#') // Skip blank lines, braces, and comments.
							continue;

						if (!ProcessShaderUniforms(pszDataBlockLine, pOutResource))
						{
							LOG_ERROR("Failed to process shader uniform config '%s'. Skipping attribute.", pszDataBlockLine);
							continue;
						}

						nIterations++;
					}
					continue;
				}
			}
			LOG_WARN("Failed to parse shader config data: '%s'.", szCurrentLine);
		}

		// Remember to close the config file.
		FileSystem::CloseFile(&hShaderCfgFile);

		return true;
	}

	//-------------------------------------------------------------------------
	// Purpose: Attempts to process a shader module configuration.
	// 
	// Input:	pszConfigData - The shader module config to be parsed.
	//			pOutResource - The shader associated with this module.
	// Output:	bool - Returns true on successful parsing, otherwise false.
	//-------------------------------------------------------------------------
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

		// Store off the module name for debugging.
		newShaderModule.pszModuleName = StringStrip(pszTrimmedModulePath, '"');

		// Release temp string data.
		if (pszTypeString)
			StringFree(pszTypeString);
		if (pszTrimmedModulePath)
			StringFree(pszTrimmedModulePath);

		// Add the new module to the resource.
		pOutResource.ShaderStages.PushBack(newShaderModule);

		return true;
	}

	//-------------------------------------------------------------------------
	// Purpose: Attempts to process a shader vertex attribute configuration.
	// 
	// Input:	pszConfigData - The attribute config to be parsed.
	//			pOutResource - The shader associated with this attribute.
	// Output:	bool - Returns true on successful parsing, otherwise false.
	//-------------------------------------------------------------------------
	static bool	ProcessShaderVertexAttribute(char* pszConfigData, ShaderResource_t& pOutResource)
	{
		ShaderResource_t::ShaderVertexAttributeConfig_t newAttribute{};
		char*											pszTrimmedData{ nullptr };
		char*											pszTempStringBuffer{ nullptr };
		size_t											commaIndex{ 0 };

		// We're assuming that the config data passed in here will be in the format of:
		// "attribute name", "attribute type".
		// If this is not the case, than the file is formatted wrong.
		pszTrimmedData = StringTrim(pszConfigData);
		commaIndex = StringIndexOf(pszTrimmedData, ',');
		if (commaIndex == kInvalidID_U32)
		{
			return false;
		}

		pszTempStringBuffer = StringDuplicate_N(pszTrimmedData, commaIndex);
		newAttribute.pszName = StringStrip(pszTempStringBuffer, '\"');
		StringFree(pszTempStringBuffer);

		pszTempStringBuffer = StringDuplicate(&pszTrimmedData[commaIndex + 1]);
		newAttribute.pszType = StringStrip(pszTempStringBuffer, '\"');
		StringFree(pszTempStringBuffer);

		// Release temp string data.
		if (pszTrimmedData)
			StringFree(pszTrimmedData);

		// Add the attribute to the resource.
		pOutResource.ShaderAttributes.PushBack(newAttribute);

		return true;
	}

	//-------------------------------------------------------------------------
	// Purpose: Attempts to process a shader uniform configuration.
	// 
	// Input:	pszConfigData - The uniform config to be parsed.
	//			pOutResource - The shader associated with this uniform.
	// Output:	bool - Returns true on successful parsing, otherwise false.
	//-------------------------------------------------------------------------
	static bool ProcessShaderUniforms(char* pszConfigData, ShaderResource_t& pOutResource)
	{
		ShaderResource_t::ShaderUniformConfig_t newUniform{};
		char*									pszTrimmedData{ nullptr };
		char*									pszTempStringBuffer{ nullptr };
		size_t									commaIndex{ 0 };
		size_t									commaIndex2{ 0 };
		size_t									commentIndex{ 0 };

		// We're assuming that the config data passed in here will be in the format of:
		// "uniform name", "uniform type", "uniform group".
		// If this is not the case, then the file is formatted wrong.
		pszTrimmedData = StringTrim(pszConfigData);
		commaIndex = StringIndexOf(pszTrimmedData, ',');
		commentIndex = StringIndexOf(pszTrimmedData, '#');
		if (commaIndex == kInvalidID_U32)
		{
			if (pszTrimmedData)
				StringFree(pszTrimmedData);
			pszTrimmedData = nullptr;

			return false;
		}

		if (commentIndex != kInvalidID_U32)
		{
			// A comment is present, attempt to only parse up to the comment start.
			char* pszTempBuffer{ nullptr };
			pszTempBuffer = StringDuplicate_N(pszTrimmedData, commentIndex);
			StringFree(pszTrimmedData);
			pszTrimmedData = StringTrim(pszTempBuffer);
			StringFree(pszTempBuffer);
		}

		// Check the uniform type to see if it's an array.
		size_t arrayBracketIndex = StringIndexOf(pszTrimmedData, '[');
		size_t arrayEndBracketIndex = StringIndexOf(pszTrimmedData, ']');

		if (arrayBracketIndex != kInvalidID_U32)
		{
			if (arrayEndBracketIndex == kInvalidID_U32)
			{
				LOG_WARN("Formatting error in shader '%s'! <%s> array declaration is not properly formatted!", pOutResource.pszShaderName, pszConfigData);
				newUniform.iArraySize = 1;
				return false;
			}

			char* pszArrayLength = StringDuplicate_N(&pszTrimmedData[arrayBracketIndex + 1], arrayEndBracketIndex - arrayBracketIndex - 1);
			newUniform.iArraySize = String_ATOI(pszArrayLength);
			StringFree(pszArrayLength);
		}

		// Uniform name first.
		pszTempStringBuffer = StringDuplicate_N(pszTrimmedData, commaIndex);
		newUniform.pszName = StringStrip(pszTempStringBuffer, '\"');
		commaIndex2 = StringIndexOf(&pszTrimmedData[commaIndex + 1], ',');
		if (commaIndex2 == kInvalidID_U32)
		{
			return false;
		}

		// Uniform type next.
		StringFree(pszTempStringBuffer);
		if (arrayBracketIndex != kInvalidID_U32)
		{
			// If the type is an array type, strip the array piece out of the string, as that was already processed.
			pszTempStringBuffer = StringDuplicate_N(&pszTrimmedData[commaIndex + 1], arrayBracketIndex - commaIndex);
			size_t typeLength = StringLength(pszTempStringBuffer);
			pszTempStringBuffer[typeLength - 1] = '\"';		// Make sure to add the '"' back to the end of the type string so we match the formatting of the other types.
			pszTempStringBuffer[typeLength] = 0;
		}
		else
		{
			// Otherwise, just copy the whole type over.
			pszTempStringBuffer = StringDuplicate_N(&pszTrimmedData[commaIndex + 1], commaIndex2);
		}
		newUniform.pszType = StringStrip(pszTempStringBuffer, '\"');

		// Uniform update group next.
		StringFree(pszTempStringBuffer);
		pszTempStringBuffer = StringDuplicate(&pszTrimmedData[commaIndex + 1 + commaIndex2 + 1]);
		newUniform.pszFreq = StringStrip(pszTempStringBuffer, '\"');

		// Release temp string data.
		if (pszTempStringBuffer)
			StringFree(pszTempStringBuffer);

		if (pszTrimmedData)
			StringFree(pszTrimmedData);

		// Add the uniform to the resource.
		pOutResource.ShaderUniforms.PushBack(newUniform);

		return true;
	}
}