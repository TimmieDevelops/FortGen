#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>

#define DEFINE_MEMBER(Name, Offset, Type) \
    Type Get##Name() const { return *reinterpret_cast<const Type*>(reinterpret_cast<uintptr_t>(this) + Offset); } \
    void Set##Name(Type Value) { *reinterpret_cast<Type*>(reinterpret_cast<uintptr_t>(this) + Offset) = Value; }

class ScanResult; // Forward declaration

class Scanner
{
public:
    static uintptr_t GetModuleBase()
    {
        return (uintptr_t)GetModuleHandle(NULL);
    }

    static uint32_t GetModuleSize()
    {
        uintptr_t base = GetModuleBase();
        if (!base) return 0;

        auto dosHeader = reinterpret_cast<PIMAGE_DOS_HEADER>(base);
        if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE) return 0;

        auto ntHeaders = reinterpret_cast<PIMAGE_NT_HEADERS>(base + dosHeader->e_lfanew);
        if (ntHeaders->Signature != IMAGE_NT_SIGNATURE) return 0;

        return ntHeaders->OptionalHeader.SizeOfImage;
    }

    static ScanResult FindString(const std::string& Str, bool bIsWide = true, bool bForward = true);
    static ScanResult FindPattern(const std::string& Pattern);
};

class ScanResult
{
private:
    uintptr_t Address;
public:
    ScanResult(uintptr_t Addr) : Address(Addr) {}
    uintptr_t GetAddress() const { return Address; }
    bool IsValid() const { return Address != 0; }

    ScanResult AbsolveOffset(int Offset = 1) const
    {
        if (!Address) return ScanResult(0);
        return ScanResult(*reinterpret_cast<const uintptr_t*>(Address + Offset));
    }

    ScanResult AbsoluteOffset(int Offset = 1) const
    {
        if (!Address) return ScanResult(0);
        return ScanResult(*reinterpret_cast<const uintptr_t*>(Address + Offset));
    }

    ScanResult RelativeOffset(int Offset = 1, int InstructionSize = 5) const
    {
        if (!Address) return ScanResult(0);
        int32_t disp = *reinterpret_cast<const int32_t*>(Address + Offset);
        return ScanResult(Address + InstructionSize + disp);
    }

    ScanResult ScanFor(const std::vector<uint8_t>& Bytes, bool bForward = true) const
    {
        uintptr_t start = Address;
        uintptr_t base = Scanner::GetModuleBase();
        uint32_t size = Scanner::GetModuleSize();
        if (!start || !base || !size) return ScanResult(0);

        size_t patternLen = Bytes.size();
        if (patternLen == 0) return ScanResult(start);

        if (bForward)
        {
            for (uintptr_t i = start; i < base + size - patternLen; ++i)
            {
                if (memcmp(reinterpret_cast<const void*>(i), Bytes.data(), patternLen) == 0)
                {
                    return ScanResult(i);
                }
            }
        }
        else
        {
            if (start > base + size - patternLen) start = base + size - patternLen;
            for (uintptr_t i = start; i >= base; --i)
            {
                if (memcmp(reinterpret_cast<const void*>(i), Bytes.data(), patternLen) == 0)
                {
                    return ScanResult(i);
                }
            }
        }

        return ScanResult(0);
    }
};

// Define Scanner methods inline so they can return ScanResult
inline ScanResult Scanner::FindString(const std::string& Str, bool bIsWide, bool bForward)
{
    uintptr_t base = GetModuleBase();
    uint32_t size = GetModuleSize();
    if (!base || !size) return ScanResult(0);

    uintptr_t strAddr = 0;

    if (bIsWide)
    {
        std::wstring wstr(Str.begin(), Str.end());
        const char* pattern = reinterpret_cast<const char*>(wstr.data());
        size_t patternLen = wstr.size() * sizeof(wchar_t);

        if (bForward)
        {
            for (uintptr_t i = base; i < base + size - patternLen; ++i)
            {
                if (memcmp(reinterpret_cast<const void*>(i), pattern, patternLen) == 0)
                {
                    strAddr = i;
                    break;
                }
            }
        }
        else
        {
            for (uintptr_t i = base + size - patternLen; i >= base; --i)
            {
                if (memcmp(reinterpret_cast<const void*>(i), pattern, patternLen) == 0)
                {
                    strAddr = i;
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
            for (uintptr_t i = base; i < base + size - patternLen; ++i)
            {
                if (memcmp(reinterpret_cast<const void*>(i), pattern, patternLen) == 0)
                {
                    strAddr = i;
                    break;
                }
            }
        }
        else
        {
            for (uintptr_t i = base + size - patternLen; i >= base; --i)
            {
                if (memcmp(reinterpret_cast<const void*>(i), pattern, patternLen) == 0)
                {
                    strAddr = i;
                    break;
                }
            }
        }
    }

    if (!strAddr) return ScanResult(0);

    // Find the cross-reference (XREF) to strAddr in the memory range.
    // On x86 (32-bit), we scan for 4-byte absolute address references to strAddr.
    if (bForward)
    {
        for (uintptr_t i = base; i < base + size - 4; ++i)
        {
            if (*reinterpret_cast<const uintptr_t*>(i) == strAddr)
            {
                // Check if the preceding byte is a push instruction (0x68) or a mov instruction (0xB8 to 0xBF)
                if (i > base)
                {
                    uint8_t prevByte = *reinterpret_cast<const uint8_t*>(i - 1);
                    if (prevByte == 0x68 || (prevByte >= 0xB8 && prevByte <= 0xBF))
                    {
                        return ScanResult(i - 1);
                    }
                }
            }
        }
    }
    else
    {
        for (uintptr_t i = base + size - 4; i >= base; --i)
        {
            if (*reinterpret_cast<const uintptr_t*>(i) == strAddr)
            {
                // Check if the preceding byte is a push instruction (0x68) or a mov instruction (0xB8 to 0xBF)
                if (i > base)
                {
                    uint8_t prevByte = *reinterpret_cast<const uint8_t*>(i - 1);
                    if (prevByte == 0x68 || (prevByte >= 0xB8 && prevByte <= 0xBF))
                    {
                        return ScanResult(i - 1);
                    }
                }
            }
        }
    }

    // Fallback to the string address itself if no XREF is found
    return ScanResult(strAddr);
}

inline ScanResult Scanner::FindPattern(const std::string& Pattern)
{
    uintptr_t base = GetModuleBase();
    uint32_t size = GetModuleSize();
    if (!base || !size) return ScanResult(0);

    std::vector<int> bytes;
    std::string currentByte;
    std::stringstream ss(Pattern);
    while (ss >> currentByte)
    {
        if (currentByte == "?" || currentByte == "??")
        {
            bytes.push_back(-1);
        }
        else
        {
            bytes.push_back(std::stoi(currentByte, nullptr, 16));
        }
    }

    if (bytes.empty()) return ScanResult(0);

    size_t patternLen = bytes.size();
    for (uintptr_t i = base; i < base + size - patternLen; ++i)
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
        {
            return ScanResult(i);
        }
    }

    return ScanResult(0);
}
