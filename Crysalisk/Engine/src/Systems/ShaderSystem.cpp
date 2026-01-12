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
	// Initialization, shutdown.
	virtual bool			Initialize(ShaderSystemConfig_t config) override;
	virtual void			Shutdown() override;

	// Acquires the resources for the provided shader name.
	virtual ShaderHandle_t	AcquireShader(const char* pszShaderName) override;
	// Releases an existing reference to a shader.
	virtual void			ReleaseShader(ShaderHandle_t hShader) override;

	// Acquires a set of instance level resources for a shader.
	virtual void			AcquireShaderInstanceResources(ShaderHandle_t hShader, unsigned int* pOutInstanceID, const char* pszResourceName) override;

	// Releases a set of instance level resources for a shader.
	virtual void			ReleaseShaderInstanceResources(ShaderHandle_t hShader, unsigned int pInstanceID) override;

	// Binds a given shader for use in rendering ops.
	virtual void			BindShader(ShaderHandle_t hShader) override;

	// Binds the currently-bound shader's per-frame uniforms.
	virtual void			BindPerFrameUniforms() override;
	// Binds the currently-bound shader's per-instance uniforms (for the provided instance).
	virtual void			BindPerInstanceUniforms(unsigned int hInstanceID) override;

	// Gets the location for a particular shader uniform.
	virtual uint16_t		GetShaderUniformLocation(ShaderHandle_t hShader, char* pszUniformName) override;
	// Sets a particular shader uniform.
	virtual void			SetShaderUniform(ShaderHandle_t hShader, uint16_t uniformLocation, unsigned int iUniformArrayIndex, unsigned int hInstanceID, void* pData) override;

private:
	// Returns the first available handle in the shader dictionary.
	ShaderHandle_t			FindFirstFreeShaderHandle() const;

private:
	// The shader system configuration passed in from the application configuration.
	ShaderSystemConfig_t	m_Config{};

	// A lookup for a particular shader.
	struct ShaderLookup
	{
		// TODO: Handle ref-counting.
		// The internal handle of the shader.
		ShaderHandle_t	ShaderIndex{ kInvalidID_U32 };
	};

	struct ShaderDictionary
	{
		// Allows for string-based lookups for shaders.
		Utl::CStringHashMap<ShaderLookup>			m_ShaderLookup{};
		// An array of shaders.
		Shader*										m_ShaderArray{ nullptr };
	};

	// The shader dictionary.
	ShaderDictionary	m_ShaderDictionary{};

	// The currently bound shader.
	ShaderHandle_t		m_hCurrentlyBoundShader{ kInvalidID_U32 };
};

// Forward Declarations
static eShaderStage				GetShaderStageFromString(const char* pszString);
static eShaderTopologyType		GetTopologyTypeFromString(const char* pszString);
static VertexAttribute_t		GetVertexAttributeDataFromString(const char* pszString);
static ShaderUniformConfig_t	GetShaderAttributeFromString(const char* pszAttributeStr, const char* pszGroupStr);
static inline uint32_t			GetAttributeStride(ShaderAttribute_t inAttribute);
static uint32_t					GetExtraByteCountToEnsureAlignment(size_t currentOffset, ShaderAttribute_t typeToAlign);
static void						AlignUniformBufferLayout(Shader* pShader, ShaderConfig_t* pShaderConfig);

//-------------------------------------------------------------------------
// Purpose - TODO: Documentation.
//-------------------------------------------------------------------------
void IShaderSystem::AllocateShaderSystemState(IShaderSystem** ppSystemState)
{
	if (!ppSystemState)
	{
		LOG_ERROR("Pointer to hold input system state was not provided! Input system creation failed!");
		return;
	}

	*ppSystemState = new CShaderSystem;
}

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

	// Invalidate our shader lookup table.
	for (int idx = 0; idx < m_Config.MaxShaders; idx++)
	{
		m_ShaderDictionary.m_ShaderLookup.ValueElementAt(idx).ShaderIndex = kInvalidID_U32;
	}

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
			m_Config.pRenderer->DestroyShader(&m_ShaderDictionary.m_ShaderArray[idx]);
			m_ShaderDictionary.m_ShaderArray[idx].m_iInternalHandle = kInvalidID_U32;
		}
	}

	delete[] m_ShaderDictionary.m_ShaderArray;
}

