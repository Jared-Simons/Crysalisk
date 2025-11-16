//=============================================================================
// Author: Jared Simons
// Date Created: 10/11/2025
// 
// Purpose: The debug renderer, used to draw simple primitives.
//=============================================================================
#pragma once

#include <Math/MathLib.h>
#include "Resources/ResourceTypes.h"

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
struct DebugRendererFrameData_t
{
	Matrix4x4	matProjection{};
	Matrix4x4	matView{};
	Vector3D	vecCameraPosition{};
	Vector3D	vecCameraForward{};
	float		flCurTime{ 0.0f };

	// DEBUGTODO: Is this the best way to handle this?
	Matrix4x4*	pTransformMatrices{ nullptr };
};

struct DebugRendererConfig_t
{
	unsigned int uMaxDebugGeometries{ 0 };
};

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
class IDebugRenderer
{
public:
	virtual bool	Initialize(DebugRendererConfig_t& pConfig) = 0;
	virtual bool	InitializeDebugShaders() = 0;
	virtual void	Shutdown() = 0;

	virtual void	Update() = 0;
	virtual void	Render(DebugRendererFrameData_t& pFrameData) = 0;

	virtual int		AddPoint(Vector& pOrigin, const float flRadius, Vector4D color, float flDuration) = 0;
	virtual int		AddLine(Vector3D ptStartPos, Vector3D ptEndPos, Vector4D color, float flDuration) = 0;
	virtual int		AddBBox(struct GeometryInstance_t& pGeometry, Vector4D color, float flDuration) = 0;
	virtual int		AddSphere(Vector& pOrigin, const float flRadius, Vector4D color, float flDuration) = 0;
	virtual int		AddText(Vector& pOrigin, FontHandle_t hFont, const char* pszText, Vector4D color, float flDuration) = 0;
	virtual int		AddCameraFrustum(struct Camera_t& pCamera, Vector4D color, float flDuration) = 0;
};

extern IDebugRenderer* g_pDebugRenderer;
