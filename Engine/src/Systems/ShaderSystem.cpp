//=============================================================================
// Author: Jared Simons
// Date Created: 3/12/2025
// 
// Purpose: The shader system.
//=============================================================================
#include "ShaderSystem.h"

#include <Core/Assertions.h>
#include <Core/Logging.h>
#include <Containers/CHashMap.h>
#include <Platform/FileSystem.h>
#include "Renderer/Renderer.h"
#include <Strings/String_Utils.h>

//-------------------------------------------------------------------------
// Shader System Implementation.
//-------------------------------------------------------------------------
class CShaderSystem : public IShaderSystem
{
public:
	virtual bool	Initialize(ShaderSystemConfig_t config) override;
	virtual void	Shutdown() override;

	virtual ShaderHandle_t	AcquireShader(const char* pszShaderName) override;

	virtual void	BindShader(ShaderHandle_t hShader) override;

private:
	ShaderConfig_t	ParseShaderConfigurationFromFile(const char* pszFilePath);
	VertexType_t	ParseVertexTypeFromString(const char* pszType);

private:
	ShaderSystemConfig_t	m_Config{};
	ShaderHandle_t			m_hCurrentlyBoundShader{ kInvalidID_U32 };

	struct ShaderLookup
	{
		ShaderHandle_t	ShaderIndex{ kInvalidID_U32 };
	};

	struct ShaderDictionary
	{
		Utl::CHashMap<const char*, ShaderLookup> m_ShaderLookup{};
		Shader* m_ShaderArray{ nullptr };
	};

	ShaderDictionary	m_ShaderDictionary{};
};

// Shader System context.
static CShaderSystem s_ShaderSystem;
IShaderSystem* g_pShaderSystem = &s_ShaderSystem;

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CShaderSystem::Initialize(ShaderSystemConfig_t config)
{
	CAssert(config.MaxShaders > 0);
	m_Config = config;
	m_ShaderDictionary.m_ShaderLookup.Resize(m_Config.MaxShaders);

	// Allocate memory for our shader array.
	m_ShaderDictionary.m_ShaderArray = (Shader*)new Shader[m_Config.MaxShaders];

	LOG_INFO("Shader system initialized! (Max shaders: %d)", config.MaxShaders);
	return true;
}

//-------------------------------------------------------------------------
// Shutdown.
//-------------------------------------------------------------------------
void CShaderSystem::Shutdown()
{
	// Release internal resources
	for (int idx = 0; idx < m_Config.MaxShaders; idx++)
	{
		if (m_ShaderDictionary.m_ShaderArray[idx].m_iInternalHandle != kInvalidID_U32)
		{
			g_pRenderer->ReleaseShaderResources(m_ShaderDictionary.m_ShaderArray[idx]);
			m_ShaderDictionary.m_ShaderArray[idx].m_iInternalHandle = kInvalidID_U32;
		}
	}

	delete[] m_ShaderDictionary.m_ShaderArray;
}

