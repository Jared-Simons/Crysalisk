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

		// Write out the header.
		CModel_FileHeader_t fileHeader{};
		fileHeader.iFileVersion = kCurrentModelVersion;

		// Vertex data info.
		fileHeader.iVertexDataOffset = sizeof(uint32_t) + sizeof(CModel_FileHeader_t);
		fileHeader.iVertexElementStride = sizeof(Vertex3D);
		fileHeader.iVertexDataCount = totalVerts;

		FileSystem::WriteBytes(&fileHeader, sizeof(CModel_FileHeader_t), &pCModelFileHandle);

		FileSystem::CloseFile(&pCModelFileHandle);

		delete[] pTempNameBuffer;

		return true;
	}
}