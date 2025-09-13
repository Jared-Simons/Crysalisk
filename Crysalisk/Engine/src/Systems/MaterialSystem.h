//=============================================================================
// Author: Jared Simons
// Date Created: 3/3/2025
// 
// Purpose: The material system.
//=============================================================================
#pragma once

#include <Defines.h>
#include <Resources/Loaders/MaterialLoader.h>
#include "Resources/ResourceTypes.h"

//-------------------------------------------------------------------------
// Configuration for the material system.
//-------------------------------------------------------------------------
struct MaterialSystemConfig_t
{
	// The maximum number of materials that can be loaded at once.
	size_t	MaxMaterials{ 0 };
};

//-------------------------------------------------------------------------
// Interface for the material system.
//-------------------------------------------------------------------------
class IMaterialSystem
{
public:
	// Initialization, shutdown.
	virtual bool				Initialize(MaterialSystemConfig_t config) = 0;
	virtual void				Shutdown() = 0;

	// Acquires the resources for a given material.
	virtual MaterialHandle_t	AcquireMaterial(const char* pszMaterialName) = 0;
	// Acquires a material given an existing resource.
	virtual MaterialHandle_t	AcquireMaterialFromResource(struct MaterialLoader::MaterialResource_t& pInResource) = 0;

	// Returns true if a given material is transparent in anyway, false if completely opaque.
	// TODO: Make this return flags instead.
	virtual uint32_t			IsMaterialTransparent(MaterialHandle_t hMaterial) const = 0;

	// Applies a given material.
	virtual void				ApplyMaterialInstance(MaterialHandle_t hMaterial) = 0;

	// Returns a handle to the default texture.
	virtual MaterialHandle_t	GetDefaultMaterialReference() = 0;

	// Applies global render data for the frame.
	virtual void				ApplyGlobalRenderData(Matrix4x4& pMatProjection, Matrix4x4& pMatView, Vector3D& pVecPosition, int renderDebugMode) = 0;
	// Applies per-draw render data for the following draw calls.
	virtual void				ApplyPerDrawRenderData(Matrix4x4& pMatModel) = 0;

	// Sets lighting data to be used during rendering.
	virtual void				SetDirectionalLightData(Vector3D vecDirection, Vector3D vecLightColor, Vector3D vecAmbientLightColor, float flAmbientStrength) = 0;
	virtual void				SetPointLightData(int lightIndex, Vector3D vecPoint, Vector3D vecLightColor, float flBrightness, float flConstant, float flLinear, float flQuadratic) = 0;
	virtual void				ApplyLightData() = 0;
};

// Global interface to the material system context.
DLL_EXPORT IMaterialSystem* g_pMaterialSystemInternal;