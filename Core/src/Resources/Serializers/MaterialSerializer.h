//=============================================================================
// Author: Jared Simons
// Date Created: 6/14/2025
// 
// Purpose: Material file serialization utilities.
//=============================================================================
#pragma once

#include <Defines.h>
#include "Resources/Loaders/MaterialLoader.h"

namespace MaterialSerializer
{
	CORE_API bool SerializeMaterialResources(const char* pszMaterialName, struct MaterialLoader::MaterialResource_t& pInResource);
}