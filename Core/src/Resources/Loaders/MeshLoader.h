//=============================================================================
// Author: Jared Simons
// Date Created: 5/29/2025
// 
// Purpose: Mesh resource loading utilities.
//=============================================================================
#pragma once

#include "Containers/CVector.h"
#include <Defines.h>
#include "Math/VectorTypes.h"
#include "Strings/String_Utils.h"

namespace MeshLoader
{
	CORE_API bool LoadMeshResource(const char* pszMeshFileName, struct MeshResource_t& pOutResource);

	//-------------------------------------------------------------------------
	// TODO: Documentation
	//-------------------------------------------------------------------------
	struct MeshResource_t
	{
		Utl::CVector<Vector3D>	Positions{};
		Utl::CVector<Vector2D>	TexCoords{};
		Utl::CVector<Vector3D>	Normals{};

		struct GeometryObject_t
		{
			char*					DebugName{ nullptr };

			GeometryObject_t() {}
			
			GeometryObject_t(const GeometryObject_t& pOther)
			{
				DebugName = StringDuplicate(pOther.DebugName);
			}

			GeometryObject_t& operator=(const GeometryObject_t& pOther)
			{
				if (pOther.DebugName)
					DebugName = StringDuplicate(pOther.DebugName);

				return *this;
			}

			~GeometryObject_t()
			{
				// Make sure to free the string.
				if (DebugName)
					StringFree(DebugName);
			}
		};

		MeshResource_t() {}
		MeshResource_t(const MeshResource_t& pOther)
		{
			Positions = pOther.Positions;
			TexCoords = pOther.TexCoords;
			Normals = pOther.Normals;
			Objects = pOther.Objects;
		}

		MeshResource_t& operator=(const MeshResource_t& pOther)
		{
			Positions = pOther.Positions;
			TexCoords = pOther.TexCoords;
			Normals = pOther.Normals;
			Objects = pOther.Objects;

			return *this;
		}

		Utl::CVector<GeometryObject_t>	Objects{};
	};
}