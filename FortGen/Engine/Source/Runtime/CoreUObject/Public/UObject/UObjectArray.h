#pragma once
#include "framework.h"
#include "Helpers/Public/Address.h"
#include "../../../Core/Public/Containers/PropertyMarcos.h"

struct FUObjectItem
{
	UE_UPROPERTY_OFFSET(class UObjectBase*, Object, Address::UObjectItem_Object)
	UE_UPROPERTY_OFFSET(int32_t, ClusterAndFlags, Address::UObjectItem_ClusterAndFlags)
	UE_UPROPERTY_OFFSET(int32_t, SerialNumber, Address::UObjectItem_SerialNumber)
};

/**
* Fixed size UObject array.
*/
class FFixedUObjectArray
{
public:
	UE_UPROPERTY_OFFSET(FUObjectItem*, Objects, Address::FixedUObjectArray_Objects)
	UE_UPROPERTY_OFFSET(int32_t, MaxElements, Address::FixedUObjectArray_MaxElements)
	UE_UPROPERTY_OFFSET(int32_t, NumElements, Address::FixedUObjectArray_NumElements)
};

class FUObjectArray
{
public:
	typedef FFixedUObjectArray TUObjectArray;
	UE_UPROPERTY_OFFSET(TUObjectArray, ObjObjects, Address::UObjectArray_ObjObjects)
};

extern FUObjectArray* GUObjectArray;