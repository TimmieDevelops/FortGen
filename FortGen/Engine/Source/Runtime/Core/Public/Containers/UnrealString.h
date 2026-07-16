#pragma once
#include "framework.h"
#include "Array.h"

// TODO: optimzed x86 fast

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

	std::string ToString() const
	{
		if (IsValid())
		{
			std::wstring WideStr(Data);
			return std::string(WideStr.begin(), WideStr.end());
		}

		return "None";
	}
};