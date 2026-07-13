#pragma once
#include "framework.h"

class Scanner
{
public:
	static uintptr_t FindPattern(const std::string& Pattern, const char* ModuleName = nullptr);
	static uintptr_t FindString(const std::string& String, bool bSearchUp = false, const char* ModuleName = nullptr);
	static bool IsBadReadPtr(void* Ptr, size_t Size);
	static bool IsValid(uintptr_t Address);
	static uintptr_t ResolveRelativeAddress(uintptr_t Address, int Offset, int InstrSize);
private:
	static uintptr_t ScanPatternInternal(uintptr_t Start, size_t Size, const int* Pattern, size_t PatternSize);
	static uintptr_t ScanStringInternal(uintptr_t Start, size_t Size, const uint8_t* StrData, size_t StrLen, bool bSearchUp);
	static uintptr_t FindReferenceInternal(uintptr_t Start, size_t Size, uintptr_t StringAddr, bool bSearchUp);
};