//=============================================================================
// Author: Jared Simons
// Date Created: 3/3/2025
// 
// Purpose: The material system.
//=============================================================================

#include "MaterialSystem.h"

#include <Containers/CHashMap.h>
#include <Core/Logging.h>
#include <Platform/FileSystem.h>
#include "Renderer/Renderer.h"
#include <Resources/Loaders/MaterialLoader.h>
#include <Strings/String_Utils.h>
#include "Systems/ShaderSystem.h"
#include "Systems/TextureSystem.h"

//-------------------------------------------------------------------------
// Purpose: Indices for each of the phong shader uniform lookups.
//-------------------------------------------------------------------------
struct PhongShaderLocations_t
{
	u16	projection{ kInvalidID_U16 };
	u16	view{ kInvalidID_U16 };
	u16	viewPos{ kInvalidID_U16 };
	u16	dirLight{ kInvalidID_U16 };
	u16	pointLight1{ kInvalidID_U16 };
	u16	pointLight2{ kInvalidID_U16 };
	u16	pointLight3{ kInvalidID_U16 };
	u16	pointLight4{ kInvalidID_U16 };
	u16	spotLight{ kInvalidID_U16 };
	u16	renderMode{ kInvalidID_U16 };
	u16	shininess{ kInvalidID_U16 };
	u16	color{ kInvalidID_U16 };
	u16	textureMaps{ kInvalidID_U16 };
	u16	model{ kInvalidID_U16 };
};
static PhongShaderLocations_t s_PhongShaderLocations{};

//-------------------------------------------------------------------------
// Material System Implementation.
//-------------------------------------------------------------------------
class CMaterialSystem : public IMaterialSystem
{
public:
	// Initialization, shutdown.
	virtual bool				Initialize(MaterialSystemConfig_t config) override;
	virtual void				PostInitialize() override;
	virtual void				Shutdown() override;

	// Acquires the resources for the provided material.
	virtual MaterialHandle_t	AcquireMaterial(const char* pszMaterialName) override;
	// Acquires a material given an existing resource.
	virtual MaterialHandle_t	AcquireMaterialFromResource(MaterialLoader::MaterialResource_t& pInResource) override;

	virtual void				ReleaseMaterial(MaterialHandle_t hShaderInstance, bool bForceResourceRelease) override;

	// Returns true if a given material is transparent in anyway, false if completely opaque.
	virtual u32			IsMaterialTransparent(MaterialHandle_t hShaderInstance) const override;

	// Applies a given material.
	virtual void				ApplyMaterialInstance(MaterialHandle_t hShaderInstance) override;

	// Returns a handle to the default texture.
	virtual MaterialHandle_t	GetDefaultMaterialReference() override;

	// Applies global render data for the frame.
	virtual void				ApplyGlobalRenderData(Matrix4x4& pMatProjection, Matrix4x4& pMatView, Vector& pVecPosition, i32 renderDebugMode) override;
	// Applies per-draw render data for the following draw calls.
	virtual void				ApplyPerDrawRenderData(Matrix4x4& pMatModel) override;

	// Sets directional light data to be used during rendering.
	virtual void				SetDirectionalLightData(Vector3D vecDirection, Vector3D vecLightColor, f32 flBrightness, Vector3D vecAmbientLightColor, f32 flAmbientStrength) override;
	virtual void				SetPointLightData(i32 lightIndex, Vector3D vecPoint, Vector3D vecLightColor, f32 flBrightness, f32 flConstant, f32 flLinear, f32 flQuadratic) override;
	virtual void				SetSpotLightData(Vector3D vecPosition, Vector3D vecDirection, Vector3D vecLightColor, f32 flBrightness, f32 flConstant, f32 flLinear, f32 flQuadratic, f32 flInnerCutoff, f32 flOuterCutoff) override;
	virtual void				ApplyLightData() override;

private:
	// Creates the default material(s) to be used in the engine.
	void						CreateDefaultMaterial();

	// Destroys the default material(s).
	void						DestroyDefaultMaterial();

	// Clears a given material dictionary entry.
	void						ClearMaterialDictionaryEntry(const char* pszMaterialName);

	// Returns the first available handle in the material dictionary.
	MaterialHandle_t			FindFirstFreeMaterialHandle() const;

private:
	// The material system configuration passed in from the application configuration.
	MaterialSystemConfig_t	m_Config{};

