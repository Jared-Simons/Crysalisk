//=============================================================================
// Author: Jared Simons
// Date Created: 2/18/2025
// 
// Purpose: CSG Geometry Generation Tool
//=============================================================================
#include "GeometryGenerator.h"

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
	cubeConfig.Vertices[(faceIdx * 6) + 1].Pos = { vecOrigin.x +   halfWidth,  vecOrigin.y + (-halfHeight), vecOrigin.z + halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 2].Pos = { vecOrigin.x +   halfWidth,  vecOrigin.y +   halfHeight,  vecOrigin.z + halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 3].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y +   halfHeight,  vecOrigin.z + halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 4].Pos = { vecOrigin.x + (-halfWidth), vecOrigin.y + (-halfHeight), vecOrigin.z + halfDepth };
	cubeConfig.Vertices[(faceIdx * 6) + 5].Pos = { vecOrigin.x +   halfWidth,  vecOrigin.y + (-halfHeight), vecOrigin.z + halfDepth };

	cubeConfig.Vertices[(faceIdx * 6) + 0].TexCoord = { 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 1].TexCoord = { 1.0f, 1.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 2].TexCoord = { 1.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 3].TexCoord = { 0.0f, 0.0f };
	cubeConfig.Vertices[(faceIdx * 6) + 4].TexCoord = { 0.0f, 1.0f };
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

			cubeConfig.Vertices[(face * 6) + vertIdx].Bitangent.x = f * (-deltaUV2.x * edge1.x - deltaUV1.x * edge2.x);
			cubeConfig.Vertices[(face * 6) + vertIdx].Bitangent.y = f * (-deltaUV2.x * edge1.y - deltaUV1.x * edge2.y);
			cubeConfig.Vertices[(face * 6) + vertIdx].Bitangent.z = f * (-deltaUV2.x * edge1.z - deltaUV1.x * edge2.z);
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

			cubeConfig.Vertices[(face * 6) + 3 + vertIdx].Bitangent.x = f * (-deltaUV2.x * edge1.x - deltaUV1.x * edge2.x);
			cubeConfig.Vertices[(face * 6) + 3 + vertIdx].Bitangent.y = f * (-deltaUV2.x * edge1.y - deltaUV1.x * edge2.y);
			cubeConfig.Vertices[(face * 6) + 3 + vertIdx].Bitangent.z = f * (-deltaUV2.x * edge1.z - deltaUV1.x * edge2.z);
		}
	}

	return cubeConfig;
}