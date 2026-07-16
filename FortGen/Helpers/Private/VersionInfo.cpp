#include "pch.h"

std::string VersionInfo::GetVersionString()
{
    uintptr_t Address = Finder::FindUKismetSystemLibrary_GetEngineVersion();
    if (Address == 0 || Address == (uintptr_t)-1)
    {
        return "";
    }

    typedef FString* (__cdecl* GetEngineVersionFn)(FString* OutVersion);
    GetEngineVersionFn GetEngineVersion = reinterpret_cast<GetEngineVersionFn>(Address);

    FString TempVersion;
    GetEngineVersion(&TempVersion);

    std::string VersionStr = TempVersion.ToString();
    return VersionStr;
}

void VersionInfo::ParseVersion(std::string& OutEngineVersion, std::string& OutChangelist)
{
    std::string FullVersion = GetVersionString();
    if (FullVersion.empty())
    {
        OutEngineVersion = "Unknown";
        OutChangelist = "0";
        return;
    }

    // Find hyphen separating version and changelist
    size_t HyphenPos = FullVersion.find('-');
    std::string VersionPart = (HyphenPos != std::string::npos) ? FullVersion.substr(0, HyphenPos) : FullVersion;

    // Extract Major.Minor from VersionPart (e.g. "4.12.0" -> "4.12")
    size_t FirstDot = VersionPart.find('.');
    if (FirstDot != std::string::npos)
    {
        size_t SecondDot = VersionPart.find('.', FirstDot + 1);
        if (SecondDot != std::string::npos)
        {
            OutEngineVersion = VersionPart.substr(0, SecondDot);
        }
        else
        {
            OutEngineVersion = VersionPart;
        }
    }
    else
    {
        OutEngineVersion = VersionPart;
    }

    // Extract Changelist (CL) after the hyphen
    if (HyphenPos != std::string::npos)
    {
        std::string CLPart;
        for (size_t i = HyphenPos + 1; i < FullVersion.length(); ++i)
        {
            char c = FullVersion[i];
            if (c >= '0' && c <= '9')
            {
                CLPart.push_back(c);
            }
            else
            {
                break;
            }
        }
        OutChangelist = CLPart.empty() ? "0" : CLPart;
    }
    else
    {
        OutChangelist = "0";
    }
}
