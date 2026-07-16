#include "pch.h"

double VersionInfo::EngineVersion = 0.0;
int VersionInfo::Changelist = 0;

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

void VersionInfo::InitParseVersion()
{
    std::string FullVersion = GetVersionString();
    if (FullVersion.empty())
    {
        EngineVersion = 0.0;
        Changelist = 0;
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
        std::string MajorMinorStr = (SecondDot != std::string::npos) ? VersionPart.substr(0, SecondDot) : VersionPart;
        try
        {
            EngineVersion = std::stod(MajorMinorStr);
        }
        catch (...)
        {
            EngineVersion = 0.0;
        }
    }
    else
    {
        try
        {
            EngineVersion = std::stod(VersionPart);
        }
        catch (...)
        {
            EngineVersion = 0.0;
        }
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
        try
        {
            Changelist = CLPart.empty() ? 0 : std::stoi(CLPart);
        }
        catch (...)
        {
            Changelist = 0;
        }
    }
    else
    {
        Changelist = 0;
    }
}
