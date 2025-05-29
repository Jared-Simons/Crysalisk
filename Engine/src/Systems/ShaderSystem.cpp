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
	virtual void			ReleaseShader(ShaderHandle_t hShader) override;

	virtual void			AcquireShaderInstanceResources(ShaderHandle_t hShader, unsigned int* pOutInstanceID) override;

	virtual void			BindShader(ShaderHandle_t hShader) override;

	virtual void			BindPerFrameUniforms() override;
	virtual void			BindPerInstanceUniforms(unsigned int hInstanceID) override;

	virtual uint16_t		GetShaderUniformLocation(ShaderHandle_t hShader, char* pszUniformName) override;
	virtual void			SetShaderUniform(ShaderHandle_t hShader, uint16_t uniformLocation, unsigned int hInstanceID, void* pData) override;

private:
	ShaderHandle_t	FindFirstFreeShaderHandle() const;

	void				ParseShaderConfigurationFromFile(const char* pszFilePath, ShaderConfig_t& pOutShaderConfig);
	ShaderAttribute_t	ParseVertexTypeFromString(const char* pszType);
	void				ParseSamplerTypesFromString(const char* pszTypeString, Utl::CVector<eKnownTextureTypes> pOutSamplerTypes);

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

	// DEPRECATED!!!
	ShaderHandle_t			m_hCurrentlyBoundShader_OLD{ kInvalidID_U32 };
};

// Shader System context.
static CShaderSystem s_ShaderSystem;
IShaderSystem* g_pShaderSystem = &s_ShaderSystem;

