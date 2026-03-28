//=============================================================================
// Author: Jared Simons
// Date Created: 6/12/2025
// 
// Purpose: Mesh file serialization utilities.
//=============================================================================
#include "MeshSerializer.h"

#include "MaterialSerializer.h"
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
	static void DeduplicateGeometry(CModel_MeshHeader_t& pCurMesh, Utl::CVector<Vector>& pPositions, Utl::CVector<Vector>& pNormals, Utl::CVector<Vector2D>& pTexCoords, Utl::CVector<CModel_MeshFaceData_t>& pFaces);
	static bool ImportOBJMaterialLibrary(const char* pszMaterialLibraryName, MeshLoader::MeshResource_t& pMeshResource);
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

		// FIXME: Reading the whole file twice is kinda bad :(
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

		// Temp geometry tables.
		// NOTE: Using nested arrays here so that we can easily de-duplicate per mesh instead of across the whole model.
		// Then just write the unique data to the final arrays.
		Utl::CVector<Utl::CVector<Vector>>					vertexPositions{};
		Utl::CVector<Utl::CVector<Vector>>					vertexNormals{};
		Utl::CVector<Utl::CVector<Vector2D>>				vertexTexCoords{};
		Utl::CVector<Utl::CVector<CModel_MeshFaceData_t>>	meshFaces{};

		// Geometry data.
		Utl::CVector<CModel_MeshHeader_t>	meshHeaders{};
		Utl::CVector<CModel_MeshFaceData_t>	finalMeshFaces{};

		// Final geometry tables.
		Utl::CVector<Vector>	finalVertexPositions{};
		Utl::CVector<Vector>	finalVertexNormals{};
		Utl::CVector<Vector2D>	finalVertexTexCoords{};
		Utl::CVector<Vector>	finalVertexTangents{};

		// Material stuff.
		// CLEANUP: Scope this better?
		char* pszMTLFilePath{ nullptr };
		// Contains all of the material names encountered (does not contain repeats!)
		Utl::CVector<char*>		materialNames{};
		char* pszMaterialName{ nullptr };
		bool bMatFound = false;

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
				if (meshHeaders.Count() > 0)
				{
					// If we currently have a mesh being processed, do some extra work to prepare it for storage.
					// FIXME: This will not apply to the final mesh in the file!
					
					// Walk through all the position vectors to determine the extents of the geometry.
					// LEFTOFF: Calculate extents per submesh.
				}

				meshHeaders.PushBack(CModel_MeshHeader_t{});
				meshHeaders.Back().pFaceIndices = new uint32_t[65535];	// Just allocate a bunch of these for now, we'll only write the ones that are used.
				meshHeaders.Back().iNumFaces = 0;
				vertexPositions.PushBack(Utl::CVector<Vector>{});
				vertexNormals.PushBack(Utl::CVector<Vector>{});
				vertexTexCoords.PushBack(Utl::CVector<Vector2D>{});
				meshFaces.PushBack(Utl::CVector<CModel_MeshFaceData_t>{});
				break;
			}

			case 'g':
			{
				// Skip new groups, we'll just put all the geometry in the same mesh.
				// TODO: How to handle meshes that use JUST 'g' instead of 'o' or 'o' and 'g'?
				if (meshHeaders.Count() < 1)
				{
					meshHeaders.PushBack(CModel_MeshHeader_t{});
					meshHeaders.Back().pFaceIndices = new uint32_t[65535];	// Just allocate a bunch of these for now, we'll only write the ones that are used.
					meshHeaders.Back().iNumFaces = 0;
					vertexPositions.PushBack(Utl::CVector<Vector>{});
					vertexNormals.PushBack(Utl::CVector<Vector>{});
					vertexTexCoords.PushBack(Utl::CVector<Vector2D>{});
					meshFaces.PushBack(Utl::CVector<CModel_MeshFaceData_t>{});
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
					// NOTE: OBJ files are 1-indexed so we need to bump down by one to store as a zero index.
					newFace.iPositionIndices[iVert] = positionIndices[iVert] - 1;
					newFace.iNormalIndices[iVert] = normalIndices[iVert] - 1;
					newFace.iTexCoordIndices[iVert] = texCoordIndices[iVert] - 1;
				}
				
				//uint32_t iFaceIndex = (uint32_t)meshFaces.Back().PushBack(newFace);
				uint32_t iFaceIndex = (uint32_t)finalMeshFaces.PushBack(newFace);
				
				// Currently, we store face indices as uint32_t. If a single mesh contains more than than 4,294,967,295 values, then we need to re-evaluate.
				if (iFaceIndex > 0xFFFFFFFF)
				{
					LOG_WARN("Mesh '%s' contains more than 4,294,967,295 faces in a single mesh. This mesh cannot be properly serialized.", pszOBJMeshPath);
					return false;
				}
				meshHeaders.Back().pFaceIndices[meshHeaders.Back().iNumFaces] = iFaceIndex;
				meshHeaders.Back().iNumFaces++;
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
						vertexPositions.Back().PushBack(newPosition);
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
						vertexNormals.Back().PushBack(newNormal);
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
						vertexTexCoords.Back().PushBack(newTexCoord);
						break;
					}
				}
				break;
			}

			case 'm':
				// Append the appropriate extension to the path.
				// FIXME: THIS ASSUMES THAT THE MTLLIB FILE HAS THE SAME NAME AS THE OBJ!!
				char* pszMTLFilePath = Utl::StringDuplicate(pszOBJMeshPath);
				uint64_t iStringLength = Utl::StringLength(pszMTLFilePath);
				pszMTLFilePath[iStringLength - 3] = 'm';
				pszMTLFilePath[iStringLength - 2] = 't';
				pszMTLFilePath[iStringLength - 1] = 'l';
				pszMTLFilePath[iStringLength - 0] = '\0';
				if (!ImportOBJMaterialLibrary(pszMTLFilePath, pOutResource))
				{
					// FIXME: What to do if this fails?
				}
				break;
				
			/*default:
				//CAssert(0);
				break;*/
			}
		}

		LOG_INFO("Compressing mesh data.");
		// TODO: This takes FOREVER on some not too large models (like sponza)
		//if (!Utl::StringCompare(pszOBJMeshPath, "Assets/Models/sponza.obj"))
			//DeduplicateGeometry(vertexPositions, vertexNormals, vertexTexCoords, meshFaces);

		for (uint64_t iMesh = 0; iMesh < meshHeaders.Count(); iMesh++)
		{
			LOG_DEBUG("Geometry deduplication progress: %d/%d", iMesh, meshHeaders.Count());
			DeduplicateGeometry(meshHeaders[iMesh], vertexPositions[iMesh], vertexNormals[iMesh], vertexTexCoords[iMesh], meshFaces[iMesh]);
		}

		// Copy all vertex data to the final arrays.
		LOG_DEBUG("Writing final vertex data.");
		for (uint64_t iMesh = 0; iMesh < meshHeaders.Count(); iMesh++)
		{
			for (uint64_t iPos = 0; iPos < vertexPositions[iMesh].Count(); iPos++)
			{
				finalVertexPositions.PushBack(vertexPositions[iMesh][iPos]);
			}

			for (uint64_t iNorm = 0; iNorm < vertexNormals[iMesh].Count(); iNorm++)
			{
				finalVertexNormals.PushBack(vertexNormals[iMesh][iNorm]);
			}

			for (uint64_t iTexCoord = 0; iTexCoord < vertexTexCoords[iMesh].Count(); iTexCoord++)
			{
				finalVertexTexCoords.PushBack(vertexTexCoords[iMesh][iTexCoord]);
			}

			// LEFTOFF: THIS IS BORKED
			/*for (uint64_t iFace = 0; iFace < meshFaces[iMesh].Count(); iFace++)
			{
				finalMeshFaces.PushBack(meshFaces[iMesh][iFace]);
			}*/
		}

		// Before writing out geometry to the file, generate tangent data and deduplicate extra data.
		if (!bModelContainsVertexTangents)	// TODO: Make generating tangets optional.
		{
			LOG_INFO("Generating tangent data for mesh.");
			GenerateTangentData(finalVertexPositions, finalVertexNormals, finalVertexTexCoords, finalVertexTangents, finalMeshFaces);
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
		// TODO: Make this optional.
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
				const float flPackedTexCoordMins = -512.0f;
				const float flPackedTexCoordMaxs = 512.0f;

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

		outFileHeader.iFaceDataOffset = outFileHeader.iMeshDataOffset + (meshHeaders.Stride() * meshHeaders.Count());
		for (int iMesh = 0; iMesh < meshHeaders.Count(); iMesh++)
		{
			outFileHeader.iFaceDataOffset += sizeof(uint16_t) * meshHeaders[iMesh].iNumFaces;
		}
		outFileHeader.iFaceDataCount = (uint32_t)finalMeshFaces.Count();
		outFileHeader.iFaceDataStride = (uint8_t)finalMeshFaces.Stride();

		outFileHeader.iPositionTableOffset = outFileHeader.iFaceDataOffset + (finalMeshFaces.Count() * finalMeshFaces.Stride());
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

		outFileHeader.iMaterialTableOffset = outFileHeader.iTangentTableOffset + (outFileHeader.iTangentStride * outFileHeader.iTangentCount);
		outFileHeader.iMaterialCount = (uint32_t)materialNames.Count();

		// Place the resource's magic number first.
		FileSystem::WriteBytes(&kModelResourceMagicNumber, sizeof(uint32_t), &hNewOutputMesh);

		// Write out the file header.
		FileSystem::WriteBytes(&outFileHeader, sizeof(CModel_FileHeader_t), &hNewOutputMesh);
		iTotalBytesWritten += sizeof(CModel_FileHeader_t);

		// Write out geometry headers.
		if (meshHeaders.Count() > 0)
		{
			for (unsigned int iMesh = 0; iMesh < meshHeaders.Count(); iMesh++)
			{
				FileSystem::WriteBytes(&meshHeaders[iMesh].iMaterialIndex, sizeof(uint32_t), &hNewOutputMesh);
				FileSystem::WriteBytes(&meshHeaders[iMesh].iNumFaces, sizeof(uint32_t), &hNewOutputMesh);
				FileSystem::WriteBytes(meshHeaders[iMesh].pFaceIndices, sizeof(uint32_t) * meshHeaders[iMesh].iNumFaces, &hNewOutputMesh);
				iTotalBytesWritten += sizeof(uint32_t) + (sizeof(uint32_t) * meshHeaders[iMesh].iNumFaces);
			}
		}

		if (finalMeshFaces.Count() > 0)
		{
			FileSystem::WriteBytes(finalMeshFaces.Data(), finalMeshFaces.Stride() * finalMeshFaces.Count(), &hNewOutputMesh);
			iTotalBytesWritten += finalMeshFaces.Stride() * finalMeshFaces.Count();
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

		// Write out material data.
		if (materialNames.Count() > 0)
		{
			for (int iMaterial = 0; iMaterial < materialNames.Count(); iMaterial++)
			{
				size_t iStringLength = Utl::StringLength(materialNames[iMaterial]);
				char szMatName[512];
				Platform::MemoryCopy(szMatName, materialNames[iMaterial], iStringLength);
				szMatName[iStringLength] = 0;
				FileSystem::WriteBytes(szMatName, iStringLength + 1, &hNewOutputMesh);
				iTotalBytesWritten += (iStringLength + 1);
			}
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

		return MeshLoader::LoadMeshResource((char*)pszOutMeshFileName, pOutResource);
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

	static void DeduplicateGeometry(CModel_MeshHeader_t& pCurMesh, Utl::CVector<Vector>& pPositions, Utl::CVector<Vector>& pNormals, Utl::CVector<Vector2D>& pTexCoords, Utl::CVector<CModel_MeshFaceData_t>& pFaces)
	{
		// LEFTOFF: IMPLEMENT ME!
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
