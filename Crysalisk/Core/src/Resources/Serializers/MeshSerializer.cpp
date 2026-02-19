//=============================================================================
// Author: Jared Simons
// Date Created: 6/12/2025
// 
// Purpose: Mesh file serialization utilities.
//=============================================================================
#include "MeshSerializer.h"

#include "Platform/FileSystem.h"

#include <stdio.h>	// for sscanf()
#include <float.h>

namespace MeshSerializer
{
	// Specific file extension for the Crysalisk model file format (CModels).
	static const char*		kCModelFileExtension = ".cmodel_c";
	// The current most up-to-date version of the CModel file format.
	static const uint32_t	kCurrentModelVersion = 2;

	// Forward declarations
	static void GenerateTangentData(Utl::CVector<Vector>& pPositions, Utl::CVector<Vector>& pNormals, Utl::CVector<Vector2D>& pTexCoords, Utl::CVector<Vector>& pOutTangents, Utl::CVector<CModel_MeshFaceData_t>& pFaces);
	static void DeduplicateGeometry(Utl::CVector<Vector>& pPositions, Utl::CVector<Vector>& pNormals, Utl::CVector<Vector2D>& pTexCoords, Utl::CVector<Vector>& pTangets, Utl::CVector<CModel_MeshFaceData_t>& pFaces);
	static uint16_t PackF32ToUint16(const float flUnitFloat);
	static float UnpackF32FromUint16(const uint16_t inputData);

	//-------------------------------------------------------------------------
	// Purpose: Takes an existing mesh resource imported from another file type, 
	// and writes it out to disk in our own binary format.
	// 
	// Input:	pszModelName - The name of the model.
	//			pInResource - The resource data to be serialized.
	// Output:	bool - Returns true on successful serialization, otherwise false.
	//-------------------------------------------------------------------------
	bool SerializeMeshResource(const char* pszModelName, MeshLoader::MeshResource_t& pInResource)
	{
		// TODO: REMOVE THIS!!

		return true;
	}

	//-------------------------------------------------------------------------
	// Purpose: TODO: Documentation.
	// 
	// Mesh serialization algorithm:
	// - Verify that source file exists						[X]
	// - Determine the mesh's vertex layout					[X]
	// - Process all elements into their own tables			[X]
	// - Write out the file header							[X]
	//		(Note that this has to happen after processing 
	//		elements so that we can get their offsets 
	//		into the byte-stream).
	// - Write out all tables								[X]
	//-------------------------------------------------------------------------
	bool SerializeOBJMesh(const char* pszOBJMeshPath, const char* pszOutMeshFileName)
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

		while (FileSystem::ReadLine(szCurrentLine, sizeof(char) * 2048, &bytesRead, &hOBJFile))
		{
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

		// Geometry data.
		Utl::CVector<CModel_MeshHeader_t>	meshHeaders{};
		Utl::CVector<CModel_MeshFaceData_t>	meshFaces{};

		// Build out geometry tables.
		Utl::CVector<Vector>	vertexPositions{};
		Utl::CVector<Vector>	vertexNormals{};
		Utl::CVector<Vector2D>	vertexTexCoords{};
		Utl::CVector<Vector>	vertexTangents{};

		Platform::MemorySet(szCurrentLine, 0, sizeof(char) * 2048);
		bytesRead = 0;

		while (FileSystem::ReadLine(szCurrentLine, sizeof(char) * 2048, &bytesRead, &hOBJFile))
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
				CModel_MeshHeader_t newMesh{};
				meshHeaders.PushBack(newMesh);
				meshHeaders.Back().pFaceIndices = new uint16_t[65535];	// Just allocate a bunch of these for now, we'll only write the ones that are used.
				meshHeaders.Back().iNumFaces = 0;
				break;
			}

