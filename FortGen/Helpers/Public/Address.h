#pragma once
#include "framework.h"

namespace Address
{
	inline uintptr_t FUObjectItem_Object = 0;
	inline uintptr_t FUObjectItem_ClusterAndFlags = 0;
	inline uintptr_t FUObjectItem_SerialNumber = 0;
	inline uintptr_t FUObjectItem_StructSize = 0;

	inline uintptr_t FFixedUObjectArray_Objects = 0;
	inline uintptr_t FFixedUObjectArray_MaxElements = 0;
	inline uintptr_t FFixedUObjectArray_NumElements = 0;

	inline uintptr_t FUObjectArray_ObjObjects = 0;

	inline uintptr_t FMemory_Realloc = 0;

	inline uintptr_t UKismetSystemLibrary_GetEngineVersion = 0;

	inline uintptr_t GUObjectArray = 0;

	inline uintptr_t FName_ToString = 0;

	inline uintptr_t UObjectBase_ObjectFlags = 0;
	inline uintptr_t UObjectBase_InternalIndex = 0;
	inline uintptr_t UObjectBase_ClassPrivate = 0;
	inline uintptr_t UObjectBase_NamePrivate = 0;
	inline uintptr_t UObjectBase_OuterPrivate = 0;

	void SetupAddress();
	void SetupOffsets();
}