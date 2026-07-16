#include "pch.h"

void* FMemory::MallocExternal(SIZE_T Count, uint32_t Alignment)
{
	return ReallocExternal(nullptr, Count, Alignment);
}

void* FMemory::ReallocExternal(void* Original, SIZE_T Count, uint32_t Alignment)
{
	static void* (*Realloc)(void* Original, SIZE_T Count, uint32_t Alignment) = reinterpret_cast<void*(*)(void*, SIZE_T, uint32_t)>(Scanner::GetModuleBase() + Address::FMemory_Realloc);
	return Realloc(Original, Count, Alignment);
}

void FMemory::FreeExternal(void* Original)
{
	ReallocExternal(Original, 0, 0);
}
