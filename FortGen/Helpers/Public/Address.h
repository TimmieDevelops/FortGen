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

	void SetupAddress();
}
