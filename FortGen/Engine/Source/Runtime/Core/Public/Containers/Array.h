#pragma once
#include "framework.h"

template<typename InElementType>
class TArray
{
public:
	InElementType* Data;
	int32_t ArrayNum;
	int32_t ArrayMax;

	FORCEINLINE TArray()
		: ArrayNum(0)
		, ArrayMax(0)
	{
	}
public:
	/**
	 * Returns the amount of slack in this array in elements.
	 *
	 * @see Num, Shrink
	 */
	FORCEINLINE int32_t GetSlack() const
	{
		return ArrayMax - ArrayNum;
	}

	FORCEINLINE const InElementType* GetData() const
	{
		return (const InElementType*)Data;
	}

	/**
	 * Helper function returning the size of the inner type.
	 *
	 * @returns Size in bytes of array type.
	 */
	FORCEINLINE uint32_t GetTypeSize() const
	{
		return sizeof(InElementType);
	}

	/**
	 * Tests if index is valid, i.e. greater than or equal to zero, and less than the number of elements in the array.
	 *
	 * @param Index Index to test.
	 * @returns True if index is valid. False otherwise.
	 */
	FORCEINLINE bool IsValidIndex(int32_t Index) const
	{
		return Index >= 0 && Index < ArrayNum;
	}

	/**
	 * Returns number of elements in array.
	 *
	 * @returns Number of elements in array.
	 * @see GetSlack
	 */
	FORCEINLINE int32_t Num() const
	{
		return ArrayNum;
	}

	/**
	 * Returns maximum number of elements in array.
	 *
	 * @returns Maximum number of elements in array.
	 * @see GetSlack
	 */
	FORCEINLINE int32_t Max() const
	{
		return ArrayMax;
	}

	/**
	 * Array bracket operator. Returns reference to element at give index.
	 *
	 * @returns Reference to indexed element.
	 */
	FORCEINLINE InElementType& operator[](int32_t Index)
	{
		return GetData()[Index];
	}

	/**
	 * Array bracket operator. Returns reference to element at give index.
	 *
	 * Const version of the above.
	 *
	 * @returns Reference to indexed element.
	 */
	FORCEINLINE const InElementType& operator[](int32_t Index) const
	{
		return GetData()[Index];
	}
};