	// A lookup for a particular material.
	struct MaterialLookup
	{
		// The internal handle of the material.
		MaterialHandle_t MaterialIndex{ kInvalidID_U32 };
		// The current reference count of the material.
		i32				 ReferenceCount{ 0 };
		// Indicates if this material should automatically release it's resources when it's reference count reaches 0.
		// NOTE: This can only be set when first acquiring the material.
		bool			 AutoRelease{ false };
	};

	struct MaterialDictionary
	{
		// Allows for string-based lookups for materials.
		Utl::CStringHashMap<MaterialLookup>			m_MaterialHashmap{};
		// An array of materials.
		Material*									m_MaterialArray{ nullptr };
	};

	// The material dictionary.
	MaterialDictionary	m_MaterialDictionary{};

	// The lookup for the default material.
	MaterialLookup		m_DefaultMaterialLookup{};

	// The currently bound shader instance (corresponds to the currently bound material).
	MaterialHandle_t	m_hCurrentlyBoundMaterialInstance{ kInvalidID_U32 };
	// The default material shader (in case a custom shader is not available).
	ShaderHandle_t		m_DefaultMaterialShader{ kInvalidID_U32 };

	// Current light data.
	struct DirectionalLight_t
	{
		Vector3D	vecDirection{};
		f32		padding1{};
		Vector3D	lightColor{};
		f32		flBrightness{};
		Vector3D	ambientLightColor{};
		f32		flAmbientStrength{};
	} dirLight;

	struct PointLight_t
	{
		Vector4D	vecPosition{};
		Vector3D	lightColor{};
		f32		flBrightness{};

		f32		flConstant{};
		f32		flLinear{};
		f32		flQuadratic{};
	} pointLights[4];

	struct SpotLight_t
	{
		Vector4D	vecPosition{};
		Vector4D	vecDirection{};
		Vector3D	lightColor{};
		f32		flBrightness{};

		f32		flConstant{};
		f32		flLinear{};
		f32		flQuadratic{};

		f32		flInnerCutoff{};
		f32		flOuterCutoff{};
	} spotLight;
};

//-------------------------------------------------------------------------
// Purpose - TODO: Documentation.
//-------------------------------------------------------------------------
void IMaterialSystem::AllocateMaterialSystemState(IMaterialSystem** ppSystemState)
{
	if (!ppSystemState)
	{
		LOG_ERROR("Pointer to hold material system state was not provided! Material system creation failed!");
		return;
	}

	*ppSystemState = new CMaterialSystem;
}

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CMaterialSystem::Initialize(MaterialSystemConfig_t config)
{
	m_Config = config;
	m_MaterialDictionary.m_MaterialHashmap.Resize(m_Config.MaxMaterials);

	// Allocate memory for our material array.
	m_MaterialDictionary.m_MaterialArray = (Material*)new Material[m_Config.MaxMaterials];

	LOG_INFO("Material system initialized! (Max materials: %d)", config.MaxMaterials);

	return true;
}


//-------------------------------------------------------------------------
// TODO: Documentation.
//-------------------------------------------------------------------------
void CMaterialSystem::PostInitialize()
{
	// Setup shader locations.
	IShaderSystem* pShaderSystem = m_Config.pShaderSystem;
	m_DefaultMaterialShader = pShaderSystem->AcquireShader("Core.Phong");
	s_PhongShaderLocations.projection = pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "projection");
	s_PhongShaderLocations.view = pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "view");
	s_PhongShaderLocations.viewPos = pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "viewPos");
	s_PhongShaderLocations.dirLight = pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "directionalLight");
	s_PhongShaderLocations.pointLight1 = pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "pointLight1");
	s_PhongShaderLocations.pointLight2 = pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "pointLight2");
	s_PhongShaderLocations.pointLight3 = pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "pointLight3");
	s_PhongShaderLocations.pointLight4 = pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "pointLight4");
	s_PhongShaderLocations.spotLight = pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "spotlight");
	s_PhongShaderLocations.renderMode = pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "renderMode");
	s_PhongShaderLocations.shininess = pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "shininess");
	s_PhongShaderLocations.color = pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "color");
	s_PhongShaderLocations.textureMaps = pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "texture_maps");
	s_PhongShaderLocations.model = pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "model");

	// Default material.
	CreateDefaultMaterial();
}

