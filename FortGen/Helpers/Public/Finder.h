#pragma once
#include "framework.h"

class Finder
{
public:
	static uintptr_t FindGUObjectArray();
	static uintptr_t FindUKismetSystemLibrary_GetEngineVersion();
	static uintptr_t FindUKismetSystemLibrary_GetPathName();
	static uintptr_t FindFMemory_Realloc();
	static uintptr_t FindFName_ToString();
	static uintptr_t FindStaticLoadObject();
	static uintptr_t FindStaticFindObject();
};