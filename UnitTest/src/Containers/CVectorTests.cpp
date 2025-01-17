//=============================================================================
// Author: Jared Simons
// Date Created: 2/15/2024
// 
// Purpose: Unit tests for Utl::CVector.
//=============================================================================
#include "CVectorTests.h"

#include "Containers/CVector.h"
#include <vector>

int UTLCVector_CreateAndDestroy()
{
	// Create an empty vector.
	Utl::CVector<int> vec;

	if (vec.Capacity() != 0)
		return FAIL;
	if (vec.Count() != 0)
		return FAIL;
	if (vec.Data() != nullptr)
		return FAIL;

	return PASS;
}

int STDVector_CreateAndDestroy()
{
	std::vector<int> vec;

	if (vec.capacity() != 0)
		return FAIL;
	if (vec.size() != 0)
		return FAIL;
	if (vec.data() != nullptr)
		return FAIL;

	return PASS;
}

int UTLCVector_CreateAndAddElements()
{
	Utl::CVector<int> vec;
	vec.PushBack(1);
	vec.PushBack(2);
	vec.PushBack(3);
	vec.PushBack(4);
	vec.PushBack(5);

	if (vec.Data() == nullptr)
		return FAIL;
	if (vec.Capacity() != 8)
		return FAIL;
	if (vec.Count() != 5)
		return FAIL;

	return PASS;
}

int STDVector_CreateAndAddElements()
{
	std::vector<int> vec;
	vec.push_back(1);
	vec.push_back(2);
	vec.push_back(3);
	vec.push_back(4);
	vec.push_back(5);

	if (vec.data() == nullptr)
		return FAIL;
	// NOTE: STL handles resizing differently than our containers, (at least the MSVC implementation does)
	// so we aren't going to check for capacity here.
	if (vec.size() != 5)
		return FAIL;

	return PASS;
}

static CVectorUnitTests s_VectorUnitTests;
UnitTestGroup* g_pCVectorUnitTests = &s_VectorUnitTests;