//-------------------------------------------------------------------------
// Shutdown.
//-------------------------------------------------------------------------
void CMaterialSystem::Shutdown()
{
	for (i32 matIdx = 0; matIdx < m_Config.MaxMaterials; matIdx++)
	{
		ReleaseMaterial(matIdx, true);	// Force release of resources.
	}

	DestroyDefaultMaterial();
	delete[] m_MaterialDictionary.m_MaterialArray;
}

//-------------------------------------------------------------------------
// Purpose: Attempts to acquire the resources for a given material.
// Input:	pszMaterialName - The material to acquire resources for.
// Output:	Returns a handle to the new material.
//-------------------------------------------------------------------------
MaterialHandle_t CMaterialSystem::AcquireMaterial(const char* pszMaterialName)
{
	MaterialLoader::MaterialResource_t	newMaterialResource{};
	MaterialHandle_t					newMaterialHandle{ kInvalidID_U32 };
	MaterialLookup*						pLookup = nullptr;
	Material*							pMaterial = nullptr;

	if (!pszMaterialName)
	{
		LOG_ERROR("AcquireMaterial() called with null material name! Returning default handle.");
		return kInvalidID_U32; // TODO: Default material handle.
	}

	pLookup = &m_MaterialDictionary.m_MaterialHashmap[pszMaterialName];

	if (pLookup->MaterialIndex == kInvalidID_U32)
	{
		// This material has not yet been loaded, so load it in.

		// Format the path string (TODO: Don't hardcode this path).
		char szFormattedResourcePath[kScratchStringBufferLength];
		Utl::StringFormat_V("%s/%s.cmat", szFormattedResourcePath, "Assets/Materials", pszMaterialName);

		// Load the material configuration resource from disk
		if (!MaterialLoader::LoadMaterialResource(szFormattedResourcePath, newMaterialResource))
		{
			LOG_ERROR("Failed to load material resource '%s'. Returning default.", pszMaterialName);
			return 0; // TODO: Default material handle.
		}

		newMaterialHandle = AcquireMaterialFromResource(newMaterialResource);

		pLookup->ReferenceCount++;
		pLookup->AutoRelease = true; // NOTE: Materials automatically release themselves by default for now.
	}
	else
	{
		// The material is already loaded, just return the handle.
		// TODO: Handle reference counting.
		newMaterialHandle = pLookup->MaterialIndex;
	}

	return newMaterialHandle;
}