// Forward Declarations
static eShaderStage				GetShaderStageFromString(const char* pszString);
static eShaderTopologyType		GetTopologyTypeFromString(const char* pszString);
static VertexAttribute_t		GetVertexAttributeDataFromString(const char* pszString);
static ShaderUniformConfig_t	GetShaderAttributeFromString(const char* pszAttributeStr, const char* pszGroupStr);
static inline size_t			GetAttributeStride(ShaderAttribute_t inAttribute);
static size_t					GetExtraByteCountToEnsureAlignment(size_t currentOffset, ShaderAttribute_t typeToAlign);
static void						AlignUniformBufferLayout(Shader* pShader, ShaderConfig_t* pShaderConfig);

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
		pShader->m_szShaderName = StringDuplicate(newShaderResource.pszShaderName);
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
		CAssert((newShaderConfig.vertexStride == sizeof(Vertex3D)) || (newShaderConfig.vertexStride == sizeof(Vertex2D)));

		// Parse the uniform configs.
		for (int uniformIdx = 0; uniformIdx < newShaderResource.ShaderUniforms.Count(); uniformIdx++)
		{
			ShaderUniformConfig_t newUniform = GetShaderAttributeFromString(newShaderResource.ShaderUniforms[uniformIdx].pszType, newShaderResource.ShaderUniforms[uniformIdx].pszFreq);
			newUniform.szName = StringDuplicate(newShaderResource.ShaderUniforms[uniformIdx].pszName);
			newUniform.iArraySize = newShaderResource.ShaderUniforms[uniformIdx].iArraySize;
			newShaderConfig.uniformConfigs.PushBack(newUniform);

			// Keep track of our group strides.
			if (newUniform.eFrequency == eShaderUniformGroup::GLOBAL)
				newShaderConfig.perFrameUBOStride += newUniform.iStride;
			else if (newUniform.eFrequency == eShaderUniformGroup::OBJECT)
				newShaderConfig.perMaterialUBOStride += newUniform.iStride;
			else
				newShaderConfig.perDrawUBOStride += newUniform.iStride;
		}

		// Make sure our uniform buffers are aligned properly.
		{
			// TODO: Handle this properly.
			size_t test = newShaderConfig.perFrameUBOStride % 16;
			newShaderConfig.perFrameUBOStride += (16 - (newShaderConfig.perFrameUBOStride % 16));
		}

		// TODO: Don't hardcode this.
		newShaderConfig.m_RendererView = g_pRenderer->m_SceneView;
		if (StringCompare(pszShaderName, "Core.UI.New"))
		{
			newShaderConfig.m_RendererView = g_pRenderer->m_UIView;
		}

		// If internal resource acquisition was successful, create a hashmap to store lookups for our uniforms.
		size_t uniformCount = newShaderResource.ShaderUniforms.Count();
		pShader->m_UniformLookup.Resize(uniformCount * 2);	// Larger than necessary number to (hopefully!) avoid the chance of a hash collision.
		pShader->m_iUniformCount = uniformCount;
		pShader->m_pUniforms = new ShaderUniform_t[uniformCount];
		size_t	iRunningPerFrameOffset{ 0 };
		size_t	iRunningPerMaterialOffset{ 0 };
		for (int idx = 0; idx < uniformCount; idx++)
		{
			ShaderUniformConfig_t& pCurrentConfig = newShaderConfig.uniformConfigs[idx];
			// Find all our buffer offsets and save off our groups.
			size_t currentStride = GetAttributeStride(pCurrentConfig.eType);
			if (pCurrentConfig.eFrequency == eShaderUniformGroup::GLOBAL)
			{
				pShader->m_pUniforms[idx].eGroup = eShaderUniformGroup::GLOBAL;
				pShader->m_pUniforms[idx].Offset = iRunningPerFrameOffset;
				iRunningPerFrameOffset += currentStride;
			}
			else if (pCurrentConfig.eFrequency == eShaderUniformGroup::OBJECT)
			{
				pShader->m_pUniforms[idx].eGroup = eShaderUniformGroup::OBJECT;
				pShader->m_pUniforms[idx].Offset = iRunningPerMaterialOffset;
				iRunningPerMaterialOffset += currentStride;
			}
			else
			{
				// We don't need to store off stride info for per-draw uniforms.
				pShader->m_pUniforms[idx].eGroup = eShaderUniformGroup::PER_DRAW;
			}
			pShader->m_pUniforms[idx].Stride = currentStride;
			pShader->m_pUniforms[idx].eType = pCurrentConfig.eType;
			
			// Store off the location.
			pShader->m_UniformLookup[pCurrentConfig.szName] = idx;
		}

		// Make sure our uniform data layout is aligned properly.
		AlignUniformBufferLayout(pShader, &newShaderConfig);

		// Save off our UBO strides.
		pShader->m_iPerFrameUBOSize = newShaderConfig.perFrameUBOStride;
		pShader->m_iPerInstanceUBOSize = newShaderConfig.perMaterialUBOStride;

		// Set the internal shader handle.
		pShader->m_iInternalHandle = newShaderHandle;

		// Acquire internal resources for the shader.
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
// TODO: Documentation.
//-------------------------------------------------------------------------
void CShaderSystem::ReleaseShader(ShaderHandle_t hShader)
{
	// TODO: clean this up and make it more flexible!
	Shader* pShader = &m_ShaderDictionary.m_ShaderArray[hShader];

	if (pShader->m_pUniforms)
		delete[] pShader->m_pUniforms;

	g_pRenderer->DestroyShader(pShader);
	pShader->m_iInternalHandle = kInvalidID_U32;
}

//-------------------------------------------------------------------------
// TODO: Documentation.
//-------------------------------------------------------------------------
void CShaderSystem::AcquireShaderInstanceResources(ShaderHandle_t hShader, unsigned int* pOutInstanceID)
{
	Shader* pShader = &m_ShaderDictionary.m_ShaderArray[hShader];
	CAssert(pShader->m_iInternalHandle != kInvalidID_U32);

	g_pRenderer->AcquireShaderInstanceResources(pShader, pOutInstanceID);
}

//-------------------------------------------------------------------------
// TODO: Documentation.
//-------------------------------------------------------------------------
void CShaderSystem::BindShader(ShaderHandle_t hShader)
{
	Shader* pShader = &m_ShaderDictionary.m_ShaderArray[hShader];
	CAssert(pShader && pShader->m_pInternalData); // Don't try to bind a nonexistant shader!

	m_hCurrentlyBoundShader = hShader;
	// The renderer will do all the work for us.
	g_pRenderer->BindShader(pShader);

	if (pShader->m_iRendererFrame != g_pRenderer->m_iCurrentRendererFrame)
	{
		// If we have not been bound yet this frame, update our per-frame descriptors.
		BindPerFrameUniforms();

		pShader->m_iRendererFrame = g_pRenderer->m_iCurrentRendererFrame;
	}
}

