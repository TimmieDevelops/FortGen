#pragma once
#include "framework.h"

// i think in the highest versions the FName can stay the same but who's knows
class FName
{
public:
	uint32_t ComparisonIndex;
	uint32_t Number;
public:
	std::string ToString() const;
};