//-------------------------------------------------------------------------
// Purpose: Acquires the resources for the provided shader name.
// Input:	pszShaderName - The name of the shader to acquire resources for.
// Output:	Returns a handle to the new shader.
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
		Utl::StringFormat_V("%s/%s.cshadercfg", szFormattedResourcePath, "Assets/Shaders", pszShaderName);

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
		pShader->m_pszShaderName = Utl::StringDuplicate(newShaderResource.pszShaderName);
		newShaderConfig.pszShaderName = Utl::StringDuplicate(newShaderResource.pszShaderName);
		newShaderConfig.eTopologyTypes = GetTopologyTypeFromString(newShaderResource.pszTopologyType);
		newShaderConfig.bDepthTestingEnabled = newShaderResource.bDepthTestingEnabled;
		newShaderConfig.bColorBlendingEnabled = newShaderResource.bColorBlendingEnabled;

		// Setup shader module configs from resource.
		size_t shaderStageCount = newShaderResource.ShaderStages.Count();
		newShaderConfig.moduleConfigs.Resize(shaderStageCount);

		for (int stageIdx = 0; stageIdx < shaderStageCount; stageIdx++)
		{
			ShaderModuleConfig_t newConfig{};
			newConfig.eStage = GetShaderStageFromString(newShaderResource.ShaderStages[stageIdx].pszStageString);
			newConfig.byteCode = newShaderResource.ShaderStages[stageIdx].pShaderModuleBytecode;
			newConfig.iCodeSize = newShaderResource.ShaderStages[stageIdx].CodeDataSize;
			//newConfig.pszModuleName = Utl::StringDuplicate(newShaderResource.ShaderStages[stageIdx].pszModuleName);

			newShaderConfig.moduleConfigs.PushBack(newConfig);
			// FIXME: This works but the string duplicate above doesn't?? Some freelist issues still present here.
			newShaderConfig.moduleConfigs[stageIdx].pszModuleName = Utl::StringDuplicate(newShaderResource.ShaderStages[stageIdx].pszModuleName);
		}

		// Parse the vertex attribute data and stride.
		for (int attribIdx = 0; attribIdx < newShaderResource.ShaderAttributes.Count(); attribIdx++)
		{
			VertexAttribute_t newAttribute = GetVertexAttributeDataFromString(newShaderResource.ShaderAttributes[attribIdx].pszType);
			newAttribute.szName = Utl::StringDuplicate(newShaderResource.ShaderAttributes[attribIdx].pszName);

			// Save the new attribute.
			newShaderConfig.vertexAttributes.PushBack(newAttribute);
			newShaderConfig.vertexStride += newAttribute.stride;	// Make sure to keep track of our vertex stride.
		}

		// Parse the uniform configs.
		for (int uniformIdx = 0; uniformIdx < newShaderResource.ShaderUniforms.Count(); uniformIdx++)
		{
			ShaderUniformConfig_t newUniform = GetShaderAttributeFromString(newShaderResource.ShaderUniforms[uniformIdx].pszType, newShaderResource.ShaderUniforms[uniformIdx].pszFreq);
			newUniform.szName = Utl::StringDuplicate(newShaderResource.ShaderUniforms[uniformIdx].pszName);
			newUniform.iArraySize = newShaderResource.ShaderUniforms[uniformIdx].iArraySize;
			newShaderConfig.uniformConfigs.PushBack(newUniform);

			// Keep track of our group strides.
			if (newUniform.eFrequency == eShaderUniformGroup::PER_FRAME)
				newShaderConfig.perFrameUBOStride += newUniform.iStride;
			else if (newUniform.eFrequency == eShaderUniformGroup::PER_MATERIAL)
				newShaderConfig.perInstanceUBOStride += newUniform.iStride;
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
		newShaderConfig.m_pRendererView = *m_Config.pRenderer->m_SceneView;
		if (Utl::StringCompare(pszShaderName, "Core.UI"))
		{
			newShaderConfig.m_pRendererView = *m_Config.pRenderer->m_UIView;
		}

		// If internal resource acquisition was successful, create a hashmap to store lookups for our uniforms.
		size_t uniformCount = newShaderResource.ShaderUniforms.Count();
		pShader->m_UniformLookup.Resize(uniformCount * 2);	// Larger than necessary number to (hopefully!) avoid the chance of a hash collision.
		pShader->m_iUniformCount = uniformCount;
		pShader->m_pUniforms = new ShaderUniform_t[uniformCount];
		uint32_t	iRunningPerFrameOffset{ 0 };
		uint32_t	iRunningPerMaterialOffset{ 0 };
		for (int idx = 0; idx < uniformCount; idx++)
		{
			ShaderUniformConfig_t& pCurrentConfig = newShaderConfig.uniformConfigs[idx];
			// Find all our buffer offsets and save off our groups.
			
			uint32_t currentStride{ 0 };
			
			// We don't want to do this for generic structures, as their strides are parsed elsewhere.
			if (pCurrentConfig.eType != ShaderAttribute_t::GenericStructure)
				currentStride = GetAttributeStride(pCurrentConfig.eType);
			else
				currentStride = pCurrentConfig.iStride;
			
			if (pCurrentConfig.eFrequency == eShaderUniformGroup::PER_FRAME)
			{
				pShader->m_pUniforms[idx].eGroup = eShaderUniformGroup::PER_FRAME;
				pShader->m_pUniforms[idx].Offset = iRunningPerFrameOffset;
				iRunningPerFrameOffset += currentStride;
			}
			else if (pCurrentConfig.eFrequency == eShaderUniformGroup::PER_MATERIAL)
			{
				pShader->m_pUniforms[idx].eGroup = eShaderUniformGroup::PER_MATERIAL;
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
			int* pUniform = pShader->m_UniformLookup.Insert(pCurrentConfig.szName);
			*pUniform = idx;
		}

		// Make sure our uniform data layout is aligned properly.
		AlignUniformBufferLayout(pShader, &newShaderConfig);

		// Save off our UBO strides.
		pShader->m_iPerFrameUBOSize = newShaderConfig.perFrameUBOStride;
		pShader->m_iPerInstanceUBOSize = newShaderConfig.perInstanceUBOStride;

		// Set the internal shader handle.
		pShader->m_iInternalHandle = newShaderHandle;
		pLookup->ShaderIndex = newShaderHandle;

		// Acquire internal resources for the shader.
		if (!m_Config.pRenderer->CreateShader(pShader, newShaderConfig))
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
// Purpose: Releases an existing reference to a shader.
// Input:	hShader - The shader to release a reference to.
// Output:	none.
//-------------------------------------------------------------------------
void CShaderSystem::ReleaseShader(ShaderHandle_t hShader)
{
	Shader* pShader = &m_ShaderDictionary.m_ShaderArray[hShader];

	if (pShader->m_pUniforms)
		delete[] pShader->m_pUniforms;

	m_Config.pRenderer->DestroyShader(pShader);
	pShader->m_iInternalHandle = kInvalidID_U32;
}

//-------------------------------------------------------------------------
// Purpose: Acquires a set of instance level resources for a shader.
// Input:	hShader - The shader to acquire instance resources from.
//			pOutInstanceID - A pointer to hold the instance ID for the new instance.
//			pszResourceName - Name to tag the shader resources with. Only used in debug builds.
// Output:	none.
//-------------------------------------------------------------------------
void CShaderSystem::AcquireShaderInstanceResources(ShaderHandle_t hShader, unsigned int* pOutInstanceID, const char* pszResourceName)
{
	Shader* pShader = &m_ShaderDictionary.m_ShaderArray[hShader];
	CAssert(pShader->m_iInternalHandle != kInvalidID_U32);

	m_Config.pRenderer->AcquireShaderInstanceResources(pShader, pOutInstanceID, pszResourceName);
}

//-------------------------------------------------------------------------
// Purpose: Releases a set of instance level resources for a shader.
// Input:	hShader - The shader to release resources for.
//			pInstanceID - The instance to be released.
// Output:	none.
//-------------------------------------------------------------------------
void CShaderSystem::ReleaseShaderInstanceResources(ShaderHandle_t hShader, unsigned int pInstanceID)
{
	Shader* pShader = &m_ShaderDictionary.m_ShaderArray[hShader];
	CAssert(pShader->m_iInternalHandle != kInvalidID_U32);

	m_Config.pRenderer->ReleaseShaderInstanceResources(pShader, pInstanceID);
}

//-------------------------------------------------------------------------
// Purpose: Binds a given shader for use in rendering ops.
// Input:	hShader - The shader to be bound.
// Output:	none.
//-------------------------------------------------------------------------
void CShaderSystem::BindShader(ShaderHandle_t hShader)
{
	// Don't bind an already bound shader!
	if (m_hCurrentlyBoundShader == hShader)
		return;

	// If we're given an invalid ID, then consider this state stale.
	// This usually happens because we need to re-bind the shader at least once a frame (these will NOT persist between frames).
	// Usually, a frame will end by clearing out this state.
	if (hShader == kInvalidID_U32)
	{
		m_hCurrentlyBoundShader = hShader;
		return;
	}

	Shader* pShader = &m_ShaderDictionary.m_ShaderArray[hShader];
	CAssert(pShader && pShader->m_iInternalHandle != kInvalidID_U32); // Don't try to bind a nonexistant shader!

	m_hCurrentlyBoundShader = hShader;
	// The renderer will do most of the work for us.
	m_Config.pRenderer->BindShader(pShader);

	if (pShader->m_iRendererFrame != m_Config.pRenderer->m_iCurrentRendererFrame)
	{
		// If we have not been bound yet this frame, update our per-frame descriptors.
		BindPerFrameUniforms();

		pShader->m_iRendererFrame = m_Config.pRenderer->m_iCurrentRendererFrame;
	}
}

//-------------------------------------------------------------------------
// Purpose: Binds the currently-bound shader's per-frame uniforms.
// Input:	none.
// Output:	none.
//-------------------------------------------------------------------------
void CShaderSystem::BindPerFrameUniforms()
{
	Shader* pShader = &m_ShaderDictionary.m_ShaderArray[m_hCurrentlyBoundShader];
	CAssert(pShader);
	m_Config.pRenderer->BindShaderPerFrameUniforms(pShader);
}

//-------------------------------------------------------------------------
// Purpose: Binds the currently-bound shader's per-instance uniforms (for the provided instance).
// Input:	hInstanceID - The instance to be bound.
// Output:	none.
//-------------------------------------------------------------------------
void CShaderSystem::BindPerInstanceUniforms(unsigned int hInstanceID)
{
	Shader* pShader = &m_ShaderDictionary.m_ShaderArray[m_hCurrentlyBoundShader];
	CAssert(pShader);
	m_Config.pRenderer->BindShaderPerInstanceUniforms(pShader, hInstanceID);
}

//-------------------------------------------------------------------------
// Purpose: Gets the location for a particular shader uniform.
// Input:	hShader - The shader whose uniform to find.
//			pszUniformName - The name of the uniform to find.
// Output:	The location of the uniform in the shader.
//-------------------------------------------------------------------------
uint16_t CShaderSystem::GetShaderUniformLocation(ShaderHandle_t hShader, char* pszUniformName)
{
	CAssert(hShader != kInvalidID_U32);
	Shader* pShader = &m_ShaderDictionary.m_ShaderArray[hShader];

	if (!pszUniformName)
	{
		LOG_ERROR("Failed to acquire shader uniform location for shader '%s'. Returning invalid handle.", pShader->m_pszShaderName);
		return kInvalidID_U16;
	}
	return pShader->m_UniformLookup[pszUniformName];
}

//-------------------------------------------------------------------------
// Purpose: Sets a particular shader uniform.
// Input:	hShader - The shader whose uniform to set.
//			uniformLocation - The location (index) of the uniform to set.
//			iUniformArrayIndex - The array index of the uniform to set. Some uniforms can be arrays, others just single values.
//			hInstanceID - The instance to be set, this only matters for per-instance uniforms. Not relevant for other groups.
//			pData - An opaque pointer to the value to be set.
// Output:	none.
//-------------------------------------------------------------------------
void CShaderSystem::SetShaderUniform(ShaderHandle_t hShader, uint16_t uniformLocation, unsigned int iUniformArrayIndex, unsigned int hInstanceID, void* pData)
{
	CAssert(hShader != kInvalidID_U16 && uniformLocation != kInvalidID_U16 && pData);
	Shader* pShader = &m_ShaderDictionary.m_ShaderArray[hShader];
	ShaderUniform_t* pUniform = &pShader->m_pUniforms[uniformLocation];

	m_Config.pRenderer->SetShaderUniform(pShader, pUniform, iUniformArrayIndex, hInstanceID, pData);
}

//-------------------------------------------------------------------------
// Purpose: Parses a shader stage from a string.
// Input:	pszString - The string representing the shader stage type.
// Output:	The typed shader stage value.
//-------------------------------------------------------------------------
static eShaderStage	GetShaderStageFromString(const char* pszString)
{
	if (!pszString)
		return eShaderStage::UNDEFINED;

	if (Utl::StringCompareInsensitive(pszString, "Vertex") || Utl::StringCompareInsensitive(pszString, "Vert"))
	{
		return eShaderStage::VERTEX;
	}
	else if (Utl::StringCompareInsensitive(pszString, "Fragment") || Utl::StringCompareInsensitive(pszString, "Frag"))
	{
		return eShaderStage::FRAGMENT;
	}
	else
	{
		LOG_WARN("Failed to parse shader eStage '%s'. Check shader configuration.", pszString);
		return eShaderStage::UNDEFINED;
	}
}

//-------------------------------------------------------------------------
// Purpose: Parses a shader topology type from a string.
// Input:	pszString - The string representing the shader topology type.
// Output:	The typed shader topology value.
//-------------------------------------------------------------------------
static eShaderTopologyType GetTopologyTypeFromString(const char* pszString)
{
	if (!pszString)
		return SHADER_TOPOLOGY_TYPE_UNDEFINED;

	if (Utl::StringCompareInsensitive(pszString, "triangle_list") || Utl::StringCompareInsensitive(pszString, "triangle"))
	{
		return SHADER_TOPOLOGY_TYPE_TRIANGLE_LIST;
	}
	else if (Utl::StringCompareInsensitive(pszString, "line_list") || Utl::StringCompareInsensitive(pszString, "line"))
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
// Purpose: Parses vertex attribute data from a string.
// Input:	pszString - The string representing vertex attribute.
// Output:	The typed vertex attribute.
//-------------------------------------------------------------------------
static VertexAttribute_t GetVertexAttributeDataFromString(const char* pszString)
{
	VertexAttribute_t attrib{};

	if (Utl::StringCompareInsensitive(pszString, "int") || Utl::StringCompareInsensitive(pszString, "int32"))
	{
		attrib.type = ShaderAttribute_t::Int32;
		attrib.stride = sizeof(int32_t);
	}
	else if (Utl::StringCompareInsensitive(pszString, "float") || Utl::StringCompareInsensitive(pszString, "float32"))
	{
		attrib.type = ShaderAttribute_t::Float32;
		attrib.stride = sizeof(float);
	}
	else if (Utl::StringCompareInsensitive(pszString, "vec2") || Utl::StringCompareInsensitive(pszString, "Vector2D_OLD"))
	{
		attrib.type = ShaderAttribute_t::Vector2;
		attrib.stride = sizeof(Vector2D);
	}
	else if (Utl::StringCompareInsensitive(pszString, "vec3") || Utl::StringCompareInsensitive(pszString, "Vector3D_OLD"))
	{
		attrib.type = ShaderAttribute_t::Vector3;
		attrib.stride = sizeof(Vector3D);
	}
	else if (Utl::StringCompareInsensitive(pszString, "vec4") || Utl::StringCompareInsensitive(pszString, "Vector4D_OLD"))
	{
		attrib.type = ShaderAttribute_t::Vector4;
		attrib.stride = sizeof(Vector4D);
	}
	else if (Utl::StringCompareInsensitive(pszString, "mat4") || Utl::StringCompareInsensitive(pszString, "Matrix4x4_OLD"))
	{
		attrib.type = ShaderAttribute_t::Matrix4;
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
// Purpose: Parses shader uniform data from a string.
// Input:	pszString - The string representing shader uniform.
// Output:	The typed shader uniform.
//-------------------------------------------------------------------------
static ShaderUniformConfig_t GetShaderAttributeFromString(const char* pszAttributeStr, const char* pszGroupStr)
{
	ShaderUniformConfig_t newUniformConfig{};

	// Get the type and stride.
	if (Utl::StringCompareInsensitive(pszAttributeStr, "int"))
	{
		newUniformConfig.eType = ShaderAttribute_t::Int32;
		newUniformConfig.iStride = sizeof(int32_t);
	}
	else if (Utl::StringCompareInsensitive(pszAttributeStr, "float32"))
	{
		newUniformConfig.eType = ShaderAttribute_t::Float32;
		newUniformConfig.iStride = sizeof(float);
	}
	else if (Utl::StringCompareInsensitive(pszAttributeStr, "vec2") || Utl::StringCompareInsensitive(pszAttributeStr, "Vector2D_OLD"))
	{
		newUniformConfig.eType = ShaderAttribute_t::Vector2;
		newUniformConfig.iStride = sizeof(Vector2D);
	}
	else if (Utl::StringCompareInsensitive(pszAttributeStr, "vec3") || Utl::StringCompareInsensitive(pszAttributeStr, "Vector3D_OLD"))
	{
		newUniformConfig.eType = ShaderAttribute_t::Vector3;
		newUniformConfig.iStride = sizeof(Vector3D);
	}
	else if (Utl::StringCompareInsensitive(pszAttributeStr, "vec4") || Utl::StringCompareInsensitive(pszAttributeStr, "Vector4D_OLD"))
	{
		newUniformConfig.eType = ShaderAttribute_t::Vector4;
		newUniformConfig.iStride = sizeof(Vector4D);
	}
	else if (Utl::StringCompareInsensitive(pszAttributeStr, "mat4") || Utl::StringCompareInsensitive(pszAttributeStr, "Matrix4x4_OLD"))
	{
		newUniformConfig.eType = ShaderAttribute_t::Matrix4;
		newUniformConfig.iStride = sizeof(Matrix4x4);
	}
	else if (Utl::StringCompareInsensitive(pszAttributeStr, "texture") || Utl::StringCompareInsensitive(pszAttributeStr, "sampler"))
	{
		newUniformConfig.eType = ShaderAttribute_t::Sampler;
		newUniformConfig.iStride = 0; // Samplers have no size.
	}
	else
	{
		// This is possibly an opaque struct of variable size, check for that possibility here.
		char* pszTempStringBuffer{ nullptr };
		pszTempStringBuffer = Utl::StringDuplicate_N(pszAttributeStr, 6);

		if (Utl::StringCompareInsensitive(pszTempStringBuffer, "struct"))
		{
			// We got a structure!
			// Get the length in bytes.
			size_t uniformDeclarationLength = Utl::StringLength(pszTempStringBuffer);
			char* pszStructLength = Utl::StringDuplicate(&pszAttributeStr[7]); // index 7 to pass over the 'struct-' chars.
			newUniformConfig.iStride = Utl::String_ATOI(pszStructLength);
			newUniformConfig.eType = ShaderAttribute_t::GenericStructure;

			if (pszStructLength)
				Utl::StringFree(pszStructLength);
		}
		else
		{
			// Nope! No structure, just error out.
			LOG_WARN("Failed to parse shader attribute '%s'. Returning undefined result!", pszAttributeStr);
			newUniformConfig.eType = ShaderAttribute_t::Undefined;
			newUniformConfig.iStride = 4;
		}

		// Free temp string data.
		if (pszTempStringBuffer)
			Utl::StringFree(pszTempStringBuffer);
	}

	// Get the uniform group.
	if (Utl::StringCompareInsensitive(pszGroupStr, "frame"))
	{
		newUniformConfig.eFrequency = eShaderUniformGroup::PER_FRAME;
	}
	else if (Utl::StringCompareInsensitive(pszGroupStr, "material"))
	{
		newUniformConfig.eFrequency = eShaderUniformGroup::PER_MATERIAL;
	}
	else if (Utl::StringCompareInsensitive(pszGroupStr, "draw"))
	{
		newUniformConfig.eFrequency = eShaderUniformGroup::PER_DRAW;
	}
	else
	{
		LOG_WARN("Failed to parse shader uniform group '%s'. Returning per-material.", pszGroupStr);
		newUniformConfig.eFrequency = eShaderUniformGroup::PER_MATERIAL;
	}

	return newUniformConfig;
}

//-------------------------------------------------------------------------
// Purpose: Gets a stride from the provided attribute.
// Input:	inAttribute - The attribute's whose stride to obtain.
// Output:	The attribute's stride.
//-------------------------------------------------------------------------
static inline uint32_t GetAttributeStride(ShaderAttribute_t inAttribute) 
{
	switch (inAttribute)
	{
	case ShaderAttribute_t::Undefined:
	case ShaderAttribute_t::Int32:
	default:
		return sizeof(int32_t);
	case ShaderAttribute_t::Float32:
		return sizeof(float);
	case ShaderAttribute_t::Vector2:
		return sizeof(Vector2D);
	case ShaderAttribute_t::Vector3:
		return sizeof(Vector3D);
	case ShaderAttribute_t::Vector4:
		return sizeof(Vector4D);
	case ShaderAttribute_t::Matrix4:
		return sizeof(Matrix4x4);
	case ShaderAttribute_t::Sampler:
		return 0; // Samplers don't count.
	}
}

//-------------------------------------------------------------------------
// Purpose: Parses a uniform group from a string.
// Input:	pszString - The string representation of the uniform group.
// Output:	The typed shader uniform group.
//-------------------------------------------------------------------------
static eShaderUniformGroup GetShaderUniformGroupFromString(const char* pszString)
{
	if (Utl::StringCompareInsensitive(pszString, "frame"))
	{
		return eShaderUniformGroup::PER_FRAME;
	}
	else if (Utl::StringCompareInsensitive(pszString, "material"))
	{
		return eShaderUniformGroup::PER_MATERIAL;
	}
	else if (Utl::StringCompareInsensitive(pszString, "draw"))
	{
		return eShaderUniformGroup::PER_DRAW;
	}
	else
	{
		LOG_WARN("Failed to parse shader uniform group '%s'. Returning per-material.", pszString);
		return eShaderUniformGroup::PER_MATERIAL;
	}
}

//-------------------------------------------------------------------------
// Purpose: Determines the amount of padding bytes that are required to align this offset to a specific boundary.
// Input:	currentOffset - The current, unaligned offset.
//			typeToAlign - The type/attribute to align to.
// Output:	The number of extra padding bytes needed for alignment.
//-------------------------------------------------------------------------
static uint32_t GetExtraByteCountToEnsureAlignment(size_t currentOffset, ShaderAttribute_t typeToAlign)
{
	uint32_t boundaryToAlignTo{ 0 };
	switch (typeToAlign)
	{
	case ShaderAttribute_t::Int32:
	case ShaderAttribute_t::Float32:
		boundaryToAlignTo = 4;
		break;

	case ShaderAttribute_t::Vector2:
		boundaryToAlignTo = 8;
		break;

	case ShaderAttribute_t::Vector3:
	case ShaderAttribute_t::Vector4:
	case ShaderAttribute_t::Matrix4:
	case ShaderAttribute_t::GenericStructure: // TODO: Proper alignment value?
		boundaryToAlignTo = 16;
		break;

	case ShaderAttribute_t::Sampler:
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
// Purpose: Given a shader, this method aligns it's uniforms to necessary boundaries.
// Input:	pShader - The shader whose uniforms are to be aligned.
//			pShaderConfig - The shader's associated configuration.
// Output:	none.
//-------------------------------------------------------------------------
static void AlignUniformBufferLayout(Shader* pShader, ShaderConfig_t* pShaderConfig)
{
	uint32_t iCurrentPerFrameOffset{ 0 };
	uint32_t iCurrentPerMaterialOffset{ 0 };
	uint32_t iCurrentPerDrawOffset{ 0 };

	size_t uniformCount = pShader->m_iUniformCount;
	size_t currentStride = 0;
	ShaderUniform_t* pCurrentUniform = nullptr;
	for (int idx = 0; idx < uniformCount; idx++)
	{
		pCurrentUniform = &pShader->m_pUniforms[idx];

		switch (pCurrentUniform->eGroup)
		{
		case eShaderUniformGroup::PER_FRAME:
			iCurrentPerFrameOffset += GetExtraByteCountToEnsureAlignment(iCurrentPerFrameOffset, pCurrentUniform->eType);
			// Update the individual uniform offset here, as it might've shifted during alignment.
			pCurrentUniform->Offset = iCurrentPerFrameOffset;
			iCurrentPerFrameOffset += pCurrentUniform->Stride;
			break;

		case eShaderUniformGroup::PER_MATERIAL:
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
	pShaderConfig->perInstanceUBOStride = iCurrentPerMaterialOffset;
	pShaderConfig->perDrawUBOStride = iCurrentPerDrawOffset;
}

//-------------------------------------------------------------------------
// Purpose: Find the first available handle in the shader dictionary.
// Input:	none.
// Output:	Returns the first available shader handle.
//-------------------------------------------------------------------------
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
