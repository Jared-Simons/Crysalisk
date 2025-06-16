//=============================================================================
// Author: Jared Simons
// Date Created: 6/12/2025
// 
// Purpose: Mesh file serialization utilities.
//=============================================================================
#include "MeshSerializer.h"

#include "Platform/FileSystem.h"

namespace MeshSerializer
{
	static const char*		kCModelFileExtension = ".cmodel_c";
	static const uint32_t	kCurrentModelVersion = 1;

	bool SerializeMeshResource(const char* pszModelName, MeshLoader::MeshResource_t& pInResource)
	{
		// Create and setup the file.

		// Chop off the file extension.
		size_t nameLength = StringLength(pszModelName);
		size_t extensionLength = StringLength(kCModelFileExtension);

		char* pTempNameBuffer = new char[nameLength + extensionLength + 1];
		StringFormat_V("%s%s", pTempNameBuffer, pszModelName, kCModelFileExtension);

		FileHandle_t pCModelFileHandle{};
		if (!FileSystem::OpenFile(pTempNameBuffer, eFileMode::FILEMODE_WRITE, true, &pCModelFileHandle))
		{
			LOG_ERROR("Failed to write CModel for model '%s'.", pszModelName);
			return false;
		}

		// Place the resource's magic number first.
		FileSystem::WriteBytes(&kModelResourceMagicNumber, sizeof(uint32_t), &pCModelFileHandle);

		// Get the total number of vertices in this model.
		uint32_t totalVerts{ 0 };

		for (int objIdx = 0; objIdx < pInResource.pObjects.Count(); objIdx++)
		{
			for (int gpIdx = 0; gpIdx < pInResource.pObjects[objIdx]->pGroups.Count(); gpIdx++)
			{
				totalVerts += pInResource.pObjects[objIdx]->pGroups[gpIdx]->Vertices.Count();
			}
		}

		// Get the total number of submeshes in this model.
		uint32_t totalSubmeshes{ 0 };

		for (int objIdx = 0; objIdx < pInResource.pObjects.Count(); objIdx++)
		{
			totalSubmeshes += pInResource.pObjects[objIdx]->pGroups.Count();
		}

		// Write out the header.
		CModel_FileHeader_t fileHeader{};
		fileHeader.iFileVersion = kCurrentModelVersion;

		// Submesh data info.
		fileHeader.iSubmeshDataOffset = sizeof(CModel_FileHeader_t);
		fileHeader.iSubmeshDataStride = sizeof(MeshLoader::MeshResource_t::GeometryGroup_t);
		fileHeader.iSubmeshDataSize = totalSubmeshes;

		FileSystem::WriteBytes(&fileHeader, sizeof(CModel_FileHeader_t), &pCModelFileHandle);

		// Write out all of the submesh data.
		for (int objIdx = 0; objIdx < pInResource.pObjects.Count(); objIdx++)
		{
			for (int gpIdx = 0; gpIdx < pInResource.pObjects[objIdx]->pGroups.Count(); gpIdx++)
			{
				CModel_SubmeshHeader_t subMesh{};

				subMesh.iVertexCount = pInResource.pObjects[objIdx]->pGroups[gpIdx]->Vertices.Count();
				subMesh.iVertexStride = pInResource.pObjects[objIdx]->pGroups[gpIdx]->Vertices.Stride();

				subMesh.iIndexCount = pInResource.pObjects[objIdx]->pGroups[gpIdx]->Indices.Count();
				subMesh.iIndexStride = pInResource.pObjects[objIdx]->pGroups[gpIdx]->Indices.Stride();

				// Material data.
				subMesh.iMaterialNameLength = StringLength(pInResource.pObjects[objIdx]->pGroups[gpIdx]->pszMaterialName);

				// Write the header, then the data.
				FileSystem::WriteBytes(&subMesh, sizeof(CModel_SubmeshHeader_t), &pCModelFileHandle);
				FileSystem::WriteBytes(pInResource.pObjects[objIdx]->pGroups[gpIdx]->pszMaterialName, subMesh.iMaterialNameLength, &pCModelFileHandle);

				FileSystem::WriteBytes(pInResource.pObjects[objIdx]->pGroups[gpIdx]->Vertices.Data(), subMesh.iVertexCount * subMesh.iVertexStride, &pCModelFileHandle);
				FileSystem::WriteBytes(pInResource.pObjects[objIdx]->pGroups[gpIdx]->Indices.Data(), subMesh.iIndexCount * subMesh.iIndexStride, &pCModelFileHandle);
			}
		}

		FileSystem::CloseFile(&pCModelFileHandle);

		delete[] pTempNameBuffer;

		return true;
	}
}