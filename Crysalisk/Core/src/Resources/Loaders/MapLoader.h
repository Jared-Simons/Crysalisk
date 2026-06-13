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
			f32		flAmbientStrength{};

			// NOTE: Does not apply to directional/sky lights.
			f32		flConstant{ 0.0f };
			f32		flLinear{ 0.0f };
			f32		flQuadratic{ 0.0f };
			f32		flBrightness{ 0.0f };

			// NOTE: These only apply to spotlights.
			f32		flInnerCutoff{ 0.0f };
			f32		flOuterCutoff{ 0.0f };
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

		u32	iMapVersion{ 0 };
		char*		pszMapName{ nullptr };

		// Static meshes.
		Utl::CVector<MeshDeclaration_t>				MeshResources{};
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

			PointLights.Clear();
		}
	};
} // namespace MapLoader
