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
// Indices for each of the phong shader lookups.
//-------------------------------------------------------------------------
struct PhongShaderLocations_t
{
	uint16_t	projection{ kInvalidID_U16 };
	uint16_t	view{ kInvalidID_U16 };
	uint16_t	viewPos{ kInvalidID_U16 };
	uint16_t	dirLight{ kInvalidID_U16 };
	uint16_t	pointLight1{ kInvalidID_U16 };
	uint16_t	pointLight2{ kInvalidID_U16 };
	uint16_t	pointLight3{ kInvalidID_U16 };
	uint16_t	pointLight4{ kInvalidID_U16 };
	uint16_t	spotLight{ kInvalidID_U16 };
	uint16_t	renderMode{ kInvalidID_U16 };
	uint16_t	shininess{ kInvalidID_U16 };
	uint16_t	color{ kInvalidID_U16 };
	uint16_t	textureMaps{ kInvalidID_U16 };
	uint16_t	model{ kInvalidID_U16 };
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
	virtual void				Shutdown() override;

	// Acquires the resources for the provided material.
	virtual MaterialHandle_t	AcquireMaterial(const char* pszMaterialName) override;
	// Acquires a material given an existing resource.
	virtual MaterialHandle_t	AcquireMaterialFromResource(MaterialLoader::MaterialResource_t& pInResource) override;

	virtual void				ReleaseMaterial(MaterialHandle_t hMaterial, bool bForceResourceRelease) override;

	// Returns true if a given material is transparent in anyway, false if completely opaque.
	virtual uint32_t			IsMaterialTransparent(MaterialHandle_t hMaterial) const override;

	// Applies a given material.
	virtual void				ApplyMaterialInstance(MaterialHandle_t hMaterial) override;

	// Returns a handle to the default texture.
	virtual MaterialHandle_t	GetDefaultMaterialReference() override;

	// Applies global render data for the frame.
	virtual void				ApplyGlobalRenderData(Matrix4x4& pMatProjection, Matrix4x4& pMatView, Vector3D_OLD& pVecPosition, int renderDebugMode) override;
	// Applies per-draw render data for the following draw calls.
	virtual void				ApplyPerDrawRenderData(Matrix4x4& pMatModel) override;

	// Sets directional light data to be used during rendering.
	virtual void				SetDirectionalLightData(Vector3D_OLD vecDirection, Vector3D_OLD vecLightColor, float flBrightness, Vector3D_OLD vecAmbientLightColor, float flAmbientStrength) override;
	virtual void				SetPointLightData(int lightIndex, Vector3D_OLD vecPoint, Vector3D_OLD vecLightColor, float flBrightness, float flConstant, float flLinear, float flQuadratic) override;
	virtual void				SetSpotLightData(Vector3D_OLD vecPosition, Vector3D_OLD vecDirection, Vector3D_OLD vecLightColor, float flBrightness, float flConstant, float flLinear, float flQuadratic, float flInnerCutoff, float flOuterCutoff) override;
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
		int				 ReferenceCount{ 0 };
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
		Vector3D_OLD	vecDirection{};
		float		padding1{};
		Vector3D_OLD	lightColor{};
		float		flBrightness{};
		Vector3D_OLD	ambientLightColor{};
		float		flAmbientStrength{};
	} dirLight;

	struct PointLight_t
	{
		Vector4D_OLD	vecPosition{};
		Vector3D_OLD	lightColor{};
		float		flBrightness{};

		float		flConstant{};
		float		flLinear{};
		float		flQuadratic{};
	} pointLights[4];

	struct SpotLight_t
	{
		Vector4D_OLD	vecPosition{};
		Vector4D_OLD	vecDirection{};
		Vector3D_OLD	lightColor{};
		float		flBrightness{};

		float		flConstant{};
		float		flLinear{};
		float		flQuadratic{};

		float		flInnerCutoff{};
		float		flOuterCutoff{};
	} spotLight;
};

// Material System context.
static CMaterialSystem s_MaterialSystem;
IMaterialSystem* g_pMaterialSystemInternal = &s_MaterialSystem;

