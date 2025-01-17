//=============================================================================
// Author: Jared Simons
// Date Created: 2/23/2025
// 
// Purpose: The texture manager.
//=============================================================================

#include "TextureSystem.h"

#include <Containers/CHashMap.h>
#include <Core/Logging.h>
#include <Memory/MemoryManager.h>

class CTextureSystem : public ITextureSystem
{
public:
	virtual bool	Initialize(TextureSystemConfig_t config) override;
	virtual void	Shutdown() override;

private:
	TextureSystemConfig_t	m_Config{};
	Utl::CHashMap<const char*, TextureHandle_t>	m_TextureHashmap{};
	Texture* m_TextureArray{ nullptr };
};

static CTextureSystem s_TextureSystem;
ITextureSystem* g_pTextureSystem = &s_TextureSystem;

bool CTextureSystem::Initialize(TextureSystemConfig_t config)
{
	m_Config = config;
	m_TextureHashmap.Resize(m_Config.MaxTextures);

	// Allocate memory for our texture array.
	m_TextureArray = (Texture*)g_pMemoryManager->Allocate(sizeof(Texture) * m_Config.MaxTextures);

	LOG_INFO("Texture system initialized! (Max textures: %d)", config.MaxTextures);

	return true;
}

void CTextureSystem::Shutdown()
{
	g_pMemoryManager->Free(m_TextureArray);
}