#include "pch.h"

std::string FName::ToString() const
{
	static void (*ToStringOG)(const FName*, FString & Out) = reinterpret_cast<void(*)(const FName*, FString&)>(Scanner::GetModuleBase() + Address::FName_ToString);
	FString Out;
	return Out.ToString();
}