#include "pch.h"

double VersionInfo::EngineVersion = 0.0;
int VersionInfo::CL = 0;

void VersionInfo::InitParseVersion()
{
	std::string FullVersion = UKismetSystemLibrary::GetEngineVersion().ToString();
	if (FullVersion.empty())
	{
		EngineVersion = 0.0;
		CL = 0;
		return;
	}

	size_t HyphenPos = FullVersion.find('-');
	std::string VersionPart = (HyphenPos != std::string::npos) ? FullVersion.substr(0, HyphenPos) : FullVersion;

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

	Logger::Log(LogLevel::Info, std::format("[VersionInfo::InitParseVersion]: EngineVersion={}", EngineVersion).c_str());

	if (HyphenPos != std::string::npos)
	{
		std::string CLPart;

		for (size_t i = HyphenPos + 1; i < FullVersion.length(); ++i)
		{
			char c = FullVersion[i];
			if (c >= '0' && c <= '9')
				CLPart.push_back(c);
			else
				break;
		}

		try
		{
			CL = CLPart.empty() ? 0 : std::stoi(CLPart);
		}
		catch (...)
		{
			CL = 0;
		}
	}
	else
	{
		CL = 0;
	}

	Logger::Log(LogLevel::Info, std::format("[VersionInfo::InitParseVersion]: CLVersion={}", CL).c_str());
}
