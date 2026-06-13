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
// Purpose: Per-frame specific data needed to render debug geometries.
//-------------------------------------------------------------------------
struct DebugRendererFrameData_t
{
	Matrix4x4	matProjection{};
	Matrix4x4	matView{};
	Vector3D	vecCameraPosition{};
	Vector3D	vecCameraForward{};
	f32		flCurTime{ 0.0f };
};

//-------------------------------------------------------------------------
// Purpose: Debug renderer system config.
//-------------------------------------------------------------------------
struct DebugRendererConfig_t
{
	u32 uMaxDebugGeometries{ 0 };

	// A pointer to the render system.
	class IRenderer* pRenderer{ nullptr };
	// A pointer to the shader system.
	class IShaderSystem* pShaderSystem{ nullptr };
};

//-------------------------------------------------------------------------
// Purpose: Interface to the debug rendering system.
//-------------------------------------------------------------------------
class IDebugGeometrySystem
{
public:
	static void		AllocateDebugGeometrySystemState(IDebugGeometrySystem** ppSystemState);

	virtual bool	Initialize(DebugRendererConfig_t& pAppConfig) = 0;
	virtual bool	InitializeDebugShaders() = 0;
	virtual void	Shutdown() = 0;

	virtual void	Update() = 0; 
	virtual void	PopulateRenderPacket(Utl::CVector<GeometryInstance_t*>& pGeometries, RenderBuffer** pOutVertexBuffer, RenderBuffer** pOutIndexBuffer) = 0;

	virtual int		AddPoint(Vector& pOrigin, const f32 flRadius, Vector4D color, f32 flDuration) = 0;
	virtual int		AddLine(Vector3D ptStartPos, Vector3D ptEndPos, Vector4D color, f32 flDuration) = 0;
	virtual int		AddBBox(struct GeometryInstance_t& pGeometry, Vector4D color, f32 flDuration) = 0;
	virtual int		AddSphere(Vector& pOrigin, const f32 flRadius, Vector4D color, f32 flDuration) = 0;
	virtual int		AddText(class IFontSystem* pFontSystem, Vector& pOrigin, FontHandle_t hFont, const char* pszText, Vector4D color, f32 flDuration) = 0;
	virtual int		AddCameraFrustum(struct Camera_t& pCamera, Vector4D color, f32 flDuration) = 0;
	virtual void	RemoveDebugGeometry(int debugGeoHandle) = 0;

	virtual void	UpdateDebugGeometryTransform(u32 debugGeoHandle, Matrix4x4 matTransform) = 0;
};
