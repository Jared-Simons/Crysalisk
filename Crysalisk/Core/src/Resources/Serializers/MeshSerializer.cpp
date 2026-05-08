//=============================================================================
// Author: Jared Simons
// Date Created: 6/12/2025
// 
// Purpose: Mesh file serialization utilities.
//=============================================================================
#include "MeshSerializer.h"

#include "MaterialSerializer.h"
#include "Platform/FileSystem.h"
#include "Utils/Utils.h"

#include <stdio.h>	// for sscanf()
#include <float.h>

namespace MeshSerializer
{
	// Specific file extension for the Crysalisk model file format (CModels).
	static const char*		kCModelFileExtension = ".cmodel_c";
	// The current most up-to-date version of the CModel file format.
	static const uint32_t	kCurrentModelVersion = 2;

	// Identifies unique vertex data
	struct UniqueVertex_t
	{
		uint32_t iPositionIdx{};
		uint32_t iNormalIdx{};
		uint32_t iTexCoordIdx{};
	};

	// Forward declarations
	static void GenerateTangentData(CModel_MeshHeader_t& pCurMesh, Utl::CVector<Vector>& pPositions, Utl::CVector<Vector>& pNormals, Utl::CVector<Vector2D>& pTexCoords, Utl::CVector<Vector>& pOutTangents, Utl::CVector<CModel_MeshFaceData_t>& pFaces);
	static void GenerateTangentData(Utl::CVector<Vertex3D>& pVertices);
	static void DeduplicateGeometry(CModel_MeshHeader_t& pCurMesh, Utl::CVector<CModel_MeshFaceData_t>& pInFaces, Utl::CVector<Vector>& pInPositions, Utl::CVector<Vector>& pInNormals, Utl::CVector<Vector2D>& pInTexCoords,
									Utl::CVector<CModel_MeshFaceData_t>& pOutFaces, Utl::CVector<Vector>& pOutPositions, Utl::CVector<Vector>& pOutNormals, Utl::CVector<Vector2D>& pOutTexCoords);
	static void DeduplicateGeometry(Utl::CVector<CModel_MeshFaceData_t>& pFaces, Utl::CVector<Vector>& pInPositions, Utl::CVector<Vector>& pInNormals, Utl::CVector<Vector2D>& pInTexCoords, Utl::CVector<UniqueVertex_t>& pOutVertices, Utl::CVector<uint32_t>& pOutIndices);
	static void PrepareIndexBuffer(CModel_MeshHeader_t& pCurMesh);
	static bool ImportOBJMaterialLibrary(const char* pszMaterialLibraryName, MeshLoader::MeshResource_t& pMeshResource);

