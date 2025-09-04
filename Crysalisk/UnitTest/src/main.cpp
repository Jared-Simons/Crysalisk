//=============================================================================
// Author: Jared Simons
// Date Created: 5/10/2024
// 
// Purpose: Entry Point for the unit testing module
//=============================================================================
#include <Defines.h>

#include "Containers/CVectorTests.h"

bool PerformUnitTests()
{	
	g_pCVectorUnitTests->PerformTests();

	return true;
}

int main()
{
	PerformUnitTests();

	return 0;
}