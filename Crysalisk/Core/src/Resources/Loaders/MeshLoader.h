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
	// Attempts to load an existing mesh asset from disk.
	CORE_API bool LoadMeshResource(char* pszMeshFileName, struct MeshResource_t& pOutResource);

	// A mesh resource generated from a file on disk.
	// Can be used by the engine at runtime for rendering.
	struct MeshResource_t
	{
		// This is an atomic piece of geometry (submesh) in the engine.
		// Contains a table of vertices and indices for rendering.
		// Can only be associated with one material.
		struct GeometryObject_t
		{
			// The name of this submesh (used for debugging purposes).
			char*					pszDebugName{ nullptr };
			// An array of vertices in this submesh.
			Utl::CVector<Vertex3D>	Vertices{};
			// An array of indices in this submesh.
			Utl::CVector<uint32_t>	Indices{};
			// The name of the material associated with this submesh.
			char*					pszMaterialName{ nullptr };

			GeometryObject_t() {}
			
			GeometryObject_t(const GeometryObject_t& pOther)
			{
				if (pOther.pszDebugName)
					pszDebugName = Utl::StringDuplicate(pOther.pszDebugName);
				if (pOther.pszMaterialName)
					pszMaterialName = Utl::StringDuplicate(pOther.pszMaterialName);

				Vertices = pOther.Vertices;
				Indices = pOther.Indices;
			}

			GeometryObject_t& operator=(const GeometryObject_t& pOther)
			{
				if (pOther.pszDebugName)
					pszDebugName = Utl::StringDuplicate(pOther.pszDebugName);
				if (pOther.pszMaterialName)
					pszMaterialName = Utl::StringDuplicate(pOther.pszMaterialName);

				Vertices = pOther.Vertices;
				Indices = pOther.Indices;

				return *this;
			}

			~GeometryObject_t()
			{
				// Make sure to free the strings.
				if (pszDebugName)
				{
					Utl::StringFree(pszDebugName);
					pszDebugName = nullptr;
				}

				if (pszMaterialName)
				{
					Utl::StringFree(pszMaterialName);
					pszMaterialName = nullptr;
				}

				Vertices.Clear();
				Indices.Clear();
			}
		};

		// This is a method of grouping submeshes together in the engine.
		// Contains an array of submesh data.
		struct GeometryGroup_t
		{
			// The name of the group.
			char*							pszDebugName{ nullptr };
			// An array of submeshes.
			Utl::CVector<GeometryObject_t>	pGroups{};

			GeometryGroup_t() {}

			GeometryGroup_t(const GeometryGroup_t& pOther)
			{
				if (pOther.pszDebugName)
					pszDebugName = Utl::StringDuplicate(pOther.pszDebugName);

				pGroups = pOther.pGroups;
			}

			GeometryGroup_t& operator=(const GeometryGroup_t& pOther)
			{
				if (pOther.pszDebugName)
					pszDebugName = Utl::StringDuplicate(pOther.pszDebugName);

				pGroups = pOther.pGroups;

				return *this;
			}

			~GeometryGroup_t()
			{
				if (pszDebugName)
					Utl::StringFree(pszDebugName);

				for (int idx = 0; idx < pGroups.Count(); idx++)
				{
					pGroups[idx].~GeometryObject_t();
				}
			}
		};

		MeshResource_t() 
		{
		}
		
		MeshResource_t(const MeshResource_t& pOther)
		{
			pObjects = pOther.pObjects;

			vecPosition = pOther.vecPosition;
			vecEulerAngles = pOther.vecEulerAngles;
			vecScale = pOther.vecScale;
		}

		MeshResource_t& operator=(const MeshResource_t& pOther)
		{
			pObjects = pOther.pObjects;
			pMaterials = pOther.pMaterials;

			vecPosition = pOther.vecPosition;
			vecEulerAngles = pOther.vecEulerAngles;
			vecScale = pOther.vecScale;

			return *this;
		}

		~MeshResource_t()
		{
			for (int idx = 0; idx < pObjects.Count(); idx++)
			{
				pObjects[idx].~GeometryGroup_t();
			}

			for (int idx = 0; idx < pMaterials.Count(); idx++)
			{
				pMaterials[idx].~MaterialResource_t();
			}
		}

		// An array of groups (which contain submesh data).
		Utl::CVector<GeometryGroup_t>						pObjects{};
		// An array of material resources related to this model.
		Utl::CVector<MaterialLoader::MaterialResource_t>	pMaterials{};

		// Transformation data.
		Vector3D						vecPosition{};
		Vector3D						vecEulerAngles{};
		Vector3D						vecScale{};
	};
}