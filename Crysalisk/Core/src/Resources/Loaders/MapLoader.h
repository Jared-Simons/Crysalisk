//=============================================================================
// Author: Jared Simons
// Date Created: 9/10/2025
// 
// Purpose: Map resource loading utilities.
//=============================================================================
#pragma once

#include "Containers/CVector.h"
#include <Defines.h>
#include "Resources/Loaders/MeshLoader.h"
#include "Strings/String_Utils.h"

namespace MapLoader
{
	// Attempts to load an existing map asset from disk.
	CORE_API bool LoadMapResource(const char* pszMapFilePath, struct MapResource_t& pOutResource);

	struct MapResource_t
	{
		// Light entities.
		struct LightResource_t
		{
			Vector3D	lightColor{};

			// NOTE: Does not apply to directional/sky lights.
			Vector3D	vecPosition{};

			// NOTE: Has no effect on point lights.
			Vector3D	vecDirection{};

			// NOTE: Only applies to directional/sky lights.
			Vector3D	ambientLightColor{};
			float		flAmbientStrength{};

			// NOTE: Does not apply to directional/sky lights.
			float		flConstant{ 0.0f };
			float		flLinear{ 0.0f };
			float		flQuadratic{ 0.0f };
			float		flBrightness{ 0.0f };

			// NOTE: These only apply to spotlights.
			float		flInnerCutoff{ 0.0f };
			float		flOuterCutoff{ 0.0f };
		};
		enum class eLightType{ INVALID, DIRECTIONAL, POINT, SPOT };

		// Mesh entity declarations.
		struct MeshDeclaration_t
		{
			char*	pszEntityName{ nullptr };
			char*	pszModelName{ nullptr };

			Vector	vecPosition{};
			Vector	vecAngles{};
			Vector	vecScale{};
		};

		uint32_t	iMapVersion{ 0 };
		char*		pszMapName{ nullptr };

		// Static meshes.
		Utl::CVector<MeshLoader::MeshResource_t>	MeshResources{};
		Utl::CVector<MeshDeclaration_t>				MeshResources_NEW{};
		LightResource_t								DirectionalLight{};
		Utl::CVector<LightResource_t>				PointLights{};
		LightResource_t								SpotLight{};

		~MapResource_t()
		{
			if (pszMapName)
			{
				Utl::StringFree(pszMapName);
				pszMapName = nullptr;
			}

			// NOTE: We need to explicitly destroy each of these because the vector destructor does not yet handle this implicitly.
			for (int idx = 0; idx < MeshResources.Capacity(); idx++)
			{
				MeshResources[idx].~MeshResource_t();
			}

			MeshResources.Clear();
			PointLights.Clear();
		}
	};
} // namespace MapLoader
