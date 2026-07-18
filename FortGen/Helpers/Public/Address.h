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
	inline uintptr_t UKismetSystemLibrary_GetPathName = 0;

	inline uintptr_t GUObjectArray = 0;
	inline uintptr_t StaticLoadObject = 0;
	inline uintptr_t StaticFindObject = 0;

	inline uintptr_t FName_ToString = 0;

	inline uintptr_t UObjectBase_ObjectFlags = 0;
	inline uintptr_t UObjectBase_InternalIndex = 0;
	inline uintptr_t UObjectBase_ClassPrivate = 0;
	inline uintptr_t UObjectBase_NamePrivate = 0;
	inline uintptr_t UObjectBase_OuterPrivate = 0;

	inline uintptr_t UField_Next = 0;
	
	inline uintptr_t UStruct_SuperStruct = 0;
	inline uintptr_t UStruct_Children = 0;
	inline uintptr_t UStruct_PropertiesSize = 0;
	inline uintptr_t UStruct_MinAlignment = 0;

	inline uintptr_t UClass_ClassDefaultObject = 0;

	inline uintptr_t UProperty_ArrayDim = 0;
	inline uintptr_t UProperty_ElementSize = 0;
	inline uintptr_t UProperty_PropertyFlags = 0;
	inline uintptr_t UProperty_OffsetInternal = 0;

	inline uintptr_t UFunction_FunctionFlags = 0;
	inline uintptr_t UFunction_Func = 0;

	inline uintptr_t UStructProperty_Struct = 0;

	inline uintptr_t UByteProperty_Enum = 0;

	inline uintptr_t UArrayProperty_Inner = 0;

	inline uintptr_t UMapProperty_KeyProp = 0;
	inline uintptr_t UMapProperty_ValueProp = 0;

	inline uintptr_t UDelegateProperty_SignatureFunction = 0;

	inline uintptr_t UMulticastDelegateProperty_SignatureFunction = 0;

	inline uintptr_t UEnum_Names = 0;

	void SetupAddress();
	void SetupOffsets();
}