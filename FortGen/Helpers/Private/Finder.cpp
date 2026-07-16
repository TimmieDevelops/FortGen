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
