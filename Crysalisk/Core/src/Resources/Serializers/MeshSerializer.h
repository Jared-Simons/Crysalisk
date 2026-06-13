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
	static const u64	kModelResourceMagicNumber = 0xCABEF;
	
	// Some file sub-headers.
	static const u32	kModelResourceSubmeshHeader = 0xCABEA;
	static const u32	kModelResourceVertexDataHeader = 0xCABEB;
	static const u32	kModelResourceIndexDataHeader = 0xCABEC;
	static const u32	kModelResourceMaterialDataHeader = 0xCABED;
	static const u32	kModelResourceStringDataHeader = 0xCABEE;

	// Writes out a model in our own binary format directly from a source file.
	CORE_API bool SerializeOBJMesh(const char* pszOBJMeshPath, const char* pszOutMeshFileName, MeshLoader::MeshResource_t& pOutResource);

	// File header for a CModel.
	struct CModel_FileHeader_t
	{
		// The file version of this asset.
		u32	iFileVersion{ 0 };

		u32	iMeshDataCount{ 0 };
		u8		iMeshDataStride{ 0 };

		u32	iVertexCount{ 0 };
		u32	iVertexStride{ 0 };
		u32	iIndexCount{ 0 };
		u32	iIndexStride{ 0 };

		u64	iMaterialCount{ 0 };

		// The extents of the whole mesh.
		Vector		vecMinExtents{};
		Vector		vecMaxExtents{};
	};

	// Holds indices specific to each face.
	struct CModel_MeshFaceData_t
	{
		u32	iPositionIndices[3]{};
		u32	iNormalIndices[3]{};
		u32	iTexCoordIndices[3]{};
		u32	iTangentIndices[3]{};
	};

	// Contains data specific to each mesh/submesh.
	struct CModel_MeshHeader_t
	{
		u32				iMeshNameIndex{ kInvalidID_U32 };
		// The index of this mesh's material in the model's material table.
		u32				iMaterialNameIndex{ kInvalidID_U32 };

		u32				iVertexCount{ 0 };
		u64				iVertexOffset{ 0 };
		u32				iIndexCount{ 0 };
		u64				iIndexOffset{ 0 };

		// The extents of this submesh.
		Vector					vecMinExtents{};
		Vector					vecMaxExtents{};
	};
}
