#include <stdio.h>
#include "Core/Logging.h"

__declspec(dllexport) void printInt(int x)
{
	LOG_VERBOSE("Test, %.4f", 1.2345);
	LOG_INFO("Test, %.4f", 1.2345);
	LOG_DEBUG("Test, %.4f", 1.2345);
	LOG_WARN("Test, %.4f", 1.2345);
	LOG_ERROR("Test, %.4f", 1.2345);
	LOG_FATAL("Test, %.4f", 1.2345);
}