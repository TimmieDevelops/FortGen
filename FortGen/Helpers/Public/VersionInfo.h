#pragma once
#include "framework.h"
#include <string>

class VersionInfo
{
public:
    static double EngineVersion;
    static int Changelist;

    static std::string GetVersionString();
    static void InitParseVersion();
};