//-------------------------------------------------------------------------
// Purpose: Acquires a material given an existing resource.
// Input:	pInResource - The resource to use for the new material.
// Output:	Returns a handle to the new material.
//-------------------------------------------------------------------------
MaterialHandle_t CMaterialSystem::AcquireMaterialFromResource(MaterialLoader::MaterialResource_t& pInResource)
{
	IShaderSystem*		pShaderSystem = m_Config.pShaderSystem;
	MaterialHandle_t	newMaterialHandle{ kInvalidID_U32 };
	MaterialLookup*		pLookup = nullptr;
	Material*			pMaterial = nullptr;

	if (pInResource.pszMaterialName == nullptr)
	{
		LOG_ERROR("AcquireMaterialFromResource() called with null material name! Returning default handle.");
		return kInvalidID_U32; // TODO: Default material handle.
	}

	pLookup = &m_MaterialDictionary.m_MaterialHashmap[pInResource.pszMaterialName];

	if (pLookup->MaterialIndex == kInvalidID_U32)
	{
		// This material does not yet exist, acquire the config's resources.
		// Find a free slot for this material to occupy.
		newMaterialHandle = FindFirstFreeMaterialHandle();
		if (newMaterialHandle == kInvalidID_U32)
		{
			LOG_ERROR("Failed to create material '%s'. Returning default handle.", pInResource.pszMaterialName);
			return kInvalidID_U32;	// TODO: Default material handle.
		}
		pMaterial = &m_MaterialDictionary.m_MaterialArray[newMaterialHandle];

		// Acquire material resources.
		pMaterial->m_pszMaterialName = Utl::StringDuplicate(pInResource.pszMaterialName);

		if (pInResource.pszShaderName)
		{
			pMaterial->m_hShaderHandle = pShaderSystem->AcquireShader(pInResource.pszShaderName);
		}
		else
		{
			// Default shader name, in case none was provided.
			pMaterial->m_hShaderHandle = pShaderSystem->AcquireShader("Core.Phong");
		}

		// Acquire shader instance resources for this material.
		pShaderSystem->AcquireShaderInstanceResources(pMaterial->m_hShaderHandle, &pMaterial->m_iInternalHandle, pInResource.pszMaterialName);

		// Texture maps.
		u64							textureMapCount = pInResource.textureMapConfigs.Count();
		Utl::CVector<TextureHandle_t>	textureHandles(textureMapCount);

		// Set some default textures, just in case.
		TextureHandle_t hDefaultDiffuse = m_Config.pTextureSystem->GetDefaultDiffuseTexture();
		TextureHandle_t hDefaultSpecular = m_Config.pTextureSystem->GetDefaultSpecularTexture();
		TextureHandle_t hDefaultNormal = m_Config.pTextureSystem->GetDefaultNormalTexture();

		textureHandles.PushBack(hDefaultDiffuse);
		textureHandles.PushBack(hDefaultSpecular);
		textureHandles.PushBack(hDefaultNormal);

		for (i32 texIdx = 0; texIdx < textureMapCount; texIdx++)
		{
			TextureHandle_t hTexture = m_Config.pTextureSystem->AcquireTexture(pInResource.textureMapConfigs[texIdx].pszTextureName, true);
			if (pInResource.textureMapConfigs[texIdx].eType == MaterialLoader::MaterialResource_t::eTextureType::DIFFUSE)
			{
				textureHandles[0] = hTexture;
			}
			else if (pInResource.textureMapConfigs[texIdx].eType == MaterialLoader::MaterialResource_t::eTextureType::SPECULAR)
			{
				textureHandles[1] = hTexture;
			}
			else if (pInResource.textureMapConfigs[texIdx].eType == MaterialLoader::MaterialResource_t::eTextureType::NORMAL)
			{
				textureHandles[2] = hTexture;
			}
		}

		// Bind the default textures for now, the actual textures will get bound after they finish loading.
		pShaderSystem->SetShaderUniform(pMaterial->m_hShaderHandle, s_PhongShaderLocations.textureMaps, 0, pMaterial->m_iInternalHandle, &hDefaultDiffuse);
		pShaderSystem->SetShaderUniform(pMaterial->m_hShaderHandle, s_PhongShaderLocations.textureMaps, 1, pMaterial->m_iInternalHandle, &hDefaultSpecular);
		pShaderSystem->SetShaderUniform(pMaterial->m_hShaderHandle, s_PhongShaderLocations.textureMaps, 2, pMaterial->m_iInternalHandle, &hDefaultNormal);

		// Mark the material so that it will check back on the texture load status.
		pMaterial->m_iMaterialFlags |= MATERIAL_FLAG_INTERNALS_UPDATED;

		// Save off the texture handles.
		pMaterial->m_hDiffuseTextureHandle = textureHandles[0];
		pMaterial->m_hSpecularTextureHandle = textureHandles[1];
		pMaterial->m_hNormalTextureHandle = textureHandles[2];

		// Set any instance-level uniform data.
		CAssert(pInResource.pszCustomKeys.Count() == pInResource.pszCustomValues.Count());
		for (i32 idx = 0; idx < pInResource.pszCustomKeys.Count(); idx++)
		{
			if (Utl::StringCompareInsensitive(pInResource.pszCustomKeys[idx], "specular_shininess"))
			{
				f32 flShininess = Utl::String_ATOF(pInResource.pszCustomValues[idx]);
				// Set the instance uniform.
				pShaderSystem->SetShaderUniform(pMaterial->m_hShaderHandle, s_PhongShaderLocations.shininess, 0, pMaterial->m_iInternalHandle, &flShininess);
			}
			else
			{
				LOG_WARN("Failed to parse custom material key '%s'.", pInResource.pszCustomKeys[idx]);
				continue;
			}
		}

		pLookup->ReferenceCount++;

		// Save off the index.
		pLookup->MaterialIndex = newMaterialHandle;
	}
	else
	{
		// This material is already loaded, just return the handle.
		newMaterialHandle = pLookup->MaterialIndex;
		pLookup->ReferenceCount++;
	}

	return newMaterialHandle;
}

