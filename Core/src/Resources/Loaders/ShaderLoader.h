//=============================================================================
// Author: Jared Simons
// Date Created: 5/21/2025
// 
// Purpose: Shader resource loading utilities.
//=============================================================================
#pragma once

#include "Containers/CVector.h"
#include <Defines.h>
#include "Strings/String_Utils.h"

namespace ShaderLoader
{
	CORE_API bool LoadShaderResource(const char* pszShaderPath, struct ShaderResource_t& pOutResource);

	struct ShaderResource_t
	{
		struct ShaderStageResource_t
		{
			char*	pszStageString{ nullptr };
			size_t	CodeDataSize{ 0 };
			byte*	pShaderCode{ nullptr };
		};

		char*								pszShaderName{ nullptr };
		Utl::CVector<ShaderStageResource_t>	ShaderStages{};

		~ShaderResource_t()
		{
			if (pszShaderName)
				StringFree(pszShaderName);
		}
	};
}