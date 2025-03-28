//=============================================================================
// Author: Jared Simons
// Date Created: 2/18/2025
// 
// Purpose: CSG Geometry Generation Tool
//=============================================================================
#include "GeometryGenerator.h"

#include <Containers/CVector.h>
#include <Core/Logging.h>
#include <Math/VertexTypes.h>
#include <Platform/FileSystem.h>
#include "Renderer/Renderer.h"
#include <Strings/String_Utils.h>

#include <stdio.h> // for scanf()

GeometryConfig_t MeshGenerator::CreateDefaultQuadConfiguration(Vector3D vecOrigin, float flWidth, float flHeight)
{
	GeometryConfig_t quadConfig{};

	quadConfig.Vertices.Resize(6);

	// Vertex positions.
	quadConfig.Vertices[0].Pos = { vecOrigin.x + (-flWidth) / 2, vecOrigin.y + 0, vecOrigin.z +   flHeight / 2 };
	quadConfig.Vertices[1].Pos = { vecOrigin.x +   flWidth / 2,  vecOrigin.y + 0, vecOrigin.z +   flHeight / 2 };
	quadConfig.Vertices[2].Pos = { vecOrigin.x +   flWidth / 2,  vecOrigin.y + 0, vecOrigin.z + (-flHeight) / 2 };
	quadConfig.Vertices[3].Pos = { vecOrigin.x + (-flWidth) / 2, vecOrigin.y + 0, vecOrigin.z +   flHeight / 2 };
	quadConfig.Vertices[4].Pos = { vecOrigin.x +   flWidth / 2,  vecOrigin.y + 0, vecOrigin.z + (-flHeight) / 2 };
	quadConfig.Vertices[5].Pos = { vecOrigin.x + (-flWidth) / 2, vecOrigin.y + 0, vecOrigin.z + (-flHeight) / 2 };

	// Texcoords.
	quadConfig.Vertices[0].TexCoord = { 0.0f, 0.0f };
	quadConfig.Vertices[1].TexCoord = { 0.0f, 1.0f };
	quadConfig.Vertices[2].TexCoord = { 1.0f, 1.0f };
	quadConfig.Vertices[3].TexCoord = { 0.0f, 0.0f };
	quadConfig.Vertices[4].TexCoord = { 1.0f, 1.0f };
	quadConfig.Vertices[5].TexCoord = { 0.0f, 1.0f };

	// TODO: This needs normal and tangent generation as well!
	/*for (int i = 0; i < 6; i++)
	{
		LOG_DEBUG("%.3f, %.3f, %.3f", quadConfig.Vertices[i].Pos.x, quadConfig.Vertices[i].Pos.y, quadConfig.Vertices[i].Pos.z);
	}*/

	return quadConfig;
}

