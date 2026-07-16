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