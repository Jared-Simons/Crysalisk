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
#include <Strings/String_Utils.h>

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

	return cubeConfig;
}

class OBJModelLoader : public IModelLoader
{
public:
	virtual GeometryConfig_t CreateConfigFromFile(const char* pszFilePath) override;
};

// TODO: This should be part of a resource system!
// We should not be accessing this directly!
static OBJModelLoader s_OBJLoader;
IModelLoader* g_pOBJLoader = &s_OBJLoader;

GeometryConfig_t OBJModelLoader::CreateConfigFromFile(const char* pszFilePath)
{
	// Make sure the file exists!
	if (!FileSystem::FileExists(pszFilePath))
	{
		LOG_ERROR("Failed to find model '%s' to load! Nothing was done.", pszFilePath);
		return GeometryConfig_t{};
	}

	FileHandle_t OBJFileHandle;
	if (!FileSystem::OpenFile(pszFilePath, eFileMode::FILEMODE_READ, false, &OBJFileHandle))
	{
		return GeometryConfig_t{};
	}

	// Parse the file's data.
	GeometryConfig_t OBJConfig{};
	Utl::CVector<Vector3D>	vertexPositions{};
	Utl::CVector<Vector3D>	vertexNormals{};
	Utl::CVector<Vector3D>	vertexTexCoords{};
	Utl::CVector<uint32_t>	vertexIndices{};
	Utl::CVector<uint32_t>	normalIndices{};
	Utl::CVector<uint32_t>	texCoordIndices{};

	char szCurrentLine[kScratchStringBufferLength]{};
	char pFirstChar{};
	char pSecondChar{};
	size_t currLine{ 0 };
	size_t bytesRead{ 0 };

	while (!FileSystem::IsEOF(&OBJFileHandle))
	{
		FileSystem::ReadLine(szCurrentLine, kScratchStringBufferLength, &bytesRead, &OBJFileHandle);

		pFirstChar = szCurrentLine[0];
		pSecondChar = szCurrentLine[1];

		// TODO: Handle materiallib file.

		switch (pFirstChar)
		{
		case '#':	// Skip comments.
		case 0:		// And blank lines
			continue;

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

					vertexPositions.PushBack(newVertex);
					break;
				}

				case 'n':	// Vertex normal.
				{
					Vector3D newNormal{};
					char pCurrValue[512];

					int currValLength = StringTrim(&szCurrentLine[2], pCurrValue);
					newNormal.x = String_ATOF(pCurrValue);

					currValLength = StringTrim(&szCurrentLine[2 + currValLength], pCurrValue);
					newNormal.y = String_ATOF(pCurrValue);

					currValLength = StringTrim(&szCurrentLine[2 + currValLength], pCurrValue);
					newNormal.z = String_ATOF(pCurrValue);

					vertexNormals.PushBack(newNormal);
					break;
				}

				case 't':	// Vertex texcoords.
				{
					Vector3D newTexCoord{};
					char pCurrValue[512];

					int currValLength = StringTrim(&szCurrentLine[2], pCurrValue);
					newTexCoord.x = String_ATOF(pCurrValue);

					currValLength = StringTrim(&szCurrentLine[2 + currValLength], pCurrValue);
					newTexCoord.y = String_ATOF(pCurrValue);

					currValLength = StringTrim(&szCurrentLine[2 + currValLength], pCurrValue);
					newTexCoord.z = String_ATOF(pCurrValue);

					vertexTexCoords.PushBack(newTexCoord);
					break;
				}
			}

			break;

		case 'f':	// Vertex Indices, these are processed in groups of 3.
				{
				char szCurrValue[512];
				unsigned int currIndex{ 2 };
				unsigned int slashIndex{ 0 };
				for (int idx = 0; idx < 4; idx++)
				{
					// Position indices first.
					StringTrim(&szCurrentLine[currIndex], szCurrValue);
					slashIndex = StringIndexOf(&szCurrentLine[currIndex], '/');

					if (slashIndex == kInvalidID_U32)
					{
						// No more data on this line, break out.
						break;
					}

					szCurrValue[slashIndex] = 0;
					vertexIndices.PushBack(String_ATOI(szCurrValue));

					// Texcoords next.
					currIndex += slashIndex + 1;
					StringTrim(&szCurrentLine[currIndex], szCurrValue);
					slashIndex = StringIndexOf(&szCurrentLine[currIndex], '/');
					szCurrValue[slashIndex] = 0;
					texCoordIndices.PushBack(String_ATOI(szCurrValue));

					// Normals next.
					currIndex += slashIndex + 1;
					int currValLength = StringTrim(&szCurrentLine[currIndex], szCurrValue);
					slashIndex = StringIndexOf(&szCurrentLine[currIndex], '/');
					szCurrValue[currValLength - 1] = 0;
					normalIndices.PushBack(String_ATOI(szCurrValue));

					currIndex += currValLength;
				}

				break;
			}
		}

		default:
			//LOG_WARN("We have unfinished business in the OBJ Loader!");
			break;
		}

		currLine++;
	}

	OBJConfig.Vertices.Resize(vertexPositions.Count());
	OBJConfig.Indices.Resize(vertexIndices.Count());

	// Pool together all the data into our own config.
	for (int idx = 0; idx < vertexPositions.Count(); idx++)
	{
		OBJConfig.Vertices[idx].Pos = vertexPositions[idx];
	}

	for (int idx = 0; idx < vertexNormals.Count(); idx++)
	{
		OBJConfig.Vertices[idx].Normal = vertexNormals[idx];
	}

	for (int idx = 0; idx < vertexTexCoords.Count(); idx++)
	{
		OBJConfig.Vertices[idx].TexCoord = vertexTexCoords[idx].AsVector2D();
	}

	for (int idx = 0; idx < vertexIndices.Count(); idx++)
	{
		OBJConfig.Indices[idx] = vertexIndices[idx];
		//LOG_INFO("%d", OBJConfig.Indices[idx]);
	}

	FileSystem::CloseFile(&OBJFileHandle);

	// NOTE: This is a bit heavy to return by value, no?
	return OBJConfig;
}