//-------------------------------------------------------------------------
// Purpose: Releases an existing reference to a material.
// Input:	hMaterial - The material to release a reference to.
//			bForceResourceRelease - Forces the material to release all it's resources,
//									regardless of it's reference count.
// Output:	none.
//-------------------------------------------------------------------------
void CMaterialSystem::ReleaseMaterial(MaterialHandle_t hShaderInstance, bool bForceResourceRelease)
{
	Material&		pMaterial = m_MaterialDictionary.m_MaterialArray[hShaderInstance];

	if (pMaterial.m_iInternalHandle == kInvalidID_U32 || pMaterial.m_pszMaterialName == nullptr)
	{
		// It's possible this material has already been released. Boot out.
		return;
	}

	MaterialLookup& pLookup = m_MaterialDictionary.m_MaterialHashmap[pMaterial.m_pszMaterialName];

	// Do nothing with default materials.
	if (pLookup.MaterialIndex == m_DefaultMaterialLookup.MaterialIndex)
	{
		return;
	}

	pLookup.ReferenceCount--;

	CAssert(pLookup.ReferenceCount >= 0);

	if (pLookup.ReferenceCount == 0 && (pLookup.AutoRelease == true || bForceResourceRelease))
	{
		LOG_VERBOSE("Releasing material '%s'.", pMaterial.m_pszMaterialName);

		m_Config.pShaderSystem->ReleaseShaderInstanceResources(pMaterial.m_hShaderHandle, pMaterial.m_iInternalHandle);

		m_Config.pTextureSystem->ReleaseTexture(pMaterial.m_hDiffuseTextureHandle);
		m_Config.pTextureSystem->ReleaseTexture(pMaterial.m_hSpecularTextureHandle);
		m_Config.pTextureSystem->ReleaseTexture(pMaterial.m_hNormalTextureHandle);

		ClearMaterialDictionaryEntry(pMaterial.m_pszMaterialName);
	}
}

//-------------------------------------------------------------------------
// Purpose:	Returns the state of a material's transparency.
// Input:	hMaterial - The material whose transparency to check for.
// Output:	Returns true if the material is transparent in any way, otherwise false.
//-------------------------------------------------------------------------
u32 CMaterialSystem::IsMaterialTransparent(MaterialHandle_t hShaderInstance) const
{
	if (hShaderInstance == kInvalidID_U32)
		//hMaterial = m_DefaultMaterialLookup.MaterialIndex;
		hShaderInstance = 0; // TODO: Use the default material here.
	Material* pMaterial = &m_MaterialDictionary.m_MaterialArray[hShaderInstance];
	//CAssert(hMaterial != kInvalidID_U32 && pMaterial && pMaterial->m_iInternalHandle != kInvalidID_U32);

	return pMaterial->m_bIsTransparent;
}

