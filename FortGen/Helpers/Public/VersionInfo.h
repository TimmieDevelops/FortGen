#pragma once
#include "framework.h"
#include <string>

class VersionInfo
{
public:
    static std::string GetVersionString();
    static void ParseVersion(std::string& OutEngineVersion, std::string& OutChangelist);
};
