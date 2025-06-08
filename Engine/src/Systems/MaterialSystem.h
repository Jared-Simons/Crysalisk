//=============================================================================
// Author: Jared Simons
// Date Created: 3/3/2025
// 
// Purpose: The material system.
//=============================================================================
#pragma once

#include <Defines.h>
#include <Resources/Loaders/MaterialLoader.h>
#include "Resources/ResourceTypes.h"

struct MaterialSystemConfig_t
{
	size_t	MaxMaterials{ 0 };
};

class IMaterialSystem
{
public:
	virtual bool				Initialize(MaterialSystemConfig_t config) = 0;
	virtual void				Shutdown() = 0;

	virtual MaterialHandle_t	AcquireMaterial(const char* pszMaterialName) = 0;
	virtual MaterialHandle_t	AcquireMaterialFromResource(struct MaterialLoader::MaterialResource_t& pInResource) = 0;

	// TODO: Make this return flags instead.
	virtual uint32_t			IsMaterialTransparent(MaterialHandle_t hMaterial) const = 0;

	virtual void				ApplyMaterialInstance(MaterialHandle_t hMaterial) = 0;

	virtual MaterialHandle_t	AcquireMaterial_OLD(const char* pszMaterialName) = 0;
	virtual MaterialReference	AcquireMaterialReference(const char* pszMaterialName) = 0;

	virtual MaterialHandle_t	AcquireMaterialFromConfig(MaterialConfig_t& pConfig) = 0;
	virtual MaterialReference	AcquireMaterialInternal(const char* pszMaterialName) = 0;

	virtual MaterialReference	GetDefaultMaterialReference() = 0;
};

DLL_EXPORT IMaterialSystem* g_pMaterialSystemInternal;