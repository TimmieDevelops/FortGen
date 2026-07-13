#pragma once
#include "framework.h"

namespace Address
{
	inline uintptr_t GUObjectArray = 0;

	// FUObjectArray
	inline uintptr_t UObjectArray_ObjObjects = 0;

	// FFixedUObjectArray
	inline uintptr_t FixedUObjectArray_Objects = 0;
	inline uintptr_t FixedUObjectArray_MaxElements = 0;
	inline uintptr_t FixedUObjectArray_NumElements = 0;

	// UObjectItem
	inline uintptr_t UObjectItem_Object = 0;
	inline uintptr_t UObjectItem_ClusterAndFlags = 0;
	inline uintptr_t UObjectItem_SerialNumber = 0;

	// FMemory
	inline uintptr_t Memory_Realloc = 0;
}