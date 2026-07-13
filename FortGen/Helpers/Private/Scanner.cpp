#include "pch.h"

uintptr_t Scanner::FindPattern(const std::string& Pattern, const char* ModuleName)
{
	HMODULE Module = GetModuleHandleA(ModuleName);
	if (!Module)
		return 0;

	MODULEINFO ModuleInfo;
	if (!GetModuleInformation(GetCurrentProcess(), Module, &ModuleInfo, sizeof(ModuleInfo)))
		return 0;

	std::vector<int> PatternBytes;
	std::stringstream ss(Pattern);
	std::string Temp;

	while (ss >> Temp)
	{
		if (Temp == "?" || Temp == "??")
		{
			PatternBytes.push_back(-1);
		}
		else
		{
			try
			{
				PatternBytes.push_back(std::stoi(Temp, nullptr, 16));
			}
			catch (...)
			{
				continue;
			}
		}
	}

	if (PatternBytes.empty())
	{

	}

	return ScanPatternInternal((uintptr_t)ModuleInfo.lpBaseOfDll, ModuleInfo.SizeOfImage, PatternBytes.data(), PatternBytes.size());
}

uintptr_t Scanner::FindString(const std::string& String, bool bSearchUp, const char* ModuleName)
{
	HMODULE Module = GetModuleHandleA(ModuleName);
	if (!Module)
		return 0;

	MODULEINFO ModuleInfo;
	if (!GetModuleInformation(GetCurrentProcess(), Module, &ModuleInfo, sizeof(ModuleInfo)))
		return 0;

	uintptr_t Start = (uintptr_t)ModuleInfo.lpBaseOfDll;
	const uint8_t* Data = (const uint8_t*)Start;
	size_t Size = ModuleInfo.SizeOfImage;

	std::vector<uintptr_t> StringAddresses;

	std::vector<uint8_t> UTF16;

	for (auto c : String)
	{
		UTF16.push_back((uint8_t)c);
		UTF16.push_back(0);
	}

	uintptr_t StringAddr = ScanStringInternal(Start, Size, UTF16.data(), UTF16.size(), bSearchUp);
	if (StringAddr)
	{
		uintptr_t Ref = FindReferenceInternal(Start, Size, StringAddr, bSearchUp);
		if (Ref)
			return Ref;
	}

	StringAddr = ScanStringInternal(Start, Size, (const uint8_t*)String.c_str(), String.length(), bSearchUp);
	if (StringAddr)
	{
		uintptr_t Ref = FindReferenceInternal(Start, Size, StringAddr, bSearchUp);
		if (Ref)
			return Ref;
	}

	return 0;
}

bool Scanner::IsBadReadPtr(void* Ptr, size_t Size)
{
	if (!Ptr || (uintptr_t)Ptr < 0x1000)
		return true;

	__try
	{
		volatile uint8_t dummy;

		for (size_t i = 0; i < Size; i += 4096)
			dummy = ((uint8_t*)Ptr)[i];

		dummy = ((uint8_t*)Ptr)[Size - 1];
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return true;
	}

	return false;
}

bool Scanner::IsValid(uintptr_t Address)
{
	return !IsBadReadPtr((void*)Address, 1);
}

uintptr_t Scanner::ResolveRelativeAddress(uintptr_t Address, int Offset, int InstrSize)
{
	if (!Address)
		return 0;

	int32_t Displacement = *(int32_t*)(Address + Offset);
	return Address + InstrSize + Displacement;
}

