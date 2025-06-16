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
#include "Systems/MaterialSystem.h"

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

void MeshGenerator::GenerateTangentData(GeometryConfig_t& pConfig)
{
	size_t triCount = pConfig.Vertices.Count() / 3;
	{
		for (int triIdx = 0; triIdx < triCount; triIdx++)
		{
			Vector3D position1 = pConfig.Vertices[(triIdx * 3) + 0].Pos;
			Vector3D position2 = pConfig.Vertices[(triIdx * 3) + 1].Pos;
			Vector3D position3 = pConfig.Vertices[(triIdx * 3) + 2].Pos;

			Vector3D edge1 = position2 - position1;
			Vector3D edge2 = position3 - position1;
			Vector2D dUV1 = pConfig.Vertices[(triIdx * 3) + 1].TexCoord - pConfig.Vertices[(triIdx * 3) + 0].TexCoord;
			Vector2D dUV2 = pConfig.Vertices[(triIdx * 3) + 2].TexCoord - pConfig.Vertices[(triIdx * 3) + 0].TexCoord;

			float dividend = (dUV1.x * dUV2.y - dUV2.x * dUV1.y);
			float f = 1.0f / dividend;

			for (int vertIdx = 0; vertIdx < 3; vertIdx++)
			{
				pConfig.Vertices[(triIdx * 3) + vertIdx].Tangent.x = f * (dUV2.y * edge1.x - dUV1.y * edge2.x);
				pConfig.Vertices[(triIdx * 3) + vertIdx].Tangent.y = f * (dUV2.y * edge1.y - dUV1.y * edge2.y);
				pConfig.Vertices[(triIdx * 3) + vertIdx].Tangent.z = f * (dUV2.y * edge1.z - dUV1.y * edge2.z);
			}
		}
	}
}

class OBJModelLoader : public IModelLoader
{
public:
	virtual Mesh CreateConfigFromFile(const char* pszFilePath) override;

private:
	void		ParseMaterialLibFile(const char* pszFilePath);

	void		WriteOBJToBinaryFile(Utl::CVector<GeometryConfig_t>& pConfigs, const char* pszOBJPath);
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
	Utl::CVector<GeometryConfig_t> newConfigs(1024); // TODO: Something in CVector's resize method is stomping on memory. If this thing needs to resize, it might completely ruin memory that comes before it. THIS NEEDS TO BE FIXED!
	Utl::CVector<Vector3D> vertices{};
	Utl::CVector<Vector3D> normals{};
	Utl::CVector<Vector2D> texCoords{};
	Utl::CVector<uint32_t> vertexIndices{};
	Utl::CVector<uint32_t> normalIndices{};
	Utl::CVector<uint32_t> texCoordIndices{};
	char* szCurrentMaterialName{};

	bool bBuildingPolygonList = false;

	char szCurrentLine[kScratchStringBufferLength]{};
	char pFirstChar{};
	char pSecondChar{};
	size_t currLine{ 1 };
	size_t bytesRead{ 0 };
	int groups{ 0 };
	//const int debugGroupLimit{ 109 };
	const int debugGroupLimit{ 1024 };
	int duplicateVertexCount{ 0 };

