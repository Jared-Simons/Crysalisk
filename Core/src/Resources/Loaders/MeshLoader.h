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
#include "Resources/Loaders/MaterialLoader.h"	// We need this in the case that models have materials to load in.
#include "Strings/String_Utils.h"

namespace MeshLoader
{
	CORE_API bool LoadMeshResource(const char* pszMeshFileName, struct MeshResource_t& pOutResource);

	//-------------------------------------------------------------------------
	// TODO: Documentation
	//-------------------------------------------------------------------------
	struct MeshResource_t
	{
		struct GeometryGroup_t
		{
			char*					pszDebugName{ nullptr };
			Utl::CVector<Vertex3D>	Vertices{};
			Utl::CVector<uint32_t>	Indices{};
			char*					pszMaterialName{ nullptr };

			GeometryGroup_t() {}
			
			GeometryGroup_t(const GeometryGroup_t& pOther)
			{
				if (pOther.pszDebugName)
					pszDebugName = StringDuplicate(pOther.pszDebugName);
				if (pOther.pszMaterialName)
					pszMaterialName = StringDuplicate(pOther.pszMaterialName);

				Vertices = pOther.Vertices;
				Indices = pOther.Indices;
			}

			GeometryGroup_t& operator=(const GeometryGroup_t& pOther)
			{
				if (pOther.pszDebugName)
					pszDebugName = StringDuplicate(pOther.pszDebugName);
				if (pOther.pszMaterialName)
					pszMaterialName = StringDuplicate(pOther.pszMaterialName);

				Vertices = pOther.Vertices;
				Indices = pOther.Indices;

				return *this;
			}

			~GeometryGroup_t()
			{
				// Make sure to free the strings.
				if (pszDebugName)
					StringFree(pszDebugName);

				if (pszMaterialName)
					StringFree(pszMaterialName);
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
			pObjects = pOther.pObjects;
		}

		MeshResource_t& operator=(const MeshResource_t& pOther)
		{
			pObjects = pOther.pObjects;

			return *this;
		}

		~MeshResource_t()
		{
			for (int idx = 0; idx < pObjects.Count(); idx++)
			{
				delete pObjects[idx];
			}
		}

		Utl::CVector<GeometryObject_t*>						pObjects{};
		Utl::CVector<MaterialLoader::MaterialResource_t>	pMaterials{};
	};
}