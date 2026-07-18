#pragma once
#include "framework.h"
#include "../../../Core/Public/Containers/PropertyMarcos.h"
#include "Helpers/Public/Address.h"

/* FUObjectItem
* Object: 0x0
* ClusterAndFlags: 0x4
* SerialNumber: 0x8
* StructSize: 0xC
*/

/* FFixedUObjectArray
* Objects: 0x0 (resize)
* MaxElements: 0x4
* NumElements: 0x8
*/

/* FUObjectArray
* ObjObjects: 0x0
*/

/**
* Single item in the UObject array.
*/
struct FUObjectItem
{
	DEFINE_MEMBER(Object, Address::FUObjectItem_Object, class UObjectBase*)
	DEFINE_MEMBER(ClusterAndFlags, Address::FUObjectItem_ClusterAndFlags, int32_t)
	DEFINE_MEMBER(SerialNumber, Address::FUObjectItem_SerialNumber, int32_t)
	DEFINE_STRUCTSIZE(Address::FUObjectItem_StructSize)
};

/**
* Fixed size UObject array.
*/
class FFixedUObjectArray
{
public:
	DEFINE_MEMBER(Objects, Address::FFixedUObjectArray_Objects, FUObjectItem*)
	DEFINE_DATAINDEX(Objects, Address::FFixedUObjectArray_Objects, FUObjectItem*, FUObjectItem::GetStructSize())
	DEFINE_MEMBER(MaxElements, Address::FFixedUObjectArray_MaxElements, int32_t)
	DEFINE_MEMBER(NumElements, Address::FFixedUObjectArray_NumElements, int32_t)
};

/***
*
* FUObjectArray replaces the functionality of GObjObjects and UObject::Index
*
* Note the layout of this data structure is mostly to emulate the old behavior and minimize code rework during code restructure.
* Better data structures could be used in the future, for example maybe all that is needed is a TSet<UObject *>
* One has to be a little careful with this, especially with the GC optimization. I have seen spots that assume
* that non-GC objects come before GC ones during iteration.
*
**/
class FUObjectArray
{
public:
	typedef FFixedUObjectArray TUObjectArray;
	DEFINE_MEMBER_REF(ObjObjects, Address::FUObjectArray_ObjObjects, TUObjectArray)
};

extern FUObjectArray* GUObjectArray;