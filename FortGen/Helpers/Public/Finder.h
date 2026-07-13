#pragma once
#include "framework.h"

enum EInstructionType : uint8_t
{
	Mov
};

class Finder
{
public:
	static void SetupAddress();
	static uintptr_t FindGUObjectArray();
	static uintptr_t FindMemory_Realloc();
private:
	static uintptr_t GetResolveInstructionAddress(uint8_t* Address, EInstructionType Type);
};