	while (!FileSystem::IsEOF(&OBJFileHandle))
	{
		FileSystem::ReadLine(szCurrentLine, kScratchStringBufferLength, &bytesRead, &OBJFileHandle);

		pFirstChar = szCurrentLine[0];
		pSecondChar = szCurrentLine[1];

		if (bBuildingPolygonList && (pFirstChar == 'o' || pFirstChar == 'g') && vertexIndices.Count() > 3 && groups <= debugGroupLimit)
		{
			GeometryConfig_t currentConfig{};
			// Flush the polygon list to the renderer and start a new config.
			// There was an active group that needs to be flushed to a config.
			for (int idx = 0; idx < vertexIndices.Count(); idx++)
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

			// Generate our tangent data.
			MeshGenerator::GenerateTangentData(currentConfig);

			// Acquire the geometry resources.
			size_t newGeometryIndex = newMesh.m_Geometries.PushBack(GeometryInstance_t{});
			newConfigs.PushBack(currentConfig);

			// Check for duplicate vertices
			// TODO: IMPLEMENT
			/*for (int idx = 0; idx < currentConfig.Vertices.Count(); idx++)
			{
				for (int currIdx = 0; currIdx < currentConfig.Vertices.Count(); currIdx++)
				{
					if (currentConfig.Vertices[currIdx] == currentConfig.Vertices[idx])
					{
						//LOG_DEBUG("Duplicate vertices found! (%d/%d)", currIdx, idx);
						duplicateVertexCount++;
					}
				}
			}*/

			// Acquire material resources.
			if (szCurrentMaterialName != nullptr)
			{
				//newMesh.m_Geometries[newGeometryIndex].hMaterial_OLD = g_pMaterialSystemInternal->AcquireMaterialInternal(szCurrentMaterialName);

				StringFree(szCurrentMaterialName);
				szCurrentMaterialName = nullptr;
			}

			g_pRenderer->AcquireGeometryResources(currentConfig, newMesh.m_Geometries[newGeometryIndex]);
			bBuildingPolygonList = false;
			groups++;

			// Clear existing data.
			vertexIndices.Clear();
			normalIndices.Clear();
			texCoordIndices.Clear();
		}

		switch (pFirstChar)
		{
		case '#':	// Skip comments.
		case 0:		// And blank lines
			break;

		case 'g':
		case 'o':
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
					
						size_t currValLength = StringTrimInPlace(&szCurrentLine[2], pCurrValue);
						newVertex.x = String_ATOF(pCurrValue);

						currValLength += StringTrimInPlace(&szCurrentLine[2 + currValLength], pCurrValue);
						newVertex.y = String_ATOF(pCurrValue);

						currValLength += StringTrimInPlace(&szCurrentLine[2 + currValLength], pCurrValue);
						newVertex.z = String_ATOF(pCurrValue);

						vertices.PushBack(newVertex);
						break;
					}

				case 'n':	// Vertex normal.
					{
						// NOTE: Right now we are expecting 3-component vectors from our OBJ files.
						Vector3D newNormal{};
						char pCurrValue[512];
					
						size_t currValLength = StringTrimInPlace(&szCurrentLine[2], pCurrValue);
						newNormal.x = String_ATOF(pCurrValue);

						currValLength += StringTrimInPlace(&szCurrentLine[2 + currValLength], pCurrValue);
						newNormal.y = String_ATOF(pCurrValue);

						currValLength += StringTrimInPlace(&szCurrentLine[2 + currValLength], pCurrValue);
						newNormal.z = String_ATOF(pCurrValue);

						normals.PushBack(newNormal);
						break;
					}

				case 't':	// Texture coord.
					{
						// NOTE: Right now we are expecting 3-component vectors from our OBJ files.
						Vector2D newTexCoord{};
						char pCurrValue[512];
					
						size_t currValLength = StringTrimInPlace(&szCurrentLine[2], pCurrValue);
						newTexCoord.x = String_ATOF(pCurrValue);

						currValLength += StringTrimInPlace(&szCurrentLine[2 + currValLength], pCurrValue);
						newTexCoord.y = String_ATOF(pCurrValue);

						texCoords.PushBack(newTexCoord);
						break;
					}
				}
			break;
			}

		case 'f':	// Polygon 'face'.
		{
			uint32_t vertexIndex[6]{};
			uint32_t texCoordIndex[6]{};
			uint32_t normalIndex[6]{};

			int elements = sscanf_s(szCurrentLine, "f %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &texCoordIndex[0], &normalIndex[0], &vertexIndex[1], &texCoordIndex[1], &normalIndex[1], &vertexIndex[2], &texCoordIndex[2], &normalIndex[2], &vertexIndex[3], &texCoordIndex[3], &normalIndex[3], &vertexIndex[4], &texCoordIndex[4], &normalIndex[4], &vertexIndex[5], &texCoordIndex[5], &normalIndex[5]);
			
			CAssert(elements == 9 || elements == 12 || elements == 15 || elements == 18);

			if (elements == 9)
			{
				vertexIndices.PushBack(vertexIndex[0]);
				vertexIndices.PushBack(vertexIndex[1]);
				vertexIndices.PushBack(vertexIndex[2]);
				texCoordIndices.PushBack(texCoordIndex[0]);
				texCoordIndices.PushBack(texCoordIndex[1]);
				texCoordIndices.PushBack(texCoordIndex[2]);
				normalIndices.PushBack(normalIndex[0]);
				normalIndices.PushBack(normalIndex[1]);
				normalIndices.PushBack(normalIndex[2]);
			} 
			else if (elements == 12)
			{
				vertexIndices.PushBack(vertexIndex[0]);
				vertexIndices.PushBack(vertexIndex[1]);
				vertexIndices.PushBack(vertexIndex[3]);
				texCoordIndices.PushBack(texCoordIndex[0]);
				texCoordIndices.PushBack(texCoordIndex[1]);
				texCoordIndices.PushBack(texCoordIndex[3]);
				normalIndices.PushBack(normalIndex[0]);
				normalIndices.PushBack(normalIndex[1]);
				normalIndices.PushBack(normalIndex[3]);

				vertexIndices.PushBack(vertexIndex[1]);
				vertexIndices.PushBack(vertexIndex[2]);
				vertexIndices.PushBack(vertexIndex[3]);
				texCoordIndices.PushBack(texCoordIndex[1]);
				texCoordIndices.PushBack(texCoordIndex[2]);
				texCoordIndices.PushBack(texCoordIndex[3]);
				normalIndices.PushBack(normalIndex[1]);
				normalIndices.PushBack(normalIndex[2]);
				normalIndices.PushBack(normalIndex[3]);
			}
			else if (elements == 18)
			{
				vertexIndices.PushBack(vertexIndex[0]);
				vertexIndices.PushBack(vertexIndex[1]);
				vertexIndices.PushBack(vertexIndex[2]);
				texCoordIndices.PushBack(texCoordIndex[0]);
				texCoordIndices.PushBack(texCoordIndex[1]);
				texCoordIndices.PushBack(texCoordIndex[2]);
				normalIndices.PushBack(normalIndex[0]);
				normalIndices.PushBack(normalIndex[1]);
				normalIndices.PushBack(normalIndex[2]);

				vertexIndices.PushBack(vertexIndex[3]);
				vertexIndices.PushBack(vertexIndex[4]);
				vertexIndices.PushBack(vertexIndex[5]);
				texCoordIndices.PushBack(texCoordIndex[3]);
				texCoordIndices.PushBack(texCoordIndex[4]);
				texCoordIndices.PushBack(texCoordIndex[5]);
				normalIndices.PushBack(normalIndex[3]);
				normalIndices.PushBack(normalIndex[4]);
				normalIndices.PushBack(normalIndex[5]);
			}
			else if (elements == 15)
			{
				vertexIndices.PushBack(vertexIndex[0]);
				vertexIndices.PushBack(vertexIndex[1]);
				vertexIndices.PushBack(vertexIndex[4]);
				texCoordIndices.PushBack(texCoordIndex[0]);
				texCoordIndices.PushBack(texCoordIndex[1]);
				texCoordIndices.PushBack(texCoordIndex[4]);
				normalIndices.PushBack(normalIndex[0]);
				normalIndices.PushBack(normalIndex[1]);
				normalIndices.PushBack(normalIndex[4]);

				vertexIndices.PushBack(vertexIndex[1]);
				vertexIndices.PushBack(vertexIndex[3]);
				vertexIndices.PushBack(vertexIndex[4]);
				texCoordIndices.PushBack(texCoordIndex[1]);
				texCoordIndices.PushBack(texCoordIndex[3]);
				texCoordIndices.PushBack(texCoordIndex[4]);
				normalIndices.PushBack(normalIndex[1]);
				normalIndices.PushBack(normalIndex[3]);
				normalIndices.PushBack(normalIndex[4]);
				
				vertexIndices.PushBack(vertexIndex[1]);
				vertexIndices.PushBack(vertexIndex[2]);
				vertexIndices.PushBack(vertexIndex[3]);
				texCoordIndices.PushBack(texCoordIndex[1]);
				texCoordIndices.PushBack(texCoordIndex[2]);
				texCoordIndices.PushBack(texCoordIndex[3]);
				normalIndices.PushBack(normalIndex[1]);
				normalIndices.PushBack(normalIndex[2]);
				normalIndices.PushBack(normalIndex[3]);
			}
			else
			{
				CAssert(0);
			}

			break;
		}

		case 'u':	// Material // TODO: Make this check better.
		{
			size_t spaceIndex = StringIndexOf(szCurrentLine, ' ');
			szCurrentMaterialName = StringDuplicate(&szCurrentLine[spaceIndex + 1]);
			break;
		}

		case 'm':	// Material Lib file. // TODO: Make this check better.
		{
			char* mtlLibPath{};
			size_t pathLength = StringLength(pszFilePath);
			mtlLibPath = StringDuplicate_N(pszFilePath, pathLength - 4);
			mtlLibPath = StringConcat(mtlLibPath, ".mtl");

			ParseMaterialLibFile(mtlLibPath);

			StringFree(mtlLibPath);
			break;
		}

		case 's':	// Smoothing groups.
			break;	// Nothing for now.

		default:
			LOG_WARN("We have unfinished business in the OBJ Loader!");
			break;
		}

		currLine++;
	}

	// Once we hit the end of the line, flush the last config and make sure it gets it's resources.
	if (bBuildingPolygonList && groups < debugGroupLimit)
	{
		GeometryConfig_t currentConfig{};
		// Flush the polygon list to the renderer and start a new config.
		// There was an active group that needs to be flushed to a config.
		for (int idx = 0; idx < vertexIndices.Count(); idx++)
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

		// Generate our tangent data.
		MeshGenerator::GenerateTangentData(currentConfig);

		// Acquire the geometry resources.
		size_t newGeometryIndex = newMesh.m_Geometries.PushBack(GeometryInstance_t{});

		// Acquire material resources.
		if (szCurrentMaterialName != nullptr)
		{
			//newMesh.m_Geometries[newGeometryIndex].hMaterial_OLD = g_pMaterialSystemInternal->AcquireMaterialInternal(szCurrentMaterialName);

			StringFree(szCurrentMaterialName);
			szCurrentMaterialName = nullptr;
		}

		g_pRenderer->AcquireGeometryResources(currentConfig, newMesh.m_Geometries[newGeometryIndex]);
		bBuildingPolygonList = false;
		groups++;
		newConfigs.PushBack(currentConfig);
	}
	//LOG_INFO("Number of duplicate vertices: %d", duplicateVertexCount);

	FileSystem::CloseFile(&OBJFileHandle);

	// TODO: Implement me!
	//LOG_INFO("Writing '%s' out to binary file.", pszFilePath);
	//WriteOBJToBinaryFile(newConfigs, pszFilePath);

	// NOTE: This is a bit heavy to return by value, no?
	return newMesh;
}

