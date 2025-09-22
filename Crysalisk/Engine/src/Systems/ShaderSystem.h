//=============================================================================
// Author: Jared Simons
// Date Created: 3/12/2025
// 
// Purpose: The shader system.
//=============================================================================
#pragma once

#include <Defines.h>
#include "Resources/ResourceTypes.h"

//-------------------------------------------------------------------------
// Configuration for the shader system.
//-------------------------------------------------------------------------
struct ShaderSystemConfig_t
{
	// The maximum number of shaders that can be loaded at once.
	size_t	MaxShaders{ 0 };
};

//-------------------------------------------------------------------------
// Interface for the shader system.
//-------------------------------------------------------------------------
class IShaderSystem
{
public:
	// Initialization, shutdown.
	virtual bool			Initialize(ShaderSystemConfig_t config) = 0;
	virtual void			Shutdown() = 0;

	// Acquires the resources for the provided shader name.
	virtual ShaderHandle_t	AcquireShader(const char* pszShaderName) = 0;
	// Releases an existing reference to a shader.
	virtual void			ReleaseShader(ShaderHandle_t hShader) = 0;

	// Acquires a set of instance level resources for a shader.
	virtual void			AcquireShaderInstanceResources(ShaderHandle_t hShader, unsigned int* pOutInstanceID) = 0;

	// Releases a set of instance level resources for a shader.
	virtual void			ReleaseShaderInstanceResources(ShaderHandle_t hShader, unsigned int pInstanceID) = 0;

	// Binds a given shader for use in rendering ops.
	virtual void			BindShader(ShaderHandle_t hShader) = 0;

	// Binds the currently-bound shader's per-frame uniforms.
	virtual void			BindPerFrameUniforms() = 0;
	// Binds the currently-bound shader's per-instance uniforms (for the provided instance).
	virtual void			BindPerInstanceUniforms(unsigned int hInstanceID) = 0;

	// Gets the location for a particular shader uniform.
	virtual uint16_t		GetShaderUniformLocation(ShaderHandle_t hShader, char* pszUniformName) = 0;
	// Sets a particular shader uniform.
	virtual void			SetShaderUniform(ShaderHandle_t hShader, uint16_t uniformLocation, unsigned int iUniformArrayIndex, unsigned int hInstanceID, void* pData) = 0;
};

// Global interface to the shader system context.
DLL_EXPORT IShaderSystem* g_pShaderSystem;