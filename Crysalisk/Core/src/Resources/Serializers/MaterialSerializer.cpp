//=============================================================================
// Author: Jared Simons
// Date Created: 6/14/2025
// 
// Purpose: Material file serialization utilities.
//=============================================================================
#include "MaterialSerializer.h"

#include "Platform/FileSystem.h"

namespace MaterialSerializer
{
	//-------------------------------------------------------------------------
	// Purpose: Takes an existing material resource imported from another file type, 
	// and writes it out to disk in our own material format.
	// 
	// Input:	pszMaterialName - The name of the material.
	//			pInResource - The resource data to be serialized.
	// Output:	bool - Returns true on successful serialization, otherwise false.
	//-------------------------------------------------------------------------
	bool SerializeMaterialResources(const char* pszMaterialName, MaterialLoader::MaterialResource_t& pInResource)
	{
		// Create and setup the file.

		// Chop off the file extension.
		size_t nameLength = Utl::StringLength(pszMaterialName);
		size_t pathLength = Utl::StringLength(kDefaultMaterialAssetPath);
		size_t extensionLength = Utl::StringLength(kMaterialFileExtension);

		char pTempNameBuffer[512];
		Utl::StringFormat_V("%s%s%s", pTempNameBuffer, kDefaultMaterialAssetPath, pszMaterialName, kMaterialFileExtension);

		FileHandle_t hCMaterialFile{};
		if (!FileSystem::OpenFile(pTempNameBuffer, eFileMode::FILEMODE_WRITE, false, &hCMaterialFile))
		{
			LOG_ERROR("Failed to write CMaterial for material '%s'.", pszMaterialName);
		}

		char tempDataBuffer[512];

		// Write out the material data.
		// First the name - this is required.
		Utl::StringFormat_V("name=%s\n", tempDataBuffer, pInResource.pszMaterialName);
		FileSystem::WriteLine(tempDataBuffer, &hCMaterialFile);

		// Next up the shader - this is also required.
		if (pInResource.pszShaderName == nullptr)
		{
			// Don't spew warnings but just state this once.
			LOG_WARN("No shader provided for material '%s'. Defaulting to 'Core.Phong'.", pInResource.pszMaterialName);

			// Default hardcoded shader.
			pInResource.pszShaderName = "Core.Phong";
		}

		tempDataBuffer[0] = 0;
		Utl::StringFormat_V("shader=%s\n", tempDataBuffer, pInResource.pszShaderName);
		FileSystem::WriteLine(tempDataBuffer, &hCMaterialFile);

		// Some texture type to string mappings.
		char	pszTexTypeStrings[3][10] = {	"diffuse",
												"specular",
												"normal" };	// NOTE: This maps directly to the MaterialLoader::MaterialResource_t::eTextureType enum!

		// Next are texture maps.
		for (int texIdx = 0; texIdx < pInResource.textureMapConfigs.Count(); texIdx++)
		{
			tempDataBuffer[0] = 0;
			CAssert(pInResource.textureMapConfigs[texIdx].eType != MaterialLoader::MaterialResource_t::eTextureType::UNDEFINED);

			Utl::StringFormat_V("map_%s=%s\n", tempDataBuffer, pszTexTypeStrings[(int)pInResource.textureMapConfigs[texIdx].eType], pInResource.textureMapConfigs[texIdx].pszTextureName);
			FileSystem::WriteLine(tempDataBuffer, &hCMaterialFile);
		}

		// Finally, write out any other key-value pairs.
		CAssert(pInResource.pszCustomKeys.Count() == pInResource.pszCustomValues.Count());
		for (int idx = 0; idx < pInResource.pszCustomKeys.Count(); idx++)
		{
			tempDataBuffer[0] = 0;

			Utl::StringFormat_V("%s=%s\n", tempDataBuffer, pInResource.pszCustomKeys[idx], pInResource.pszCustomValues[idx]);
			FileSystem::WriteLine(tempDataBuffer, &hCMaterialFile);
		}

		// Close the file
		FileSystem::CloseFile(&hCMaterialFile);

		return true;
	}
}