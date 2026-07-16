#include "pch.h"

uintptr_t Scanner::GetModuleBase()
{
	return (uintptr_t)GetModuleHandle(NULL);
}

uint32_t Scanner::GetModuleSize()
{
	uintptr_t ImageBase = GetModuleBase();
	if (!ImageBase) return 0;
	auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ImageBase);
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return 0;
	auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(ImageBase + dosHeader->e_lfanew);
	if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return 0;
	return ntHeaders->OptionalHeader.SizeOfImage;
}

std::vector<Scanner::MemoryRange> Scanner::GetExecutableRanges()
{
	std::vector<MemoryRange> ranges;

	uintptr_t ImageBase = GetModuleBase();
	if (!ImageBase) return ranges;

	auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(ImageBase);
	if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return ranges;
	auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(ImageBase + dosHeader->e_lfanew);
	if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return ranges;

	auto sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
	for (int i = 0; i < ntHeaders->FileHeader.NumberOfSections; ++i)
	{
		auto& section = sectionHeader[i];
		if (section.Characteristics & IMAGE_SCN_MEM_EXECUTE)
			ranges.push_back({ ImageBase + section.VirtualAddress, ImageBase + section.VirtualAddress + section.Misc.VirtualSize });
	}

	if (ranges.empty())
		ranges.push_back({ ImageBase, ImageBase + ntHeaders->OptionalHeader.SizeOfImage });

	return ranges;
}

ScanResult Scanner::FindString(const std::string& Str, bool bIsWide, bool bForward)
{
	uintptr_t ImageBase = GetModuleBase();
	uintptr_t Size = GetModuleSize();
	if (!ImageBase || !Size) return ScanResult(0);

	uintptr_t StrAddr = 0;

	if (bIsWide)
	{
		std::wstring wstr(Str.begin(), Str.end());
		const char* pattern = reinterpret_cast<const char*>(wstr.data());
		size_t patternLen = wstr.size() * sizeof(wchar_t);

		if (bForward)
		{
			for (uintptr_t i = ImageBase; i < ImageBase + Size - patternLen; ++i)
			{
				if (memcmp(reinterpret_cast<const void*>(i), pattern, patternLen) == 0)
				{
					StrAddr = i;
					break;
				}
			}
		}
		else
		{
			for (uintptr_t i = ImageBase + Size - patternLen; i >= ImageBase; --i)
			{
				if (memcmp(reinterpret_cast<const void*>(i), pattern, patternLen) == 0)
				{
					StrAddr = i;
					break;
				}
			}
		}
	}
	else
	{
		const char* pattern = Str.data();
		size_t patternLen = Str.size();

		if (bForward)
		{
			for (uintptr_t i = ImageBase; i < ImageBase + Size - patternLen; ++i)
			{
				if (memcmp(reinterpret_cast<const void*>(i), pattern, patternLen) == 0)
				{
					StrAddr = i;
					break;
				}
			}
		}
		else
		{
			for (uintptr_t i = ImageBase + Size - patternLen; i >= ImageBase; --i)
			{
				if (memcmp(reinterpret_cast<const void*>(i), pattern, patternLen) == 0)
				{
					StrAddr = i;
					break;
				}
			}
		}
	}

	if (!StrAddr)
		return ScanResult(0);

	if (bForward)
	{
		for (uintptr_t i = ImageBase; i < ImageBase + Size - 4; ++i)
		{
			if (*reinterpret_cast<const uintptr_t*>(i) == StrAddr)
			{
				uint8_t prevByte = *reinterpret_cast<const uint8_t*>(i - 1);
				if (prevByte == 0x68 || (prevByte >= 0xB8 && prevByte <= 0xBF))
					return ScanResult(i - 1);
				return ScanResult(i - 1);
			}
		}
	}
	else
	{
		for (uintptr_t i = ImageBase + Size -4; i >= ImageBase; --i)
		{
			if (*reinterpret_cast<const uintptr_t*>(i) == StrAddr)
			{
				uint8_t prevByte = *reinterpret_cast<const uint8_t*>(i - 1);
				if (prevByte == 0x68 || (prevByte >= 0xB8 && prevByte <= 0xBF))
					return ScanResult(i - 1);
				return ScanResult(i - 1);
			}
		}
	}

	// fallback
	return ScanResult(StrAddr);
}

ScanResult Scanner::FindPattern(const std::string& Pattern)
{
	uintptr_t ImageBase = GetModuleBase();
	uintptr_t Size = GetModuleSize();
	if (!ImageBase || !Size) return ScanResult(0);

	std::vector<int> bytes;
	std::string currentByte;
	std::stringstream ss(Pattern);

	while (ss >> currentByte)
	{
		if (currentByte == "?" || currentByte == "??")
			bytes.push_back(-1);
		else
			bytes.push_back(std::stoi(currentByte, nullptr, 16));
	}

	if (bytes.empty())
		return ScanResult(0);

	size_t patternLen = bytes.size();
	auto ranges = Scanner::GetExecutableRanges();

	for (const auto& range : ranges)
	{
		if (range.End < range.Start || (range.End - range.Start) < patternLen)
			continue;

		for (uintptr_t i = range.Start; i < range.End - patternLen; ++i)
		{
			bool found = true;

			for (size_t j = 0; j < patternLen; ++j)
			{
				if (bytes[j] != -1 && reinterpret_cast<const uint8_t*>(i)[j] != bytes[j])
				{
					found = false;
					break;
				}
			}

			if (found)
				return ScanResult(i);
		}
	}

	return ScanResult(0);
}

ScanResult::ScanResult(uintptr_t Addr) :
	Address(Addr)
{
}

uintptr_t ScanResult::GetAddress() const
{
	return Address;
}

ScanResult ScanResult::ScanFor(const std::vector<uint8_t>& Bytes, bool bForward) const
{
	uintptr_t Start = Address;
	uintptr_t ImageBase = Scanner::GetModuleBase();
	uintptr_t Size = Scanner::GetModuleSize();
	if (!Start || !ImageBase || !Size)
		return ScanResult(0);

	size_t patternLen = Bytes.size();
	if (patternLen == 0)
		return ScanResult(0);

	if (bForward)
	{
		for (uintptr_t i = Start; i < ImageBase + Size - patternLen; ++i)
		{
			if (memcmp(reinterpret_cast<const void*>(i), Bytes.data(), patternLen) == 0)
				return ScanResult(i);
		}
	}
	else
	{
		if (Start > ImageBase + Size - patternLen)
			Start + Size - patternLen;

		for (uintptr_t i = Start; i >= ImageBase; --i)
		{
			if (memcmp(reinterpret_cast<const void*>(i), Bytes.data(), patternLen) == 0)
				return ScanResult(i);
		}
	}

	return ScanResult(0);
}

bool ScanResult::IsValid() const
{
	return Address != 0;
}

ScanResult ScanResult::AbsoluteOffset(int Offset) const
{
	if (!Address) return ScanResult(0);
	return ScanResult(*reinterpret_cast<const uintptr_t*>(Address + Offset));
}

ScanResult ScanResult::RelativeOffset(int Offset, int InstructionSize) const
{
	if (!Address) return ScanResult(0);
	int32_t disp = *reinterpret_cast<const int32_t*>(Address + Offset);
	return ScanResult(*reinterpret_cast<const uintptr_t*>(Address + InstructionSize + disp));
}
