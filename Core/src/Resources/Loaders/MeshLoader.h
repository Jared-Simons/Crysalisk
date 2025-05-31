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
#include "Math/VertexTypes.h"
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

		struct GeometryGroup_t
		{
			char*					DebugName{ nullptr };
			Utl::CVector<Vertex3D>	Vertices{};

			GeometryGroup_t() {}
			
			GeometryGroup_t(const GeometryGroup_t& pOther)
			{
				DebugName = StringDuplicate(pOther.DebugName);
			}

			GeometryGroup_t& operator=(const GeometryGroup_t& pOther)
			{
				if (pOther.DebugName)
					DebugName = StringDuplicate(pOther.DebugName);

				Vertices = pOther.Vertices;

				return *this;
			}

			~GeometryGroup_t()
			{
				// Make sure to free the string.
				if (DebugName)
					StringFree(DebugName);
			}
		};

		struct GeometryObject_t
		{
			char*							pszDebugName{ nullptr };
			Utl::CVector<GeometryGroup_t*>	pGroups{};

			GeometryObject_t() {}

			GeometryObject_t(const GeometryObject_t& pOther)
			{
				if (pOther.pszDebugName)
					pszDebugName = StringDuplicate(pOther.pszDebugName);

				pGroups = pOther.pGroups;
			}

			GeometryObject_t& operator=(const GeometryObject_t& pOther)
			{
				if (pOther.pszDebugName)
					pszDebugName = StringDuplicate(pOther.pszDebugName);

				pGroups = pOther.pGroups;

				return *this;
			}

			~GeometryObject_t()
			{
				if (pszDebugName)
					StringFree(pszDebugName);

				for (int idx = 0; idx < pGroups.Count(); idx++)
				{
					delete pGroups[idx];
				}
			}
		};

		MeshResource_t() {}
		MeshResource_t(const MeshResource_t& pOther)
		{
			Positions = pOther.Positions;
			TexCoords = pOther.TexCoords;
			Normals = pOther.Normals;
			Groups = pOther.Groups;
		}

		MeshResource_t& operator=(const MeshResource_t& pOther)
		{
			Positions = pOther.Positions;
			TexCoords = pOther.TexCoords;
			Normals = pOther.Normals;
			Groups = pOther.Groups;

			return *this;
		}

		~MeshResource_t()
		{
			for (int idx = 0; idx < Groups.Count(); idx++)
			{
				delete Groups[idx];
			}
		}

		Utl::CVector<GeometryGroup_t*>	Groups{};
		Utl::CVector<GeometryObject_t*>	pObjects{};
	};
}