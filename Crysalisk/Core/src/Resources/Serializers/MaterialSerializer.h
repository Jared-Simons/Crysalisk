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
	// Takes an existing material resource imported from another file type, 
	// and writes it out to disk in our own material format.
	CORE_API bool SerializeMaterialResources(const char* pszMaterialName, struct MaterialLoader::MaterialResource_t& pInResource);
}