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
#include <Resources/Loaders/ShaderLoader.h>
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

	virtual ShaderHandle_t	AcquireShader_OLD(const char* pszShaderName) override;

	virtual void	BindShader(ShaderHandle_t hShader) override;

private:
	ShaderHandle_t	FindFirstFreeShaderHandle() const;

	void			ParseShaderConfigurationFromFile(const char* pszFilePath, ShaderConfig_t& pOutShaderConfig);
	VertexType_t	ParseVertexTypeFromString(const char* pszType);
	void			ParseSamplerTypesFromString(const char* pszTypeString, Utl::CVector<eKnownTextureTypes> pOutSamplerTypes);

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

// Forward Declarations
static eShaderStage			GetShaderStageFromString(const char* pszString);
static eShaderTopologyType	GetTopologyTypeFromString(const char* pszString);
static VertexAttribute_t	GetVertexAttributeDataFromString(const char* pszString);

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
// TODO: Documentation.
//-------------------------------------------------------------------------
ShaderHandle_t CShaderSystem::AcquireShader(const char* pszShaderName)
{
	ShaderLoader::ShaderResource_t	newShaderResource{};
	ShaderHandle_t					newShaderHandle = kInvalidID_U32;
	ShaderLookup*					pLookup = &m_ShaderDictionary.m_ShaderLookup[pszShaderName];
	Shader*							pShader = nullptr;

	if (!pszShaderName)
	{
		LOG_ERROR("AcquireShader() called with null shader name! Returning null handle.");
		return kInvalidID_U32;
	}

	if (pLookup->ShaderIndex == kInvalidID_U32)
	{
		// This shader has not yet been loaded, so load it in.

		// Format the path string (TODO: Don't hardcode this path).
		char szFormattedResourcePath[kScratchStringBufferLength];
		StringFormat_V("%s/%s.cshadercfg", szFormattedResourcePath, "Assets/Shaders", pszShaderName);

		// Load up a shader configuration from disk
		if (!ShaderLoader::LoadShaderResource(szFormattedResourcePath, newShaderResource))
		{
			LOG_ERROR("Failed to load resources for shader '%s'. Aborting creation.", pszShaderName);
			return 0;
		}

		// Find a free slot for this shader to occupy.
		newShaderHandle = FindFirstFreeShaderHandle();
		if (newShaderHandle == kInvalidID_U32)
		{
			LOG_ERROR("Failed to create shader '%s'. Returning null handle.", pszShaderName);
			return kInvalidID_U32;
		}
		pShader = &m_ShaderDictionary.m_ShaderArray[newShaderHandle];

		ShaderConfig_t	newShaderConfig{};

		// Copy over config data.
		newShaderConfig.eTopologyTypes = GetTopologyTypeFromString(newShaderResource.pszTopologyType);
		newShaderConfig.bDepthTestingEnabled = newShaderResource.bDepthTestingEnabled;
		newShaderConfig.bColorBlendingEnabled = newShaderResource.bColorBlendingEnabled;

		// Setup shader module configs from resource.
		size_t shaderStageCount = newShaderResource.ShaderStages.Count();
		newShaderConfig.moduleConfigs.Resize(shaderStageCount);

		for (int stageIdx = 0; stageIdx < shaderStageCount; stageIdx++)
		{
			ShaderModuleConfig_t newConfig{};
			newConfig.stage = GetShaderStageFromString(newShaderResource.ShaderStages[stageIdx].pszStageString);
			newConfig.byteCode = newShaderResource.ShaderStages[stageIdx].pShaderModuleBytecode;
			newConfig.codeSize = newShaderResource.ShaderStages[stageIdx].CodeDataSize;

			newShaderConfig.moduleConfigs.PushBack(newConfig);
		}

		// Parse the vertex attribute data and stride.
		for (int attribIdx = 0; attribIdx < newShaderResource.ShaderAttributes.Count(); attribIdx++)
		{
			VertexAttribute_t newAttribute = GetVertexAttributeDataFromString(newShaderResource.ShaderAttributes[attribIdx].pszType);
			newAttribute.szName = StringDuplicate(newShaderResource.ShaderAttributes[attribIdx].pszName);

			// Save the new attribute.
			newShaderConfig.vertexAttributes.PushBack(newAttribute);
			newShaderConfig.vertexStride += newAttribute.stride;	// Make sure to keep track of our vertex stride.
		}
		// For now, make sure that the math adds up so we're configurable with an existing vertex type.
		CAssert((newShaderConfig.vertexStride == sizeof(Vertex3D)) || (newShaderConfig.vertexStride == sizeof(Vertex2D)))

		// Create internal resources for the shader.
		if (!g_pRenderer->CreateShader(pShader, newShaderConfig))
		{
			LOG_ERROR("Failed to create internal resources for shader '%s'. Returning null handle.", pszShaderName);
			// TODO: Release created resources and reset state.
			return false;
		}
	}
	else
	{
		// This shader's resources are already loaded, just return the index.
		newShaderHandle = pLookup->ShaderIndex;
	}

	return newShaderHandle;
}