//-------------------------------------------------------------------------
// Purpose:	Binds a given material.
// Input:	hMaterial - The material to be bound.
// Output:	none.
// TODO: Make sure we can assert that a frame is currently being rendered when this function is called.
//-------------------------------------------------------------------------
void CMaterialSystem::ApplyMaterialInstance(MaterialHandle_t hShaderInstance)
{
	if (hShaderInstance == kInvalidID_U32)
		hShaderInstance = 0; // TODO: This is not elegant

	// Don't do any extra work!
	if (m_hCurrentlyBoundMaterialInstance == hShaderInstance)
		return;

	Material& pMaterial = m_MaterialDictionary.m_MaterialArray[hShaderInstance];

	// If the instance is invalid, then we can't draw!
	if (pMaterial.m_iInternalHandle == kInvalidID_U32)
		return;

	// See if we need to check internals.
	if (pMaterial.m_iMaterialFlags & MATERIAL_FLAG_INTERNALS_UPDATED)
	{
		bool bDiffuseLoaded = false;
		bool bSpecularLoaded = false;
		bool bNormalLoaded = false;

		// This material is being used but has new textures, update internals to point to the new textures.
		if (m_Config.pTextureSystem->IsTextureLoaded(pMaterial.m_hDiffuseTextureHandle))
		{
			m_Config.pShaderSystem->SetShaderUniform(pMaterial.m_hShaderHandle, s_PhongShaderLocations.textureMaps, 0, pMaterial.m_iInternalHandle, &pMaterial.m_hDiffuseTextureHandle);
			bDiffuseLoaded = true;

			// Use the diffuse map to determine material transparency.
			pMaterial.m_bIsTransparent = m_Config.pTextureSystem->IsTextureFlagSet(pMaterial.m_hDiffuseTextureHandle, TEXTURE_FLAG_IS_TRANSPARENT);
		}

		if (m_Config.pTextureSystem->IsTextureLoaded(pMaterial.m_hSpecularTextureHandle))
		{
			m_Config.pShaderSystem->SetShaderUniform(pMaterial.m_hShaderHandle, s_PhongShaderLocations.textureMaps, 1, pMaterial.m_iInternalHandle, &pMaterial.m_hSpecularTextureHandle);
			bSpecularLoaded = true;
		}

		if (m_Config.pTextureSystem->IsTextureLoaded(pMaterial.m_hNormalTextureHandle))
		{
			m_Config.pShaderSystem->SetShaderUniform(pMaterial.m_hShaderHandle, s_PhongShaderLocations.textureMaps, 2, pMaterial.m_iInternalHandle, &pMaterial.m_hNormalTextureHandle);
			bNormalLoaded = true;
		}

		// If all of our textures are loaded, we can stop checking.
		if (bDiffuseLoaded && bSpecularLoaded && bNormalLoaded)
		{
			pMaterial.m_iMaterialFlags &= ~MATERIAL_FLAG_INTERNALS_UPDATED;
		}

		// Bind the appropriate shader.
		m_Config.pShaderSystem->BindShader(m_MaterialDictionary.m_MaterialArray[hShaderInstance].m_hShaderHandle);

		// Bind the appropriate material instance.
		m_Config.pShaderSystem->BindPerInstanceUniforms(m_MaterialDictionary.m_MaterialArray[m_DefaultMaterialLookup.MaterialIndex].m_iInternalHandle);

		m_hCurrentlyBoundMaterialInstance = m_DefaultMaterialLookup.MaterialIndex;
	}
	else
	{
		// Bind the appropriate shader.
		m_Config.pShaderSystem->BindShader(m_MaterialDictionary.m_MaterialArray[hShaderInstance].m_hShaderHandle);

		// Bind the appropriate material instance.
		m_Config.pShaderSystem->BindPerInstanceUniforms(m_MaterialDictionary.m_MaterialArray[hShaderInstance].m_iInternalHandle);

		m_hCurrentlyBoundMaterialInstance = hShaderInstance;
	}
}

//-------------------------------------------------------------------------
// Returns a reference to the default texture.
//-------------------------------------------------------------------------
MaterialHandle_t CMaterialSystem::GetDefaultMaterialReference()
{
	return m_DefaultMaterialLookup.MaterialIndex;
}

//-------------------------------------------------------------------------
// Applies global render data for the frame.
// NOTE: This lives here and not in the renderer because we need access
// to Material-Specific shader information.
//-------------------------------------------------------------------------
void CMaterialSystem::ApplyGlobalRenderData(Matrix4x4& pMatProjection, Matrix4x4& pMatView, Vector& pVecPosition, i32 renderDebugMode)
{
	IShaderSystem* pShaderSystem = m_Config.pShaderSystem;

	pShaderSystem->BindShader(m_DefaultMaterialShader);

	pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.projection, 0, 0, &pMatProjection);
	pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.view, 0, 0, &pMatView);
	pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.viewPos, 0, 0, &pVecPosition);
	pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.renderMode, 0, 0, &renderDebugMode);

	// Clear out the currently bound material instance!
	// This is necessary so that we properly re-bind material instances every frame.
	m_hCurrentlyBoundMaterialInstance = kInvalidID_U32;
}

//-------------------------------------------------------------------------
// Applies per-draw render data for the following draw calls.
// NOTE: This lives here and not in the renderer because we need access
// to Material-Specific shader information.
//-------------------------------------------------------------------------
void CMaterialSystem::ApplyPerDrawRenderData(Matrix4x4& pMatModel)
{
	m_Config.pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.model, 0, 0, &pMatModel);
}

//-------------------------------------------------------------------------
// Sets directional light data to be used during rendering.
// NOTE: This lives here and not in the renderer because we need access
// to Material-Specific shader information.
//-------------------------------------------------------------------------
void CMaterialSystem::SetDirectionalLightData(Vector3D vecDirection, Vector3D vecLightColor, f32 flBrightness, Vector3D vecAmbientLightColor, f32 flAmbientStrength)
{
	dirLight.vecDirection = Vector3D{ M_DEG2RAD_F(vecDirection.x), M_DEG2RAD_F(vecDirection.y), M_DEG2RAD_F(vecDirection.z) };;
	dirLight.flBrightness = flBrightness;
	dirLight.lightColor = vecLightColor;
	dirLight.ambientLightColor = vecAmbientLightColor;
	dirLight.flAmbientStrength = flAmbientStrength;
}

