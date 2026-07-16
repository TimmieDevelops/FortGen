#pragma once
#include "framework.h"

#define DEFAULT_ALIGNMENT 0

struct FMemory
{
	static void* MallocExternal(SIZE_T Count, uint32_t Alignment = DEFAULT_ALIGNMENT);
	static void* ReallocExternal(void* Original, SIZE_T Count, uint32_t Alignment = DEFAULT_ALIGNMENT);
	static void FreeExternal(void* Original);
};