	//-------------------------------------------------------------------------
	// Purpose: Attempts to write an OBJ mesh out in our own mesh format.
	//-------------------------------------------------------------------------
	// NEW ATTEMPT!
	bool SerializeOBJMesh_Test(const char* pszOBJMeshPath, const char* pszOutMeshFileName, MeshLoader::MeshResource_t& pOutResource)
	{
		// Make sure the path is valid.
		// We can technically proceed without having an output file name, 
		// we'll just make it the same as the input file name with a new extension.
		if (!pszOBJMeshPath)
		{
			LOG_ERROR("SerializeOBJMesh() requires a path to a mesh to serialize! Mesh serialization failed.");
			return false;
		}

		FileHandle_t hOBJFile{};

		if (!FileSystem::FileExists(pszOBJMeshPath))
		{
			// Check with an extension added on, just in case.
			char szTempStringBuffer[2048]{};
			Utl::StringFormat_V("%s.obj", szTempStringBuffer, pszOBJMeshPath);

			if (!FileSystem::FileExists(szTempStringBuffer))
			{
				LOG_ERROR("SerializeOBJMesh() - Source file '%s' does not exist. Mesh serialization failed.");
				return false;
			}
			else
			{
				// We found the file after adding the extension, open 'er up!
				if (!FileSystem::OpenFile(szTempStringBuffer, FILEMODE_READ, false, &hOBJFile))
				{
					LOG_ERROR("Failed to open OBJ file '%s' for reading.", szTempStringBuffer);
					return false;
				}
			}
		}
		else
		{
			// Found the file.
			if (!FileSystem::OpenFile(pszOBJMeshPath, FILEMODE_READ, false, &hOBJFile))
			{
				LOG_ERROR("Failed to open OBJ file '%s' for reading.", pszOBJMeshPath);
				return false;
			}
		}

		LOG_INFO("Serializing mesh '%s'.", pszOBJMeshPath);

		// Create a new file/overwrite the existing file to store the new mesh.
		FileHandle_t hNewOutputMesh{};
		uint64_t iTotalBytesWritten{ 0 };
		uint64_t iOriginalByteSize = FileSystem::FileSize(&hOBJFile);
		if (pszOutMeshFileName == nullptr)
		{
			// Just steal the input file path and put our extension on it.
			char szTempStringBuffer[2048]{};
			Utl::StringFormat_V("%s.%s", szTempStringBuffer, pszOBJMeshPath, kCModelFileExtension);	// TODO: We might need to strip the original OBJ extension off the input path.
			CAssert(0);	// We need to test this.
		}
		else
		{
			// Attach our extension onto the output mesh.
			char szTempStringBuffer[2048];
			Utl::StringFormat_V("%s%s", szTempStringBuffer, pszOutMeshFileName, kCModelFileExtension);
			if (!FileSystem::OpenFile(szTempStringBuffer, FILEMODE_WRITE, true, &hNewOutputMesh))
			{
				LOG_ERROR("Failed to open new file for writing mesh '%s'. Mesh serialization failed.", pszOBJMeshPath);
				return false;
			}
		}

		CModel_FileHeader_t	outFileHeader{};
		outFileHeader.iFileVersion = kCurrentModelVersion;

		// First determine the vertex format for this model.
		// This makes the assumption that there is a singular vertex layout that is valid for the entire file.
		bool bModelContainsVertexPositions{ false };
		bool bModelContainsVertexNormals{ false };
		bool bModelContainsVertexTexCoords{ false };
		bool bModelContainsVertexTangents{ false };

		char szCurrentLine[2048]{};
		char firstChar{};
		char secondChar{};
		size_t bytesRead{ 0 };

		LOG_INFO("Determining vertex layout");

		bool bAllNecessaryAttributesFound = false;
		while (FileSystem::ReadLine(szCurrentLine, sizeof(char) * 2048, &bytesRead, &hOBJFile) && !bAllNecessaryAttributesFound)
		{
			bAllNecessaryAttributesFound = bModelContainsVertexPositions && bModelContainsVertexNormals && bModelContainsVertexTexCoords;
			firstChar = szCurrentLine[0];
			secondChar = szCurrentLine[1];
			if (firstChar == 'v')
			{
				// Need to take a look at the second char to confirm.
				if (secondChar == ' ')
				{
					// Vertex position.
					bModelContainsVertexPositions = true;
				}
				else if (secondChar == 'n')
				{
					// Vertex normal.
					bModelContainsVertexNormals = true;
				}
				else if (secondChar == 't')
				{
					// Vertex texcoords.
					bModelContainsVertexTexCoords = true;
				}
			}
		}

		// Spew some vertex layout information:
		LOG_DEBUG("Mesh contains position data: \t%s", bModelContainsVertexPositions ? "true" : "false");
		LOG_DEBUG("Mesh contains normal data: \t%s", bModelContainsVertexNormals ? "true" : "false");
		LOG_DEBUG("Mesh contains texcoord data: \t%s", bModelContainsVertexTexCoords ? "true" : "false");
		LOG_DEBUG("Mesh contains tangent data: \t%s", bModelContainsVertexTangents ? "true" : "false");

		// Build a bitfield representing the layout and store it in the header.
		uint16_t uVertexLayout{ VERTEX_LAYOUT_DATA_UNDEFINED };
		if (bModelContainsVertexPositions)
			uVertexLayout |= VERTEX_LAYOUT_DATA_POSITION;
		if (bModelContainsVertexNormals)
			uVertexLayout |= VERTEX_LAYOUT_DATA_NORMAL;
		if (bModelContainsVertexTexCoords)
			uVertexLayout |= VERTEX_LAYOUT_DATA_TEXCOORD;
		if (bModelContainsVertexTangents)
			uVertexLayout |= VERTEX_LAYOUT_DATA_TANGENT;

		outFileHeader.iVertexLayoutFlags = uVertexLayout;

		// Rewind the file so we can read it through again.
		FileSystem::ReturnFilePointerToHead(&hOBJFile);

		// Temp geometry tables.
		Utl::CVector<CModel_MeshHeader_t>		meshHeaders{};
		Utl::CVector<CModel_MeshFaceData_t>		meshFaces{};
		Utl::CVector<Utl::CVector<Vector>>		tempVertexPositions{};
		Utl::CVector<Utl::CVector<Vector>>		tempVertexNormals{};
		Utl::CVector<Utl::CVector<Vector2D>>	tempVertexTexcoords{};

		// Write out header data.
		FileSystem::WriteBytes(&kModelResourceMagicNumber, sizeof(uint32_t), &hNewOutputMesh);

		// REMOVE ME IMMEDIATELY!
		bool debugStop = false;

		while (FileSystem::ReadLine(szCurrentLine, sizeof(char) * 2048, &bytesRead, &hOBJFile) && !debugStop)
		{
			char* pFirstChar = &szCurrentLine[0];
			char* pSecondChar = &szCurrentLine[1];

			switch (*pFirstChar)
			{
			case 'o':
				// Finalize any existing geometry data before moving onto the next one.
				// FIXME: This won't apply to the last mesh in the file.
				if (meshHeaders.Count() > 0)
				{
					Utl::CVector<Vertex3D> vertices{};
					Utl::CVector<int16_t> packedVertexData{};

					// Need 3 indices per face.
					meshHeaders.Back().iIndexCount = (uint32_t)meshFaces.Count() * 3;
					PrepareIndexBuffer(meshHeaders.Back());

					// Get rid of any duplicate data.
					//DeduplicateGeometry(meshHeaders.Back(), meshFaces, tempVertexPositions.Back(), tempVertexNormals.Back(), tempVertexTexcoords.Back(), vertices);

					// Generate tangents before packing.
					// FIXME: Implement.

					// Calculate extents for this mesh.
					for (int iVert = 0; iVert < vertices.Count(); iVert++)
					{
						Vector& pPos = vertices[iVert].Pos;
						if (pPos.x < meshHeaders.Back().vecMinExtents.x)
							meshHeaders.Back().vecMinExtents.x = pPos.x;
						if (pPos.y < meshHeaders.Back().vecMinExtents.y)
							meshHeaders.Back().vecMinExtents.y = pPos.y;
						if (pPos.z < meshHeaders.Back().vecMinExtents.z)
							meshHeaders.Back().vecMinExtents.z = pPos.z;

						if (pPos.x > meshHeaders.Back().vecMaxExtents.x)
							meshHeaders.Back().vecMaxExtents.x = pPos.x;
						if (pPos.y > meshHeaders.Back().vecMaxExtents.y)
							meshHeaders.Back().vecMaxExtents.y = pPos.y;
						if (pPos.z > meshHeaders.Back().vecMaxExtents.z)
							meshHeaders.Back().vecMaxExtents.z = pPos.z;
					}

					// Pack existing vertex data and write it out to the file.
					// FIXME: DRY!
					for (int iVert = 0; iVert < vertices.Count(); iVert++)
					{
						// Positions.
						{
							float flNormalizedX = (vertices[iVert].Pos.x - meshHeaders.Back().vecMinExtents.x) / (meshHeaders.Back().vecMaxExtents.x - meshHeaders.Back().vecMinExtents.x);
							float flNormalizedY = (vertices[iVert].Pos.y - meshHeaders.Back().vecMinExtents.y) / (meshHeaders.Back().vecMaxExtents.y - meshHeaders.Back().vecMinExtents.y);
							float flNormalizedZ = (vertices[iVert].Pos.z - meshHeaders.Back().vecMinExtents.z) / (meshHeaders.Back().vecMaxExtents.z - meshHeaders.Back().vecMinExtents.z);

							uint16_t packedPosX = PackF32ToUint16(flNormalizedX);
							uint16_t packedPosY = PackF32ToUint16(flNormalizedY);
							uint16_t packedPosZ = PackF32ToUint16(flNormalizedZ);
							packedVertexData.PushBack(packedPosX);
							packedVertexData.PushBack(packedPosY);
							packedVertexData.PushBack(packedPosZ);
						}

						// Normals.
						{
							float flNormalizedX = (vertices[iVert].Normal.x + 1.0f) / 2.0f;
							float flNormalizedY = (vertices[iVert].Normal.y + 1.0f) / 2.0f;
							float flNormalizedZ = (vertices[iVert].Normal.z + 1.0f) / 2.0f;

							uint16_t packedPosX = PackF32ToUint16(flNormalizedX);
							uint16_t packedPosY = PackF32ToUint16(flNormalizedY);
							uint16_t packedPosZ = PackF32ToUint16(flNormalizedZ);
							packedVertexData.PushBack(packedPosX);
							packedVertexData.PushBack(packedPosY);
							packedVertexData.PushBack(packedPosZ);
						}

						// Texcoords.
						{
							const float flPackedTexCoordMins = -64.0f;
							const float flPackedTexCoordMaxs = 64.0f;

							float flNormalizedTexCoordX = (vertices[iVert].TexCoord.x + flPackedTexCoordMins) / (flPackedTexCoordMaxs - flPackedTexCoordMins);
							float flNormalizedTexCoordY = (vertices[iVert].TexCoord.y + flPackedTexCoordMins) / (flPackedTexCoordMaxs - flPackedTexCoordMins);

							flNormalizedTexCoordX = (flNormalizedTexCoordX + 1.0f) / 2.0f;
							flNormalizedTexCoordY = (flNormalizedTexCoordY + 1.0f) / 2.0f;

							uint16_t packedTexCoordX = PackF32ToUint16(flNormalizedTexCoordX);
							uint16_t packedTexCoordY = PackF32ToUint16(flNormalizedTexCoordY);
							packedVertexData.PushBack(packedTexCoordX);
							packedVertexData.PushBack(packedTexCoordY);
						}
					}
					meshHeaders.Back().iVertexCount = (uint32_t)vertices.Count();

					// Write the data out to the file.
					FileSystem::WriteBytes(&kModelResourceSubmeshHeader, sizeof(uint32_t), &hNewOutputMesh);
					FileSystem::WriteBytes(&meshHeaders.Back(), sizeof(CModel_MeshHeader_t), &hNewOutputMesh);
					FileSystem::WriteBytes(&meshHeaders.Back().pIndexData, sizeof(uint32_t) * meshHeaders.Back().iIndexCount, &hNewOutputMesh);
					FileSystem::WriteBytes(packedVertexData.Data(), packedVertexData.Stride() * packedVertexData.Count(), &hNewOutputMesh);

					CAssert(0);
				}

				if (tempVertexPositions.Count() + 1 > 1)
				{
					debugStop = true;
					break;
				}
				// Setup a new array for us to read geometry into.
				meshHeaders.PushBack(CModel_MeshHeader_t{});
				meshFaces.Clear();
				tempVertexPositions.PushBack(Utl::CVector<Vector>{});
				tempVertexNormals.PushBack(Utl::CVector<Vector>{});
				tempVertexTexcoords.PushBack(Utl::CVector<Vector2D>{});
				break;

			case 'g':
				// New face data.
				break;

			case 'f':	// Polygon 'face'.
			{
				uint32_t	normalIndices[3];
				uint32_t	positionIndices[3];
				uint32_t	texCoordIndices[3];

				int elements = sscanf_s(szCurrentLine, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", &positionIndices[0], &texCoordIndices[0], &normalIndices[0], &positionIndices[1], &texCoordIndices[1], &normalIndices[1], &positionIndices[2], &texCoordIndices[2], &normalIndices[2]);

				CAssert(elements == 9);

				// Write the elements out to the mesh header.
				CModel_MeshFaceData_t newFace{};
				for (int iVert = 0; iVert < 3; iVert++)
				{
					// NOTE: OBJ files are 1-indexed so we need to bump down by one to store as a zero index.
					newFace.iPositionIndices[iVert] = positionIndices[iVert] - 1;
					newFace.iNormalIndices[iVert] = normalIndices[iVert] - 1;
					newFace.iTexCoordIndices[iVert] = texCoordIndices[iVert] - 1;
				}

				uint32_t iFaceIndex = (uint32_t)meshFaces.PushBack(newFace);
				break;
			}

			case 'v':
				{
					// Vertex data of some kind, need to look at the second char.
					switch (*pSecondChar)
					{
						case ' ':	// Vertex positions.
						{
							Vector newPosition{};
							int numElements = sscanf_s(&szCurrentLine[2], "%f %f %f", &newPosition.x, &newPosition.y, &newPosition.z);
							CAssert(numElements == 3);

							// Add it to the table.
							tempVertexPositions.Back().PushBack(newPosition);
							break;
						}
						break;

						case 'n':	// Vertex normals.
						{
							Vector newNormal{};
							int numElements = sscanf_s(&szCurrentLine[2], "%f %f %f", &newNormal.x, &newNormal.y, &newNormal.z);
							CAssert(numElements == 3);

							// When we serialize these, we're gonna quantize with -1.0 and 1.0 being our bounds.
							CAssert(newNormal.x <= 1.0f && newNormal.x >= -1.0f);
							CAssert(newNormal.y <= 1.0f && newNormal.y >= -1.0f);
							CAssert(newNormal.z <= 1.0f && newNormal.z >= -1.0f);

							// Add it to the table.
							tempVertexNormals.Back().PushBack(newNormal);
							break;
						}

						case 't':	// Vertex texture coordinates.
						{
							Vector2D newTexCoord{};
							int numElements = sscanf_s(&szCurrentLine[2], "%f %f", &newTexCoord.x, &newTexCoord.y);
							CAssert(numElements == 2);

							// When we serialize these, we're gonna quantize with -512.0 and 512.0 being our bounds. (We're being VERY generous with repeated textures).
							CAssert(newTexCoord.x <= 512.0f && newTexCoord.x >= -512.0f);
							CAssert(newTexCoord.y <= 512.0f && newTexCoord.y >= -512.0f);

							// Add it to the table.
							tempVertexTexcoords.Back().PushBack(newTexCoord);
							break;
						}
					}
				}
				break;
			}
		}

		// Close the new file.
		FileSystem::CloseFile(&hNewOutputMesh);

		// Close the original source file.
		FileSystem::CloseFile(&hOBJFile);

		return true;
	}

	bool SerializeOBJMesh(const char* pszOBJMeshPath, const char* pszOutMeshFileName, MeshLoader::MeshResource_t& pOutResource)
	{
		// Make sure the path is valid.
		// We can technically proceed without having an output file name, 
		// we'll just make it the same as the input file name with a new extension.
		if (!pszOBJMeshPath)
		{
			LOG_ERROR("SerializeOBJMesh() requires a path to a mesh to serialize! Mesh serialization failed.");
			return false;
		}

		FileHandle_t hOBJFile{};

		if (!FileSystem::FileExists(pszOBJMeshPath))
		{
			// Check with an extension added on, just in case.
			char szTempStringBuffer[2048]{};
			Utl::StringFormat_V("%s.obj", szTempStringBuffer, pszOBJMeshPath);

			if (!FileSystem::FileExists(szTempStringBuffer))
			{
				LOG_ERROR("SerializeOBJMesh() - Source file '%s' does not exist. Mesh serialization failed.");
				return false;
			}
			else
			{
				// We found the file after adding the extension, open 'er up!
				if (!FileSystem::OpenFile(szTempStringBuffer, FILEMODE_READ, false, &hOBJFile))
				{
					LOG_ERROR("Failed to open OBJ file '%s' for reading.", szTempStringBuffer);
					return false;
				}
			}
		}
		else
		{
			// Found the file.
			if (!FileSystem::OpenFile(pszOBJMeshPath, FILEMODE_READ, false, &hOBJFile))
			{
				LOG_ERROR("Failed to open OBJ file '%s' for reading.", pszOBJMeshPath);
				return false;
			}
		}

		LOG_INFO("Serializing mesh '%s'.", pszOBJMeshPath);

		// Create a new file/overwrite the existing file to store the new mesh.
		FileHandle_t hNewOutputMesh{};
		uint64_t iTotalBytesWritten{ 0 };
		uint64_t iOriginalByteSize = FileSystem::FileSize(&hOBJFile);
		if (pszOutMeshFileName == nullptr)
		{
			// Just steal the input file path and put our extension on it.
			char szTempStringBuffer[2048]{};
			Utl::StringFormat_V("%s.%s", szTempStringBuffer, pszOBJMeshPath, kCModelFileExtension);	// TODO: We might need to strip the original OBJ extension off the input path.
			CAssert(0);	// We need to test this.
		}
		else
		{
			// Attach our extension onto the output mesh.
			char szTempStringBuffer[2048];
			Utl::StringFormat_V("%s%s", szTempStringBuffer, pszOutMeshFileName, kCModelFileExtension);
			if (!FileSystem::OpenFile(szTempStringBuffer, FILEMODE_WRITE, true, &hNewOutputMesh))
			{
				LOG_ERROR("Failed to open new file for writing mesh '%s'. Mesh serialization failed.", pszOBJMeshPath);
				return false;
			}
		}

		CModel_FileHeader_t	outFileHeader{};
		outFileHeader.iFileVersion = kCurrentModelVersion;

		// First determine the vertex format for this model.
		// This makes the assumption that there is a singular vertex layout that is valid for the entire file.
		bool bModelContainsVertexPositions{ false };
		bool bModelContainsVertexNormals{ false };
		bool bModelContainsVertexTexCoords{ false };
		bool bModelContainsVertexTangents{ false };

		char szCurrentLine[2048]{};
		char firstChar{};
		char secondChar{};
		size_t bytesRead{ 0 };

		LOG_INFO("Determining vertex layout");

		bool bAllNecessaryAttributesFound = false;
		while (FileSystem::ReadLine(szCurrentLine, sizeof(char) * 2048, &bytesRead, &hOBJFile) && !bAllNecessaryAttributesFound)
		{
			bAllNecessaryAttributesFound = bModelContainsVertexPositions && bModelContainsVertexNormals && bModelContainsVertexTexCoords;
			firstChar = szCurrentLine[0];
			secondChar = szCurrentLine[1];
			if (firstChar == 'v')
			{
				// Need to take a look at the second char to confirm.
				if (secondChar == ' ')
				{
					// Vertex position.
					bModelContainsVertexPositions = true;
				}
				else if (secondChar == 'n')
				{
					// Vertex normal.
					bModelContainsVertexNormals = true;
				}
				else if (secondChar == 't')
				{
					// Vertex texcoords.
					bModelContainsVertexTexCoords = true;
				}
			}
		}

		// Spew some vertex layout information:
		LOG_DEBUG("Mesh contains position data: \t%s", bModelContainsVertexPositions ? "true" : "false");
		LOG_DEBUG("Mesh contains normal data: \t%s", bModelContainsVertexNormals ? "true" : "false");
		LOG_DEBUG("Mesh contains texcoord data: \t%s", bModelContainsVertexTexCoords ? "true" : "false");
		LOG_DEBUG("Mesh contains tangent data: \t%s", bModelContainsVertexTangents ? "true" : "false");

		// Build a bitfield representing the layout and store it in the header.
		uint16_t uVertexLayout{ VERTEX_LAYOUT_DATA_UNDEFINED };
		if (bModelContainsVertexPositions)
			uVertexLayout |= VERTEX_LAYOUT_DATA_POSITION;
		if (bModelContainsVertexNormals)
			uVertexLayout |= VERTEX_LAYOUT_DATA_NORMAL;
		if (bModelContainsVertexTexCoords)
			uVertexLayout |= VERTEX_LAYOUT_DATA_TEXCOORD;
		if (bModelContainsVertexTangents)
			uVertexLayout |= VERTEX_LAYOUT_DATA_TANGENT;

		outFileHeader.iVertexLayoutFlags = uVertexLayout;

		// Rewind the file so we can read it through again.
		FileSystem::ReturnFilePointerToHead(&hOBJFile);

		// Temp geometry tables.
		Utl::CVector<Vector>								vertexPositions{};
		Utl::CVector<Vector>								vertexNormals{};
		Utl::CVector<Vector2D>								vertexTexCoords{};
		Utl::CVector<CModel_MeshFaceData_t>					meshFaces{};

		// Geometry data.
		Utl::CVector<CModel_MeshHeader_t>	meshHeaders{};
		Utl::CVector<CModel_MeshFaceData_t>	finalMeshFaces{};

		// Final geometry tables.
		Utl::CVector<Vector>	finalVertexPositions{};
		Utl::CVector<Vector>	finalVertexNormals{};
		Utl::CVector<Vector2D>	finalVertexTexCoords{};
		Utl::CVector<Vector>	finalVertexTangents{};

		Utl::CVector<float>		finalVertexData{};
		Utl::CVector<uint32_t>	finalIndexData{};
		uint64_t				iUniqueVertCounter{ 0 };

		// Material stuff.
		char* pszMTLFilePath{ nullptr };
		// Contains all of the material names encountered (does not contain repeats!)
		Utl::CVector<char*>		materialNames{};
		char* pszMaterialName{ nullptr };
		bool bMatFound = false;

		// TEMP DEBUG REMOVE ME!
		const int iDebugMeshCount = 400;
		bool bDebugStop = false;

		Platform::MemorySet(szCurrentLine, 0, sizeof(char) * 2048);
		bytesRead = 0;

		while (FileSystem::ReadLine(szCurrentLine, sizeof(char) * 2048, &bytesRead, &hOBJFile) && !bDebugStop)
		{
			firstChar = szCurrentLine[0];
			secondChar = szCurrentLine[1];

			switch (firstChar)
			{
			case '#':	// Skip comments.
			case 0:		// And blank lines
				break;

			case 'o':	// New geometry object.
			{
				if (meshHeaders.Count() > 0)
				{
					//Utl::CVector<Vertex3D>			uniqueVertices{};
					Utl::CVector<uint32_t>			uniqueIndices{};
					Utl::CVector<UniqueVertex_t>	finalVertexIndices{};
					DeduplicateGeometry(meshFaces, vertexPositions, vertexNormals, vertexTexCoords, finalVertexIndices, uniqueIndices);
					// FIXME: IMPLEMENT
					// GenerateTangentData(uniqueVertices);

					meshHeaders.Back().iVertexCount = (uint32_t)finalVertexIndices.Count();
					meshHeaders.Back().iVertexOffset = iUniqueVertCounter;
					meshHeaders.Back().iIndexCount = (uint32_t)uniqueIndices.Count();
					meshHeaders.Back().iIndexOffset = sizeof(uint32_t) * finalIndexData.Count();	// This is the byte offset into the index data.

					// Make sure the vertex/index offsets are 4-byte aligned!
					// UNDONE: Maybe we don't want to offset the vertex buffer?
					// If we simply offset the index buffer then we only have to re-bind the index buffer at those offsets and bind the vertex buffer once per mesh,
					// not once per drawcall!
					/*uint64_t vertexAlignmentOffset = 0;
					if (meshHeaders.Back().iVertexOffset % 4 != 0)
					{
						vertexAlignmentOffset = 4 - (meshHeaders.Back().iVertexOffset % 4);
						meshHeaders.Back().iVertexOffset += vertexAlignmentOffset;
					}

					for (int idx = 0; idx < vertexAlignmentOffset; idx++)
					{
						finalVertexData.PushBack((float)0.0f);
					}*/

					uint64_t indexAlignmentOffset = 0;
					if (meshHeaders.Back().iIndexOffset % 4 != 0)
					{
						indexAlignmentOffset = 4 - (meshHeaders.Back().iIndexOffset % 4);
						meshHeaders.Back().iIndexOffset += indexAlignmentOffset;
					}

					for (int idx = 0; idx < indexAlignmentOffset; idx++)
					{
						// Fill in some blank data to the index buffer to make sure we're aligned.
						finalIndexData.PushBack((uint32_t)0);
					}

					uint32_t indexBufferBase = (uint32_t)iUniqueVertCounter;
					iUniqueVertCounter += finalVertexIndices.Count();

					// Write out the unique data to the output tables.
					for (uint32_t iVert = 0; iVert < finalVertexIndices.Count(); iVert++)
					{
						finalVertexData.PushBack(vertexPositions[finalVertexIndices[iVert].iPositionIdx].x);
						finalVertexData.PushBack(vertexPositions[finalVertexIndices[iVert].iPositionIdx].y);
						finalVertexData.PushBack(vertexPositions[finalVertexIndices[iVert].iPositionIdx].z);

						finalVertexData.PushBack(vertexNormals[finalVertexIndices[iVert].iNormalIdx].x);
						finalVertexData.PushBack(vertexNormals[finalVertexIndices[iVert].iNormalIdx].y);
						finalVertexData.PushBack(vertexNormals[finalVertexIndices[iVert].iNormalIdx].z);

						finalVertexData.PushBack(vertexTexCoords[finalVertexIndices[iVert].iTexCoordIdx].x);
						finalVertexData.PushBack(vertexTexCoords[finalVertexIndices[iVert].iTexCoordIdx].y);

						// FIXME: WRITE TANGENTS
						finalVertexData.PushBack(0.0f);
						finalVertexData.PushBack(0.0f);
						finalVertexData.PushBack(0.0f);
					}

					for (uint32_t idx = 0; idx < uniqueIndices.Count(); idx++)
					{
						finalIndexData.PushBack(uniqueIndices[idx] + indexBufferBase);
					}

					// If we currently have a mesh being processed, do some extra work to prepare it for storage.
					// FIXME: This will not apply to the final mesh in the file!

					// Walk through all the position vectors to determine the extents of the geometry.
					// LEFTOFF: Calculate extents per submesh.
				}

				if (meshHeaders.Count() >= iDebugMeshCount)
				{
					bDebugStop = true;
					break;
				}

				meshHeaders.PushBack(CModel_MeshHeader_t{});
				//vertexPositions.Clear();
				//vertexNormals.Clear();
				//vertexTexCoords.Clear();
				meshFaces.Clear();

				break;
			}

			case 'g':
			{
				continue;
			}

			case 'f':	// Polygon 'face'.
			{
				uint32_t	positionIndices[3];
				uint32_t	normalIndices[3];
				uint32_t	texCoordIndices[3];

				int elements = sscanf_s(szCurrentLine, "f %d/%d/%d %d/%d/%d %d/%d/%d\n", &positionIndices[0], &texCoordIndices[0], &normalIndices[0], &positionIndices[1], &texCoordIndices[1], &normalIndices[1], &positionIndices[2], &texCoordIndices[2], &normalIndices[2]);

				CAssert(elements == 9);

				// Write the elements out to the mesh header.
				CModel_MeshFaceData_t newFace{};
				for (int iVert = 0; iVert < 3; iVert++)
				{
					// NOTE: OBJ files are 1-indexed so we need to bump down by one to store as a zero index.
					newFace.iPositionIndices[iVert] = positionIndices[iVert] - 1;
					newFace.iNormalIndices[iVert] = normalIndices[iVert] - 1;
					newFace.iTexCoordIndices[iVert] = texCoordIndices[iVert] - 1;
				}
				meshFaces.PushBack(newFace);
				break;
			}

			case 'u':
				// New Material.
				// Optimize, maybe this could be made faster with a hashmap??
				// This could become taxing on VERY large models.

				// Get just the material name.
				pszMaterialName = &szCurrentLine[7];
				bMatFound = false;

				// Check to see if we've seen this material yet.
				for (int iMat = 0; iMat < materialNames.Count(); iMat++)
				{
					if (Utl::StringCompare(pszMaterialName, materialNames[iMat]))
					{
						// This material already exists in the table!
						bMatFound = true;
						meshHeaders.Back().iMaterialIndex = iMat;
						break;
					}
				}

				// This material still needs to be added.
				if (!bMatFound)
				{
					materialNames.PushBack(Utl::StringDuplicate(pszMaterialName));
					meshHeaders.Back().iMaterialIndex = (uint32_t)materialNames.Count() - 1;
					break;
				}
				else
				{
					// Just break out.
					break;
				}

			case 's':	// Smoothing groups, not using these.
				continue;

			case 'v':
			{
				// Need to check the second char.
				switch (secondChar)
				{
					case ' ':	// Vertex position.
					{
						Vector newPosition{};
						int numElements = sscanf_s(&szCurrentLine[2], "%f %f %f", &newPosition.x, &newPosition.y, &newPosition.z);
						CAssert(numElements == 3);

						// Add it to the table.
						vertexPositions.PushBack(newPosition);
						break;
					}

					case 'n':	// Vertex normals.
					{
						Vector newNormal{};
						int numElements = sscanf_s(&szCurrentLine[2], "%f %f %f", &newNormal.x, &newNormal.y, &newNormal.z);
						CAssert(numElements == 3);

						// When we serialize these, we're gonna quantize with -1.0 and 1.0 being our bounds.
						CAssert(newNormal.x <= 1.0f && newNormal.x >= -1.0f);
						CAssert(newNormal.y <= 1.0f && newNormal.y >= -1.0f);
						CAssert(newNormal.z <= 1.0f && newNormal.z >= -1.0f);

						// Add it to the table.
						vertexNormals.PushBack(newNormal);
						break;
					}

					case 't':	// Vertex texture coordinates.
					{
						Vector2D newTexCoord{};
						int numElements = sscanf_s(&szCurrentLine[2], "%f %f", &newTexCoord.x, &newTexCoord.y);
						CAssert(numElements == 2);

						// When we serialize these, we're gonna quantize with -512.0 and 512.0 being our bounds. (We're being VERY generous with repeated textures).
						CAssert(newTexCoord.x <= 512.0f && newTexCoord.x >= -512.0f);
						CAssert(newTexCoord.y <= 512.0f && newTexCoord.y >= -512.0f);

						// Add it to the table.
						vertexTexCoords.PushBack(newTexCoord);
						break;
					}
				}
				break;
			}

			case 'm':
				// Append the appropriate extension to the path.
				// NOTE: This assumes that the mtllib file has the same name as the OBJ.
				char* pszMTLFilePath = Utl::StringDuplicate(pszOBJMeshPath);
				uint64_t iStringLength = Utl::StringLength(pszMTLFilePath);
				pszMTLFilePath[iStringLength - 3] = 'm';
				pszMTLFilePath[iStringLength - 2] = 't';
				pszMTLFilePath[iStringLength - 1] = 'l';
				pszMTLFilePath[iStringLength - 0] = '\0';
				if (!ImportOBJMaterialLibrary(pszMTLFilePath, pOutResource))
				{
					LOG_ERROR("Failed to import '%s' material library! Will use default material.", pszMTLFilePath);
				}
				break;
				
			/*default:
				//CAssert(0);
				break;*/
			}
		}

		// Serialize the final mesh (this won't be handled in the switch statement).
		{
			//Utl::CVector<Vertex3D>			uniqueVertices{};
			Utl::CVector<uint32_t>			uniqueIndices{};
			Utl::CVector<UniqueVertex_t>	finalVertexIndices{};
			DeduplicateGeometry(meshFaces, vertexPositions, vertexNormals, vertexTexCoords, finalVertexIndices, uniqueIndices);
			// FIXME: IMPLEMENT
			// GenerateTangentData(uniqueVertices);

			meshHeaders.Back().iVertexCount = (uint32_t)finalVertexIndices.Count();
			meshHeaders.Back().iVertexOffset = iUniqueVertCounter;
			meshHeaders.Back().iIndexCount = (uint32_t)uniqueIndices.Count();
			meshHeaders.Back().iIndexOffset = sizeof(uint32_t) * finalIndexData.Count();	// This is the byte offset into the index data.

			// Make sure the vertex/index offsets are 4-byte aligned!
			// UNDONE: Maybe we don't want to offset the vertex buffer?
			// If we simply offset the index buffer then we only have to re-bind the index buffer at those offsets and bind the vertex buffer once per mesh,
			// not once per drawcall!
			/*uint64_t vertexAlignmentOffset = 0;
			if (meshHeaders.Back().iVertexOffset % 4 != 0)
			{
				vertexAlignmentOffset = 4 - (meshHeaders.Back().iVertexOffset % 4);
				meshHeaders.Back().iVertexOffset += vertexAlignmentOffset;
			}

			for (int idx = 0; idx < vertexAlignmentOffset; idx++)
			{
				finalVertexData.PushBack((float)0.0f);
			}*/

			uint64_t indexAlignmentOffset = 0;
			if (meshHeaders.Back().iIndexOffset % 4 != 0)
			{
				indexAlignmentOffset = 4 - (meshHeaders.Back().iIndexOffset % 4);
				meshHeaders.Back().iIndexOffset += indexAlignmentOffset;
			}

			for (int idx = 0; idx < indexAlignmentOffset; idx++)
			{
				// Fill in some blank data to the index buffer to make sure we're aligned.
				finalIndexData.PushBack((uint32_t)0);
			}

			uint32_t indexBufferBase = (uint32_t)iUniqueVertCounter;
			iUniqueVertCounter += finalVertexIndices.Count();

			// Write out the unique data to the output tables.
			for (uint32_t iVert = 0; iVert < finalVertexIndices.Count(); iVert++)
			{
				finalVertexData.PushBack(vertexPositions[finalVertexIndices[iVert].iPositionIdx].x);
				finalVertexData.PushBack(vertexPositions[finalVertexIndices[iVert].iPositionIdx].y);
				finalVertexData.PushBack(vertexPositions[finalVertexIndices[iVert].iPositionIdx].z);

				finalVertexData.PushBack(vertexNormals[finalVertexIndices[iVert].iNormalIdx].x);
				finalVertexData.PushBack(vertexNormals[finalVertexIndices[iVert].iNormalIdx].y);
				finalVertexData.PushBack(vertexNormals[finalVertexIndices[iVert].iNormalIdx].z);

				finalVertexData.PushBack(vertexTexCoords[finalVertexIndices[iVert].iTexCoordIdx].x);
				finalVertexData.PushBack(vertexTexCoords[finalVertexIndices[iVert].iTexCoordIdx].y);

				// FIXME: WRITE TANGENTS
				finalVertexData.PushBack(0.0f);
				finalVertexData.PushBack(0.0f);
				finalVertexData.PushBack(0.0f);
			}

			for (uint32_t idx = 0; idx < uniqueIndices.Count(); idx++)
			{
				finalIndexData.PushBack(uniqueIndices[idx] + indexBufferBase);
			}

			// If we currently have a mesh being processed, do some extra work to prepare it for storage.
			// FIXME: This will not apply to the final mesh in the file!

			// Walk through all the position vectors to determine the extents of the geometry.
			// LEFTOFF: Calculate extents per submesh.
		}


		LOG_INFO("Compressing mesh data.");

		for (uint64_t iMesh = 0; iMesh < meshHeaders.Count(); iMesh++)
		{
			LOG_DEBUG("Geometry deduplication progress: %d/%d", iMesh + 1, meshHeaders.Count());
			//DeduplicateGeometry(meshHeaders[iMesh], meshFaces[iMesh], vertexPositions, vertexNormals, vertexTexCoords, finalMeshFaces, finalVertexPositions, finalVertexNormals, finalVertexTexCoords);
		}
		if (!bModelContainsVertexTangents)
		{
			// FIXME: Where to put this?
			outFileHeader.iVertexLayoutFlags |= VERTEX_LAYOUT_DATA_TANGENT;
		}

		// Walk through all the position vectors to determine the extents of the geometry.
		for (int iPos = 0; iPos < finalVertexPositions.Count(); iPos++)
		{
			Vector& pPos = finalVertexPositions[iPos];
			if (pPos.x < outFileHeader.vecMinExtents.x)
				outFileHeader.vecMinExtents.x = pPos.x;
			if (pPos.y < outFileHeader.vecMinExtents.y)
				outFileHeader.vecMinExtents.y = pPos.y;
			if (pPos.z < outFileHeader.vecMinExtents.z)
				outFileHeader.vecMinExtents.z = pPos.z;

			if (pPos.x > outFileHeader.vecMaxExtents.x)
				outFileHeader.vecMaxExtents.x = pPos.x;
			if (pPos.y > outFileHeader.vecMaxExtents.y)
				outFileHeader.vecMaxExtents.y = pPos.y;
			if (pPos.z > outFileHeader.vecMaxExtents.z)
				outFileHeader.vecMaxExtents.z = pPos.z;
		}

		// Pack vectors into uints.
		Utl::CVector<uint16_t>	packedPositions{};
		Utl::CVector<uint16_t>	packedNormals{};
		Utl::CVector<uint16_t>	packedTexCoords{};
		Utl::CVector<uint16_t>	packedTangents{};

		// Positions.
		{
			for (int iPos = 0; iPos < finalVertexPositions.Count(); iPos++)
			{
				float flNormalizedX = (finalVertexPositions[iPos].x - outFileHeader.vecMinExtents.x) / (outFileHeader.vecMaxExtents.x - outFileHeader.vecMinExtents.x);
				float flNormalizedY = (finalVertexPositions[iPos].y - outFileHeader.vecMinExtents.y) / (outFileHeader.vecMaxExtents.y - outFileHeader.vecMinExtents.y);
				float flNormalizedZ = (finalVertexPositions[iPos].z - outFileHeader.vecMinExtents.z) / (outFileHeader.vecMaxExtents.z - outFileHeader.vecMinExtents.z);

				uint16_t packedPosX = PackF32ToUint16(flNormalizedX);
				uint16_t packedPosY = PackF32ToUint16(flNormalizedY);
				uint16_t packedPosZ = PackF32ToUint16(flNormalizedZ);
				packedPositions.PushBack(packedPosX);
				packedPositions.PushBack(packedPosY);
				packedPositions.PushBack(packedPosZ);
			}
		}

		// Normals.
		{
			for (int iNormal = 0; iNormal < finalVertexNormals.Count(); iNormal++)
			{
				float flNormalizedNormalX = (finalVertexNormals[iNormal].x + 1.0f) / (2.0f);
				float flNormalizedNormalY = (finalVertexNormals[iNormal].y + 1.0f) / (2.0f);
				float flNormalizedNormalZ = (finalVertexNormals[iNormal].z + 1.0f) / (2.0f);

				uint16_t packedNormalX = PackF32ToUint16(flNormalizedNormalX);
				uint16_t packedNormalY = PackF32ToUint16(flNormalizedNormalY);
				uint16_t packedNormalZ = PackF32ToUint16(flNormalizedNormalZ);
				packedNormals.PushBack(packedNormalX);
				packedNormals.PushBack(packedNormalY);
				packedNormals.PushBack(packedNormalZ);
			}
		}

		// Texcoords.
		{
			for (int iTexCoord = 0; iTexCoord < finalVertexTexCoords.Count(); iTexCoord++)
			{
				const float flPackedTexCoordMins = -64.0f;
				const float flPackedTexCoordMaxs = 64.0f;

				float flNormalizedTexCoordX = (finalVertexTexCoords[iTexCoord].x + flPackedTexCoordMins) / (flPackedTexCoordMaxs - flPackedTexCoordMins);
				float flNormalizedTexCoordY = (finalVertexTexCoords[iTexCoord].y + flPackedTexCoordMins) / (flPackedTexCoordMaxs - flPackedTexCoordMins);

				flNormalizedTexCoordX = (flNormalizedTexCoordX + 1.0f) / 2.0f;
				flNormalizedTexCoordY = (flNormalizedTexCoordY + 1.0f) / 2.0f;

				uint16_t packedTexCoordX = PackF32ToUint16(flNormalizedTexCoordX);
				uint16_t packedTexCoordY = PackF32ToUint16(flNormalizedTexCoordY);
				packedTexCoords.PushBack(packedTexCoordX);
				packedTexCoords.PushBack(packedTexCoordY);
			}
		}

		// Tangents.
		{
			for (int iTangent = 0; iTangent < finalVertexTangents.Count(); iTangent++)
			{
				// Tangents are already normalized. Just bring from [-1, 1] to [0, 1].
				float flNormalizedTangentX = (finalVertexTangents[iTangent].x + 1.0f) / 2.0f;
				float flNormalizedTangentY = (finalVertexTangents[iTangent].y + 1.0f) / 2.0f;
				float flNormalizedTangentZ = (finalVertexTangents[iTangent].z + 1.0f) / 2.0f;

				CAssert(flNormalizedTangentX >= 0.0f && flNormalizedTangentX <= 1.0f);
				CAssert(flNormalizedTangentY >= 0.0f && flNormalizedTangentY <= 1.0f);
				CAssert(flNormalizedTangentZ >= 0.0f && flNormalizedTangentZ <= 1.0f);

				uint16_t packedTangentX = PackF32ToUint16(flNormalizedTangentX);
				uint16_t packedTangentY = PackF32ToUint16(flNormalizedTangentY);
				uint16_t packedTangentZ = PackF32ToUint16(flNormalizedTangentZ);
				packedTangents.PushBack(packedTangentX);
				packedTangents.PushBack(packedTangentY);
				packedTangents.PushBack(packedTangentZ);
			}
		}

		// Setup offsets/counts.
		outFileHeader.iMeshDataOffset = sizeof(uint64_t) + sizeof(CModel_FileHeader_t);	// Accounting for the magic number.
		outFileHeader.iMeshDataCount = (uint32_t)meshHeaders.Count();
		outFileHeader.iMeshDataStride = (uint8_t)meshHeaders.Stride();
		outFileHeader.iPositionCount = (uint32_t)finalVertexData.Count();

		outFileHeader.iFaceDataOffset = outFileHeader.iMeshDataOffset + (meshHeaders.Stride() * meshHeaders.Count());
		for (int iMesh = 0; iMesh < meshHeaders.Count(); iMesh++)
		{
			outFileHeader.iFaceDataOffset += sizeof(uint32_t) * meshHeaders[iMesh].iNumFaces;
		}

		// Place the resource's magic number first.
		FileSystem::WriteBytes(&kModelResourceMagicNumber, sizeof(uint32_t), &hNewOutputMesh);
		iTotalBytesWritten += sizeof(uint32_t);

		// Write out the file header.
		FileSystem::WriteBytes(&outFileHeader, sizeof(CModel_FileHeader_t), &hNewOutputMesh);
		iTotalBytesWritten += sizeof(CModel_FileHeader_t);

		// FIXME: Header for submeshes?
		for (int iMesh = 0; iMesh < meshHeaders.Count(); iMesh++)
		{
			FileSystem::WriteBytes(&meshHeaders[iMesh], sizeof(CModel_MeshHeader_t), &hNewOutputMesh);
			iTotalBytesWritten += sizeof(CModel_MeshHeader_t);
		}

		// FIXME: Pack data into uints.
		FileSystem::WriteBytes(finalVertexData.Data(), finalVertexData.Stride() * finalVertexData.Count(), &hNewOutputMesh);
		iTotalBytesWritten += finalVertexData.Stride() * finalVertexData.Count();

		FileSystem::WriteBytes(finalIndexData.Data(), finalIndexData.Stride() * finalIndexData.Count(), &hNewOutputMesh);
		iTotalBytesWritten += finalIndexData.Stride() * finalIndexData.Count();

		// Clean up arrays.
		for (int iMesh = 0; iMesh < meshHeaders.Count(); iMesh++)
		{
			if (meshHeaders[iMesh].pFaceIndices != nullptr)
			{
				delete[] meshHeaders[iMesh].pFaceIndices;
				meshHeaders[iMesh].pFaceIndices = nullptr;
			}

			if (meshHeaders[iMesh].pIndexData != nullptr)
			{
				delete[] meshHeaders[iMesh].pIndexData;
				meshHeaders[iMesh].pIndexData = nullptr;
			}
		}

		// Close the new file.
		FileSystem::CloseFile(&hNewOutputMesh);

		// Close the original source file.
		FileSystem::CloseFile(&hOBJFile);

		float flPercentReduced = ((float)iTotalBytesWritten / iOriginalByteSize);
		flPercentReduced *= 100.0f;
		LOG_INFO("Mesh '%s' compressed from %d bytes to %d bytes ->(%.2f%%)", pszOBJMeshPath, iOriginalByteSize, iTotalBytesWritten, flPercentReduced);

		return MeshLoader::LoadMeshResource((char*)pszOutMeshFileName, pOutResource);
	}

	static void PrepareIndexBuffer(CModel_MeshHeader_t& pCurMesh)
	{
		pCurMesh.pIndexData = new uint32_t[pCurMesh.iIndexCount];

		// NOTE: We are assigning an index for each vert. These will get reassigned when we deduplicate geometry.
		for (uint32_t iFace = 0; iFace < pCurMesh.iIndexCount / 3; iFace++)
		{
			for (uint8_t iVert = 0; iVert < 3; iVert++)
			{
				pCurMesh.pIndexData[(iFace * 3) + iVert] = (iFace * 3) + iVert;
			}
		}
	}

	static void DeduplicateGeometry(Utl::CVector<CModel_MeshFaceData_t>& pFaces, Utl::CVector<Vector>& pInPositions, Utl::CVector<Vector>& pInNormals, Utl::CVector<Vector2D>& pInTexCoords, Utl::CVector<UniqueVertex_t>& pOutVertices, Utl::CVector<uint32_t>& pOutIndices)
	{
		Utl::CVector<UniqueVertex_t> uniqueVertices{};
		uint32_t iPosIdxBase = pFaces[0].iPositionIndices[0];
		uint32_t iNormIdxBase = pFaces[0].iNormalIndices[0];
		uint32_t iTexCoordIdxBase = pFaces[0].iTexCoordIndices[0];

		for (int iFace = 0; iFace < pFaces.Count(); iFace++)
		{
			for (int iVert = 0; iVert < 3; iVert++)
			{
				bool bUnique = true;

				UniqueVertex_t currentVert{};
				currentVert.iPositionIdx = pFaces[iFace].iPositionIndices[iVert];
				currentVert.iNormalIdx = pFaces[iFace].iNormalIndices[iVert];
				currentVert.iTexCoordIdx = pFaces[iFace].iTexCoordIndices[iVert];

				{
					if (currentVert.iPositionIdx < iPosIdxBase)
						iPosIdxBase = currentVert.iPositionIdx;
					if (currentVert.iNormalIdx < iNormIdxBase)
						iNormIdxBase = currentVert.iNormalIdx;
					if (currentVert.iTexCoordIdx < iTexCoordIdxBase)
						iTexCoordIdxBase = currentVert.iTexCoordIdx;
				}

				int iUniqueVert = 0;
				for (iUniqueVert = 0; iUniqueVert < uniqueVertices.Count(); iUniqueVert++)
				{
					if (uniqueVertices[iUniqueVert].iPositionIdx == currentVert.iPositionIdx &&
						uniqueVertices[iUniqueVert].iNormalIdx == currentVert.iNormalIdx &&
						uniqueVertices[iUniqueVert].iTexCoordIdx == currentVert.iTexCoordIdx)
					{
						// Duplicate vert found!
						bUnique = false;
						break;
					}
				}

				if (bUnique)
				{
					// Add it to the table.
					uniqueVertices.PushBack(currentVert);
					pOutIndices.PushBack(iUniqueVert);
					//pCurMesh.pIndexData[(iFace * 3) + iVert] = (uint32_t)uniqueVertices.Count() - 1;
				}
				else
				{
					// Not unique, need to reassign face indices.
					pOutIndices.PushBack(iUniqueVert);
					//pCurMesh.pIndexData[(iFace * 3) + iVert] = iUniqueVert;
				}
			}
		}

		// At this point, the index buffer is properly setup, now just write out the unique vertices.
		for (int iVert = 0; iVert < uniqueVertices.Count(); iVert++)
		{
			pOutVertices.PushBack(uniqueVertices[iVert]);
		}
	}

	static void DeduplicateGeometry(CModel_MeshHeader_t& pCurMesh, Utl::CVector<CModel_MeshFaceData_t>& pInFaces, Utl::CVector<Vector>& pInPositions, Utl::CVector<Vector>& pInNormals, Utl::CVector<Vector2D>& pInTexCoords,
									Utl::CVector<CModel_MeshFaceData_t>& pOutFaces, Utl::CVector<Vector>& pOutPositions, Utl::CVector<Vector>& pOutNormals, Utl::CVector<Vector2D>& pOutTexCoords)
	{
		// FIXME: Is pOutFaces necessary???

		// Identifies unique vertex data
		struct UniqueVertex_t
		{
			uint32_t iIndex{};
			uint32_t iPositionIdx{};
			uint32_t iNormalIdx{};
			uint32_t iTexCoordIdx{};
		};
		Utl::CVector<UniqueVertex_t> uniqueVertices{};

		CModel_MeshFaceData_t*	pCurFace = nullptr;
		bool					bUniqueVert = true;
		for (uint32_t iFace = 0; iFace < pCurMesh.iNumFaces; iFace++)
		{
			pCurFace = &pInFaces[pCurMesh.pFaceIndices[iFace]];
			for (uint32_t iOrigVert = 0; iOrigVert < 3; iOrigVert++)
			{
				bUniqueVert = true; 
				uint32_t iUniqueVerts = 0;
				for (iUniqueVerts = 0; iUniqueVerts < uniqueVertices.Count() && bUniqueVert; iUniqueVerts++)
				{
					if (pCurFace->iPositionIndices[iOrigVert] == uniqueVertices[iUniqueVerts].iPositionIdx &&
						pCurFace->iNormalIndices[iOrigVert] == uniqueVertices[iUniqueVerts].iNormalIdx &&
						pCurFace->iTexCoordIndices[iOrigVert] == uniqueVertices[iUniqueVerts].iTexCoordIdx)
					{
						bUniqueVert = false;
						break;
					}
				}

				if (!bUniqueVert)
				{
					// Reassign indices.
					pCurMesh.pIndexData[(iFace * 3) + iOrigVert] = uniqueVertices[iUniqueVerts].iIndex;
				}
				else
				{
					// Copy over the unique indices.
					UniqueVertex_t uniqueVert{};
					uniqueVert.iPositionIdx = pCurFace->iPositionIndices[iOrigVert];
					uniqueVert.iNormalIdx = pCurFace->iNormalIndices[iOrigVert];
					uniqueVert.iTexCoordIdx = pCurFace->iTexCoordIndices[iOrigVert];
					uniqueVert.iIndex = (uint32_t)uniqueVertices.Count();
					pCurMesh.pIndexData[(iFace * 3) + iOrigVert] = uniqueVert.iIndex;
					uniqueVertices.PushBack(uniqueVert);
				}
			}
		}

		// At this point, all duplicate vertex data has had their indices reassigned. Write out only the unique data.
		for (uint64_t iVerts = 0; iVerts < uniqueVertices.Count(); iVerts++)
		{
			UniqueVertex_t& pCurVert = uniqueVertices[iVerts];

			pOutPositions.PushBack(pInPositions[pCurVert.iPositionIdx]);
			pOutNormals.PushBack(pInNormals[pCurVert.iNormalIdx]);
			pOutTexCoords.PushBack(pInTexCoords[pCurVert.iTexCoordIdx]);
		}
	}

	static void GenerateTangentData(CModel_MeshHeader_t& pCurMesh, Utl::CVector<Vector>& pPositions, Utl::CVector<Vector>& pNormals, Utl::CVector<Vector2D>& pTexCoords, Utl::CVector<Vector>& pOutTangents, Utl::CVector<CModel_MeshFaceData_t>& pFaces)
	{
		uint64_t iFaceCount = pFaces.Count();
		uint64_t invalidTangentCount{ 0 };
		for (int iFace = 0; iFace < iFaceCount; iFace++)
		{
			uint32_t iVert0 = pCurMesh.pIndexData[(iFace * 3) + 0];
			uint32_t iVert1 = pCurMesh.pIndexData[(iFace * 3) + 1];
			uint32_t iVert2 = pCurMesh.pIndexData[(iFace * 3) + 2];

			Vector& pPos0 = pPositions[iVert0];
			Vector& pPos1 = pPositions[iVert1];
			Vector& pPos2 = pPositions[iVert2];

			Vector2D& pUV0 = pTexCoords[iVert0];
			Vector2D& pUV1 = pTexCoords[iVert1];
			Vector2D& pUV2 = pTexCoords[iVert2];

			Vector edge1 = pPos1 - pPos0;
			Vector edge2 = pPos2 - pPos0;

			Vector2D deltaUV1 = pUV1 - pUV0;
			Vector2D deltaUV2 = pUV2 - pUV0;

			float flDividend = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;
			float f = 1.0f / flDividend;
			CAssert(!(pPos0 == pPos1));
			CAssert(!(pPos1 == pPos2));
			CAssert(!(pPos0 == pPos2));

			if (M_ABS(flDividend) < FLT_EPSILON)
			{
				// Don't average this value with the existing tangent data if it's invalid (It wil just fubar results further.)
				invalidTangentCount++;
				continue;
			}

			Vector vecTangent{};
			vecTangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			vecTangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			vecTangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

			vecTangent.NormalizeInPlace();

			pOutTangents.PushBack(vecTangent);

			// Assign this new tangent index to the 3 verts we're processing.
			for (int iVert = 0; iVert < 3; iVert++)
			{
				pFaces[iFace].iTangentIndices[iVert] = (uint32_t)(pOutTangents.Count() - 1);
			}
		}

		// Report invalid tangents.
		if (invalidTangentCount > 0)
		{
			LOG_WARN("Mesh contains %d invalid tangents. Lighting will be broken. Please verify that model sources are properly setup.", invalidTangentCount);
		}
	}

	static void GenerateTangentData(Utl::CVector<Vertex3D>& pVertices)
	{
		// FIXME: IMPLEMENT!
	}

	static bool ImportOBJMaterialLibrary(const char* pszMaterialLibraryName, MeshLoader::MeshResource_t& pMeshResource)
	{
		CASSERTMSG(FileSystem::FileExists(pszMaterialLibraryName), "Missing OBJ Material File?");
		FileHandle_t hMtlFile{};

		if (!FileSystem::OpenFile(pszMaterialLibraryName, eFileMode::FILEMODE_READ, false, &hMtlFile))
		{
			LOG_ERROR("Failed to open OBJ MTL LIB file: '%s'.", pszMaterialLibraryName);
			return false;
		}
		
		// Read through the file.
		char	szCurrentLine[2048];
		size_t	iBytesRead{ 0 };
		size_t	iCurrentLineLength{ 0 };
		char	szKey[512];
		char	szValue[512];

		Utl::CVector<MaterialLoader::MaterialResource_t> processedMaterialEntries{};

		while (FileSystem::ReadLine(szCurrentLine, 2048, &iBytesRead, &hMtlFile))
		{
			iCurrentLineLength = Utl::StringLength(szCurrentLine);
			// Skip blank lines and comments.
			if (iCurrentLineLength <= 0 || szCurrentLine[0] == '#')
				continue;

			size_t iSpaceIndex = Utl::StringIndexOf(szCurrentLine, ' ');
			CAssert(iSpaceIndex != kInvalidID_U32);

			// Split into key/value pairs.
			Platform::MemoryCopy(szKey, szCurrentLine, iSpaceIndex);
			szKey[iSpaceIndex] = '\0';
			Platform::MemoryCopy(szValue, &szCurrentLine[iSpaceIndex + 1], iBytesRead - iSpaceIndex);
			szValue[iBytesRead - 1] = '\0';

			// FIXME: We should be able to do this in place.
			char* pszTrimmedKey = Utl::StringTrim(szKey);

			if (Utl::StringCompare(pszTrimmedKey, "newmtl"))	// New material declaration.
			{
				processedMaterialEntries.PushBack(MaterialLoader::MaterialResource_t{});
				processedMaterialEntries.Back().pszMaterialName = Utl::StringDuplicate(szValue);
				processedMaterialEntries.Back().pszShaderName = Utl::StringDuplicate("Core.Phong");		// Default hardcoded shader.
			}
			else if (Utl::StringCompare(pszTrimmedKey, "map_Kd"))	// Diffuse map.
			{
				MaterialLoader::MaterialResource_t::textureMapConfig_t newMap{};
				newMap.pszTextureName = Utl::StringDuplicate(szValue);
				newMap.eType = MaterialLoader::MaterialResource_t::eTextureType::DIFFUSE;
				processedMaterialEntries.Back().textureMapConfigs.PushBack(newMap);
			}
			else if (Utl::StringCompare(pszTrimmedKey, "map_Ks"))	// Specular map.
			{
				MaterialLoader::MaterialResource_t::textureMapConfig_t newMap{};
				newMap.pszTextureName = Utl::StringDuplicate(szValue);
				newMap.eType = MaterialLoader::MaterialResource_t::eTextureType::SPECULAR;
				processedMaterialEntries.Back().textureMapConfigs.PushBack(newMap);
			}
			else if (Utl::StringCompare(pszTrimmedKey, "map_bump"))	// Normal map.
			{
				MaterialLoader::MaterialResource_t::textureMapConfig_t newMap{};
				newMap.pszTextureName = Utl::StringDuplicate(szValue);
				newMap.eType = MaterialLoader::MaterialResource_t::eTextureType::NORMAL;
				processedMaterialEntries.Back().textureMapConfigs.PushBack(newMap);
			}
			else if (Utl::StringCompare(pszTrimmedKey, "Ns"))	// Specular exponent.
			{
				processedMaterialEntries.Back().pszCustomKeys.PushBack("specular_shininess");
				processedMaterialEntries.Back().pszCustomValues.PushBack((char*)0);
				processedMaterialEntries.Back().pszCustomValues.Back() = Utl::StringDuplicate(szValue);
			}

			Utl::StringFree(pszTrimmedKey);
			pszTrimmedKey = nullptr;
		}

		FileSystem::CloseFile(&hMtlFile);

		// Write out all of these materials.
		for (int iMat = 0; iMat < processedMaterialEntries.Count(); iMat++)
		{
			LOG_INFO("Saving material '%s'.", processedMaterialEntries[iMat].pszMaterialName);
			if (!MaterialSerializer::SerializeMaterialResources(processedMaterialEntries[iMat].pszMaterialName, processedMaterialEntries[iMat]))
			{
				LOG_ERROR("Failed to serialize material resource '%s'...", processedMaterialEntries[iMat].pszMaterialName);
			}
		}

		return true;
	}

}	// namespace MeshSerializer