			case 'g':
			{
				// Skip new groups, we'll just put all the geometry in the same mesh.
				// TODO: How to handle meshes that use JUST 'g' instead of 'o' or 'o' and 'g'?
				if (meshHeaders.Count() < 1)
				{
					meshHeaders.PushBack(CModel_MeshHeader_t{});
					meshHeaders.Back().pFaceIndices = new uint16_t[65535];	// Just allocate a bunch of these for now, we'll only write the ones that are used.
					meshHeaders.Back().iNumFaces = 0;
				}
				continue;
			}

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
					newFace.iPositionIndices[iVert] = positionIndices[iVert];
					newFace.iNormalIndices[iVert] = normalIndices[iVert];
					newFace.iTexCoordIndices[iVert] = texCoordIndices[iVert];
				}
				
				uint16_t iFaceIndex = (uint16_t)meshFaces.PushBack(newFace);
				
				// Currently, we store face indices as uint16_t. If a single mesh contains more than than 65535 values, then we need to re-evaluate.
				if (iFaceIndex > (1U << 16) - 1)
				{
					LOG_WARN("Mesh '%s' contains more than 65,535 faces in a single submesh. This mesh cannot be properly serialized.", pszOBJMeshPath);
					return false;
				}
				meshHeaders.Back().pFaceIndices[meshHeaders.Back().iNumFaces] = iFaceIndex;
				meshHeaders.Back().iNumFaces++;
				break;
			}

			case 'u':
				// New Material.
				// TODO: Implement!
				break;

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

						// Add it to the table.
						vertexNormals.PushBack(newNormal);
						break;
					}

					case 't':	// Vertex texture coordinates.
					{
						Vector2D newTexCoord{};
						int numElements = sscanf_s(&szCurrentLine[2], "%f %f", &newTexCoord.x, &newTexCoord.y);
						CAssert(numElements == 2);

						// Add it to the table.
						vertexTexCoords.PushBack(newTexCoord);
						break;
					}
				}
				break;
			}

			case 'm':
				LOG_WARN("SerializeOBJMesh() -> IMPLEMENT MATERIAL LIBRARY PROCESSING!");
				break;
				
			default:
				//CAssert(0);
				break;
			}
		}

		// Before writing out geometry to the file, generate tangent data and deduplicate extra data.
		if (!bModelContainsVertexTangents)	// TODO: Make generating tangets optional.
		{
			LOG_INFO("Generating tangent data for mesh.");
			GenerateTangentData(vertexPositions, vertexNormals, vertexTexCoords, vertexTangents, meshFaces);
			outFileHeader.iVertexLayoutFlags |= VERTEX_LAYOUT_DATA_TANGENT;
		}

		LOG_INFO("Compressing mesh data.");
		// TODO: This takes FOREVER on some not too large models (like sponza)
		//DeduplicateGeometry(vertexPositions, vertexNormals, vertexTexCoords, vertexTangents, meshFaces);

		// Walk through all the position vectors to determine the extents of the geometry.
		for (int iPos = 0; iPos < vertexPositions.Count(); iPos++)
		{
			Vector& pPos = vertexPositions[iPos];
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
		// TODO: Make this optional.
		Utl::CVector<uint16_t>	packedPositions{};
		Utl::CVector<uint16_t>	packedNormals{};
		Utl::CVector<uint16_t>	packedTexCoords{};
		Utl::CVector<uint16_t>	packedTangents{};

		// Positions.
		{
			for (int iPos = 0; iPos < vertexPositions.Count(); iPos += 3)
			{
				float flNormalizedX = (vertexPositions[iPos].x - outFileHeader.vecMinExtents.x) / (outFileHeader.vecMaxExtents.x - outFileHeader.vecMinExtents.x);
				float flNormalizedY = (vertexPositions[iPos].y - outFileHeader.vecMinExtents.y) / (outFileHeader.vecMaxExtents.y - outFileHeader.vecMinExtents.y);
				float flNormalizedZ = (vertexPositions[iPos].z - outFileHeader.vecMinExtents.z) / (outFileHeader.vecMaxExtents.z - outFileHeader.vecMinExtents.z);

				uint16_t packedPosX = PackF32ToUint16(flNormalizedX);
				uint16_t packedPosY = PackF32ToUint16(flNormalizedY);
				uint16_t packedPosZ = PackF32ToUint16(flNormalizedZ);
				packedPositions.PushBack(packedPosX);
				packedPositions.PushBack(packedPosY);
				packedPositions.PushBack(packedPosZ);

				// Test to demonstrate how to unpack floats stored in uint16s. Remove once model deserialization is working!
				/* {
					float flUnpackedUnitFloat = UnpackF32FromUint16(packedPosX);
					float flReference = vertexPositions[iPos].x;
					float flScaledFloat = (flUnpackedUnitFloat) * (outFileHeader.vecMaxExtents.x - outFileHeader.vecMinExtents.x) + outFileHeader.vecMinExtents.x;

					LOG_WARN("BREAKPOINT!!!");
				}*/
			}
		}

		// Normals.
		{
			for (int iNormal = 0; iNormal < vertexNormals.Count(); iNormal += 3)
			{
				float flNormalX = vertexNormals[iNormal].x;
				float flNormalY = vertexNormals[iNormal].y;
				float flNormalZ = vertexNormals[iNormal].z;

				uint16_t packedNormalX = PackF32ToUint16(flNormalX);
				uint16_t packedNormalY = PackF32ToUint16(flNormalY);
				uint16_t packedNormalZ = PackF32ToUint16(flNormalZ);
				packedNormals.PushBack(packedNormalX);
				packedNormals.PushBack(packedNormalY);
				packedNormals.PushBack(packedNormalZ);
			}
		}

		// Texcoords.
		{
			for (int iTexCoord = 0; iTexCoord < vertexTexCoords.Count(); iTexCoord += 2)
			{
				float flTexCoordX = vertexTexCoords[iTexCoord].x;
				float flTexCoordY = vertexTexCoords[iTexCoord].y;

				uint16_t packedNormalX = PackF32ToUint16(flTexCoordX);
				uint16_t packedNormalY = PackF32ToUint16(flTexCoordY);
				packedTexCoords.PushBack(packedNormalX);
				packedTexCoords.PushBack(packedNormalY);
			}
		}

		// Tangents.
		{
			for (int iTangent = 0; iTangent < vertexTangents.Count(); iTangent += 3)
			{
				float flTangentX = vertexTangents[iTangent].x;
				float flTangentY = vertexTangents[iTangent].y;
				float flTangentZ = vertexTangents[iTangent].z;

				uint16_t packedTangentX = PackF32ToUint16(flTangentX);
				uint16_t packedTangentY = PackF32ToUint16(flTangentY);
				uint16_t packedTangentZ = PackF32ToUint16(flTangentZ);
				packedTangents.PushBack(packedTangentX);
				packedTangents.PushBack(packedTangentY);
				packedTangents.PushBack(packedTangentZ);
			}
		}

		// Setup offsets/counts.
		outFileHeader.iMeshDataOffset = sizeof(uint64_t) + sizeof(CModel_FileHeader_t);	// Accounting for the magic number.
		outFileHeader.iMeshDataCount = (uint32_t)meshHeaders.Count();
		outFileHeader.iMeshDataStride = (uint8_t)meshHeaders.Stride();

		outFileHeader.iFaceDataOffset = outFileHeader.iMeshDataOffset + (meshHeaders.Stride() * meshHeaders.Count());
		for (int iMesh = 0; iMesh < meshHeaders.Count(); iMesh++)
		{
			outFileHeader.iFaceDataOffset += sizeof(uint16_t) * meshHeaders[iMesh].iNumFaces;
		}
		outFileHeader.iFaceDataCount = (uint32_t)meshFaces.Count();
		outFileHeader.iFaceDataStride = (uint8_t)meshFaces.Stride();

		outFileHeader.iPositionTableOffset = outFileHeader.iFaceDataOffset + (meshFaces.Count() * meshFaces.Stride());
		outFileHeader.iPositionCount = (uint32_t)packedPositions.Count();
		outFileHeader.iPositionStride = (uint8_t)packedPositions.Stride();

		outFileHeader.iNormalTableOffset = outFileHeader.iPositionTableOffset + (outFileHeader.iPositionStride * outFileHeader.iPositionCount);
		outFileHeader.iNormalCount = (uint32_t)packedNormals.Count();
		outFileHeader.iNormalStride = (uint8_t)packedNormals.Stride();

		outFileHeader.iTexCoordTableOffset = outFileHeader.iNormalTableOffset + (outFileHeader.iNormalStride * outFileHeader.iNormalCount);
		outFileHeader.iTexCoordCount = (uint32_t)packedTexCoords.Count();
		outFileHeader.iTexCoordStride = (uint8_t)packedTexCoords.Stride();

		outFileHeader.iTangentTableOffset = outFileHeader.iTexCoordTableOffset + (outFileHeader.iTexCoordStride * outFileHeader.iTexCoordCount);
		outFileHeader.iTangentCount = (uint32_t)packedTangents.Count();
		outFileHeader.iTangentStride = (uint8_t)packedTangents.Stride();

		// Place the resource's magic number first.
		FileSystem::WriteBytes(&kModelResourceMagicNumber, sizeof(uint32_t), &hNewOutputMesh);

		// Write out the file header.
		FileSystem::WriteBytes(&outFileHeader, sizeof(CModel_FileHeader_t), &hNewOutputMesh);
		iTotalBytesWritten += sizeof(CModel_FileHeader_t);

		// Write out geometry headers.
		if (meshHeaders.Count() > 0)
		{
			FileSystem::WriteBytes(meshHeaders.Data(), meshHeaders.Stride() * meshHeaders.Count(), &hNewOutputMesh);
			iTotalBytesWritten += meshHeaders.Stride() * meshHeaders.Count();

			for (int iMesh = 0; iMesh < meshHeaders.Count(); iMesh++)
			{
				FileSystem::WriteBytes(meshHeaders[iMesh].pFaceIndices, sizeof(uint16_t) * meshHeaders[iMesh].iNumFaces, &hNewOutputMesh);
				iTotalBytesWritten += sizeof(uint16_t) * meshHeaders[iMesh].iNumFaces;
			}
		}

		if (meshFaces.Count() > 0)
		{
			FileSystem::WriteBytes(meshFaces.Data(), meshFaces.Stride() * meshFaces.Count(), &hNewOutputMesh);
			iTotalBytesWritten += meshFaces.Stride() * meshFaces.Count();
		}

		// Write out geometry tables.
		if (packedPositions.Count() > 0)
		{
			FileSystem::WriteBytes(packedPositions.Data(), packedPositions.Stride() * packedPositions.Count(), &hNewOutputMesh);
			iTotalBytesWritten += packedPositions.Stride() * packedPositions.Count();
		}
		if (packedNormals.Count() > 0)
		{
			FileSystem::WriteBytes(packedNormals.Data(), packedNormals.Stride() * packedNormals.Count(), &hNewOutputMesh);
			iTotalBytesWritten += packedNormals.Stride() * packedNormals.Count();
		}
		if (packedTexCoords.Count() > 0)
		{
			FileSystem::WriteBytes(packedTexCoords.Data(), packedTexCoords.Stride() * packedTexCoords.Count(), &hNewOutputMesh);
			iTotalBytesWritten += packedTexCoords.Stride() * packedTexCoords.Count();
		}
		if (packedTangents.Count() > 0)
		{
			FileSystem::WriteBytes(packedTangents.Data(), packedTangents.Stride() * packedTangents.Count(), &hNewOutputMesh);
			iTotalBytesWritten += packedTangents.Stride() * packedTangents.Count();
		}

		// Clean up arrays.
		for (int iMesh = 0; iMesh < meshHeaders.Count(); iMesh++)
		{
			if (meshHeaders[iMesh].pFaceIndices != nullptr)
			{
				delete[] meshHeaders[iMesh].pFaceIndices;
				meshHeaders[iMesh].pFaceIndices = nullptr;
			}
		}

		// Close the new file.
		FileSystem::CloseFile(&hNewOutputMesh);

		// Close the original source file.
		FileSystem::CloseFile(&hOBJFile);

		float flPercentReduced = ((float)iTotalBytesWritten / iOriginalByteSize);
		flPercentReduced *= 100.0f;
		LOG_INFO("Mesh '%s' compressed from %d bytes to %d bytes ->(%.2f%%)", pszOBJMeshPath, iOriginalByteSize, iTotalBytesWritten, flPercentReduced);

		return true;
	}

	static void GenerateTangentData(Utl::CVector<Vector>& pPositions, Utl::CVector<Vector>& pNormals, Utl::CVector<Vector2D>& pTexCoords, Utl::CVector<Vector>& pOutTangents, Utl::CVector<CModel_MeshFaceData_t>& pFaces)
	{
		uint64_t iFaceCount = pFaces.Count();
		uint64_t invalidTangentCount{ 0 };
		pOutTangents.Resize(iFaceCount);
		for (int iFace = 0; iFace < iFaceCount; iFace++)
		{
			Vector& pPos0 = pPositions[pFaces[iFace].iPositionIndices[0]];
			Vector& pPos1 = pPositions[pFaces[iFace].iPositionIndices[1]];
			Vector& pPos2 = pPositions[pFaces[iFace].iPositionIndices[2]];

			Vector2D& pUV0 = pTexCoords[pFaces[iFace].iTexCoordIndices[0]];
			Vector2D& pUV1 = pTexCoords[pFaces[iFace].iTexCoordIndices[1]];
			Vector2D& pUV2 = pTexCoords[pFaces[iFace].iTexCoordIndices[2]];

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

	static void DeduplicateGeometry(Utl::CVector<Vector>& pPositions, Utl::CVector<Vector>& pNormals, Utl::CVector<Vector2D>& pTexCoords, Utl::CVector<Vector>& pTangents, Utl::CVector<CModel_MeshFaceData_t>& pFaces)
	{
		uint64_t iOriginalVertDataSize = (pPositions.Stride() * pPositions.Count())
										  + (pNormals.Stride() * pNormals.Count())
										  + (pTexCoords.Stride() * pTexCoords.Count())
										  + (pTangents.Stride() * pTangents.Count());

		uint64_t iCompressedVertDataSize = 0;
		float	 flPercentReduced = 0;

		Utl::CVector<CModel_MeshFaceData_t>	newFaceData{};
		Utl::CVector<Vector>	uniquePositions(512);
		Utl::CVector<Vector>	uniqueNormals(512);
		Utl::CVector<Vector2D>	uniqueTexCoords(512);
		Utl::CVector<Vector>	uniqueTangents(512);
		uint64_t				iUniqueVertCount = 0;
		bool					bUniqueVert = true;
		uint64_t				iVertCount = pFaces.Count() * 3;

		// Maps original index data to new, compressed index data.
		Utl::CVector<uint64_t>	origPositionToNewPosition(pPositions.Count());
		Utl::CVector<uint64_t>	origNormalToNewNormal(pNormals.Count());
		Utl::CVector<uint64_t>	origTexcoordToNewTexcoord(pTexCoords.Count());
		Utl::CVector<uint64_t>	origTangentToNewTangent(pTangents.Count());

		for (int iFace = 0; iFace < pFaces.Count(); iFace++)
		{
			int iVert = 0;

			for (iVert = 0; iVert < 3; iVert++)
			{
				Vector* pCurPos = &pPositions[pFaces[iFace].iPositionIndices[iVert]];
				Vector* pCurNormal = &pNormals[pFaces[iFace].iNormalIndices[iVert]];
				Vector2D* pCurTexCoord = &pTexCoords[pFaces[iFace].iTexCoordIndices[iVert]];
				Vector* pCurTangent = &pTangents[pFaces[iFace].iTangentIndices[iVert]];

				bool bUniquePos = true;
				bool bUniqueNormal = true;
				bool bUniqueTexCoord = true;
				bool bUniqueTangent = true;

				// Check for unique positions.
				for (int iPos = 0; iPos < uniquePositions.Count() && bUniquePos; iPos++)
				{
					if (uniquePositions[iPos] == *pCurPos)
					{
						// Not unique!
						bUniquePos = false;

						// Save off the index for remapping later.
						origPositionToNewPosition.PushBack(iPos);
						break;
					}
				}

				if (bUniquePos)
				{
					uniquePositions.PushBack(*pCurPos);
					// Save off the index for remapping later.
					origPositionToNewPosition.PushBack((iFace * 3) + iVert);
				}

				// Check for unique normals.
				for (int iNormal = 0; iNormal < uniqueNormals.Count() && bUniqueNormal; iNormal++)
				{
					if (uniqueNormals[iNormal] == *pCurNormal)
					{
						// Not unique!
						bUniqueNormal = false;

						// Save off the index for remapping later.
						origNormalToNewNormal.PushBack(iNormal);
						break;
					}
				}

				if (bUniqueNormal)
				{
					uniqueNormals.PushBack(*pCurNormal);

					// Save off the index for remapping later.
					origNormalToNewNormal.PushBack((iFace * 3) + iVert);
				}

				// Check for unique texcoords.
				for (int iTexCoord = 0; iTexCoord < uniqueTexCoords.Count() && bUniqueTexCoord; iTexCoord++)
				{
					if (uniqueTexCoords[iTexCoord] == *pCurTexCoord)
					{
						// Not unique!
						bUniqueTexCoord = false;

						// Save off the index for remapping later.
						origTexcoordToNewTexcoord.PushBack(iTexCoord);
						break;
					}
				}

				if (bUniqueTexCoord)
				{
					uniqueTexCoords.PushBack(*pCurTexCoord);

					// Save off the index for remapping later.
					origTexcoordToNewTexcoord.PushBack((iFace * 3) + iVert);
				}

				// Check for unique tangents.
				for (int iTangent = 0; iTangent < uniqueTangents.Count() && bUniqueTangent; iTangent++)
				{
					if (uniqueTangents[iTangent] == *pCurTangent)
					{
						// Not unique!
						bUniqueTangent = false;

						// Save off the index for remapping later.
						origTangentToNewTangent.PushBack(iTangent);
						break;
					}
				}

				if (bUniqueTangent)
				{
					uniqueTangents.PushBack(*pCurTangent);

					// Save off the index for remapping later.
					origTangentToNewTangent.PushBack((iFace * 3) + iVert);
				}
			}
		}

		CAssert(origPositionToNewPosition.Count() == pFaces.Count() * 3);

		// Verify that the new data fits within our limits.
		const uint64_t MAX_TOTAL_VERTS_SUPPORTED = (1ULL << 32) - 1;
		if (uniquePositions.Count() > MAX_TOTAL_VERTS_SUPPORTED ||
			uniqueNormals.Count() > MAX_TOTAL_VERTS_SUPPORTED ||
			uniqueTexCoords.Count() > MAX_TOTAL_VERTS_SUPPORTED ||
			uniqueTangents.Count() > MAX_TOTAL_VERTS_SUPPORTED)
		{
			LOG_WARN("Mesh contains more than 4,294,967,295 unique vertices. This mesh cannot be properly serialized.");
			return;
		}

		// Walk through the original faces and remap the indices to the compressed data.
		for (int iFace = 0; iFace < pFaces.Count(); iFace++)
		{
			for (int iVert = 0; iVert < 3; iVert++)		// 3 verts per face.
			{
				pFaces[iFace].iPositionIndices[iVert] = (uint32_t)origPositionToNewPosition[(iFace * 3) + iVert];
				pFaces[iFace].iNormalIndices[iVert] = (uint32_t)origNormalToNewNormal[(iFace * 3) + iVert];
				pFaces[iFace].iTexCoordIndices[iVert] = (uint32_t)origTexcoordToNewTexcoord[(iFace * 3) + iVert];
				pFaces[iFace].iTangentIndices[iVert] = (uint32_t)origTangentToNewTangent[(iFace * 3) + iVert];
			}
		}

		LOG_DEBUG("Mesh positions compressed from %d to %d (%f%%)", pPositions.Count(), uniquePositions.Count(), ((float)uniquePositions.Count() / (float)pPositions.Count()) * 100.0f);
		LOG_DEBUG("Mesh normals compressed from %d to %d (%f%%)", pNormals.Count(), uniqueNormals.Count(), ((float)uniqueNormals.Count() / (float)pNormals.Count()) * 100.0f);
		LOG_DEBUG("Mesh texcoords compressed from %d to %d (%f%%)", pTexCoords.Count(), uniqueTexCoords.Count(), ((float)uniqueTexCoords.Count() / (float)pTexCoords.Count()) * 100.0f);
		LOG_DEBUG("Mesh tangents compressed from %d to %d (%f%%)", pTangents.Count(), uniqueTangents.Count(), ((float)uniqueTangents.Count() / (float)pTangents.Count()) * 100.0f);

		iCompressedVertDataSize = (uniquePositions.Count() * uniquePositions.Stride()) + 
									(uniqueNormals.Count() * uniqueNormals.Stride()) +
									(uniqueTexCoords.Count() * uniqueTexCoords.Stride()) +
									(uniqueTangents.Count() * uniqueTangents.Stride());
		flPercentReduced = ((float)iCompressedVertDataSize / iOriginalVertDataSize);
		flPercentReduced *= 100.0f;

		LOG_INFO("Mesh data compressed from %d bytes to %d bytes ->(%.2f%%)", iOriginalVertDataSize, iCompressedVertDataSize, flPercentReduced);

		// Copy over the compressed data to the output, release original data.
		Utl::CVector<Vector> oldPositions = pPositions;
		pPositions = uniquePositions;
		oldPositions.Clear();

		Utl::CVector<Vector> oldNormals = pNormals;
		pNormals = uniqueNormals;
		oldNormals.Clear();

		Utl::CVector<Vector2D> oldTexCoords = pTexCoords;
		pTexCoords = uniqueTexCoords;
		oldTexCoords.Clear();

		Utl::CVector<Vector> oldTangents = pTangents;
		pTangents = uniqueTangents;
		oldTangents.Clear();
	}

	static uint16_t PackF32ToUint16(const float flUnitFloat)
	{
		const unsigned int numIntervals = 1U << 16;	// Number of unique values that can be stored in a uint16.

		// Scale the unit value to fit within this range.
		float flScaledFloat = flUnitFloat * (numIntervals - 1U);

		// Round and truncate to an int.
		uint16_t nRounded = (uint16_t)(flScaledFloat + 0.5f);

		// Make sure we don't go outside of our valid range.
		if (nRounded > numIntervals - 1)
			nRounded = numIntervals - 1U;

		return nRounded;
	}

	static float UnpackF32FromUint16(const uint16_t inputData)
	{
		const unsigned int numIntervals = 1U << 16;	// Number of unique values that can be stored in a uint16.

		// Scale the data and store in a float.
		float flUnitFloat = ((float)inputData / numIntervals);

		return flUnitFloat;
	}

}	// namespace MeshSerializer
