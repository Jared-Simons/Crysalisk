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
	CORE_API bool LoadMaterialResource(const char* pszMaterialPath, struct MaterialResource_t& pOutResource);

	struct MaterialResource_t
	{
		enum class eTextureType
		{
			UNDEFINED = -1,
			DIFFUSE,
			SPECULAR,
			NORMAL,
		};

		struct eTextureMapConfig_t
		{
			char*			pszTextureName{ nullptr };
			eTextureType	eType{ eTextureType::UNDEFINED };

			eTextureMapConfig_t() {}

			eTextureMapConfig_t(const eTextureMapConfig_t& pOther)
			{
				if (pOther.pszTextureName)
					pszTextureName = StringDuplicate(pOther.pszTextureName);
				eType = pOther.eType;
			}

			eTextureMapConfig_t& operator=(const eTextureMapConfig_t& pOther)
			{
				if (pOther.pszTextureName)
					pszTextureName = StringDuplicate(pOther.pszTextureName);
				eType = pOther.eType;

				return *this;
			}

			~eTextureMapConfig_t()
			{
				if (pszTextureName)
					StringFree(pszTextureName);
			}
		};

		char*								pszMaterialName{ nullptr };
		char*								pszShaderName{ nullptr };
		Utl::CVector<eTextureMapConfig_t>	textureMapConfigs{};
		Utl::CVector<char*>					pszCustomKeys{};
		Utl::CVector<char*>					pszCustomValues{};

		MaterialResource_t() {}

		MaterialResource_t(const MaterialResource_t& pOther)
		{
			if (pOther.pszMaterialName)
				pszMaterialName = StringDuplicate(pOther.pszMaterialName);

			if (pOther.pszShaderName)
				pszShaderName = StringDuplicate(pOther.pszShaderName);

			textureMapConfigs = pOther.textureMapConfigs;
			pszCustomKeys = pOther.pszCustomKeys;
			pszCustomValues = pOther.pszCustomValues;
		}

		MaterialResource_t& operator=(const MaterialResource_t& pOther)
		{
			if (pOther.pszMaterialName)
				pszMaterialName = StringDuplicate(pOther.pszMaterialName);

			if (pOther.pszShaderName)
				pszShaderName = StringDuplicate(pOther.pszShaderName);

			textureMapConfigs = pOther.textureMapConfigs;
			pszCustomKeys = pOther.pszCustomKeys;
			pszCustomValues = pOther.pszCustomValues;

			return *this;
		}

		~MaterialResource_t()
		{
			if (pszMaterialName)
				StringFree(pszMaterialName);

			if (pszShaderName)
				StringFree(pszShaderName);

			textureMapConfigs.Clear();
			pszCustomKeys.Clear();
			pszCustomValues.Clear();
		}
	};
}