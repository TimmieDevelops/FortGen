#pragma once
#include "framework.h"

class VersionInfo
{
public:
	static double EngineVersion;
	static int CL;
public:
	static void InitParseVersion();
};