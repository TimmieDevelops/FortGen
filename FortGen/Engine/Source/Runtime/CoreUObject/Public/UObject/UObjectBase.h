#pragma once
#include "framework.h"
#include "../../../Core/Public/UObject/NameTypes.h"
#include "../../../Core/Public/Containers/PropertyMarcos.h"
#include "Helpers/Public/Address.h"

/* UObjectBase
* ObjectFlags: 0x4 (int32_t)
* InternalIndex: 0x8 (int32_t)
* ClassPrivate: 0xC (class UClass*)
* NamePrivate: 0x10 (class FName)
* OuterPrivate: 0x18 (class UObject*)
*/

class UObjectBase
{
public:
	void** VTable;
	DEFINE_MEMBER(ObjectFlags, Address::UObjectBase_ObjectFlags, int32_t)
	DEFINE_MEMBER(InternalIndex, Address::UObjectBase_InternalIndex, int32_t)
	DEFINE_MEMBER(ClassPrivate, Address::UObjectBase_ClassPrivate, class UClass*)
	DEFINE_MEMBER(NamePrivate, Address::UObjectBase_NamePrivate, FName)
	DEFINE_MEMBER(OuterPrivate, Address::UObjectBase_OuterPrivate, class UObject*)
};

class UPackage : public UObjectBase {};

class UObject : public UObjectBase
{
public:
	std::string GetName() const;
	std::string GetFullName() const;
	std::string GetPathName() const;
	void GetPathName(const UObject* StopOuter, std::string& ResultString) const;
	class UPackage* GetOutermost() const;
};

class UClass : public UObject
{
public:
	class UObject* GetDefaultObject() const;
};

class UObject* StaticFindObject(class UClass* Class, class UObject* InOuter, const wchar_t* Name, bool ExactClass = false);
class UObject* StaticFindObject(class UClass* Class, class UObject* InOuter, const char* Name, bool ExactClass = false);