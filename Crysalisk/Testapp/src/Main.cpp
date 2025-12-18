//=============================================================================
// Author: Jared Simons
// Date Created: 14/2/2024
// 
// Purpose: The game executable's entrypoint.
//=============================================================================
#include <Application.h>

#include <Core/Logging.h>

class CGameApp : public ICrysApplication
{
public:
	virtual bool Application_Init() override;
	virtual bool Application_Run() override;
	virtual void Application_Shutdown() override;
};

static CGameApp s_GameApp{};

// Cryaslisk Engine entry point.
extern "C"
{
	__declspec(dllimport) int Crysalisk_Main(ApplicationConfig_t& pAppConfig);
}

int main(int argc, char** argv)
{
	ApplicationConfig_t appConfig{};
	appConfig.pApplication = &s_GameApp;

	Crysalisk_Main(appConfig);

	return 0;
}

bool CGameApp::Application_Init()
{
	LOG_DEBUG("Initializing Testapp");

	return true;
}

bool CGameApp::Application_Run()
{
	return true;
}

void CGameApp::Application_Shutdown()
{
}
