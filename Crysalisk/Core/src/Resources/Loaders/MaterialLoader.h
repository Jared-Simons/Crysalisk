//=============================================================================
// Author: Jared Simons
// Date Created: 5/28/2025
// 
// Purpose: Material resource loading utilities.
//=============================================================================
#pragma once

#include "Containers/CVector.h"
#include <Defines.h>
#include "Strings/String_Utils.h"

namespace MaterialLoader
{
	// Attempts to load a material asset from disk.
	CORE_API bool LoadMaterialResource(const char* pszMaterialPath, struct MaterialResource_t& pOutResource);

	// A material resource configured from a cmat file on disk.
	struct MaterialResource_t
	{
		// Different texture types.
		enum class eTextureType
		{
			UNDEFINED = -1,
			DIFFUSE,
			SPECULAR,
			NORMAL,
		};

		// A configuration for a texture map.
		// Will be used by the engine at runtime to configure a material's textures.
		struct textureMapConfig_t
		{
			// The name of the texture.
			char*			pszTextureName{ nullptr };
			// The type of the texture.
			eTextureType	eType{ eTextureType::UNDEFINED };

			textureMapConfig_t() {}

			textureMapConfig_t(const textureMapConfig_t& pOther)
			{
				if (pOther.pszTextureName)
					pszTextureName = Utl::StringDuplicate(pOther.pszTextureName);
				eType = pOther.eType;
			}

			textureMapConfig_t& operator=(const textureMapConfig_t& pOther)
			{
				if (pOther.pszTextureName)
					pszTextureName = Utl::StringDuplicate(pOther.pszTextureName);
				eType = pOther.eType;

				return *this;
			}

			~textureMapConfig_t()
			{
				if (pszTextureName)
					Utl::StringFree(pszTextureName);
			}
		};

		// The material's name.
		// NOTE: This is not written out to the .cmat file
		char*								pszMaterialName{ nullptr };
		// The name of the material's shader.
		char*								pszShaderName{ nullptr };
		// An array of texture map configurations for the material.
		Utl::CVector<textureMapConfig_t>	textureMapConfigs{};
		// Arrays of custom key-values for material configuration.
		Utl::CVector<char*>					pszCustomKeys{};
		Utl::CVector<char*>					pszCustomValues{};

		MaterialResource_t() {}

		MaterialResource_t(const MaterialResource_t& pOther)
		{
			if (pOther.pszMaterialName)
				pszMaterialName = Utl::StringDuplicate(pOther.pszMaterialName);

			if (pOther.pszShaderName)
				pszShaderName = Utl::StringDuplicate(pOther.pszShaderName);

			textureMapConfigs = pOther.textureMapConfigs;
			pszCustomKeys = pOther.pszCustomKeys;
			pszCustomValues = pOther.pszCustomValues;
		}

		MaterialResource_t& operator=(const MaterialResource_t& pOther)
		{
			if (pOther.pszMaterialName)
				pszMaterialName = Utl::StringDuplicate(pOther.pszMaterialName);

			if (pOther.pszShaderName)
				pszShaderName = Utl::StringDuplicate(pOther.pszShaderName);

			textureMapConfigs = pOther.textureMapConfigs;
			pszCustomKeys = pOther.pszCustomKeys;
			pszCustomValues = pOther.pszCustomValues;

			return *this;
		}

		~MaterialResource_t()
		{
			if (pszMaterialName)
			{
				Utl::StringFree(pszMaterialName);
				pszMaterialName = nullptr;
			}

			if (pszShaderName)
			{
				Utl::StringFree(pszShaderName);
				pszShaderName = nullptr;
			}

			textureMapConfigs.Clear();

			for (int idx = 0; idx < pszCustomKeys.Count(); idx++)
			{
				if (pszCustomKeys[idx])
				{
					Utl::StringFree(pszCustomKeys[idx]);
					pszCustomKeys[idx] = nullptr;
				}
			}
			pszCustomKeys.Clear();
			
			for (int idx = 0; idx < pszCustomValues.Count(); idx++)
			{
				if (pszCustomValues[idx])
				{
					Utl::StringFree(pszCustomValues[idx]);
					pszCustomValues[idx] = nullptr;
				}
			}
			pszCustomValues.Clear();
		}
	};
}