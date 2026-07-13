#pragma once
#include "framework.h"
#include "Array.h"

class FString : public TArray<wchar_t>
{
public:
	/**
	 * Create empty string of given size with zero terminating character
	 *
	 * @param Slack length of empty string to create
	 */
	FORCEINLINE void Empty(int32_t Slack = 0)
	{
		// Data.Empty(Slack);
	}

	/**
	 * Test whether this string is empty
	 *
	 * @return true if this string is empty, otherwise return false.
	 */
	FORCEINLINE bool IsEmpty() const
	{
		return Num() <= 1;
	}

	/**
	 * Tests if index is valid, i.e. greater than or equal to zero, and less than the number of characters in this string (excluding the null terminator).
	 *
	 * @param Index Index to test.
	 *
	 * @returns True if index is valid. False otherwise.
	 */
	FORCEINLINE bool IsValidIndex(int32_t Index) const
	{
		return Index >= 0 && Index < Len();
	}

	/** Get the length of the string, excluding terminating character */
	FORCEINLINE int32_t Len() const
	{
		return Num() ? Num() - 1 : 0;
	}

	std::string ToString() const
	{

	}
};