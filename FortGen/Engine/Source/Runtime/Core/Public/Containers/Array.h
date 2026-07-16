#pragma once
#include "framework.h"

#ifndef FORCEINLINE
#if defined(_MSC_VER)
#define FORCEINLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define FORCEINLINE inline __attribute__((always_inline))
#else
#define FORCEINLINE inline
#endif
#endif

template<typename InElementType>
class TArray
{
public:
	InElementType* Data;
	int32_t ArrayNum;
	int32_t ArrayMax;
public:
	/**
	 * Constructor, initializes element number counters.
	 */
	FORCEINLINE TArray()
		: ArrayNum(0)
		, ArrayMax(0)
	{
	}

	FORCEINLINE InElementType* GetData()
	{
		return Data;
	}

	FORCEINLINE const InElementType* GetData() const
	{
		return (const InElementType*)Data;
	}

	FORCEINLINE int32_t GetSlack() const
	{
		return ArrayMax - ArrayNum;
	}

	FORCEINLINE bool IsValidIndex(int32_t Index) const
	{
		return Index >= 0 && Index < ArrayNum;
	}

	FORCEINLINE int32_t Num() const
	{
		return ArrayNum;
	}

	FORCEINLINE int32_t Max() const
	{
		return ArrayMax;
	}

	FORCEINLINE InElementType& operator[](int32_t Index)
	{
		return Data[Index];
	}

	FORCEINLINE const InElementType& operator[](int32_t Index) const
	{
		return Data[Index];
	}
};
