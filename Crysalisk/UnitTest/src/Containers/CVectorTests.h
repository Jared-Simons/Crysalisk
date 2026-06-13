//=============================================================================
// Author: Jared Simons
// Date Created: 2/15/2024
// 
// Purpose: Unit tests for Utl::CVector.
//=============================================================================
#pragma once

#include "UnitTestType.h"

// Test declarations.
i32 UTLCVector_CreateAndDestroy();
i32 STDVector_CreateAndDestroy();
i32 UTLCVector_CreateAndAddElements();
i32 STDVector_CreateAndAddElements();

class CVectorUnitTests : public UnitTestGroup
{
public:
	CVectorUnitTests()
		: UnitTestGroup("Utl::CVector unit tests")
	{
		RegisterTestFunc(UTLCVector_CreateAndDestroy, "CVector - Create and Destroy", false);
		RegisterTestFunc(STDVector_CreateAndDestroy, "STDVector - Create and Destroy", true);
		RegisterTestFunc(UTLCVector_CreateAndAddElements, "CVector - Create and add elements", false);
		RegisterTestFunc(STDVector_CreateAndAddElements, "STDVector - Create and add elements", true);
	}

private:
};

extern UnitTestGroup* g_pCVectorUnitTests;