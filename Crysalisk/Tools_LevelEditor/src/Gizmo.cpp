//=============================================================================
// Author: Jared Simons
// Date Created: 7/23/2026
// 
// Purpose: Editor gizmo.
//=============================================================================
#include "Gizmo.h"

// Core
#include <Core/Assertions.h>
#include <Math/Ray.h>

// Engine
#include <Application.h>
#include <Core/Engine.h>
#include <Renderer/Renderer.h>
#include <Systems/ShaderSystem.h>

static const f32 GIZMO_RADIUS = 0.25f;
static const f32 GIZMO_MINS_EPSILON = 0.01f;

bool Gizmo_Create(class IRenderer* pRenderer, Gizmo* pOutGizmo)
{
	CAssert(pRenderer && pOutGizmo);

	// Create gizmo geometry.
	Vector verts[6]{};
	verts[0] = { 0.0f, 0.0f, 0.0f };
	verts[1] = { GIZMO_RADIUS, 0.0f, 0.0f };
	verts[2] = { 0.0f, 0.0f, 0.0f };
	verts[3] = { 0.0f, GIZMO_RADIUS, 0.0f };
	verts[4] = { 0.0f, 0.0f, 0.0f };
	verts[5] = { 0.0f, 0.0f, GIZMO_RADIUS };

	// Lift the gizmo off the ground slightly so it doesn't z-fight with the grid.
	for (u8 iVert = 0; iVert < 6; iVert++)
	{
		verts[iVert].y += 0.0001;
	}

	Vector4D colors[6]{};
	colors[0] = { 1.0f, 0.0f, 0.0f, 1.0f };
	colors[1] = { 1.0f, 0.0f, 0.0f, 1.0f };
	colors[2] = { 0.0f, 1.0f, 0.0f, 1.0f };
	colors[3] = { 0.0f, 1.0f, 0.0f, 1.0f };
	colors[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
	colors[5] = { 0.0f, 0.0f, 1.0f, 1.0f };

	// Create the vertex stream.
	// Vertex format is [Pos: x y z | Color: r g b a]
	u64 numFloats = (3 * 6) + (4 * 6);	// 3 floats per position vector * 6 verts + 4 floats per color * 6 verts.
	Utl::CVector<f32> vertexData(numFloats);

	for (u8 iVert = 0; iVert < 6; iVert++)
	{
		vertexData.PushBack(verts[iVert].x);
		vertexData.PushBack(verts[iVert].y);
		vertexData.PushBack(verts[iVert].z);
		vertexData.PushBack(colors[iVert].x);
		vertexData.PushBack(colors[iVert].y);
		vertexData.PushBack(colors[iVert].z);
		vertexData.PushBack(colors[iVert].w);
	}

	RenderBuffer* pVertexBuffer = pRenderer->GetRenderBufferByName("Debug_Renderer_Line_Vertex_Buffer");
	RenderBuffer* pIndexBuffer = pRenderer->GetRenderBufferByName("Debug_Renderer_Line_Index_Buffer");

	pRenderer->AcquireGeometryResources(*pVertexBuffer, *pIndexBuffer, 6, sizeof(Vector) + sizeof(Vector4D), 0, vertexData.Data(), 0, nullptr, &pOutGizmo->geometry);

	// Acquire new shader instance resources.
	IShaderSystem* pShaderSystem = g_pEngine->GetEngineInterfaces()->pShaderSystem;
	ShaderHandle_t hShader = pShaderSystem->AcquireShader("Debug.Line.VertColor");
	pOutGizmo->geometry.hShader = hShader;

	pOutGizmo->mins[0] = { -GIZMO_RADIUS, -GIZMO_MINS_EPSILON, -GIZMO_MINS_EPSILON };
	pOutGizmo->mins[1] = { -GIZMO_MINS_EPSILON, -GIZMO_RADIUS, -GIZMO_MINS_EPSILON };
	pOutGizmo->mins[2] = { -GIZMO_MINS_EPSILON, -GIZMO_MINS_EPSILON, -GIZMO_RADIUS };

	pOutGizmo->maxs[0] = { GIZMO_RADIUS, GIZMO_MINS_EPSILON, GIZMO_MINS_EPSILON };
	pOutGizmo->maxs[1] = { GIZMO_MINS_EPSILON, GIZMO_RADIUS, GIZMO_MINS_EPSILON };
	pOutGizmo->maxs[2] = { GIZMO_MINS_EPSILON, GIZMO_MINS_EPSILON, GIZMO_RADIUS };

	return true;
}

void Gizmo_Destroy(class IRenderer* pRenderer, Gizmo* pGizmo)
{
	CAssert(pRenderer && pGizmo);

	// TODO: Actually release resources.

	Platform::MemorySet(pGizmo, 0, sizeof(Gizmo));
}

void Gizmo_Update(Gizmo* pGizmo)
{
	CAssert(pGizmo);

	if (pGizmo->geometryDirty)
	{
		// Create the vertex stream.
		// Vertex format is [Pos: x y z | Color: r g b a]
		u64 numFloats = (3 * 6) + (4 * 6);	// 3 floats per position vector * 6 verts + 4 floats per color * 6 verts.
		Utl::CVector<f32> vertexData(numFloats);

		Vector verts[6]{};
		verts[0] = { 0.0f, 0.0f, 0.0f };
		verts[1] = { GIZMO_RADIUS, 0.0f, 0.0f };
		verts[2] = { 0.0f, 0.0f, 0.0f };
		verts[3] = { 0.0f, GIZMO_RADIUS, 0.0f };
		verts[4] = { 0.0f, 0.0f, 0.0f };
		verts[5] = { 0.0f, 0.0f, GIZMO_RADIUS };
		
		if (pGizmo->selectedAxis == 0)
		{
			// X-axis hovered.
			Vector4D colors[6]{};
			colors[0] = { 1.0f, 1.0f, 0.0f, 1.0f };
			colors[1] = { 1.0f, 1.0f, 0.0f, 1.0f };
			colors[2] = { 0.0f, 1.0f, 0.0f, 1.0f };
			colors[3] = { 0.0f, 1.0f, 0.0f, 1.0f };
			colors[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
			colors[5] = { 0.0f, 0.0f, 1.0f, 1.0f };

			for (u8 iVert = 0; iVert < 6; iVert++)
			{
				vertexData.PushBack(verts[iVert].x);
				vertexData.PushBack(verts[iVert].y);
				vertexData.PushBack(verts[iVert].z);
				vertexData.PushBack(colors[iVert].x);
				vertexData.PushBack(colors[iVert].y);
				vertexData.PushBack(colors[iVert].z);
				vertexData.PushBack(colors[iVert].w);
			}
		}
		else if (pGizmo->selectedAxis == 1)
		{
			// Y-axis hovered.
			Vector4D colors[6]{};
			colors[0] = { 1.0f, 0.0f, 0.0f, 1.0f };
			colors[1] = { 1.0f, 0.0f, 0.0f, 1.0f };
			colors[2] = { 1.0f, 1.0f, 0.0f, 1.0f };
			colors[3] = { 1.0f, 1.0f, 0.0f, 1.0f };
			colors[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
			colors[5] = { 0.0f, 0.0f, 1.0f, 1.0f };

			for (u8 iVert = 0; iVert < 6; iVert++)
			{
				vertexData.PushBack(verts[iVert].x);
				vertexData.PushBack(verts[iVert].y);
				vertexData.PushBack(verts[iVert].z);
				vertexData.PushBack(colors[iVert].x);
				vertexData.PushBack(colors[iVert].y);
				vertexData.PushBack(colors[iVert].z);
				vertexData.PushBack(colors[iVert].w);
			}
		}
		else if (pGizmo->selectedAxis == 2)
		{
			// Z-axis hovered.
			Vector4D colors[6]{};
			colors[0] = { 1.0f, 0.0f, 0.0f, 1.0f };
			colors[1] = { 1.0f, 0.0f, 0.0f, 1.0f };
			colors[2] = { 0.0f, 1.0f, 0.0f, 1.0f };
			colors[3] = { 0.0f, 1.0f, 0.0f, 1.0f };
			colors[4] = { 1.0f, 1.0f, 0.0f, 1.0f };
			colors[5] = { 1.0f, 1.0f, 0.0f, 1.0f };

			for (u8 iVert = 0; iVert < 6; iVert++)
			{
				vertexData.PushBack(verts[iVert].x);
				vertexData.PushBack(verts[iVert].y);
				vertexData.PushBack(verts[iVert].z);
				vertexData.PushBack(colors[iVert].x);
				vertexData.PushBack(colors[iVert].y);
				vertexData.PushBack(colors[iVert].z);
				vertexData.PushBack(colors[iVert].w);
			}
		}
		else
		{
			// Nothing selected!
			Vector verts[6]{};
			verts[0] = { 0.0f, 0.0f, 0.0f };
			verts[1] = { GIZMO_RADIUS, 0.0f, 0.0f };
			verts[2] = { 0.0f, 0.0f, 0.0f };
			verts[3] = { 0.0f, GIZMO_RADIUS, 0.0f };
			verts[4] = { 0.0f, 0.0f, 0.0f };
			verts[5] = { 0.0f, 0.0f, GIZMO_RADIUS };

			Vector4D colors[6]{};
			colors[0] = { 1.0f, 0.0f, 0.0f, 1.0f };
			colors[1] = { 1.0f, 0.0f, 0.0f, 1.0f };
			colors[2] = { 0.0f, 1.0f, 0.0f, 1.0f };
			colors[3] = { 0.0f, 1.0f, 0.0f, 1.0f };
			colors[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
			colors[5] = { 0.0f, 0.0f, 1.0f, 1.0f };

			for (u8 iVert = 0; iVert < 6; iVert++)
			{
				vertexData.PushBack(verts[iVert].x);
				vertexData.PushBack(verts[iVert].y);
				vertexData.PushBack(verts[iVert].z);
				vertexData.PushBack(colors[iVert].x);
				vertexData.PushBack(colors[iVert].y);
				vertexData.PushBack(colors[iVert].z);
				vertexData.PushBack(colors[iVert].w);
			}
		}

		IRenderer* pRenderer = g_pEngine->GetEngineInterfaces()->pRenderer;
		RenderBuffer* pVertexBuffer = pRenderer->GetRenderBufferByName("Debug_Renderer_Line_Vertex_Buffer");
		RenderBuffer* pIndexBuffer = pRenderer->GetRenderBufferByName("Debug_Renderer_Line_Index_Buffer");

		pRenderer->UpdateGeometryResources(pVertexBuffer, pIndexBuffer, 6, sizeof(Vector) + sizeof(Vector4D), vertexData.Data(), 0, nullptr, pGizmo->geometry);

		pGizmo->geometryDirty = false;
	}
}

void Gizmo_BeginInteraction(struct Gizmo* pGizmo, Ray_t* pRay)
{
	pGizmo->dragging = true;
	pGizmo->lastDragPos = pRay->vecEnd;

	// Setup the interaction plane.
	Vector normal{ 0.0f, 0.0f, 0.0f };
	Vector cameraBack = -(g_pEngine->GetApplication()->GetMainCamera()->vecFront);

	switch (pGizmo->selectedAxis)
	{
	case 0:	// X-Axis
		normal = { 0.0f, 0.0f, -1.0f };
		break;
	case 1:	// Y-Axis
		normal = cameraBack;
		break;
	case 2: // Z-Axis
		normal = { 1.0f, 0.0f, 0.0f };
		break;
	}

	pGizmo->interactionPlane = Plane::PlaneCreate(pGizmo->origin, normal);
}

void Gizmo_EndInteraction(struct Gizmo* pGizmo)
{
	pGizmo->dragging = false;

	// Clear out the interaction plane state.
	Platform::MemorySet(&pGizmo->interactionPlane, 0, sizeof(Plane));
}

void Gizmo_HandleInteraction(struct Gizmo* pGizmo, struct Camera_t* pCamera)
{
	CAssert(pGizmo && pCamera);

	// Cast a ray and see if we intersect with the gizmo.
	i32 iMouseX = 0;
	i32 iMouseY = 0;
	IInputSystem* pInputSystem = g_pEngine->GetEngineInterfaces()->pInputSystem;
	pInputSystem->GetCurrentMousePos(iMouseX, iMouseY);

	Ray_t ray = GetRayFromScreen(g_pEngine->GetEngineWindow(), pCamera, iMouseX, iMouseY);

	Vector4D vecLocalDirection = ray.vecDirection.ToVector4D();
	vecLocalDirection.w = 0.0f;

	if (!RayIntersectsPlane(&ray, &pGizmo->interactionPlane))
	{
		// TODO: Back-face/plane check.
	}

	// Take the delta between the current gizmo position and new ray intersection.
	Vector prevPos = pGizmo->lastDragPos;
	Vector newPos = ray.vecEnd;
	Vector delta = newPos - prevPos;

	// Move the gizmo along the plane that contains the selected axis.
	pGizmo->origin.flElements[pGizmo->selectedAxis] += delta.flElements[pGizmo->selectedAxis];

	// Update the last drag position for the next drag event.
	pGizmo->lastDragPos = newPos;
}

bool Gizmo_Raycast(struct Gizmo* pGizmo, struct Ray_t* pRay, u8* pOutHitAxis)
{
	CASSERTMSG(pGizmo && pRay && pOutHitAxis, "Gizmo_Raycast() - requires valid pointers to a gizmo, ray, and int to store the hit axis!");

	*pOutHitAxis = kInvalidID_U8;

	// Convert the ray to the gizmo's local space.
	Ray_t localRay;
	Matrix4x4 gizmoWorldToLocal = pGizmo->geometry.matLocalToWorldTransform.Inverse();
	Vector4D vecDirection = pRay->vecDirection.ToVector4D();
	vecDirection.w = 0.0f;

	localRay.vecOrigin = gizmoWorldToLocal * pRay->vecOrigin;
	localRay.vecDirection = (gizmoWorldToLocal * vecDirection).AsVector3D();

	// These properties are already in local space.
	Vector vecCenter = pGizmo->geometry.vecCenter;
	for (u8 iAxis = 0; iAxis < 3; iAxis++)
	{
		Vector vecMins = pGizmo->mins[iAxis];
		Vector vecMaxs = pGizmo->maxs[iAxis];

		f32 flTMin = kFLT_MAX;
		f32 flTMax = kFLT_MIN;

		Vector tMin = (vecMins - localRay.vecOrigin) / localRay.vecDirection;
		Vector tMax = (vecMaxs - localRay.vecOrigin) / localRay.vecDirection;

		Vector tNear = { FloatMin(tMin.x, tMax.x), FloatMin(tMin.y, tMax.y), FloatMin(tMin.z, tMax.z) };
		Vector tFar = { FloatMax(tMin.x, tMax.x), FloatMax(tMin.y, tMax.y), FloatMax(tMin.z, tMax.z) };

		f32 flTNearMax = FloatMax(FloatMax(tNear.x, tNear.y), tNear.z);
		f32 flTFarMin = FloatMin(FloatMin(tFar.x, tFar.y), tFar.z);

		if ((flTNearMax <= flTFarMin) && (flTNearMax < pRay->flDistance) && (flTNearMax > 0.0f))	// NOTE: In this particular test, the distance should always be > 0, as the objects in the view frustum should be along the front of the ray
		{																							// (the transforms involved in calculating the ray direction already accounts for Camera Forward = -Z axis).
			pRay->bDidHit = true;
			pRay->flDistance = flTNearMax;
			pRay->vecEnd = pRay->vecOrigin + (pRay->vecDirection * pRay->flDistance);

			// Return the axis hit.
			*pOutHitAxis = iAxis;
		}
	}

	if (*pOutHitAxis != kInvalidID_U8)
	{
		return true;
	}
	else
	{
		return false;
	}
}