//-------------------------------------------------------------------------
// TODO: Documentation.
//-------------------------------------------------------------------------
void CShaderSystem::BindPerFrameUniforms()
{
	Shader* pShader = &m_ShaderDictionary.m_ShaderArray[m_hCurrentlyBoundShader];
	CAssert(pShader);
	g_pRenderer->BindShaderPerFrameUniforms(pShader);
}

void CShaderSystem::BindPerInstanceUniforms(unsigned int hInstanceID)
{
	Shader* pShader = &m_ShaderDictionary.m_ShaderArray[m_hCurrentlyBoundShader];
	CAssert(pShader);
	g_pRenderer->BindShaderPerInstanceUniforms(pShader, hInstanceID);
}

//-------------------------------------------------------------------------
// TODO: Documentation.
//-------------------------------------------------------------------------
uint16_t CShaderSystem::GetShaderUniformLocation(ShaderHandle_t hShader, char* pszUniformName)
{
	CAssert(hShader != kInvalidID_U32);
	if (!pszUniformName)
	{
		LOG_ERROR("Failed to acquire shader uniform location for '%s'. Returning invalid handle.", pszUniformName);
		return kInvalidID_U16;
	}

	Shader* pShader = &m_ShaderDictionary.m_ShaderArray[hShader];
	return pShader->m_UniformLookup[pszUniformName];
}

//-------------------------------------------------------------------------
// TODO: Documentation.
//-------------------------------------------------------------------------
void CShaderSystem::SetShaderUniform(ShaderHandle_t hShader, uint16_t uniformLocation, unsigned int hInstanceID, void* pData)
{
	CAssert(hShader != kInvalidID_U16 && uniformLocation != kInvalidID_U16 && pData);
	Shader* pShader = &m_ShaderDictionary.m_ShaderArray[hShader];
	ShaderUniform_t* pUniform = &pShader->m_pUniforms[uniformLocation];

	g_pRenderer->SetShaderUniform(pShader, pUniform, hInstanceID, pData);
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
		attrib.type = ShaderAttribute_t::Int32;
		attrib.stride = sizeof(int32_t);
	}
	else if (StringCompareInsensitive(pszString, "vec2") || StringCompareInsensitive(pszString, "vector2d"))
	{
		attrib.type = ShaderAttribute_t::Vector2;
		attrib.stride = sizeof(Vector2D);
	}
	else if (StringCompareInsensitive(pszString, "vec3") || StringCompareInsensitive(pszString, "vector3d"))
	{
		attrib.type = ShaderAttribute_t::Vector3;
		attrib.stride = sizeof(Vector3D);
	}
	else if (StringCompareInsensitive(pszString, "vec4") || StringCompareInsensitive(pszString, "vector4d"))
	{
		attrib.type = ShaderAttribute_t::Vector4;
		attrib.stride = sizeof(Vector4D);
	}
	else if (StringCompareInsensitive(pszString, "mat4") || StringCompareInsensitive(pszString, "matrix4x4"))
	{
		attrib.type = ShaderAttribute_t::Matrix4x4;
		attrib.stride = sizeof(Matrix4x4);
	}
	else
	{
		LOG_WARN("Failed to parse vertex type '%s'. Returning undefined result!", pszString);
		attrib.type = ShaderAttribute_t::Undefined;
		attrib.stride = 4;
	}

	return attrib;
}

