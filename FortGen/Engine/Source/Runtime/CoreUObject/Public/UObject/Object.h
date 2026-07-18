#pragma once
#include "framework.h"
#include "UObjectBaseUtility.h"
#include "UObjectGlobals.h"

class UObject : public UObjectBaseUtility
{
public:
	std::string GetPackageName() const;
	std::string GetNameCPP() const;
public:
	template<typename T>
	T* Cast(class UClass* Class = T::StaticClass())
	{
		return (this && IsA(Class)) ? static_cast<T*>(this) : nullptr;
	}

	template<typename T>
	const T* Cast(class UClass* Class = T::StaticClass()) const
	{
		return (this && IsA(Class)) ? static_cast<const T*>(this) : nullptr;
	}
};