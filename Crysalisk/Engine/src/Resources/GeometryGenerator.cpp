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

//-------------------------------------------------------------------------
// Purpose: Creates a simple quad configuration that can be used for rendering.
// Input:	vecOrigin - The origin of the quad in world space.
//			flWidth - The width of the quad.
//			flHeight - The height of the quad.
// Output:	Returns a GeometryConfig_t that can be uploaded and used during rendering.
//-------------------------------------------------------------------------
GeometryConfig_t MeshGenerator::CreateDefaultQuadConfiguration(Vector3D_OLD vecOrigin, float flWidth, float flHeight)
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

	// Normals.
	quadConfig.Vertices[0].Normal = { 0.0f, 1.0f, 0.0f };
	quadConfig.Vertices[1].Normal = { 0.0f, 1.0f, 0.0f };
	quadConfig.Vertices[2].Normal = { 0.0f, 1.0f, 0.0f };
	quadConfig.Vertices[3].Normal = { 0.0f, 1.0f, 0.0f };
	quadConfig.Vertices[4].Normal = { 0.0f, 1.0f, 0.0f };
	quadConfig.Vertices[5].Normal = { 0.0f, 1.0f, 0.0f };

	// Make sure we get our tangents as well.
	GenerateTangentData(quadConfig);

	quadConfig.Vertices.SetCount(6);

	return quadConfig;
}

//-------------------------------------------------------------------------
// Purpose: Creates a simple cube configuration that can be used for rendering.
// Input:	vecOrigin - The origin of the cube in world space.
//			flWidth - The width of the cube.
//			flHeight - The height of the cube.
//			flDepth - The depth of the cube.
// Output:	Returns a GeometryConfig_t that can be uploaded and used during rendering.
//-------------------------------------------------------------------------
GeometryConfig_t MeshGenerator::CreateDefaultCubeConfiguration(Vector3D_OLD vecOrigin, float flWidth, float flHeight, float flDepth)
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

	cubeConfig.Vertices.SetCount(36);

	// Calculate tangent and bitangent data.
	GenerateTangentData(cubeConfig);

	return cubeConfig;
}

//-------------------------------------------------------------------------
// Purpose: Given a geometry configuration, generates tangent data necessary for lighting calculations.
// Input:	pConfig - The geometry config to generate tangents for.
// Output:	none.
//-------------------------------------------------------------------------
void MeshGenerator::GenerateTangentData(GeometryConfig_t& pConfig)
{
	size_t triCount = pConfig.Vertices.Count() / 3;
	{
		for (int triIdx = 0; triIdx < triCount; triIdx++)
		{
			Vector3D_OLD position1 = pConfig.Vertices[(triIdx * 3) + 0].Pos;
			Vector3D_OLD position2 = pConfig.Vertices[(triIdx * 3) + 1].Pos;
			Vector3D_OLD position3 = pConfig.Vertices[(triIdx * 3) + 2].Pos;

			Vector3D_OLD edge1 = position2 - position1;
			Vector3D_OLD edge2 = position3 - position1;
			Vector2D_OLD dUV1 = pConfig.Vertices[(triIdx * 3) + 1].TexCoord - pConfig.Vertices[(triIdx * 3) + 0].TexCoord;
			Vector2D_OLD dUV2 = pConfig.Vertices[(triIdx * 3) + 2].TexCoord - pConfig.Vertices[(triIdx * 3) + 0].TexCoord;

			float dividend = (dUV1.x * dUV2.y - dUV2.x * dUV1.y);
			float f = 1.0f / dividend;

			if (dividend < 0.001f && dividend > -0.001f)
			{
				LOG_WARN("Invalid tangent data detected.");
			}

			for (int vertIdx = 0; vertIdx < 3; vertIdx++)
			{
				pConfig.Vertices[(triIdx * 3) + vertIdx].Tangent.x = f * (dUV2.y * edge1.x - dUV1.y * edge2.x);
				pConfig.Vertices[(triIdx * 3) + vertIdx].Tangent.y = f * (dUV2.y * edge1.y - dUV1.y * edge2.y);
				pConfig.Vertices[(triIdx * 3) + vertIdx].Tangent.z = f * (dUV2.y * edge1.z - dUV1.y * edge2.z);

				pConfig.Vertices[(triIdx * 3) + vertIdx].Tangent.NormalizeInPlace();
			}
		}
	}
}