//-------------------------------------------------------------------------
// TODO: Documentation pls.
//-------------------------------------------------------------------------
static ShaderUniformConfig_t GetShaderAttributeFromString(const char* pszAttributeStr, const char* pszGroupStr)
{
	ShaderUniformConfig_t newUniformConfig{};

	// Get the type and stride.
	if (StringCompareInsensitive(pszAttributeStr, "int"))
	{
		newUniformConfig.eType = ShaderAttribute_t::Int32;
		newUniformConfig.iStride = sizeof(int32_t);
	}
	else if (StringCompareInsensitive(pszAttributeStr, "vec2") || StringCompareInsensitive(pszAttributeStr, "vector2d"))
	{
		newUniformConfig.eType = ShaderAttribute_t::Vector2;
		newUniformConfig.iStride = sizeof(Vector2D);
	}
	else if (StringCompareInsensitive(pszAttributeStr, "vec3") || StringCompareInsensitive(pszAttributeStr, "vector3d"))
	{
		newUniformConfig.eType = ShaderAttribute_t::Vector3;
		newUniformConfig.iStride = sizeof(Vector3D);
	}
	else if (StringCompareInsensitive(pszAttributeStr, "vec4") || StringCompareInsensitive(pszAttributeStr, "vector4d"))
	{
		newUniformConfig.eType = ShaderAttribute_t::Vector4;
		newUniformConfig.iStride = sizeof(Vector4D);
	}
	else if (StringCompareInsensitive(pszAttributeStr, "mat4") || StringCompareInsensitive(pszAttributeStr, "matrix4x4"))
	{
		newUniformConfig.eType = ShaderAttribute_t::Matrix4x4;
		newUniformConfig.iStride = sizeof(Matrix4x4);
	}
	else if (StringCompareInsensitive(pszAttributeStr, "texture") || StringCompareInsensitive(pszAttributeStr, "sampler"))
	{
		newUniformConfig.eType = ShaderAttribute_t::Texture;
		newUniformConfig.iStride = 0; // Samplers have no size.
	}
	else
	{
		LOG_WARN("Failed to parse shader attribute '%s'. Returning undefined result!", pszAttributeStr);
		newUniformConfig.eType = ShaderAttribute_t::Undefined;
		newUniformConfig.iStride = 4;
	}

	// Get the uniform group.
	if (StringCompareInsensitive(pszGroupStr, "frame"))
	{
		newUniformConfig.eFrequency = eShaderUniformGroup::GLOBAL;
	}
	else if (StringCompareInsensitive(pszGroupStr, "material"))
	{
		newUniformConfig.eFrequency = eShaderUniformGroup::OBJECT;
	}
	else if (StringCompareInsensitive(pszGroupStr, "draw"))
	{
		newUniformConfig.eFrequency = eShaderUniformGroup::PER_DRAW;
	}
	else
	{
		LOG_WARN("Failed to parse shader uniform group '%s'. Returning per-material.", pszGroupStr);
		newUniformConfig.eFrequency = eShaderUniformGroup::OBJECT;
	}

	return newUniformConfig;
}

//-------------------------------------------------------------------------
// TODO: Documentation pls.
//-------------------------------------------------------------------------
inline size_t GetAttributeStride(ShaderAttribute_t inAttribute) 
{
	switch (inAttribute)
	{
	case ShaderAttribute_t::Undefined:
	case ShaderAttribute_t::Int32:
	default:
		return sizeof(int32_t);
	case ShaderAttribute_t::Vector2:
		return sizeof(Vector2D);
	case ShaderAttribute_t::Vector3:
		return sizeof(Vector3D);
	case ShaderAttribute_t::Vector4:
		return sizeof(Vector4D);
	case ShaderAttribute_t::Matrix4x4:
		return sizeof(Matrix4x4);
	case ShaderAttribute_t::Texture:
		return 0; // Samplers don't count.
	}
}

//-------------------------------------------------------------------------
// TODO: Documentation pls.
//-------------------------------------------------------------------------
static size_t GetExtraByteCountToEnsureAlignment(size_t currentOffset, ShaderAttribute_t typeToAlign)
{
	size_t boundaryToAlignTo{ 0 };
	switch (typeToAlign)
	{
	case ShaderAttribute_t::Int32:
		boundaryToAlignTo = 4;
		break;

	case ShaderAttribute_t::Vector2:
		boundaryToAlignTo = 8;
		break;

	case ShaderAttribute_t::Vector3:
	case ShaderAttribute_t::Vector4:
	case ShaderAttribute_t::Matrix4x4:
		boundaryToAlignTo = 16;
		break;

	case ShaderAttribute_t::Texture:
		return 0;

	default:
		CAssert(0);
	}

	if (currentOffset % boundaryToAlignTo != 0)
	{
		// Need to push forward some.
		return boundaryToAlignTo - (currentOffset % boundaryToAlignTo);
	}
	else
	{
		return 0;
	}
}

