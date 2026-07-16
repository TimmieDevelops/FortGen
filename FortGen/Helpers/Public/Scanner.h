#pragma once
#include "framework.h"

class ScanResult
{
private:
	uintptr_t Address;
public:
	ScanResult(uintptr_t Addr);
public:
	uintptr_t GetAddress() const;
	ScanResult ScanFor(const std::vector<uint8_t>& Bytes, bool bForward = true) const;
	bool IsValid() const;
	ScanResult AbsoluteOffset(int Offset = 1) const;
	ScanResult RelativeOffset(int Offset = 1, int InstructionSize = 5) const;
};

class Scanner
{
public:

	struct MemoryRange
	{
		uintptr_t Start;
		uintptr_t End;
	};

	static uintptr_t GetModuleBase();
	static uint32_t GetModuleSize();
	static std::vector<MemoryRange> GetExecutableRanges();
public:
	static ScanResult FindString(const std::string& Str, bool bIsWide = true, bool bForward = true);
	static ScanResult FindPattern(const std::string& Pattern);
};