GeometryConfig_t MeshGenerator::CreateDefaultCubeConfiguration(Vector3D vecOrigin, float flWidth, float flHeight, float flDepth)
{
	GeometryConfig_t cubeConfig{};
	int faceIdx = 0;

	const float halfWidth = flWidth / 2;
	const float halfHeight = flHeight / 2;
	const float halfDepth = flDepth / 2;

	cubeConfig.Vertices.Resize(36);

	// Front face
	cubeConfig.Vertices[(faceIdx * 6) + 0].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y +   halfHeight,  vecOrigin.z + halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 1].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y + (-halfHeight), vecOrigin.z + halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 2].Pos = { vecOrigin.x +   halfWidth,  vecOrigin.y + (-halfHeight), vecOrigin.z + halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 3].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y +   halfHeight,  vecOrigin.z + halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 4].Pos = { vecOrigin.x +   halfWidth,  vecOrigin.y + (-halfHeight), vecOrigin.z + halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 5].Pos = { vecOrigin.x +   halfWidth,  vecOrigin.y +   halfHeight,  vecOrigin.z + halfDepth };

	cubeConfig.Vertices[(faceIdx * 6) + 0].TexCoord = { 0.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 1].TexCoord = { 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 2].TexCoord = { 1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 3].TexCoord = { 0.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 4].TexCoord = { 1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 5].TexCoord = { 1.0f, 1.0f };

	cubeConfig.Vertices[(faceIdx * 6) + 0].Normal = { 0.0f, 0.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 1].Normal = { 0.0f, 0.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 2].Normal = { 0.0f, 0.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 3].Normal = { 0.0f, 0.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 4].Normal = { 0.0f, 0.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 5].Normal = { 0.0f, 0.0f, 1.0f };
	faceIdx++;

	// Back face
	cubeConfig.Vertices[(faceIdx * 6) + 0].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y +  halfHeight,   vecOrigin.z + (-halfDepth) };
	cubeConfig.Vertices[(faceIdx * 6) + 1].Pos = { vecOrigin.x +   halfWidth,  vecOrigin.y +  halfHeight,   vecOrigin.z + (-halfDepth) };
	cubeConfig.Vertices[(faceIdx * 6) + 2].Pos = { vecOrigin.x +   halfWidth,  vecOrigin.y + (-halfHeight), vecOrigin.z + (-halfDepth) };
	cubeConfig.Vertices[(faceIdx * 6) + 3].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y +  halfHeight,   vecOrigin.z + (-halfDepth) };
	cubeConfig.Vertices[(faceIdx * 6) + 4].Pos = { vecOrigin.x +   halfWidth,  vecOrigin.y + (-halfHeight), vecOrigin.z + (-halfDepth) };
	cubeConfig.Vertices[(faceIdx * 6) + 5].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y + (-halfHeight), vecOrigin.z + (-halfDepth) };

	cubeConfig.Vertices[(faceIdx * 6) + 0].TexCoord = { 1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 1].TexCoord = { 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 2].TexCoord = { 0.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 3].TexCoord = { 1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 4].TexCoord = { 0.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 5].TexCoord = { 1.0f, 1.0f };

	cubeConfig.Vertices[(faceIdx * 6) + 0].Normal = { 0.0f, 0.0f, -1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 1].Normal = { 0.0f, 0.0f, -1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 2].Normal = { 0.0f, 0.0f, -1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 3].Normal = { 0.0f, 0.0f, -1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 4].Normal = { 0.0f, 0.0f, -1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 5].Normal = { 0.0f, 0.0f, -1.0f };
	faceIdx++;

	// Left face
	cubeConfig.Vertices[(faceIdx * 6) + 0].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y +   halfHeight,   vecOrigin.z + (-halfDepth) };
	cubeConfig.Vertices[(faceIdx * 6) + 1].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y + (-halfHeight),  vecOrigin.z +   halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 2].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y +   halfHeight,   vecOrigin.z +   halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 3].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y +   halfHeight,   vecOrigin.z + (-halfDepth) };
	cubeConfig.Vertices[(faceIdx * 6) + 4].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y + (-halfHeight),  vecOrigin.z + (-halfDepth) };
	cubeConfig.Vertices[(faceIdx * 6) + 5].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y + (-halfHeight),  vecOrigin.z +   halfDepth };

	cubeConfig.Vertices[(faceIdx * 6) + 0].TexCoord = { 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 1].TexCoord = { 1.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 2].TexCoord = { 1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 3].TexCoord = { 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 4].TexCoord = { 0.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 5].TexCoord = { 1.0f, 1.0f };

	cubeConfig.Vertices[(faceIdx * 6) + 0].Normal = { -1.0f, 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 1].Normal = { -1.0f, 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 2].Normal = { -1.0f, 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 3].Normal = { -1.0f, 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 4].Normal = { -1.0f, 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 5].Normal = { -1.0f, 0.0f, 0.0f };
	faceIdx++;

	// Right face
	cubeConfig.Vertices[(faceIdx * 6) + 0].Pos = { vecOrigin.x + halfWidth, vecOrigin.y +   halfHeight,  vecOrigin.z + (-halfDepth) };
	cubeConfig.Vertices[(faceIdx * 6) + 1].Pos = { vecOrigin.x + halfWidth, vecOrigin.y +   halfHeight,  vecOrigin.z +   halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 2].Pos = { vecOrigin.x + halfWidth, vecOrigin.y + (-halfHeight), vecOrigin.z +   halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 3].Pos = { vecOrigin.x + halfWidth, vecOrigin.y +   halfHeight,  vecOrigin.z + (-halfDepth) };
	cubeConfig.Vertices[(faceIdx * 6) + 4].Pos = { vecOrigin.x + halfWidth, vecOrigin.y + (-halfHeight), vecOrigin.z +   halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 5].Pos = { vecOrigin.x + halfWidth, vecOrigin.y + (-halfHeight), vecOrigin.z + (-halfDepth) };

	cubeConfig.Vertices[(faceIdx * 6) + 0].TexCoord = { 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 1].TexCoord = { 1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 2].TexCoord = { 1.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 3].TexCoord = { 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 4].TexCoord = { 1.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 5].TexCoord = { 0.0f, 1.0f };

	cubeConfig.Vertices[(faceIdx * 6) + 0].Normal = { 1.0f, 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 1].Normal = { 1.0f, 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 2].Normal = { 1.0f, 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 3].Normal = { 1.0f, 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 4].Normal = { 1.0f, 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 5].Normal = { 1.0f, 0.0f, 0.0f };
	faceIdx++;

	// Top face
	cubeConfig.Vertices[(faceIdx * 6) + 0].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y + halfHeight, vecOrigin.z + (-halfDepth) };
	cubeConfig.Vertices[(faceIdx * 6) + 1].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y + halfHeight, vecOrigin.z +  halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 2].Pos = { vecOrigin.x +   halfWidth,  vecOrigin.y + halfHeight, vecOrigin.z + (-halfDepth) };
	cubeConfig.Vertices[(faceIdx * 6) + 3].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y + halfHeight, vecOrigin.z +  halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 4].Pos = { vecOrigin.x +   halfWidth,  vecOrigin.y + halfHeight, vecOrigin.z +  halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 5].Pos = { vecOrigin.x +   halfWidth,  vecOrigin.y + halfHeight, vecOrigin.z + (-halfDepth) };

	cubeConfig.Vertices[(faceIdx * 6) + 0].TexCoord = { 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 1].TexCoord = { 0.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 2].TexCoord = { 1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 3].TexCoord = { 0.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 4].TexCoord = { 1.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 5].TexCoord = { 1.0f, 0.0f };

	cubeConfig.Vertices[(faceIdx * 6) + 0].Normal = { 0.0f, 1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 1].Normal = { 0.0f, 1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 2].Normal = { 0.0f, 1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 3].Normal = { 0.0f, 1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 4].Normal = { 0.0f, 1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 5].Normal = { 0.0f, 1.0f, 0.0f };
	faceIdx++;

	// Bottom face
	cubeConfig.Vertices[(faceIdx * 6) + 0].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y + (-halfHeight),  vecOrigin.z +   halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 1].Pos = { vecOrigin.x +   halfWidth,  vecOrigin.y + (-halfHeight),  vecOrigin.z + (-halfDepth) };
	cubeConfig.Vertices[(faceIdx * 6) + 2].Pos = { vecOrigin.x +   halfWidth,  vecOrigin.y + (-halfHeight),  vecOrigin.z +   halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 3].Pos = { vecOrigin.x +   halfWidth,  vecOrigin.y + (-halfHeight),  vecOrigin.z + (-halfDepth) };
	cubeConfig.Vertices[(faceIdx * 6) + 4].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y + (-halfHeight),  vecOrigin.z +   halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 5].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y + (-halfHeight),  vecOrigin.z + (-halfDepth) };

	cubeConfig.Vertices[(faceIdx * 6) + 0].TexCoord = { 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 1].TexCoord = { 1.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 2].TexCoord = { 1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 3].TexCoord = { 1.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 4].TexCoord = { 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 5].TexCoord = { 0.0f, 1.0f };

	cubeConfig.Vertices[(faceIdx * 6) + 0].Normal = { 0.0f, -1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 1].Normal = { 0.0f, -1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 2].Normal = { 0.0f, -1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 3].Normal = { 0.0f, -1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 4].Normal = { 0.0f, -1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 5].Normal = { 0.0f, -1.0f, 0.0f };
	faceIdx++;

	// Calculate tangent and bitangent data.
	for (int face = 0; face < 6; face++)
	{
		Vector3D position1 = cubeConfig.Vertices[(face * 6) + 0].Pos;
		Vector3D position2 = cubeConfig.Vertices[(face * 6) + 1].Pos;
		Vector3D position3 = cubeConfig.Vertices[(face * 6) + 2].Pos;

		// First triangle
		Vector3D edge1 = position2 - position1;
		Vector3D edge2 = position3 - position1;
		Vector2D deltaUV1 = cubeConfig.Vertices[(face * 6) + 1].TexCoord - cubeConfig.Vertices[(face * 6) + 0].TexCoord;
		Vector2D deltaUV2 = cubeConfig.Vertices[(face * 6) + 2].TexCoord - cubeConfig.Vertices[(face * 6) + 0].TexCoord;

		float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
		for (int vertIdx = 0; vertIdx < 3; vertIdx++)
		{
			cubeConfig.Vertices[(face * 6) + vertIdx].Tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			cubeConfig.Vertices[(face * 6) + vertIdx].Tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			cubeConfig.Vertices[(face * 6) + vertIdx].Tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
		}

		// Second triangle
		position1 = cubeConfig.Vertices[(face * 6) + 3].Pos;
		position2 = cubeConfig.Vertices[(face * 6) + 4].Pos;
		position3 = cubeConfig.Vertices[(face * 6) + 5].Pos;

		edge1 = position2 - position1;
		edge2 = position3 - position1;
		deltaUV1 = cubeConfig.Vertices[(face * 6) + 4].TexCoord - cubeConfig.Vertices[(face * 6) + 3].TexCoord;
		deltaUV2 = cubeConfig.Vertices[(face * 6) + 5].TexCoord - cubeConfig.Vertices[(face * 6) + 3].TexCoord;

		for (int vertIdx = 0; vertIdx < 3; vertIdx++)
		{
			cubeConfig.Vertices[(face * 6) + 3 + vertIdx].Tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
			cubeConfig.Vertices[(face * 6) + 3 + vertIdx].Tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
			cubeConfig.Vertices[(face * 6) + 3 + vertIdx].Tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
		}
	}

	cubeConfig.Vertices.SetCount(36);

	return cubeConfig;
}

