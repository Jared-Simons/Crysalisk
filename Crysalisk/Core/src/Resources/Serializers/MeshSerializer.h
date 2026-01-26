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
	// A Crysalisk-specific model asset (.cmodel_c file) must start with this magic number to be considered a valid resource.
	static const uint64_t	kModelResourceMagicNumber = 0xCABED;

	// Takes an existing mesh resource imported from another file type, and write it out to disk
	// in our own binary format.
	CORE_API bool SerializeMeshResource(const char* pszModelName, struct MeshLoader::MeshResource_t& pInResource);

	// TODO: Documentation.
	typedef uint16_t VertexLayoutFlags_t;

	enum eVertexLayoutDataFlags
	{
		VERTEX_LAYOUT_DATA_UNDEFINED = 0x00,
		VERTEX_LAYOUT_DATA_POSITION = 0x01,
		VERTEX_LAYOUT_DATA_TEXCOORD = 0x02,
		VERTEX_LAYOUT_DATA_NORMAL = 0x04,
		VERTEX_LAYOUT_DATA_TANGENT = 0x08,
	};

	// File header for a CModel.
	struct CModel_FileHeader_t
	{
		// The file version of this asset.
		uint32_t	iFileVersion{ 0 };

		// A set of bitflags that defines this mesh's vertex layout.
		VertexLayoutFlags_t	iVertexLayoutFlags{ 0 };

		// The offset from the start of the file in bytes where the submesh data begins.
		uint64_t	iSubmeshDataOffset{ 0 };
		// The number of submeshes contained in this file.
		uint64_t	iSubmeshDataSize{ 0 };
		// The stride of each submesh element.
		uint64_t	iSubmeshDataStride{ 0 };
	};

	// Contains data specific to each submesh.
	struct CModel_SubmeshHeader_t
	{
		// A random magic number that identifies the start of a block as containing submesh data.
		uint64_t	iSubmeshMagic{ 0xBC };

		// The length in bytes of each vertex.
		uint64_t	iVertexStride{ 0 };
		// The number of vertices in this submesh.
		uint64_t	iVertexCount{ 0 };

		// The length in bytes of each index.
		uint64_t	iIndexStride{ 0 };
		// The number of indices in this submesh.
		uint64_t	iIndexCount{ 0 };

		// The length of the submesh name (does not include a null-terminator).
		uint64_t	iSubmeshNameLength{ 0 };
		// The length of the submesh's material name (does not include a null-terminator).
		uint64_t	iMaterialNameLength{ 0 };
	};
}