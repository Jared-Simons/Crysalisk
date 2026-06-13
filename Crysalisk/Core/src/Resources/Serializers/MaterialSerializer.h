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
	// The default asset path to store material in.
	static const char* kDefaultMaterialAssetPath = "Assets/Materials/";
	// Specific file extension for the Crysalisk material file format (CMaterials).
	static const char* kMaterialFileExtension = ".cmat";

	// Takes an existing material resource imported from another file type, 
	// and writes it out to disk in our own material format.
	CORE_API bool SerializeMaterialResources(const char* pszMaterialName, struct MaterialLoader::MaterialResource_t& pInResource);
}