class OBJModelLoader : public IModelLoader
{
public:
	virtual Mesh CreateConfigFromFile(const char* pszFilePath) override;
};

// TODO: This should be part of a resource system!
// We should not be accessing this directly!
static OBJModelLoader s_OBJLoader;
IModelLoader* g_pOBJLoader = &s_OBJLoader;

Mesh OBJModelLoader::CreateConfigFromFile(const char* pszFilePath)
{
	// Make sure the file exists!
	if (!FileSystem::FileExists(pszFilePath))
	{
		LOG_ERROR("Failed to find model '%s' to load! Nothing was done.", pszFilePath);
		//return GeometryConfig_t{};
	}

	FileHandle_t OBJFileHandle;
	if (!FileSystem::OpenFile(pszFilePath, eFileMode::FILEMODE_READ, false, &OBJFileHandle))
	{
		//return GeometryConfig_t{};
	}

	// Parse the file's data.
	Mesh newMesh{};
	GeometryConfig_t OBJConfig{};
	Utl::CVector<Vector3D> vertices{};
	Utl::CVector<Vector3D> normals{};
	Utl::CVector<Vector2D> texCoords{};
	Utl::CVector<uint32_t> vertexIndices{};
	Utl::CVector<uint32_t> normalIndices{};
	Utl::CVector<uint32_t> texCoordIndices{};

	Utl::CVector<uint32_t> drawingIndices{};
	int currIndex{ 0 };

	bool bBuildingPolygonList = false;

	char szCurrentLine[kScratchStringBufferLength]{};
	char pFirstChar{};
	char pSecondChar{};
	size_t currLine{ 1 };
	size_t bytesRead{ 0 };
	int groups{ 0 };

	while (!FileSystem::IsEOF(&OBJFileHandle))
	{
		FileSystem::ReadLine(szCurrentLine, kScratchStringBufferLength, &bytesRead, &OBJFileHandle);

		pFirstChar = szCurrentLine[0];
		pSecondChar = szCurrentLine[1];

		// TODO: Handle materiallib file.

		if (bBuildingPolygonList && (pFirstChar != 'f' && pFirstChar != 's' && pFirstChar != 'u' && pFirstChar != '#'))
		{
			GeometryConfig_t currentConfig{};
			// Flush the polygon list to the renderer and start a new config.
			if (groups > -1 && groups < 1)
			{
				// There was an active group that needs to be flushed to a config.
				for (int idx = 0; idx < vertexIndices.Count() - 1; idx++)
				{
					Vertex3D newVertex;

					uint32_t vertexIndex = vertexIndices[idx];
					newVertex.Pos = vertices[vertexIndex - 1];

					uint32_t texCoordIndex = texCoordIndices[idx];
					newVertex.TexCoord = texCoords[texCoordIndex - 1];

					uint32_t normalIndex = normalIndices[idx];
					newVertex.Normal = normals[normalIndex - 1];

					currentConfig.Vertices.PushBack(newVertex);
				}

				// Also tack on our indices.
				// TODO: Figure this thing out
				//currentConfig.Indices = drawingIndices;

				// Acquire the geometry resources.
				size_t newGeometryIndex = newMesh.m_Geometries.PushBack(GeometryInstance_t{});
				g_pRenderer->AcquireGeometryResources(currentConfig, newMesh.m_Geometries[newGeometryIndex]);
			}
			bBuildingPolygonList = false;
			groups++;

			// Clear existing data.
			//vertices.Clear();
			//normals.Clear();
			//texCoords.Clear();
			vertexIndices.Clear();
			normalIndices.Clear();
			texCoordIndices.Clear();
			drawingIndices.Clear();
			currIndex = 0;
		}

		switch (pFirstChar)
		{
		case '#':	// Skip comments.
		case 0:		// And blank lines
			break;

		case 'g':
			{
				bBuildingPolygonList = true;				
				break;
			}

		case 'v':
			{
			switch (pSecondChar)
			{
				case ' ':	// Vertex position.
					{
						// NOTE: Right now we are expecting 3-component vectors from our OBJ files.
						Vector3D newVertex{};
						char pCurrValue[512];
					
						int currValLength = StringTrim(&szCurrentLine[2], pCurrValue);
						newVertex.x = String_ATOF(pCurrValue);

						currValLength += StringTrim(&szCurrentLine[2 + currValLength], pCurrValue);
						newVertex.y = String_ATOF(pCurrValue);

						currValLength += StringTrim(&szCurrentLine[2 + currValLength], pCurrValue);
						newVertex.z = String_ATOF(pCurrValue);

						vertices.PushBack(newVertex);
						break;
					}

				case 'n':	// Vertex normal.
					{
						// NOTE: Right now we are expecting 3-component vectors from our OBJ files.
						Vector3D newNormal{};
						char pCurrValue[512];
					
						int currValLength = StringTrim(&szCurrentLine[2], pCurrValue);
						newNormal.x = String_ATOF(pCurrValue);

						currValLength += StringTrim(&szCurrentLine[2 + currValLength], pCurrValue);
						newNormal.y = String_ATOF(pCurrValue);

						currValLength += StringTrim(&szCurrentLine[2 + currValLength], pCurrValue);
						newNormal.z = String_ATOF(pCurrValue);

						normals.PushBack(newNormal);
						break;
					}

				case 't':	// Texture coord.
					{
						// NOTE: Right now we are expecting 3-component vectors from our OBJ files.
						Vector2D newTexCoord{};
						char pCurrValue[512];
					
						int currValLength = StringTrim(&szCurrentLine[2], pCurrValue);
						newTexCoord.x = String_ATOF(pCurrValue);

						currValLength += StringTrim(&szCurrentLine[2 + currValLength], pCurrValue);
						newTexCoord.y = String_ATOF(pCurrValue);

						texCoords.PushBack(newTexCoord);
						break;
					}
				}
			break;
			}

		case 'f':	// Polygon 'face'.
		{
			uint32_t vertexIndex[4]{};
			uint32_t texCoordIndex[4]{};
			uint32_t normalIndex[4]{};

			int elements = sscanf(szCurrentLine, "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &texCoordIndex[0], &normalIndex[0], &vertexIndex[1], &texCoordIndex[1], &normalIndex[1], &vertexIndex[2], &texCoordIndex[2], &normalIndex[2], &vertexIndex[3], &texCoordIndex[3], &normalIndex[3]);
			CAssert(elements == 9 || elements == 12);

			vertexIndices.PushBack(vertexIndex[0]);
			vertexIndices.PushBack(vertexIndex[1]);
			vertexIndices.PushBack(vertexIndex[2]);
			texCoordIndices.PushBack(texCoordIndex[0]);
			texCoordIndices.PushBack(texCoordIndex[1]);
			texCoordIndices.PushBack(texCoordIndex[2]);
			normalIndices.PushBack(normalIndex[0]);
			normalIndices.PushBack(normalIndex[1]);
			normalIndices.PushBack(normalIndex[2]);

			if (elements > 9)
			{
				vertexIndices.PushBack(vertexIndex[3]);
				texCoordIndices.PushBack(texCoordIndex[3]);
				normalIndices.PushBack(normalIndex[3]);
				for (int idx = 0; idx < 4; idx++)
				{
					drawingIndices.PushBack(currIndex + 0);
					drawingIndices.PushBack(currIndex + 1);
					drawingIndices.PushBack(currIndex + 3);
					drawingIndices.PushBack(currIndex + 1);
					drawingIndices.PushBack(currIndex + 2);
					drawingIndices.PushBack(currIndex + 3);
					currIndex += 4;
				}
			}
			else
			{
				for (int idx = 0; idx < 3; idx++)
				{
					drawingIndices.PushBack(currIndex);
					currIndex++;
				}
			}

			break;
		}

		default:
			//LOG_WARN("We have unfinished business in the OBJ Loader!");
			break;
		}

		currLine++;
	}
	LOG_INFO("%d", groups);

	FileSystem::CloseFile(&OBJFileHandle);

	// NOTE: This is a bit heavy to return by value, no?
	return newMesh;
}