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
	static const uint64_t	kModelResourceMagicNumber = 0xCABEF;
	
	// Some file sub-headers.
	static const uint32_t	kModelResourceSubmeshHeader = 0xCABEA;
	static const uint32_t	kModelResourceVertexDataHeader = 0xCABEB;
	static const uint32_t	kModelResourceIndexDataHeader = 0xCABEC;
	static const uint32_t	kModelResourceMaterialDataHeader = 0xCABED;
	static const uint32_t	kModelResourceStringDataHeader = 0xCABEE;

	// Writes out a model in our own binary format directly from a source file.
	CORE_API bool SerializeOBJMesh(const char* pszOBJMeshPath, const char* pszOutMeshFileName, MeshLoader::MeshResource_t& pOutResource);

	// File header for a CModel.
	struct CModel_FileHeader_t
	{
		// The file version of this asset.
		uint32_t	iFileVersion{ 0 };

		// The extents of the whole mesh.
		Vector		vecMinExtents{};
		Vector		vecMaxExtents{};

		uint64_t	iMeshDataOffset{ 0 };
		uint32_t	iMeshDataCount{ 0 };
		uint8_t		iMeshDataStride{ 0 };

		uint32_t	iTotalVertexCount{ 0 };
		uint32_t	iVertexStride{ 0 };
		uint32_t	iTotalIndexCount{ 0 };
		uint32_t	iTotalIndexStride{ 0 };

		uint64_t	iMaterialTableOffset{ 0 };
		uint64_t	iMaterialCount{ 0 };
		// NOTE: Material stride will depend on each string.
		// The data is tightly packed, so the strings will be right next to each other.
	};

	// Holds indices specific to each face.
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
		uint32_t				iMeshNameIndex{ kInvalidID_U32 };
		// The index of this mesh's material in the model's material table.
		uint32_t				iMaterialNameIndex{ kInvalidID_U32 };

		uint32_t				iVertexCount{ 0 };
		uint64_t				iVertexOffset{ 0 };
		uint32_t				iIndexCount{ 0 };
		uint64_t				iIndexOffset{ 0 };

		// The extents of this submesh.
		Vector					vecMinExtents{};
		Vector					vecMaxExtents{};
	};
}
