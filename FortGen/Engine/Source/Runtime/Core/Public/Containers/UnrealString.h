#pragma once
#include "Array.h"
#include <string>

class FString : public TArray<wchar_t>
{
public:
	FORCEINLINE FString()
	{
		Data = nullptr;
		ArrayNum = 0;
		ArrayMax = 0;
	}

	FORCEINLINE const wchar_t* operator*() const
	{
		return Data;
	}

	FORCEINLINE bool IsValid() const
	{
		return Data != nullptr && ArrayNum > 0;
	}

	std::string ToString() const
	{
		if (IsValid())
		{
			std::wstring WideStr(Data);
			return std::string(WideStr.begin(), WideStr.end());
		}
		return "";
	}
};
