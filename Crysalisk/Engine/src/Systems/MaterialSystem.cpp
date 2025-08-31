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

	// Returns true if a given material is transparent in anyway, false if completely opaque.
	virtual uint32_t			IsMaterialTransparent(MaterialHandle_t hMaterial) const override;

	// Applies a given material.
	virtual void				ApplyMaterialInstance(MaterialHandle_t hMaterial) override;

	// Returns a handle to the default texture.
	virtual MaterialHandle_t	GetDefaultMaterialReference() override;

	// Applies global render data for the frame.
	virtual void				ApplyGlobalRenderData(Matrix4x4& pMatProjection, Matrix4x4& pMatView, Vector3D& pVecPosition) override;

private:
	// Creates the default textures to be used in the engine.
	void						CreateDefaultMaterial();

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
		Utl::CHashMap<const char*, MaterialLookup>	m_MaterialHashmap{};
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
	s_PhongShaderLocations.renderMode = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "renderMode");
	s_PhongShaderLocations.shininess = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "shininess");
	s_PhongShaderLocations.color = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "color");
	s_PhongShaderLocations.textureMaps = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "texture_maps");
	s_PhongShaderLocations.model = g_pShaderSystem->GetShaderUniformLocation(m_DefaultMaterialShader, "model");

	// Default material.
	// TODO: Make this work with new shader system.
	CreateDefaultMaterial();

	// TEMP!
	// Set up a temporary light sources.
	{
		struct DirectionalLight_t
		{
			Vector3D	vecDirection{};
			float		padding1{};
			Vector3D	lightColor{};
			float		padding2{};
			Vector3D	ambientLightColor{};
			float		flAmbientStrength{};
		};

		struct PointLight_t
		{
			Vector3D	vecPosition{};
			float		padding1{};
			Vector3D	lightColor{};

			float		flConstant{};
			float		flLinear{};
			float		flQuadratic{};
		};

		DirectionalLight_t testLight{};
		testLight.vecDirection = { -0.2f, -1.0f, -0.3f };
		testLight.lightColor = { 1.0f, 0.785f, 0.534f };
		testLight.ambientLightColor = { 0.592f, 0.679f, 0.812f };
		testLight.flAmbientStrength = 0.3f;

		PointLight_t	light1{};
		light1.vecPosition = { 0.7f, 2.0f, 2.0f };
		light1.lightColor = { 0.8f, 0.8f, 0.8f };
		light1.flConstant = 1.0f;
		light1.flLinear = 0.09f;
		light1.flQuadratic = 0.032f;

		PointLight_t	light2{};
		light2.vecPosition = { 2.3f, 2.0f, -4.0f };
		light2.lightColor = { 1.0f, 0.0f, 0.0f };
		light2.flConstant = 1.0f;
		light2.flLinear = 0.09f;
		light2.flQuadratic = 0.032f;

		PointLight_t	light3{};
		light3.vecPosition = { -4.0f, 2.0f, -12.0f };
		light3.lightColor = { 0.0f, 1.0f, 0.0f };
		light3.flConstant = 1.0f;
		light3.flLinear = 0.09f;
		light3.flQuadratic = 0.032f;

		PointLight_t	light4{};
		light4.vecPosition = { 0.0f, 2.0f, -3.0f };
		light4.lightColor = { 0.0f, 0.0f, 1.0f };
		light4.flConstant = 1.0f;
		light4.flLinear = 0.09f;
		light4.flQuadratic = 0.032f;

		g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.dirLight, 0, 0, &testLight);
		g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.pointLight1, 0, 0, &light1);
		g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.pointLight2, 0, 0, &light2);
		g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.pointLight3, 0, 0, &light3);
		g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.pointLight4, 0, 0, &light4);
	}
	// END TEMP LIGHTING!

	LOG_INFO("Material system initialized! (Max materials: %d)", config.MaxMaterials);

	return true;
}

//-------------------------------------------------------------------------
// Shutdown.
//-------------------------------------------------------------------------
void CMaterialSystem::Shutdown()
{
	g_pShaderSystem->ReleaseShader(m_DefaultMaterialShader);

	delete[] m_MaterialDictionary.m_MaterialArray;
	LOG_WARN("TODO: IMPLEMENT MATERIAL SYSTEM RESOURCE RELEASE METHOD!!!");
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
		g_pShaderSystem->AcquireShaderInstanceResources(pMaterial->m_hShaderHandle, &pMaterial->m_iInternalHandle);

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
void CMaterialSystem::ApplyGlobalRenderData(Matrix4x4& pMatProjection, Matrix4x4& pMatView, Vector3D& pVecPosition)
{
	g_pShaderSystem->BindShader(m_DefaultMaterialShader);

	// TODO: Don't hardcode these!
	//g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.projection, 0, 0, &pMatProjection);
	//g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.view, 0, 0, &pMatView);
	//g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, s_PhongShaderLocations.viewPos, 0, 0, &pVecPosition);

	g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, 0, 0, 0, &pMatProjection);
	g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, 1, 0, 0, &pMatView);
	g_pShaderSystem->SetShaderUniform(m_DefaultMaterialShader, 2, 0, 0, &pVecPosition);
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

	MaterialLookup* newMaterialLookup = &m_DefaultMaterialLookup;
	CAssert(newMaterialLookup->MaterialIndex == kInvalidID_U32); // Make sure this slot is not in use.
	newMaterialLookup->MaterialIndex = matIdx;

	// Acquire shader instance resources for this material.
	g_pShaderSystem->AcquireShaderInstanceResources(pNewMaterial->m_hShaderHandle, &pNewMaterial->m_iInternalHandle);

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