//-------------------------------------------------------------------------
// Loads a material at the provided path, and acquires it's resources if it
// hasn't been loaded.
// Returns a handle to the shader.
//-------------------------------------------------------------------------
ShaderHandle_t CShaderSystem::AcquireShader_OLD(const char* pszShaderName)
{
	// Make sure our state is ready to go.
	if (!m_ShaderDictionary.m_ShaderArray)
	{
		LOG_WARN("CShaderSystem::AcquireShader_OLD() called before material system was initialized! Returning NULL handle.");
		return 0;
	}

	if (!pszShaderName || !pszShaderName[0])
	{
		LOG_ERROR("AcquireShader_OLD() requires a shader to load. Nothing was done.");
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
		ShaderConfig_t newShaderConfig{};
		ParseShaderConfigurationFromFile(formattedShaderPath, newShaderConfig);

		// TODO: DONT HARDCORE THIS!
		if (StringCompare(newShaderConfig.pszShaderName, "Core.Phong"))
		{
			newShaderConfig.vertexStride = sizeof(Vertex3D);
		}
		else
		{
			newShaderConfig.vertexStride = sizeof(Vertex2D);
		}
		
		// Acquire shader resources.
		if (newShaderConfig.pszShaderName != nullptr)
		{
			newShader->m_szShaderName = StringDuplicate(newShaderConfig.pszShaderName);
		}

		newShaderConfig.m_RendererView = g_pRenderer->m_SceneView;

		// Acquire renderer resources.
		if (!g_pRenderer->AcquireShaderResources_OLD(*newShader, newShaderConfig))
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
static eShaderStage	GetShaderStageFromString(const char* pszString)
{
	if (!pszString)
		return eShaderStage::UNDEFINED;

	if (StringCompareInsensitive(pszString, "Vertex") || StringCompareInsensitive(pszString, "Vert"))
	{
		return eShaderStage::VERTEX;
	}
	else if (StringCompareInsensitive(pszString, "Fragment") || StringCompareInsensitive(pszString, "Frag"))
	{
		return eShaderStage::FRAGMENT;
	}
	else
	{
		LOG_WARN("Failed to parse shader stage '%s'. Check shader configuration.", pszString);
		return eShaderStage::UNDEFINED;
	}
}

//-------------------------------------------------------------------------
// TODO: Documentation pls.
//-------------------------------------------------------------------------
static eShaderTopologyType GetTopologyTypeFromString(const char* pszString)
{
	if (!pszString)
		return SHADER_TOPOLOGY_TYPE_UNDEFINED;

	if (StringCompareInsensitive(pszString, "triangle_list") || StringCompareInsensitive(pszString, "triangle"))
	{
		return SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST;
	}
	else if (StringCompareInsensitive(pszString, "line_list") || StringCompareInsensitive(pszString, "line"))
	{
		return SHADER_TOPOLOGY_TYPE_LINE_LIST;
	}
	else
	{
		LOG_WARN("Failed to parse topology type '%s'. Check shader configuration.", pszString);
		return SHADER_TOPOLOGY_TYPE_UNDEFINED;
	}
}

//-------------------------------------------------------------------------
// TODO: Documentation pls.
//-------------------------------------------------------------------------
static VertexAttribute_t GetVertexAttributeDataFromString(const char* pszString)
{
	VertexAttribute_t attrib{};

	if (StringCompareInsensitive(pszString, "int"))
	{
		attrib.type = VertexType_t::Int32;
		attrib.stride = sizeof(int32_t);
	}
	else if (StringCompareInsensitive(pszString, "vec2") || StringCompareInsensitive(pszString, "vector2d"))
	{
		attrib.type = VertexType_t::Vector2;
		attrib.stride = sizeof(Vector2D);
	}
	else if (StringCompareInsensitive(pszString, "vec3") || StringCompareInsensitive(pszString, "vector3d"))
	{
		attrib.type = VertexType_t::Vector3;
		attrib.stride = sizeof(Vector3D);
	}
	else if (StringCompareInsensitive(pszString, "vec4") || StringCompareInsensitive(pszString, "vector4d"))
	{
		attrib.type = VertexType_t::Vector4;
		attrib.stride = sizeof(Vector4D);
	}
	else if (StringCompareInsensitive(pszString, "mat4") || StringCompareInsensitive(pszString, "matrix4x4"))
	{
		attrib.type = VertexType_t::Matrix4x4;
		attrib.stride = sizeof(Matrix4x4);
	}
	else
	{
		LOG_WARN("Failed to parse vertex type '%s'. Returning undefined result!", pszString);
		attrib.type = VertexType_t::Undefined;
		attrib.stride = 4;
	}

	return attrib;
}

//-------------------------------------------------------------------------
// TODO: Documentation pls.
//-------------------------------------------------------------------------
void CShaderSystem::ParseShaderConfigurationFromFile(const char* pszFilePath, ShaderConfig_t& pOutShaderConfig)
{
	// Open the file
	FileHandle_t shaderConfigFile{};
	if (!FileSystem::OpenFile(pszFilePath, eFileMode::FILEMODE_READ, false, &shaderConfigFile))
	{
		LOG_ERROR("ParseShaderConfigurationFromFile -> Could not open file '%s'. Nothing was done.", pszFilePath);
		FileSystem::CloseFile(&shaderConfigFile);
	}

	// TODO: We're resizing explicitly here because for whatever reason, something in memory allocation (during this resize) is stomping the memory that holds the names of the modules.
	// This needs to be fixed in at the allocator level.
	pOutShaderConfig.uniformConfigs.Resize(10);

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
			pOutShaderConfig.pszShaderName = StringStrip(value, *"\""); // Strip the quotes out of the value
		}
		else if (StringCompare(key, "num_shader_stages"))
		{
			pOutShaderConfig.numShaderStages = String_ATOI(value);
			pOutShaderConfig.pszShaderStageFileNames.Resize(pOutShaderConfig.numShaderStages);
		}
		else if (StringCompare(key, "vertex_file") || StringCompare(key, "fragment_file"))
		{
			int newIdx = 0;
			for (newIdx = 0; newIdx < pOutShaderConfig.numShaderStages; newIdx++)
			{
				if (pOutShaderConfig.pszShaderStageFileNames[newIdx] == nullptr)
					break;
			}

			pOutShaderConfig.pszShaderStageFileNames.PushBack(StringStrip(value, *"\"")); // Strip the quotes out of the value
		}
		else if (StringCompare(key, "topology_type"))
		{
			if (StringCompare(value, "line_list"))
			{
				pOutShaderConfig.eTopologyTypes |= LINE_LIST;
			}

			if (StringCompare(value, "triangle_list"))
			{
				pOutShaderConfig.eTopologyTypes |= TRIANGLE_LIST;
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

			pOutShaderConfig.vertexAttributes.PushBack(currentAttribute);
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

			pOutShaderConfig.uniformConfigs.PushBack(newUniform);
		}
		else if (StringCompare(key, "sampler_types"))
		{
			// Parse the sampler types.
			ParseSamplerTypesFromString(value, pOutShaderConfig.samplerTypes);
		}
		else
		{
			LOG_WARN("Failed to parse shader config key: '%s'.", key);
		}
	}

	FileSystem::CloseFile(&shaderConfigFile);
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

void CShaderSystem::ParseSamplerTypesFromString(const char* pszTypeString, Utl::CVector<eKnownTextureTypes> pOutSamplerTypes)
{
	LOG_DEBUG("Sampler types: %s", pszTypeString);
	char* pszTypes = StringDuplicate(pszTypeString);
	int commaIndex = StringIndexOf(pszTypeString, ',');
	int startIndex = 0;

	while (commaIndex != kInvalidID_U32)
	{
		// TODO: Clean this logic up.
		char* pszCurrentType = StringDuplicate_N(&pszTypeString[startIndex], commaIndex);

		if (StringCompare(pszCurrentType, "diffuse"))
		{
			pOutSamplerTypes.PushBack(eKnownTextureTypes::TEXTURE_TYPE_DIFFUSE);
		}
		else if (StringCompare(pszCurrentType, "specular"))
		{
			pOutSamplerTypes.PushBack(eKnownTextureTypes::TEXTURE_TYPE_SPECULAR);
		}
		else if (StringCompare(pszCurrentType, "normal"))
		{
			pOutSamplerTypes.PushBack(eKnownTextureTypes::TEXTURE_TYPE_NORMAL);
		}
		else
		{
			pOutSamplerTypes.PushBack(eKnownTextureTypes::TEXTURE_TYPE_UNDEFINED);
		}

		StringFree(pszCurrentType);
		startIndex += commaIndex + 1;
		commaIndex = StringIndexOf(&pszTypeString[startIndex], ',');

		if (commaIndex == kInvalidID_U32)
		{
			// There should be one more type after the last comma.
			pszCurrentType = StringDuplicate(&pszTypeString[startIndex]);
			if (StringCompare(pszCurrentType, "diffuse"))
			{
				pOutSamplerTypes.PushBack(eKnownTextureTypes::TEXTURE_TYPE_DIFFUSE);
			}
			else if (StringCompare(pszCurrentType, "specular"))
			{
				pOutSamplerTypes.PushBack(eKnownTextureTypes::TEXTURE_TYPE_SPECULAR);
			}
			else if (StringCompare(pszCurrentType, "normal"))
			{
				pOutSamplerTypes.PushBack(eKnownTextureTypes::TEXTURE_TYPE_NORMAL);
			}
			else
			{
				pOutSamplerTypes.PushBack(eKnownTextureTypes::TEXTURE_TYPE_UNDEFINED);
			}

			StringFree(pszCurrentType);
		}
	}
}

void CShaderSystem::BindShader(ShaderHandle_t hShader)
{
	if (m_ShaderDictionary.m_ShaderArray[hShader].m_iInternalHandle != kInvalidID_U32)
	{
		g_pRenderer->BindShader(m_ShaderDictionary.m_ShaderArray[hShader]);
	}
}

ShaderHandle_t CShaderSystem::FindFirstFreeShaderHandle() const
{
	for (int idx = 0; idx < m_Config.MaxShaders; idx++)
	{
		if (m_ShaderDictionary.m_ShaderArray[idx].m_iInternalHandle == kInvalidID_U32)
			return idx;
	}

	LOG_ERROR("Failed to find a free slot in the shader dictionary. Adjust config to allow for more.");
	return kInvalidID_U32;
}