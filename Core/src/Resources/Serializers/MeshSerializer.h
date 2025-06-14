//=============================================================================
// Author: Jared Simons
// Date Created: 6/12/2025
// 
// Purpose: Mesh file serialization utilities.
//=============================================================================
#pragma once

#include <Defines.h>
#include "Resources/Loaders/MeshLoader.h"

namespace MeshSerializer
{
	static const uint64_t	kModelResourceMagicNumber = 0xCABED;	// Random trash magic number for now.

	CORE_API bool SerializeMeshResource(const char* pszModelName, struct MeshLoader::MeshResource_t& pInResource);

	struct CModel_FileHeader_t
	{
		uint32_t	iFileVersion{ 0 };

		uint64_t	iVertexDataOffset{ 0 };
		uint64_t	iVertexDataCount{ 0 };
		uint64_t	iVertexElementStride{ 0 };

		uint64_t	iIndexDataOffset{ 0 };
		uint64_t	iIndexDataCount{ 0 };
		uint64_t	iIndexElementStride{ 0 };

		uint64_t	iSubmeshDataOffset{ 0 };
		uint64_t	iSubmeshDataSize{ 0 };
		uint64_t	iSubmeshDataStride{ 0 };
	};
}