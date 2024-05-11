#pragma once

#include "UnitTestType.h"

class TestUnitGroup;
class UnitTestGroup;

int MyTestFunc1()
{
	if (1 == 1)
		return PASS;
}

int MyTestFunc2()
{
	if (1 == 2)
		return PASS;
	else
		return FAIL;
}

class TestUnitGroup : public UnitTestGroup
{
public:
	TestUnitGroup(const char* debugTestName)
		: UnitTestGroup(debugTestName)
	{
		RegisterTestFunc(MyTestFunc1);
		RegisterTestFunc(MyTestFunc2);
	}
};

static TestUnitGroup MyTest("Basic Tests");
UnitTestGroup* MyTests = &MyTest;

