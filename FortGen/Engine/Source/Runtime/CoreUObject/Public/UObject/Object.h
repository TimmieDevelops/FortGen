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
	static class UClass* StaticClass();
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

	template<typename T>
	T GetProperty(int32_t Offset) const
	{
		return *reinterpret_cast<const T*>(reinterpret_cast<const uint8_t*>(this) + Offset);
	}

	template<typename T>
	void SetProperty(int32_t Offset, const T& Value)
	{
		*reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(this) + Offset) = Value;
	}
};