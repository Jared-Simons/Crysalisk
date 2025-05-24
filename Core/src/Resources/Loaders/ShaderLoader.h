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

	//-------------------------------------------------------------------------
	// TODO: Documentation
	//-------------------------------------------------------------------------
	struct ShaderResource_t
	{
		struct ShaderStageResource_t
		{
			char*				pszStageString{ nullptr };
			size_t				CodeDataSize{ 0 };
			Utl::CVector<byte>	pShaderModuleBytecode{};

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
			}

			ShaderStageResource_t& operator=(const ShaderStageResource_t& pOther)
			{
				if (pOther.pszStageString)
					pszStageString = StringDuplicate(pOther.pszStageString);
				CodeDataSize = pOther.CodeDataSize;
				pShaderModuleBytecode = pOther.pShaderModuleBytecode;

				return *this;
			}

			~ShaderStageResource_t()
			{
				StringFree(pszStageString);
			}
		};

		struct ShaderVertexAttributeConfig_t
		{
			char*	pszName{ nullptr };
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

		struct ShaderUniformConfig_t
		{
			char*	pszName{ nullptr };
			char*	pszType{ nullptr };
			char*	pszFreq{ nullptr };
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

		char*										pszShaderName{ nullptr };
		Utl::CVector<ShaderStageResource_t>			ShaderStages{};
		char*										pszTopologyType;
		bool										bDepthTestingEnabled{ false };
		bool										bColorBlendingEnabled{ false };
		Utl::CVector<ShaderVertexAttributeConfig_t>	ShaderAttributes{};
		Utl::CVector<ShaderUniformConfig_t>			ShaderUniforms{};

		~ShaderResource_t()
		{
			if (pszShaderName)
				StringFree(pszShaderName);
		}
	};
}