//-------------------------------------------------------------------------
// Loads a material at the provided path, and acquires it's resources if it
// hasn't been loaded.
// Returns a handle to the shader.
//-------------------------------------------------------------------------
ShaderHandle_t CShaderSystem::AcquireShader(const char* pszShaderName)
{
	// Make sure our state is ready to go.
	if (!m_ShaderDictionary.m_ShaderArray)
	{
		LOG_WARN("CShaderSystem::AcquireShader() called before material system was initialized! Returning NULL handle.");
		return 0;
	}

	if (!pszShaderName || !pszShaderName[0])
	{
		LOG_ERROR("AcquireShader() requires a shader to load. Nothing was done.");
		return -1;
	}

	Shader* newShader = nullptr;
	int shaderIdx = -1;

	// Get an entry into the hashmap.
	ShaderLookup* newShaderLookup = &m_ShaderDictionary.m_ShaderLookup[pszShaderName];
	if (!newShaderLookup)
	{
		LOG_ERROR("Failed to find free space for a new shader in the shader hashmap, returning default.");
		return 0; // TODO: Default shader.
	}

	if (newShaderLookup->ShaderIndex == kInvalidID_U32) // This shader does not yet exist in our hashmap.
	{
		char formattedShaderPath[kScratchStringBufferLength];
		*formattedShaderPath = *pszShaderName;

		// Check to see if the shader exists.
		if (!FileSystem::FileExists(pszShaderName))
		{
			// It's possible that we were passed the name of the shader without a path.
			// Tack on the path and try again.
			StringFormat_V("%s/%s.%s", formattedShaderPath, "Assets/Shaders", pszShaderName, "cshadercfg");

			if (!FileSystem::FileExists(formattedShaderPath))
			{
				LOG_ERROR("Shader '%s' can't be found! Shader acquisition failed.", pszShaderName);
				return -1;
			}
		}

		// Get at the first free shader slot.
		for (shaderIdx = 0; shaderIdx < m_Config.MaxShaders; shaderIdx++)
		{
			if (!m_ShaderDictionary.m_ShaderArray[shaderIdx].m_szShaderName)
			{
				newShader = &m_ShaderDictionary.m_ShaderArray[shaderIdx];
				break;
			}
		}

		if (!newShader)
		{
			LOG_ERROR("Could not load shader '%s' because all shader slots were full, returning default shader.", pszShaderName);
			return 0; // TODO: Default shader.
		}
		newShader->m_iInternalHandle = shaderIdx;

		// Parse the shader configuration.
		ShaderConfig_t newShaderConfiguration = ParseShaderConfigurationFromFile(formattedShaderPath);
		
		// Acquire shader resources.
		if (newShaderConfiguration.pszShaderName != nullptr)
		{
			newShader->m_szShaderName = StringDuplicate(newShaderConfiguration.pszShaderName);
		}

		// Acquire renderer resources.
		if (!g_pRenderer->AcquireShaderResources(*newShader, newShaderConfiguration))
		{
			LOG_ERROR("Failed to acquire renderer resources for shader '%s'. Returning default shader.", pszShaderName);
			// TODO: Free resources and return default shader.
			return 0;
		}
	}
	else
	{
		newShader = &m_ShaderDictionary.m_ShaderArray[newShaderLookup->ShaderIndex];
	}

	if (!newShader)
	{
		LOG_ERROR("Failed to load/create shader '%s'. Returning default shader.", pszShaderName);
		return 0; // TODO: Default shader.
	}

	if (newShaderLookup->ShaderIndex == kInvalidID_U32)
		newShaderLookup->ShaderIndex = shaderIdx;

	LOG_VERBOSE("Successfully acquired shader: '%s', at index %d", newShader->m_szShaderName, newShaderLookup->ShaderIndex);
	return newShaderLookup->ShaderIndex;
}

