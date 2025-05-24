//=============================================================================
// Author: Jared Simons
// Date Created: 3/12/2025
// 
// Purpose: The shader system.
//=============================================================================
#pragma once

#include <Defines.h>
#include "Resources/ResourceTypes.h"

struct ShaderSystemConfig_t
{
	size_t	MaxShaders{ 0 };
};

class IShaderSystem
{
public:
	virtual bool	Initialize(ShaderSystemConfig_t config) = 0;
	virtual void	Shutdown() = 0;

	virtual ShaderHandle_t	AcquireShader(const char* pszShaderName) = 0;
	virtual void			ReleaseShader(ShaderHandle_t hShader) = 0;

	virtual void			BindShader(ShaderHandle_t hShader) = 0;
	virtual void			BindPerFrameUniforms() = 0;

	virtual uint16_t		GetShaderUniformLocation(ShaderHandle_t hShader, char* pszUniformName) = 0;
	virtual void			SetShaderUniform(ShaderHandle_t hShader, uint16_t uniformLocation, void* pData) = 0;

	virtual ShaderHandle_t	AcquireShader_OLD(const char* pszShaderName) = 0;

	virtual void	BindShader_OLD(ShaderHandle_t hShader) = 0;
};

DLL_EXPORT IShaderSystem* g_pShaderSystem;