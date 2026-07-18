#include "pch.h"

std::string FName::ToString() const
{
	static void (__thiscall *ToStringOG)(const FName*, FString & Out) = reinterpret_cast<void(__thiscall*)(const FName*, FString&)>(Scanner::GetModuleBase() + Address::FName_ToString);
	FString Out;
	ToStringOG(this, Out);
	return Out.ToString();
}