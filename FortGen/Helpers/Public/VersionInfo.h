#pragma once
#include "framework.h"
#include <string>

class VersionInfo
{
public:
    static std::string GetVersionString();
    static void InitParseVersion(std::string& OutEngineVersion, std::string& OutChangelist);
};
