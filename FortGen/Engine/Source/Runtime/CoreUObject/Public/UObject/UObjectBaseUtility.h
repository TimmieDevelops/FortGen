#pragma once
#include "framework.h"
#include "UObjectBase.h"

class UObjectBaseUtility : public UObjectBase
{
public:
	std::string GetName() const;
	std::string GetFullName() const;
	std::string GetPathName() const;
	bool IsA(const class UClass* SomeBase) const;
	class UPackage* GetOutermost() const;
};