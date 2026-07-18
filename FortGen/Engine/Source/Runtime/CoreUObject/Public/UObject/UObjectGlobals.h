#pragma once
#include "framework.h"
#include "Helpers/Public/Address.h"
#include "Helpers/Public/Scanner.h"

template<typename T = class UObject >
static T* StaticLoadObject(const std::string& Name, class UClass* StaticClass = T::StaticClass())
{
	static UObject* (*StaticLoadObjectOG)(class UClass* ObjectClass, class UObject* InOuter, const TCHAR * InName, const TCHAR * Filename, uint32_t LoadFlags, class UPackageMap* Sandbox, bool bAllowObjectReconciliation) = reinterpret_cast<UObject * (*)(class UClass*, class UObject*, const TCHAR*, const TCHAR*, uint32_t, class UPackageMap*, bool)>(Scanner::GetModuleBase() + Address::StaticLoadObject);
	return static_cast<T*>(StaticLoadObjectOG(StaticClass, nullptr, std::wstring(Name.begin(), Name.end()).c_str(), nullptr, 0, nullptr, false));
}

template<typename T = class UObject>
static T* StaticFindObject(const std::string& Name)
{
	static UObject* (*StaticFindObjectOG)(class UClass* Class, class UObject* InOuter, const TCHAR * Name, bool bExactClass) = reinterpret_cast<UObject * (*)(class UClass*, class UObject*, const TCHAR*, bool)>(Scanner::GetModuleBase() + Address::StaticFindObject);
	return static_cast<T*>(StaticFindObjectOG(nullptr, nullptr, std::wstring(Name.begin(), Name.end()).c_str(), false));
}