uintptr_t Scanner::ScanPatternInternal(uintptr_t Start, size_t Size, const int* Pattern, size_t PatternSize)
{
	const uint8_t* Data = (const uint8_t*)Start;
	__try
	{
		for (size_t i = 0; i <= Size - PatternSize; ++i)
		{
			bool Found = true;

			for (size_t j = 0; j < PatternSize; ++j)
			{
				if (Pattern[j] != -1 && Data[i + j] != (uint8_t)Pattern[j])
				{
					Found = false;
					break;
				}
			}

			if (Found)
				return Start + i;
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return 0;
	}

	return 0;
}

uintptr_t Scanner::ScanStringInternal(uintptr_t Start, size_t Size, const uint8_t* StrData, size_t StrLen, bool bSearchUp)
{
	const uint8_t* Data = (const uint8_t*)Start;
	__try
	{
		if (bSearchUp)
		{
			for (size_t i = Size - StrLen; i != (size_t)-1; --i)
			{
				if (memcmp(Data + i, StrData, StrLen) == 0)
					return Start + i;
			}
		}
		else
		{
			for (size_t i = 0; i <= Size - StrLen; ++i)
			{
				if (memcmp(Data + i, StrData, StrLen) == 0)
					return Start + i;
			}
		}
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return 0;
	}

	return 0;
}

uintptr_t Scanner::FindReferenceInternal(uintptr_t Start, size_t Size, uintptr_t StringAddr, bool bSearchUp)
{
	const uint8_t* Data = (const uint8_t*)Start;

	__try
	{
#ifdef _WIN64
		if (bSearchUp)
		{
			for (size_t i = Size - 8; i != (size_t)-1; --i)
			{
				int32_t Displacement = *(int32_t*)(Data + i);

				if (Start + i + 4 + (uintptr_t)Displacement == StringAddr)
				{
					if (i >= 3 && Data[i - 3] == 0x48 && (Data[i - 2] == 0x8D || Data[i - 2] == 0x8B) && (Data[i - 1] & 0xC7) == 0x05)
						return Start + i - 3;

					if (i >= 3 && Data[i - 3] == 0x48 && Data[i - 2] == 0x8B && (Data[i - 1] & 0xC7) == 0x05)
						return Start + i - 3;

					if (i >= 2 && Data[i - 2] == 0x8B && (Data[i - 1] & 0xC7) == 0x05)
						return Start + i - 2;

					if (i >= 2 && Data[i - 2] == 0x8D && (Data[i - 1] & 0xC7) == 0x05)
						return Start + i - 2;

					return Start + i;
				}

				if (*(uintptr_t*)(Data + i) == StringAddr)
					return Start + i;
			}
		}
		else
		{
			for (size_t i = 0; i <= Size - 8; ++i)
			{
				int32_t Displacement = *(int32_t*)(Data + i);
				if (Start + i + 4 + (uintptr_t)Displacement == StringAddr)
				{
					if (i >= 3 && Data[i - 3] == 0x48 && (Data[i - 2] == 0x8D || Data[i - 2] == 0x8B) && (Data[i - 1] & 0xC7) == 0x05)
						return Start + i - 3;

					if (i >= 3 && Data[i - 3] == 0x48 && Data[i - 2] == 0x8B && (Data[i - 1] & 0xC7) == 0x05)
						return Start + i - 3;

					if (i >= 2 && Data[i - 2] == 0x8B && (Data[i - 1] & 0xC7) == 0x05)
						return Start + i - 2;

					if (i >= 2 && Data[i - 2] == 0x8D && (Data[i - 1] & 0xC7) == 0x05)
						return Start + i - 2;

					return Start + i;
				}

				if (*(uintptr_t*)(Data + i) == StringAddr)
					return Start + i;
			}
		}
#else
		if (bSearchUp)
		{
			for (size_t i = Size - 4; i != (size_t)-1; --i)
			{
				if (*(uint32_t*)(Data + i) == (uint32_t)StringAddr)
				{
					if (i >= 1 && (Data[i - 1] == 0x68 || (Data[i - 1] >= 0xB8 && Data[i - 1] <= 0xBF)))
						return Start + i - 1;

					return Start + i;
				}
			}
		}
		else
		{
			for (size_t i = 0; i <= Size - 4; ++i)
			{
				if (*(uint32_t*)(Data + i) == (uint32_t)StringAddr)
				{
					if (i >= 1 && (Data[i - 1] == 0x68 || (Data[i - 1] >= 0xB8 && Data[i - 1] <= 0xBF)))
						return Start + i - 1;

					return Start + i;
				}
			}
		}
#endif
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return 0;
	}

	return uintptr_t();
}