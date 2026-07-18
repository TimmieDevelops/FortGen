#pragma once
#include "framework.h"
#include "Object.h"
#include "Script.h"
#include "../../../Core/Public/Containers/Array.h"
#include "../../../Core/Public/Containers/Map.h"

/* UField:
* Next: 0x1C
*/

/* UStruct
* SuperStruct: 0x20
* Children: 0x24
* PropertiesSize: 0x28
* MinAlignment: 0x2C
*
*/

class UField : public UObject
{
public:
	DEFINE_MEMBER(Next, Address::UField_Next, UField*)
};

class UStruct : public UField
{
public:
	DEFINE_MEMBER(SuperStruct, Address::UStruct_SuperStruct, UStruct*)
	DEFINE_MEMBER(Children, Address::UStruct_Children, UField*)
	DEFINE_MEMBER(PropertiesSize, Address::UStruct_PropertiesSize, int32_t)
	DEFINE_MEMBER(MinAlignment, Address::UStruct_MinAlignment, int32_t)
public:
	static class UClass* StaticClass()
	{
		static class UClass* Clss = nullptr;
		if (!Clss)
			Clss = StaticFindObject<class UClass>("/Script/CoreUObject.Struct");
		return Clss;
	}
};

class UClass : public UStruct
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.Class", UClass)
	DEFINE_MEMBER(ClassDefaultObject, Address::UClass_ClassDefaultObject, UObject*)
};


class UEnum : public UField
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.Enum", UEnum)
	DEFINE_MEMBER_REF(Names, Address::UEnum_Names, TArray<TPair<FName, uint8_t>>)
};

class UScriptStruct : public UStruct
{
public:
	DEFINE_STATICCLASS("/Script/CoreUObject.ScriptStruct", UScriptStruct)
};

class UFunction : public UStruct
{
public:
	using FNativeFuncPtr = void(__thiscall*)(UObject*, struct FFrame*, void* const); // adding FFrame soon i guess
	DEFINE_STATICCLASS("/Script/CoreUObject.Function", UFunction)
	DEFINE_MEMBER(FunctionFlags, Address::UFunction_FunctionFlags, EFunctionFlags)
	DEFINE_MEMBER(Func, Address::UFunction_Func, FNativeFuncPtr)
};