//=============================================================================
// Author: Jared Simons
// Date Created: 3/3/2025
// 
// Purpose: The material system.
//=============================================================================
#pragma once

#include <Defines.h>
#include "Resources/ResourceTypes.h"

struct MaterialSystemConfig_t
{
	size_t	MaxMaterials{ 0 };
};

class IMaterialSystem
{
public:
	virtual bool	Initialize(MaterialSystemConfig_t config) = 0;
	virtual void	Shutdown() = 0;

	virtual MaterialHandle_t	AcquireMaterial(const char* pszMaterialName) = 0;
	virtual MaterialReference	AcquireMaterialReference(const char* pszMaterialName) = 0;
};

DLL_EXPORT IMaterialSystem* g_pMaterialSystem;