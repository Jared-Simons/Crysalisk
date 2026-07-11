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
	u64	MaxMaterials{ 0 };
	// A pointer to the texture system.
	class ITextureSystem* pTextureSystem{ nullptr };
	// A pointer to the shader system.
	class IShaderSystem* pShaderSystem{ nullptr };
};

//-------------------------------------------------------------------------
// Interface for the material system.
//-------------------------------------------------------------------------
class IMaterialSystem
{
public:
	static void					AllocateMaterialSystemState(IMaterialSystem** ppSystemState);

	// Initialization, shutdown.
	virtual bool				Initialize(MaterialSystemConfig_t config) = 0;
	virtual void				PostInitialize(struct EngineInterfaces_t* pEngineSystems) = 0;
	virtual void				Shutdown() = 0;

	// Acquires the resources for a given material.
	virtual MaterialHandle_t	AcquireMaterial(const char* pszMaterialName) = 0;
	// Acquires a material given an existing resource.
	virtual MaterialHandle_t	AcquireMaterialFromResource(struct MaterialLoader::MaterialResource_t& pInResource) = 0;

	virtual void				ReleaseMaterial(MaterialHandle_t hShaderInstance, bool bForceResourceRelease) = 0;

	// Returns true if a given material is transparent in anyway, false if completely opaque.
	// TODO: Make this return flags instead.
	virtual u32			IsMaterialTransparent(MaterialHandle_t hShaderInstance) const = 0;

	// Applies a given material.
	virtual void				ApplyMaterialInstance(MaterialHandle_t hShaderInstance) = 0;

	// Returns a handle to the default texture.
	virtual MaterialHandle_t	GetDefaultMaterialReference() = 0;

	// Applies global render data for the frame.
	virtual void				ApplyGlobalRenderData(Matrix4x4& pMatProjection, Matrix4x4& pMatView, Vector& pVecPosition, i32 renderDebugMode) = 0;
	// Applies per-draw render data for the following draw calls.
	virtual void				ApplyPerDrawRenderData(Matrix4x4& pMatModel) = 0;

	// Sets lighting data to be used during rendering.
	virtual void				SetDirectionalLightData(Vector3D vecDirection, Vector3D vecLightColor, f32 flBrightness, Vector3D vecAmbientLightColor, f32 flAmbientStrength) = 0;
	virtual void				SetPointLightData(i32 lightIndex, Vector3D vecPoint, Vector3D vecLightColor, f32 flBrightness, f32 flConstant, f32 flLinear, f32 flQuadratic) = 0;
	virtual void				SetSpotLightData(Vector3D vecPosition, Vector3D vecDirection, Vector3D vecLightColor, f32 flBrightness, f32 flConstant, f32 flLinear, f32 flQuadratic, f32 flInnerCutoff, f32 flOuterCutoff) = 0;
	virtual void				ApplyLightData() = 0;
};