//-------------------------------------------------------------------------
// TODO: Documentation pls.
//-------------------------------------------------------------------------
static void AlignUniformBufferLayout(Shader* pShader, ShaderConfig_t* pShaderConfig)
{
	size_t iCurrentPerFrameOffset{ 0 };
	size_t iCurrentPerMaterialOffset{ 0 };
	size_t iCurrentPerDrawOffset{ 0 };

	size_t uniformCount = pShader->m_iUniformCount;
	size_t currentStride = 0;
	ShaderUniform_t* pCurrentUniform = nullptr;
	for (int idx = 0; idx < uniformCount; idx++)
	{
		pCurrentUniform = &pShader->m_pUniforms[idx];

		switch (pCurrentUniform->eGroup)
		{
		case eShaderUniformGroup::GLOBAL:
			iCurrentPerFrameOffset += GetExtraByteCountToEnsureAlignment(iCurrentPerFrameOffset, pCurrentUniform->eType);
			// Update the individual uniform offset here, as it might've shifted during alignment.
			pCurrentUniform->Offset = iCurrentPerFrameOffset;
			iCurrentPerFrameOffset += pCurrentUniform->Stride;
			break;

		case eShaderUniformGroup::OBJECT:
			iCurrentPerMaterialOffset += GetExtraByteCountToEnsureAlignment(iCurrentPerMaterialOffset, pCurrentUniform->eType);
			// Update the individual uniform offset here, as it might've shifted during alignment.
			pCurrentUniform->Offset = iCurrentPerMaterialOffset;
			iCurrentPerMaterialOffset += pCurrentUniform->Stride;
			break;
			
		case eShaderUniformGroup::PER_DRAW:
			iCurrentPerDrawOffset += GetExtraByteCountToEnsureAlignment(iCurrentPerDrawOffset, pCurrentUniform->eType);
			// Update the individual uniform offset here, as it might've shifted during alignment.
			pCurrentUniform->Offset = iCurrentPerDrawOffset;
			iCurrentPerDrawOffset += pCurrentUniform->Stride;
			break;

		default:
			CAssert(0);
		}
	}

	// Update the UBO strides.
	pShaderConfig->perFrameUBOStride = iCurrentPerFrameOffset;
	pShaderConfig->perMaterialUBOStride = iCurrentPerMaterialOffset;
	pShaderConfig->perDrawUBOStride = iCurrentPerDrawOffset;
}

//-------------------------------------------------------------------------
// TODO: Documentation pls.
//-------------------------------------------------------------------------
static eShaderUniformGroup GetShaderUniformGroupFromString(const char* pszString) 
{
	if (StringCompareInsensitive(pszString, "frame"))
	{
		return eShaderUniformGroup::GLOBAL;
	}
	else if (StringCompareInsensitive(pszString, "material"))
	{
		return eShaderUniformGroup::OBJECT;
	}
	else if (StringCompareInsensitive(pszString, "draw"))
	{
		return eShaderUniformGroup::PER_DRAW;
	}
	else
	{
		LOG_WARN("Failed to parse shader uniform group '%s'. Returning per-material.", pszString);
		return eShaderUniformGroup::OBJECT;
	}
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
			newUniform.eFrequency = (eShaderUniformGroup)String_ATOI(currentChar);

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

ShaderAttribute_t CShaderSystem::ParseVertexTypeFromString(const char* pszType)
{
	if (StringCompare(pszType, "int"))
	{
		return ShaderAttribute_t::Int32;
	}
	else if (StringCompare(pszType, "vec2"))
	{
		return ShaderAttribute_t::Vector2;
	}
	else if (StringCompare(pszType, "vec3"))
	{
		return ShaderAttribute_t::Vector3;
	}
	else if (StringCompare(pszType, "vec4"))
	{
		return ShaderAttribute_t::Vector4;
	}
	else if (StringCompare(pszType, "mat4"))
	{
		return ShaderAttribute_t::Matrix4x4;
	}
	else if (StringCompare(pszType, "texture"))
	{
		return ShaderAttribute_t::Texture;
	}
	else
	{
		LOG_WARN("ParseVertexTypeFromString() -> Failed to parse: '%s'", pszType);
		return ShaderAttribute_t::Undefined;
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

ShaderHandle_t CShaderSystem::FindFirstFreeShaderHandle() const
{
	for (int idx = 0; idx < m_Config.MaxShaders; idx++)
	{
		if (m_ShaderDictionary.m_ShaderArray[idx].m_iInternalHandle == kInvalidID_U32 && m_ShaderDictionary.m_ShaderArray[idx].m_pInternalData == nullptr)
			return idx;
	}

	LOG_ERROR("Failed to find a free slot in the shader dictionary. Adjust config to allow for more.");
	return kInvalidID_U32;
}