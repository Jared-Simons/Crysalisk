//=============================================================================
// Author: Jared Simons
// Date Created: 5/29/2025
// 
// Purpose: Mesh resource loading utilities.
//=============================================================================
#pragma once

#include "Containers/CVector.h"
#include <Defines.h>
#include "Math/MathLib.h"
#include "Math/VertexTypes.h"
#include "Resources/Loaders/MaterialLoader.h"	// We need this in the case that models have materials to load in.
#include "Strings/String_Utils.h"

namespace MeshLoader
{
	struct MeshLoaderParams_t
	{
		char*					pszMeshPath{ nullptr };
		struct MeshResource_t*	pOutResource{ nullptr };
	};

	// Attempts to load an existing mesh asset from disk.
	CORE_API bool LoadMeshResource(char* pszMeshFileName, struct MeshResource_t& pOutResource);

	CORE_API int LoadMesh_Threaded(void* pParams, void** pJobResults);

	// A mesh resource generated from a file on disk.
	// Can be used by the engine at runtime for rendering.
	struct MeshResource_t
	{
		// This is a method of grouping submeshes together in the engine.
		// Contains an array of submesh data.
		struct GeometryObject_t
		{
			// The raw vertex data.
			// This is dynamically allocated, must be freed by caller.
			byte*		pVertexData{ nullptr };
		};

		MeshResource_t() 
		{
		}
		
		MeshResource_t(const MeshResource_t& pOther)
		{
			pMeshes = pOther.pMeshes;

			vecPosition = pOther.vecPosition;
			vecEulerAngles = pOther.vecEulerAngles;
			vecScale = pOther.vecScale;
		}

		MeshResource_t& operator=(const MeshResource_t& pOther)
		{
			pMeshes = pOther.pMeshes;
			pMaterials = pOther.pMaterials;

			vecPosition = pOther.vecPosition;
			vecEulerAngles = pOther.vecEulerAngles;
			vecScale = pOther.vecScale;

			return *this;
		}

		~MeshResource_t()
		{
			for (int idx = 0; idx < pMaterials.Capacity(); idx++)
			{
				pMaterials[idx].~MaterialResource_t();
			}
		}

		// Vertices are assembled into flat buffers.
		// This is done so that we don't need many different types of vertices for different permutations (I.E. a vertex with/without vertex color).
		// These flags are a bitfield that indicate the layout of the buffer.
		uint16_t						iVertexLayoutFlags{ 0 };
		// An array of meshes (or submeshes) in this model.
		Utl::CVector<GeometryObject_t>	pMeshes{};
		// TODO: DEPRECATE THIS
		// An array of submeshes.
		//Utl::CVector<GeometryObject_t>						pSubmeshes{};
		// An array of material resources related to this model.
		Utl::CVector<MaterialLoader::MaterialResource_t>	pMaterials{};

		// Transformation data.
		Vector3D						vecPosition{};
		Vector3D						vecEulerAngles{};
		Vector3D						vecScale{};
	};
}