//=============================================================================
// Author: Jared Simons
// Date Created: 10/11/2025
// 
// Purpose: The debug renderer, used to draw simple primitives.
//=============================================================================
#pragma once

#include <Math/MathLib.h>

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
struct DebugRendererFrameData_t
{
	Matrix4x4	matProjection{};
	Matrix4x4	matView{};
	Vector3D	vecCameraPosition{};
	Vector3D	vecCameraForward{};

	// DEBUGTODO: Is this the best way to handle this?
	Matrix4x4*	pTransformMatrices{ nullptr };
};

//-------------------------------------------------------------------------
// DEBUGTODO: Documentation.
//-------------------------------------------------------------------------
class IDebugRenderer
{
public:
	virtual bool	Initialize() = 0;
	virtual bool	InitializeDebugShaders() = 0;
	virtual void	Shutdown() = 0;

	virtual void	Render(DebugRendererFrameData_t& pFrameData) = 0;

	virtual int		AddBBox(struct GeometryInstance_t& pGeometry, Vector4D color) = 0;
	virtual int		AddCameraFrustum(struct Camera_t& pCamera, Vector4D color) = 0;
};

extern IDebugRenderer* g_pDebugRenderer;