//-------------------------------------------------------------------------
// Sets point light data to be used during rendering.
// NOTE: This lives here and not in the renderer because we need access
// to Material-Specific shader information.
//-------------------------------------------------------------------------
void CMaterialSystem::SetPointLightData(i32 lightIndex, Vector3D vecPoint, Vector3D vecLightColor, f32 flBrightness, f32 flConstant, f32 flLinear, f32 flQuadratic)
{
	pointLights[lightIndex].vecPosition = vecPoint;
	pointLights[lightIndex].lightColor = vecLightColor;
	pointLights[lightIndex].flBrightness = flBrightness;
	pointLights[lightIndex].flConstant = flConstant;
	pointLights[lightIndex].flLinear = flLinear;
	pointLights[lightIndex].flQuadratic = flQuadratic;
}

//-------------------------------------------------------------------------
// Sets spot light data to be used during rendering.
// NOTE: This lives here and not in the renderer because we need access
// to Material-Specific shader information.
//-------------------------------------------------------------------------
void CMaterialSystem::SetSpotLightData(Vector3D vecPosition, Vector3D vecDirection, Vector3D vecLightColor, f32 flBrightness, f32 flConstant, f32 flLinear, f32 flQuadratic, f32 flInnerCutoff, f32 flOuterCutoff)
{
	CAssert(flInnerCutoff <= flOuterCutoff);

	spotLight.vecPosition = vecPosition;
	// Direction needs to be converted from degrees to radians.
	spotLight.vecDirection = Vector3D{ M_DEG2RAD_F(vecDirection.x), M_DEG2RAD_F(vecDirection.y), M_DEG2RAD_F(vecDirection.z) };
	spotLight.lightColor = vecLightColor;
	spotLight.flBrightness = flBrightness;
	spotLight.flConstant = flConstant;
	spotLight.flLinear = flLinear;
	spotLight.flQuadratic = flQuadratic;
	spotLight.flInnerCutoff = M_COS_F(M_DEG2RAD_F(flInnerCutoff));
	spotLight.flOuterCutoff = M_COS_F(M_DEG2RAD_F(flOuterCutoff));
}

//-------------------------------------------------------------------------
// Pushes current light data to the shaders for use.
// NOTE: This lives here and not in the renderer because we need access
// to Material-Specific shader information.
//-------------------------------------------------------------------------
void CMaterialSystem::ApplyLightData()
{
	IShaderSystem* pShaderSystem = m_Config.pShaderSystem;

	pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.dirLight, 0, 0, &dirLight);
	pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.pointLight1, 0, 0, &pointLights[0]);
	pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.pointLight2, 0, 0, &pointLights[1]);
	pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.pointLight3, 0, 0, &pointLights[2]);
	pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.pointLight4, 0, 0, &pointLights[3]);
	pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.spotLight, 0, 0, &spotLight);
}

