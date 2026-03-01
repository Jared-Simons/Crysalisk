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

	// Writes out a model in our own binary format directly from a source file.
	CORE_API bool SerializeOBJMesh(const char* pszOBJMeshPath, const char* pszOutMeshFileName);

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

		// The extents of the whole mesh.
		Vector		vecMinExtents{};
		Vector		vecMaxExtents{};

		// A set of bitflags that defines this mesh's vertex layout.
		VertexLayoutFlags_t	iVertexLayoutFlags{ 0 };

		// Offsets from the start of the file in bytes of the different data tables.
		uint64_t	iMeshDataOffset{ 0 };
		uint32_t	iMeshDataCount{ 0 };
		uint8_t		iMeshDataStride{ 0 };

		uint64_t	iFaceDataOffset{ 0 };
		uint32_t	iFaceDataCount{ 0 };
		uint8_t		iFaceDataStride{ 0 };

		uint64_t	iPositionTableOffset{ 0 };
		uint32_t	iPositionCount{ 0 };
		uint8_t		iPositionStride{ 0 };

		uint64_t	iNormalTableOffset{ 0 };
		uint32_t	iNormalCount{ 0 };
		uint8_t		iNormalStride{ 0 };

		uint64_t	iTexCoordTableOffset{ 0 };
		uint32_t	iTexCoordCount{ 0 };
		uint8_t		iTexCoordStride{ 0 };

		uint64_t	iTangentTableOffset{ 0 };
		uint32_t	iTangentCount{ 0 };
		uint8_t		iTangentStride{ 0 };
	};

	struct CModel_MeshFaceData_t
	{
		uint32_t	iPositionIndices[3]{};
		uint32_t	iNormalIndices[3]{};
		uint32_t	iTexCoordIndices[3]{};
		uint32_t	iTangentIndices[3]{};
	};

	// Contains data specific to each mesh/submesh.
	struct CModel_MeshHeader_t
	{
		uint16_t				iNumFaces{ 0 };
		uint16_t*				pFaceIndices{ nullptr };
		// The extents of this submesh.
		Vector					vecMinExtents{};
		Vector					vecMaxExtents{};
	};
}
