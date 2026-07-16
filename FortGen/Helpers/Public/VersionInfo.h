#pragma once
#include "framework.h"
#include <string>

class VersionInfo
{
public:
    static double EngineVersion;
    static int CL;

    static std::string GetVersionString();
    static void InitParseVersion();
};
