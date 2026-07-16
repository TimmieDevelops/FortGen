#pragma once
#include "Array.h"
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

class FString : public TArray<wchar_t>
{
public:
    FString()
    {
        Data = nullptr;
        ArrayNum = 0;
        ArrayMax = 0;
    }

    const wchar_t* operator*() const
    {
        return Data;
    }

    bool IsValid() const
    {
        return Data != nullptr && ArrayNum > 0;
    }

    std::string ToString() const
    {
        if (!Data || ArrayNum <= 0)
        {
            return std::string();
        }

        // Find actual length (up to first null character or ArrayNum)
        int32_t Length = 0;
        while (Length < ArrayNum && Data[Length] != L'\0')
        {
            Length++;
        }

        if (Length <= 0)
        {
            return std::string();
        }

        // Fast path for ASCII optimization on Win32 (x86)
        std::string Result;
        Result.reserve(Length);

        bool bIsAscii = true;
        for (int32_t i = 0; i < Length; ++i)
        {
            wchar_t Char = Data[i];
            // If character value is > 127, it's non-ASCII, so we fall back
            if (static_cast<unsigned short>(Char) > 127)
            {
                bIsAscii = false;
                break;
            }
            Result.push_back(static_cast<char>(Char));
        }

        if (bIsAscii)
        {
            return Result;
        }

#ifdef _WIN32
        // Fallback using WideCharToMultiByte with CP_UTF8
        int RequiredSize = WideCharToMultiByte(CP_UTF8, 0, Data, Length, nullptr, 0, nullptr, nullptr);
        if (RequiredSize > 0)
        {
            std::string FallbackResult(RequiredSize, '\0');
            WideCharToMultiByte(CP_UTF8, 0, Data, Length, &FallbackResult[0], RequiredSize, nullptr, nullptr);
            return FallbackResult;
        }
#else
        // Cross-platform manual UTF-8 conversion fallback
        std::string FallbackResult;
        FallbackResult.reserve(Length * 3);
        for (int32_t i = 0; i < Length; ++i)
        {
            wchar_t Char = Data[i];
            unsigned short Val = static_cast<unsigned short>(Char);
            if (Val < 0x80)
            {
                FallbackResult.push_back(static_cast<char>(Val));
            }
            else if (Val < 0x800)
            {
                FallbackResult.push_back(static_cast<char>(0xC0 | (Val >> 6)));
                FallbackResult.push_back(static_cast<char>(0x80 | (Val & 0x3F)));
            }
            else
            {
                FallbackResult.push_back(static_cast<char>(0xE0 | (Val >> 12)));
                FallbackResult.push_back(static_cast<char>(0x80 | ((Val >> 6) & 0x3F)));
                FallbackResult.push_back(static_cast<char>(0x80 | (Val & 0x3F)));
            }
        }
        return FallbackResult;
#endif

        return std::string();
    }
};
