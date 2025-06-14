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

		// Get the total number of indices in this model.
		uint32_t totalIndices{ 0 };

		for (int objIdx = 0; objIdx < pInResource.pObjects.Count(); objIdx++)
		{
			for (int gpIdx = 0; gpIdx < pInResource.pObjects[objIdx]->pGroups.Count(); gpIdx++)
			{
				totalIndices += pInResource.pObjects[objIdx]->pGroups[gpIdx]->Indices.Count();
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

		// Vertex data info.
		fileHeader.iVertexDataOffset = sizeof(uint32_t) + sizeof(CModel_FileHeader_t);
		fileHeader.iVertexElementStride = sizeof(Vertex3D);
		fileHeader.iVertexDataCount = totalVerts;

		// Index data info.
		fileHeader.iIndexDataOffset = fileHeader.iVertexDataOffset + (fileHeader.iVertexDataCount * fileHeader.iVertexElementStride);
		fileHeader.iIndexElementStride = sizeof(uint32_t);
		fileHeader.iIndexDataCount = totalIndices;

		// Submesh data info.
		fileHeader.iSubmeshDataOffset = fileHeader.iIndexDataOffset + (fileHeader.iIndexDataCount * fileHeader.iIndexElementStride);
		fileHeader.iSubmeshDataStride = sizeof(MeshLoader::MeshResource_t::GeometryGroup_t);
		fileHeader.iSubmeshDataSize = totalSubmeshes;

		FileSystem::WriteBytes(&fileHeader, sizeof(CModel_FileHeader_t), &pCModelFileHandle);

		// Write out all of the vertex data.
		for (int objIdx = 0; objIdx < pInResource.pObjects.Count(); objIdx++)
		{
			for (int gpIdx = 0; gpIdx < pInResource.pObjects[objIdx]->pGroups.Count(); gpIdx++)
			{
				Utl::CVector<Vertex3D>& pVertices = pInResource.pObjects[objIdx]->pGroups[gpIdx]->Vertices;
				FileSystem::WriteBytes(pVertices.Data(), pVertices.Count() * pVertices.Stride(), &pCModelFileHandle);
			}
		}

		// Write out all of the index data.
		for (int objIdx = 0; objIdx < pInResource.pObjects.Count(); objIdx++)
		{
			for (int gpIdx = 0; gpIdx < pInResource.pObjects[objIdx]->pGroups.Count(); gpIdx++)
			{
				Utl::CVector<uint32_t>& pIndices = pInResource.pObjects[objIdx]->pGroups[gpIdx]->Indices;
				FileSystem::WriteBytes(pIndices.Data(), pIndices.Count() * pIndices.Stride(), &pCModelFileHandle);
			}
		}

		// Write out all of the submesh data.
		for (int objIdx = 0; objIdx < pInResource.pObjects.Count(); objIdx++)
		{
			FileSystem::WriteBytes(pInResource.pObjects[objIdx]->pGroups.Data(), sizeof(MeshLoader::MeshResource_t::GeometryGroup_t) * pInResource.pObjects[objIdx]->pGroups.Count(), &pCModelFileHandle);
		}

		FileSystem::CloseFile(&pCModelFileHandle);

		delete[] pTempNameBuffer;

		return true;
	}
}