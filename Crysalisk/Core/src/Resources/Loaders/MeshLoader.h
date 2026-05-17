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
	CORE_API void UnloadMeshResource(struct MeshResource_t& pResource);

	CORE_API int LoadMesh_Threaded(void* pParams, void** pJobResults);

	// A mesh resource generated from a file on disk.
	// Can be used by the engine at runtime for rendering.
	struct MeshResource_t
	{
		// This is a method of grouping submeshes together in the engine.
		// Contains an array of submesh data.
		struct GeometryObject_t
		{
			uint64_t	iMeshNameIndex{ kInvalidID_U32 };
			uint64_t	iMaterialNameIndex{ kInvalidID_U32 };

			// The raw vertex data.
			// This is dynamically allocated, must be freed by caller.
			uint64_t	iVertexCount{ 0 };
			uint64_t	iVertexOffset{ 0 };
			byte*		pVertexData{ nullptr };
			uint64_t	iIndexCount{ 0 };
			uint64_t	iIndexOffset{ 0 };
			uint32_t*	iIndexData{ nullptr };

			Vector		vecMinExtents{};
			Vector		vecMaxExtents{};
		};

		MeshResource_t() 
		{
		}
		
		MeshResource_t(const MeshResource_t& pOther)
		{
			pMeshes = pOther.pMeshes;
		}

		MeshResource_t& operator=(const MeshResource_t& pOther)
		{
			pMeshes = pOther.pMeshes;
			return *this;
		}

		~MeshResource_t()
		{
		}

		// An array of meshes (or submeshes) in this model.
		Utl::CVector<GeometryObject_t>	pMeshes{};
		Utl::CVector<char*>				pMaterialNames{};
	};
}