//-------------------------------------------------------------------------
// Initialize.
//-------------------------------------------------------------------------
bool CMaterialSystem::Initialize(MaterialSystemConfig_t config)
{
	m_Config = config;
	m_MaterialDictionary.m_MaterialHashmap.Resize(m_Config.MaxMaterials);

	// Allocate memory for our material array.
	m_MaterialDictionary.m_MaterialArray = (Material*)new Material[m_Config.MaxMaterials];

	// Setup shader locations.
	m_DefaultMaterialShader = g_pShaderSystem->AcquireShader("Core.Phong");
	s_PhongShaderLocations.projection = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "projection");
	s_PhongShaderLocations.view = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "view");
	s_PhongShaderLocations.viewPos = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "viewPos");
	s_PhongShaderLocations.dirLight = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "directionalLight");
	s_PhongShaderLocations.pointLight1 = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "pointLight1");
	s_PhongShaderLocations.pointLight2 = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "pointLight2");
	s_PhongShaderLocations.pointLight3 = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "pointLight3");
	s_PhongShaderLocations.pointLight4 = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "pointLight4");
	s_PhongShaderLocations.spotLight = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "spotlight");
	s_PhongShaderLocations.renderMode = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "renderMode");
	s_PhongShaderLocations.shininess = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "shininess");
	s_PhongShaderLocations.color = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "color");
	s_PhongShaderLocations.textureMaps = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "texture_maps");
	s_PhongShaderLocations.model = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "model");

	// Default material.
	CreateDefaultMaterial();

	LOG_INFO("Material system initialized! (Max materials: %d)", config.MaxMaterials);

	return true;
}

//-------------------------------------------------------------------------
// Shutdown.
//-------------------------------------------------------------------------
void CMaterialSystem::Shutdown()
{
	for (int matIdx = 0; matIdx < m_Config.MaxMaterials; matIdx++)
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
			pMaterial->m_hShaderHandle = g_pShaderSystem->AcquireShader(pInResource.pszShaderName);
		}
		else
		{
			// Default shader name, in case none was provided.
			pMaterial->m_hShaderHandle = g_pShaderSystem->AcquireShader("Core.Phong");
		}

		// Acquire shader instance resources for this material.
		g_pShaderSystem->AcquireShaderInstanceResources(pMaterial->m_hShaderHandle, &pMaterial->m_iInternalHandle, pInResource.pszMaterialName);

		// Texture maps.
		size_t							textureMapCount = pInResource.textureMapConfigs.Count();
		Utl::CVector<TextureHandle_t>	textureHandles(textureMapCount);

		// Set some default textures, just in case.
		TextureHandle_t hDefaultDiffuse = g_pTextureSystem_Internal->GetDefaultDiffuseTexture();
		TextureHandle_t hDefaultSpecular = g_pTextureSystem_Internal->GetDefaultSpecularTexture();
		TextureHandle_t hDefaultNormal = g_pTextureSystem_Internal->GetDefaultNormalTexture();

		textureHandles.PushBack(hDefaultDiffuse);
		textureHandles.PushBack(hDefaultSpecular);
		textureHandles.PushBack(hDefaultNormal);

		for (int texIdx = 0; texIdx < textureMapCount; texIdx++)
		{
			TextureHandle_t hTexture = g_pTextureSystem_Internal->AcquireTexture(pInResource.textureMapConfigs[texIdx].pszTextureName, true);
			textureHandles[texIdx] = hTexture;
		}

		// Bind the texture maps to the shader instance.
		for (int texIdx = 0; texIdx < 3; texIdx++)
		{
			// TODO: This only works for the hard-coded phong shader. This will need to be made more robust, on a per-shader basis later.
			int iTextureArrayIndex = texIdx;
			g_pShaderSystem->SetShaderUniform(pMaterial->m_hShaderHandle, s_PhongShaderLocations.textureMaps, iTextureArrayIndex, pMaterial->m_iInternalHandle, &textureHandles[texIdx]);

			// Use the diffuse map to determine material transparency.
			if (texIdx < textureMapCount && pInResource.textureMapConfigs[texIdx].eType == MaterialLoader::MaterialResource_t::eTextureType::DIFFUSE)
			{
				pMaterial->m_bIsTransparent = g_pTextureSystem_Internal->IsTextureFlagSet(textureHandles[texIdx], TEXTURE_FLAG_IS_TRANSPARENT);
			}
		}

		// Save off the texture handles.
		pMaterial->m_hDiffuseTextureHandle = textureHandles[0];
		pMaterial->m_hSpecularTextureHandle = textureHandles[1];
		pMaterial->m_hNormalTextureHandle = textureHandles[2];

		// Set any instance-level uniform data.
		CAssert(pInResource.pszCustomKeys.Count() == pInResource.pszCustomValues.Count());
		for (int idx = 0; idx < pInResource.pszCustomKeys.Count(); idx++)
		{
			if (Utl::StringCompareInsensitive(pInResource.pszCustomKeys[idx], "shininess"))
			{
				float flShininess = Utl::String_ATOF(pInResource.pszCustomValues[idx]);
				// Set the instance uniform.
				g_pShaderSystem->SetShaderUniform(pMaterial->m_hShaderHandle, s_PhongShaderLocations.shininess, 0, pMaterial->m_iInternalHandle, &flShininess);
			}
			else
			{
				LOG_WARN("Failed to parse custom material key '%s'.", pInResource.pszCustomKeys[idx]);
				continue;
			}
		}

		pLookup->ReferenceCount = 0;

		// Save off the index.
		pLookup->MaterialIndex = newMaterialHandle;
	}
	else
	{
		// This material is already loaded, just return the handle.
		newMaterialHandle = pLookup->MaterialIndex;
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
void CMaterialSystem::ReleaseMaterial(MaterialHandle_t hMaterial, bool bForceResourceRelease)
{
	Material&		pMaterial = m_MaterialDictionary.m_MaterialArray[hMaterial];

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

		g_pShaderSystem->ReleaseShaderInstanceResources(pMaterial.m_hShaderHandle, pMaterial.m_iInternalHandle);

		g_pTextureSystem_Internal->ReleaseTexture(pMaterial.m_hDiffuseTextureHandle);
		g_pTextureSystem_Internal->ReleaseTexture(pMaterial.m_hSpecularTextureHandle);
		g_pTextureSystem_Internal->ReleaseTexture(pMaterial.m_hNormalTextureHandle);

		ClearMaterialDictionaryEntry(pMaterial.m_pszMaterialName);
	}
}