//-------------------------------------------------------------------------
// Purpose:	Creates the default, fallback material.
// Input:	none.
// Output:	none.
//-------------------------------------------------------------------------
void CMaterialSystem::CreateDefaultMaterial()
{
	IShaderSystem* pShaderSystem = m_Config.pShaderSystem;
	MaterialHandle_t matIdx = FindFirstFreeMaterialHandle();
	Material* pNewMaterial = &m_MaterialDictionary.m_MaterialArray[matIdx];

	pNewMaterial->m_pszMaterialName = Utl::StringDuplicate("_DefaultMaterial");
	pNewMaterial->m_hShaderHandle = pShaderSystem->AcquireShader("Core.Phong");
	pNewMaterial->m_hDiffuseTextureHandle = m_Config.pTextureSystem->GetDefaultDiffuseTexture();
	pNewMaterial->m_hSpecularTextureHandle = m_Config.pTextureSystem->GetDefaultSpecularTexture();
	pNewMaterial->m_hNormalTextureHandle = m_Config.pTextureSystem->GetDefaultNormalTexture();

	MaterialLookup* newMaterialLookup = m_MaterialDictionary.m_MaterialHashmap.Insert(pNewMaterial->m_pszMaterialName);
	CAssert(newMaterialLookup->MaterialIndex == kInvalidID_U32); // Make sure this slot is not in use.
	newMaterialLookup->MaterialIndex = matIdx;
	// Storing this off in two places, this second lookup is dedicated to the default material, and should never change until it's released.
	m_DefaultMaterialLookup.MaterialIndex = matIdx;
	m_DefaultMaterialLookup.AutoRelease = false;
	m_DefaultMaterialLookup.ReferenceCount = 0;

	// Acquire shader instance resources for this material.
	pShaderSystem->AcquireShaderInstanceResources(pNewMaterial->m_hShaderHandle, &pNewMaterial->m_iInternalHandle, pNewMaterial->m_pszMaterialName);

	// Texture maps.
	Utl::CVector<TextureHandle_t>	textureHandles(3);
	textureHandles.PushBack(pNewMaterial->m_hDiffuseTextureHandle);
	textureHandles.PushBack(pNewMaterial->m_hSpecularTextureHandle);
	textureHandles.PushBack(pNewMaterial->m_hNormalTextureHandle);

	// Bind the texture maps to the shader instance.
	for (i32 texIdx = 0; texIdx < textureHandles.Count(); texIdx++)
	{
		// TODO: This only works for the hard-coded phong shader. This will need to be made more robust, on a per-shader basis later.
		i32 iTextureArrayIndex = texIdx;
		pShaderSystem->SetShaderUniform(pNewMaterial->m_hShaderHandle, s_PhongShaderLocations.textureMaps, iTextureArrayIndex, pNewMaterial->m_iInternalHandle, &textureHandles[texIdx]);
	}

	f32 flShininess = 32;
	// Set the instance uniform.
	pShaderSystem->SetShaderUniform(pNewMaterial->m_hShaderHandle, s_PhongShaderLocations.shininess, 0, pNewMaterial->m_iInternalHandle, &flShininess);
}

//-------------------------------------------------------------------------
// Purpose:	Releases resources for the default material.
// Input:	none.
// Output:	none.
//-------------------------------------------------------------------------
void CMaterialSystem::DestroyDefaultMaterial()
{
	// NOTE: Don't have to worry about releasing default textures here, 
	// the texture system will take care of that.

	MaterialLookup* pLookup = &m_DefaultMaterialLookup;
	CAssert(pLookup && pLookup->MaterialIndex != kInvalidID_U32);
	Material* pMaterial = &m_MaterialDictionary.m_MaterialArray[pLookup->MaterialIndex];

	m_Config.pShaderSystem->ReleaseShaderInstanceResources(pMaterial->m_hShaderHandle, pMaterial->m_iInternalHandle);

	ClearMaterialDictionaryEntry(pMaterial->m_pszMaterialName);
}

//-------------------------------------------------------------------------
// Purpose: Clears a given material dictionary entry.
// Input:	pszMaterialName - The name of the material whose entry to vacate.
// Output:	none.
//-------------------------------------------------------------------------
void CMaterialSystem::ClearMaterialDictionaryEntry(const char* pszMaterialName)
{
	CAssert(pszMaterialName);

	MaterialLookup* pLookup = &m_MaterialDictionary.m_MaterialHashmap[pszMaterialName];
	Material*		pMaterial = &m_MaterialDictionary.m_MaterialArray[pLookup->MaterialIndex];

	CAssert(pLookup && pMaterial);

	pLookup->MaterialIndex = kInvalidID_U32;
	pLookup->ReferenceCount = 0;
	pLookup->AutoRelease = false;

	if (pMaterial->m_pszMaterialName)
	{
		Utl::StringFree(pMaterial->m_pszMaterialName);
		pMaterial->m_pszMaterialName = nullptr;
	}

	pMaterial->m_iInternalHandle = kInvalidID_U32;
}

//-------------------------------------------------------------------------
// Purpose: Find the first available handle in the material dictionary.
// Input:	none.
// Output:	Returns the first available material handle.
//-------------------------------------------------------------------------
MaterialHandle_t CMaterialSystem::FindFirstFreeMaterialHandle() const
{
	for (i32 idx = 0; idx < m_Config.MaxMaterials; idx++)
	{
		if (m_MaterialDictionary.m_MaterialArray[idx].m_iInternalHandle == kInvalidID_U32)
			return idx;
	}

	LOG_ERROR("Failed to find a free slot in the material dictionary. Adjust config to allow for more.");
	return kInvalidID_U32;
}