#include "pch.h"

uintptr_t Finder::FindGUObjectArray()
{
	auto StringRef = Scanner::FindString("SHOWPENDINGKILLS");
	if (!StringRef.IsValid())
	{
		Logger::Log(LogLevel::Warning, "[Finder::FindGUObjectArray]: StringRef is valid.");
		return -1;
	}

	/* OT6.5 (4.12)
	* A1 AC 3A DC 02
	*/
	auto Scanner = StringRef.ScanFor({ 0xA1, 0xAC, 0x3A });
	if (!Scanner.IsValid())
	{
		Logger::Log(LogLevel::Warning, "[Finder::FindGUObjectArray]: Scanner is valid.");
		return -1;
	}

	return Scanner.AbsoluteOffset().GetAddress();
}

uintptr_t Finder::FindUKismetSystemLibrary_GetEngineVersion()
{
	auto AddressRef = Scanner::FindPattern("6A ? FF 74 24 ? E8 ? ? ? ? 8B C8 E8 ? ? ? ? 8B 44 24"); // OT6.5 (UE4.12)
	if (!AddressRef.IsValid())
	{
		Logger::Log(LogLevel::Warning, "[Finder::FindUKismetSystemLibrary_GetEngineVersion]: AddressRef is valid.");
		return -1;
	}

	return AddressRef.GetAddress();
}

uintptr_t Finder::FindFMemory_Realloc()
{
	auto AddressRef = Scanner::FindPattern("8B 0D ? ? ? ? 85 C9 75 ? E8 ? ? ? ? 8B 0D ? ? ? ? FF 74 24 ? 8B 01 FF 74 24 ? FF 74 24"); // OT6.5(UE4.12)
	if (!AddressRef.IsValid())
	{
		Logger::Log(LogLevel::Warning, "[Finder::FindFMemory_Realloc]: AddressRef is valid.");
		return -1;
	}

	return AddressRef.GetAddress();
}

uintptr_t Finder::FindFName_ToString()
{
	auto AddressRef = Scanner::FindPattern("53 56 57 8B D9 E8 ? ? ? ? 8B 3B"); // OT6.5 (UE4.12)
	if (!AddressRef.IsValid())
	{
		Logger::Log(LogLevel::Warning, "[Finder::FindFName_ToString]: AddressRef is valid.");
		return -1;
	}

	return AddressRef.GetAddress();
}
