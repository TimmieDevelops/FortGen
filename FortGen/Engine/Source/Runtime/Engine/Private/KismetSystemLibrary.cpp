#include "pch.h"

FString UKismetSystemLibrary::GetEngineVersion()
{
	static FString(*GetEngineVersionOG)() = reinterpret_cast<FString(*)()>(Scanner::GetModuleBase() + Address::UKismetSystemLibrary_GetEngineVersion);
	return GetEngineVersionOG();
}

FString UKismetSystemLibrary::GetPathName(const UObject* Object)
{
	static FString(*GetPathNameOG)(const UObject * Object) = reinterpret_cast<FString(*)(const UObject*)>(Scanner::GetModuleBase() + Address::UKismetSystemLibrary_GetPathName);
	return GetPathNameOG(Object);
}