//-------------------------------------------------------------------------
// TODO: Documentation pls.
//-------------------------------------------------------------------------
ShaderConfig_t CShaderSystem::ParseShaderConfigurationFromFile(const char* pszFilePath)
{
	ShaderConfig_t newShaderConfig{};

	// Open the file
	FileHandle_t shaderConfigFile{};
	if (!FileSystem::OpenFile(pszFilePath, eFileMode::FILEMODE_READ, false, &shaderConfigFile))
	{
		LOG_ERROR("ParseShaderConfigurationFromFile -> Could not open file '%s'. Nothing was done.", pszFilePath);
		FileSystem::CloseFile(&shaderConfigFile);
		return newShaderConfig;
	}

	// Parse the file's contents.
	while (!FileSystem::IsEOF(&shaderConfigFile))
	{
		char currentLine[14000];
		size_t currentLineLength{ 0 };
		size_t bytesRead{ 0 };
		size_t equalIndex{ 0 };
		char key[500];
		char value[500];

		FileSystem::ReadLine(currentLine, 14000, &bytesRead, &shaderConfigFile);
		currentLineLength = StringLength(currentLine);

		if (currentLineLength <= 0 || currentLine[0] == '#') // Blank line or comment
			continue;

		equalIndex = StringIndexOf(currentLine, '=');
		memcpy(key, currentLine, equalIndex);
		key[equalIndex] = '\0';
		memcpy(value, &currentLine[equalIndex + 1], bytesRead - equalIndex);
		value[bytesRead - 1] = '\0';

		if (StringCompare(key, "name"))
		{
			newShaderConfig.pszShaderName = StringStrip(value, *"\""); // Strip the quotes out of the value
		}
		else if (StringCompare(key, "num_shader_stages"))
		{
			newShaderConfig.numShaderStages = String_ATOI(value);
			newShaderConfig.pszShaderStageFileNames = new const char* [newShaderConfig.numShaderStages];
		}
		else if (StringCompare(key, "vertex_file") || StringCompare(key, "fragment_file"))
		{
			int newIdx = 0;
			for (newIdx = 0; newIdx < newShaderConfig.numShaderStages; newIdx++)
			{
				if (newShaderConfig.pszShaderStageFileNames[newIdx] == nullptr)
					break;
			}

			newShaderConfig.pszShaderStageFileNames[newIdx] = StringStrip(value, *"\""); // Strip the quotes out of the value
		}
		else if (StringCompare(key, "topology_type"))
		{
			if (StringCompare(value, "line_list"))
			{
				newShaderConfig.eTopologyTypes |= LINE_LIST;
			}

			if (StringCompare(value, "triangle_list"))
			{
				newShaderConfig.eTopologyTypes |= TRIANGLE_LIST;
			}
		}
		else if (StringCompare(key, "attribute")) // Vertex attribute
		{
			// Vertex attributes are CSV lists.
			char* currentChar = &value[0];
			int commaIndex = StringIndexOf(currentChar, ',');
			VertexAttribute_t currentAttribute{};

			// Get the name first.
			currentAttribute.szName = StringDuplicate_N(value, commaIndex);
			currentChar = value + commaIndex + 1;
			// Next the type.
			currentAttribute.type = ParseVertexTypeFromString(currentChar);

			newShaderConfig.vertexAttributes.PushBack(currentAttribute);
		}
		else if (StringCompare(key, "uniform"))
		{
			// Uniform configs are CSV lists.
			char* currentChar = &value[0];
			int commaIndex = StringIndexOf(currentChar, ',');
			ShaderUniformConfig_t newUniform{};

			// Get the name first.
			newUniform.szName = StringDuplicate_N(value, commaIndex);
			currentChar = value + commaIndex + 1;

			// Next the type.
			commaIndex = StringIndexOf(currentChar, ',');
			char* pTypeString = StringDuplicate_N(currentChar, commaIndex);

			// Check to see if the if this is uniform is an array.
			int arrayStartIndex = StringIndexOf(pTypeString, '[');
			if (arrayStartIndex != kInvalidID_U32)
			{
				int arrayEndIndex = StringIndexOf(pTypeString, ']');
				if (arrayEndIndex == kInvalidID_U32)
				{
					// Syntax error in shader configuration!!!
					CAssert(0);
					LOG_ERROR("ParseShaderConfigurationFromFile() -> Shader Uniform '%s' declares an array type with a syntax error!", value);
					continue;
				}
				else
				{
					// Get the array size and save it off.
					int currIndex = arrayStartIndex + 1;
					int currPower = 1;
					while (currIndex != arrayEndIndex)
					{
						newUniform.iArraySize += (int)pow(pTypeString[currIndex] - '0', currPower);
						currPower++;
						currIndex++;
					}
				}
			}
			else
			{
				// No array, just one element.
				newUniform.iArraySize = 1;
			}

			// Strip off the array elements.
			if (arrayStartIndex != kInvalidID_U32)
				pTypeString[arrayStartIndex] = '\0';

			newUniform.eType = ParseVertexTypeFromString(pTypeString);
			StringFree(pTypeString);
			currentChar = currentChar + commaIndex + 1;

			// Next the uniform update level.
			int updateLevel = String_ATOI(currentChar);
			newUniform.eFrequency = (eShaderUniformUpdateFrequency)String_ATOI(currentChar);

			newShaderConfig.uniformConfigs.PushBack(newUniform);
		}
		else
		{
			LOG_WARN("Failed to parse shader config key: '%s'.", key);
		}
	}

	FileSystem::CloseFile(&shaderConfigFile);

	return newShaderConfig;
}

VertexType_t CShaderSystem::ParseVertexTypeFromString(const char* pszType)
{
	if (StringCompare(pszType, "int"))
	{
		return VertexType_t::Int32;
	}
	else if (StringCompare(pszType, "vec2"))
	{
		return VertexType_t::Vector2;
	}
	else if (StringCompare(pszType, "vec3"))
	{
		return VertexType_t::Vector3;
	}
	else if (StringCompare(pszType, "vec4"))
	{
		return VertexType_t::Vector4;
	}
	else if (StringCompare(pszType, "mat4"))
	{
		return VertexType_t::Matrix4x4;
	}
	else if (StringCompare(pszType, "texture"))
	{
		return VertexType_t::Texture;
	}
	else
	{
		LOG_WARN("ParseVertexTypeFromString() -> Failed to parse: '%s'", pszType);
		return VertexType_t::Undefined;
	}
}

void CShaderSystem::BindShader(ShaderHandle_t hShader)
{
	if (m_ShaderDictionary.m_ShaderArray[hShader].m_iInternalHandle != kInvalidID_U32)
	{
		g_pRenderer->BindShader(m_ShaderDictionary.m_ShaderArray[hShader]);
	}
}