//-------------------------------------------------------------------------
// Purpose:	Returns the state of a material's transparency.
// Input:	hMaterial - The material whose transparency to check for.
// Output:	Returns true if the material is transparent in any way, otherwise false.
//-------------------------------------------------------------------------
uint32_t CMaterialSystem::IsMaterialTransparent(MaterialHandle_t hMaterial) const
{
	if (hMaterial == kInvalidID_U32)
		//hMaterial = m_DefaultMaterialLookup.MaterialIndex;
		hMaterial = 0; // TODO: Use the default material here.
	Material* pMaterial = &m_MaterialDictionary.m_MaterialArray[hMaterial];
	//CAssert(hMaterial != kInvalidID_U32 && pMaterial && pMaterial->m_iInternalHandle != kInvalidID_U32);

	return pMaterial->m_bIsTransparent;
}

//-------------------------------------------------------------------------
// Purpose:	Binds a given material.
// Input:	hMaterial - The material to be bound.
// Output:	none.
// TODO: Make sure we can assert that a frame is currently being rendered when this function is called.
//-------------------------------------------------------------------------
void CMaterialSystem::ApplyMaterialInstance(MaterialHandle_t hMaterial)
{
	//CAssert(hMaterial != kInvalidID_U32);
	if (hMaterial == kInvalidID_U32)
		hMaterial = 0; // TODO: This is not elegant

	// Don't do any extra work!
	if (m_hCurrentlyBoundMaterialInstance == hMaterial)
		return;

	// Bind the appropriate shader.
	g_pShaderSystem->BindShader(m_MaterialDictionary.m_MaterialArray[hMaterial].m_hShaderHandle);

	// Bind the appropriate material instance.
	g_pShaderSystem->BindPerInstanceUniforms(m_MaterialDictionary.m_MaterialArray[hMaterial].m_iInternalHandle);

	m_hCurrentlyBoundMaterialInstance = hMaterial;
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
void CMaterialSystem::ApplyGlobalRenderData(Matrix4x4& pMatProjection, Matrix4x4& pMatView, Vector3D_OLD& pVecPosition, int renderDebugMode)
{
	g_pShaderSystem->BindShader(m_DefaultMaterialShader);

	g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.projection, 0, 0, &pMatProjection);
	g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.view, 0, 0, &pMatView);
	g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.viewPos, 0, 0, &pVecPosition);
	g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.renderMode, 0, 0, &renderDebugMode);
}

//-------------------------------------------------------------------------
// Applies per-draw render data for the following draw calls.
// NOTE: This lives here and not in the renderer because we need access
// to Material-Specific shader information.
//-------------------------------------------------------------------------
void CMaterialSystem::ApplyPerDrawRenderData(Matrix4x4& pMatModel)
{
	g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.model, 0, 0, &pMatModel);
}

