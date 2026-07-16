#include "pch.h"

FString UKismetSystemLibrary::GetEngineVersion()
{
	static FString(*GetEngineVersionOG)() = reinterpret_cast<FString(*)()>(Scanner::GetModuleBase() + Address::UKismetSystemLibrary_GetEngineVersion);
	return GetEngineVersionOG();
}
