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
	// Attempts to load an existing shader asset from disk.
	CORE_API bool LoadShaderResource(const char* pszShaderPath, struct ShaderResource_t& pOutResource);

	// A shader resource generated from a configuration file on disk.
	// Can be used by the engine at runtime to create shaders.
	struct ShaderResource_t
	{
		// Represents the configuration for an individual shader stage.
		struct ShaderStageResource_t
		{
			// A string representing this stage (e.g. 'vert' or 'frag').
			char*				pszStageString{ nullptr };
			// The size in bytes of this module's bytecode.
			size_t				CodeDataSize{ 0 };
			// The bytecode for this shader stage.
			Utl::CVector<byte>	pShaderModuleBytecode{};
			// The name of the shader module.
			char*				pszModuleName{ nullptr };

			ShaderStageResource_t()
				: pszStageString{ nullptr }
				, CodeDataSize{ 0 }
				, pShaderModuleBytecode{} {}


			ShaderStageResource_t(const ShaderStageResource_t& pOther)
			{
				if (pOther.pszStageString)
					pszStageString = StringDuplicate(pOther.pszStageString);
				CodeDataSize = pOther.CodeDataSize;
				pShaderModuleBytecode = pOther.pShaderModuleBytecode;
				if (pOther.pszModuleName)
					pszModuleName = StringDuplicate(pOther.pszModuleName);
			}

			ShaderStageResource_t& operator=(const ShaderStageResource_t& pOther)
			{
				if (pOther.pszStageString)
					pszStageString = StringDuplicate(pOther.pszStageString);
				CodeDataSize = pOther.CodeDataSize;
				pShaderModuleBytecode = pOther.pShaderModuleBytecode;
				if (pOther.pszModuleName)
					pszModuleName = StringDuplicate(pOther.pszModuleName);

				return *this;
			}

			~ShaderStageResource_t()
			{
				StringFree(pszStageString);
				if (pszModuleName)
					StringFree(pszModuleName);

				pszModuleName = nullptr;
			}
		};

		// Represents the configuration for a single vertex attribute.
		struct ShaderVertexAttributeConfig_t
		{
			// The name of the attribute.
			char*	pszName{ nullptr };
			// The attribute's type.
			char*	pszType { nullptr };

			ShaderVertexAttributeConfig_t() {}

			ShaderVertexAttributeConfig_t(const ShaderVertexAttributeConfig_t& pOther)
			{
				if (pOther.pszName)
					pszName = StringDuplicate(pOther.pszName);

				if (pOther.pszType)
					pszType = StringDuplicate(pOther.pszType);
			}

			ShaderVertexAttributeConfig_t& operator=(const ShaderVertexAttributeConfig_t& pOther)
			{
				if (pOther.pszName)
					pszName = StringDuplicate(pOther.pszName);

				if (pOther.pszType)
					pszType = StringDuplicate(pOther.pszType);

				return *this;
			}

			~ShaderVertexAttributeConfig_t()
			{
				if (pszName)
					StringFree(pszName);

				if (pszType)
					StringFree(pszType);
			}
		};

		// Represents the configuration for a single shader uniform.
		struct ShaderUniformConfig_t
		{
			// The name of the uniform.
			char*	pszName{ nullptr };
			// The uniform's type.
			char*	pszType{ nullptr };
			// The frequency at which the uniform is updated (per-frame, per-instance, per-draw).
			char*	pszFreq{ nullptr };
			// The array size of this uniform.
			int		iArraySize{ 1 };

			ShaderUniformConfig_t() {}

			ShaderUniformConfig_t(const ShaderUniformConfig_t& pOther)
			{
				if (pOther.pszName)
					pszName = StringDuplicate(pOther.pszName);

				if (pOther.pszType)
					pszType = StringDuplicate(pOther.pszType);

				if (pOther.pszFreq)
					pszFreq = StringDuplicate(pOther.pszFreq);

				iArraySize = pOther.iArraySize;
			}

			ShaderUniformConfig_t& operator=(const ShaderUniformConfig_t& pOther)
			{
				if (pOther.pszName)
					pszName = StringDuplicate(pOther.pszName);

				if (pOther.pszType)
					pszType = StringDuplicate(pOther.pszType);

				if (pOther.pszFreq)
					pszFreq = StringDuplicate(pOther.pszFreq);

				iArraySize = pOther.iArraySize;

				return *this;
			}

			~ShaderUniformConfig_t()
			{
				if (pszName)
					StringFree(pszName);

				if (pszType)
					StringFree(pszType);

				if (pszFreq)
					StringFree(pszFreq);
			}
		};

		// The name of the shader.
		char*										pszShaderName{ nullptr };
		// An array of shader stage configurations.
		Utl::CVector<ShaderStageResource_t>			ShaderStages{};
		// The type of topology rendering this shader supports.
		char*										pszTopologyType;
		// Whether or not this shader will perform depth testing.
		bool										bDepthTestingEnabled{ false };
		// Whether or not this shader will perform color blending.
		bool										bColorBlendingEnabled{ false };
		// An array of shader vertex attribute configurations.
		Utl::CVector<ShaderVertexAttributeConfig_t>	ShaderAttributes{};
		// An array of shader uniform configurations.
		Utl::CVector<ShaderUniformConfig_t>			ShaderUniforms{};

		~ShaderResource_t()
		{
			if (pszShaderName)
				StringFree(pszShaderName);
		}
	};
}