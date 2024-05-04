//=============================================================================
// Author: Jared Simons
// Date Created: 5/2/2024
// 
// Purpose: Some utility memory functions to be used throughout the engine.
//=============================================================================
#pragma once

class CMemorySystem
{
public:
	bool MemorySystem_Initialize();
	void MemorySystem_Shutdown();

private:
	unsigned int	m_numAllocations = 0;
};

extern CMemorySystem* g_pMemorySystem;