//-------------------------------------------------------------------------
// Sets directional light data to be used during rendering.
// NOTE: This lives here and not in the renderer because we need access
// to Material-Specific shader information.
//-------------------------------------------------------------------------
void CMaterialSystem::SetDirectionalLightData(Vector3D_OLD vecDirection, Vector3D_OLD vecLightColor, float flBrightness, Vector3D_OLD vecAmbientLightColor, float flAmbientStrength)
{
	dirLight.vecDirection = Vector3D_OLD{ M_DEG2RAD_F(vecDirection.x), M_DEG2RAD_F(vecDirection.y), M_DEG2RAD_F(vecDirection.z) };;
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
void CMaterialSystem::SetPointLightData(int lightIndex, Vector3D_OLD vecPoint, Vector3D_OLD vecLightColor, float flBrightness, float flConstant, float flLinear, float flQuadratic)
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
void CMaterialSystem::SetSpotLightData(Vector3D_OLD vecPosition, Vector3D_OLD vecDirection, Vector3D_OLD vecLightColor, float flBrightness, float flConstant, float flLinear, float flQuadratic, float flInnerCutoff, float flOuterCutoff)
{
	CAssert(flInnerCutoff < flOuterCutoff);

	spotLight.vecPosition = vecPosition;
	// Direction needs to be converted from degrees to radians.
	spotLight.vecDirection = Vector3D_OLD{ M_DEG2RAD_F(vecDirection.x), M_DEG2RAD_F(vecDirection.y), M_DEG2RAD_F(vecDirection.z) };
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
	g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.dirLight, 0, 0, &dirLight);
	g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.pointLight1, 0, 0, &pointLights[0]);
	g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.pointLight2, 0, 0, &pointLights[1]);
	g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.pointLight3, 0, 0, &pointLights[2]);
	g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.pointLight4, 0, 0, &pointLights[3]);
	g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.spotLight, 0, 0, &spotLight);
}

//-------------------------------------------------------------------------
// Purpose:	Creates the default, fallback material.
// Input:	none.
// Output:	none.
//-------------------------------------------------------------------------
void CMaterialSystem::CreateDefaultMaterial()
{
	int matIdx = FindFirstFreeMaterialHandle();
	Material* pNewMaterial = &m_MaterialDictionary.m_MaterialArray[matIdx];

	pNewMaterial->m_pszMaterialName = Utl::StringDuplicate("_DefaultMaterial");
	pNewMaterial->m_hShaderHandle = g_pShaderSystem->AcquireShader("Core.Phong");
	pNewMaterial->m_hDiffuseTextureHandle = g_pTextureSystem_Internal->GetDefaultDiffuseTexture();
	pNewMaterial->m_hSpecularTextureHandle = g_pTextureSystem_Internal->GetDefaultSpecularTexture();
	pNewMaterial->m_hNormalTextureHandle = g_pTextureSystem_Internal->GetDefaultNormalTexture();

	MaterialLookup* newMaterialLookup = m_MaterialDictionary.m_MaterialHashmap.Insert(pNewMaterial->m_pszMaterialName);
	CAssert(newMaterialLookup->MaterialIndex == kInvalidID_U32); // Make sure this slot is not in use.
	newMaterialLookup->MaterialIndex = matIdx;
	// Storing this off in two places, this second lookup is dedicated to the default material, and should never change until it's released.
	m_DefaultMaterialLookup.MaterialIndex = matIdx;
	m_DefaultMaterialLookup.AutoRelease = false;
	m_DefaultMaterialLookup.ReferenceCount = 0;

	// Acquire shader instance resources for this material.
	g_pShaderSystem->AcquireShaderInstanceResources(pNewMaterial->m_hShaderHandle, &pNewMaterial->m_iInternalHandle, pNewMaterial->m_pszMaterialName);

	// Texture maps.
	Utl::CVector<TextureHandle_t>	textureHandles(3);
	textureHandles.PushBack(pNewMaterial->m_hDiffuseTextureHandle);
	textureHandles.PushBack(pNewMaterial->m_hSpecularTextureHandle);
	textureHandles.PushBack(pNewMaterial->m_hNormalTextureHandle);

	// Bind the texture maps to the shader instance.
	for (int texIdx = 0; texIdx < textureHandles.Count(); texIdx++)
	{
		// TODO: This only works for the hard-coded phong shader. This will need to be made more robust, on a per-shader basis later.
		int iTextureArrayIndex = texIdx;
		g_pShaderSystem->SetShaderUniform(pNewMaterial->m_hShaderHandle, s_PhongShaderLocations.textureMaps, iTextureArrayIndex, pNewMaterial->m_iInternalHandle, &textureHandles[texIdx]);
	}

	float flShininess = 32;
	// Set the instance uniform.
	g_pShaderSystem->SetShaderUniform(pNewMaterial->m_hShaderHandle, s_PhongShaderLocations.shininess, 0, pNewMaterial->m_iInternalHandle, &flShininess);
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

	g_pShaderSystem->ReleaseShaderInstanceResources(pMaterial->m_hShaderHandle, pMaterial->m_iInternalHandle);

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
	for (int idx = 0; idx < m_Config.MaxMaterials; idx++)
	{
		if (m_MaterialDictionary.m_MaterialArray[idx].m_iInternalHandle == kInvalidID_U32)
			return idx;
	}

	LOG_ERROR("Failed to find a free slot in the material dictionary. Adjust config to allow for more.");
	return kInvalidID_U32;
}