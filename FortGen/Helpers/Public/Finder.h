#pragma once
#include "framework.h"

class Finder
{
public:
	static uintptr_t FindGUObjectArray();
	static uintptr_t FindUKismetSystemLibrary_GetEngineVersion();
	static uintptr_t FindFMemory_Realloc();
};