void OBJModelLoader::ParseMaterialLibFile(const char* pszFilePath)
{
	FileHandle_t fileHandle{};
	if (!FileSystem::OpenFile(pszFilePath, eFileMode::FILEMODE_READ, false, &fileHandle))
	{
		LOG_ERROR("Failed to find/open OBJ material library file '%s.'", pszFilePath);
		return;
	}

	char szCurrentLine[kScratchStringBufferLength]{};
	char pFirstChar{};
	char pSecondChar{};
	size_t currLine{ 1 };
	size_t bytesRead{ 0 };
	int groups{ 0 };
	char* pKey;
	char* pValue;

	MaterialConfig_t currentMaterialConfig{};

	size_t spaceIndex{ 0 };

	while (!FileSystem::IsEOF(&fileHandle))
	{
		FileSystem::ReadLine(szCurrentLine, kScratchStringBufferLength, &bytesRead, &fileHandle);

		szCurrentLine[bytesRead] = 0;
		spaceIndex = StringIndexOf(szCurrentLine, ' ');
		pFirstChar = szCurrentLine[0];
		pSecondChar = szCurrentLine[1];

		if (spaceIndex != kInvalidID_U32)
		{
			// Just in case the newline starts with a '\t'.
			int startIndex = (szCurrentLine[0] == '\t') ? 1 : 0;
			pKey = StringDuplicate_N(&szCurrentLine[startIndex], spaceIndex - 1);
			pValue = StringDuplicate(&szCurrentLine[spaceIndex + 1]);
		}

		if (szCurrentLine[0] == '\t')
		{
			pFirstChar = szCurrentLine[1];
			pSecondChar = szCurrentLine[2];
		}

		switch (pFirstChar)
		{
		case '#':	// Skip comments.
		case 0:		// And blank lines
			break;

		case 'n':
		case 'N':
			{
				switch (pSecondChar)
				{
					case 'e':
					{
						if (currentMaterialConfig.pszMaterialName != nullptr)
						{
							// A config currently exists, acquire it's resources now.
							/*if (g_pMaterialSystemInternal->AcquireMaterialFromConfig(currentMaterialConfig) == kInvalidID_U32)
							{
								LOG_ERROR("Failed to acquire material '%s'.", currentMaterialConfig.pszMaterialName);
							}*/
							//currentMaterialConfig.~MaterialConfig_t();
							currentMaterialConfig.vecDiffuseColor = { 0,0,0 };
						}
						//LOG_INFO("New Material '%s'.", &szCurrentLine[spaceIndex]);
						currentMaterialConfig.pszMaterialName = StringDuplicate(&szCurrentLine[spaceIndex + 1]);

						break;
					}

					case 's':	// Specular exponent.
					{
						currentMaterialConfig.iShininess = String_ATOI(&szCurrentLine[spaceIndex]);
						break;
					}
				}
				break;
			}

		case 'm':
			{
				switch (pSecondChar)
				{
					case 'a':
					{
						if (StringCompare(pKey, "map_Kd") || StringCompare(pKey, "map_Ka"))
						{
							currentMaterialConfig.pszDiffuseTextureName = StringDuplicate(pValue);
						}
						else if (StringCompare(pKey, "map_Ks"))
						{
							currentMaterialConfig.pszSpecularTextureName = StringDuplicate(pValue);
						}
						else if (StringCompare(pKey, "map_Disp") || StringCompare(pKey, "map_bump"))
						{
							currentMaterialConfig.pszNormalTextureName = StringDuplicate(pValue);
						}

						break;
					}
					break;
				}
				
				break;
			}

		case 'K':
			{
				switch (pSecondChar)
				{
					case 'd':
					{
						//sscanf(szCurrentLine, "Kd %f %f %f\n", &currentMaterialConfig.vecDiffuseColor.x, &currentMaterialConfig.vecDiffuseColor.y, &currentMaterialConfig.vecDiffuseColor.z);
						break;
					}
				}

				break;
			}

		default:
			break;
		}

		if (spaceIndex != kInvalidID_U32)
		{
			StringFree(pKey);
			StringFree(pValue);
		}

		currLine++;
	}

	// Flush the last config and acquire it's resources.
	/*if (g_pMaterialSystemInternal->AcquireMaterialFromConfig(currentMaterialConfig) == kInvalidID_U32)
	{
		LOG_ERROR("Failed to acquire material '%s'.", currentMaterialConfig.pszMaterialName);
	}*/
	//currentMaterialConfig.~MaterialConfig_t();
	currentMaterialConfig.vecDiffuseColor = { 0,0,0 };

	FileSystem::CloseFile(&fileHandle);
}

void OBJModelLoader::WriteOBJToBinaryFile(Utl::CVector<GeometryConfig_t>& pConfigs, const char* pszOBJPath)
{
	char* pNewFilePath = StringDuplicate(pszOBJPath);
	size_t pathLength = StringLength(pNewFilePath);
	pNewFilePath[pathLength - 4] = 0;
	StringFormat("%s.%s", pNewFilePath, "cmodel_c", pNewFilePath);

	FileHandle_t newFileHandle{};
	if (!FileSystem::OpenFile(pNewFilePath, eFileMode::FILEMODE_WRITE, true, &newFileHandle))
	{
		LOG_ERROR("Failed to write '%s' to binary file.", pNewFilePath);
		return;
	}

	FileSystem::CloseFile(&newFileHandle);
}