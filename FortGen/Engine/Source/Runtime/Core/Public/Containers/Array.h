#pragma once
#include "framework.h"

template<typename InElementType>
class TArray
{
public:
	InElementType* Data;
	int32_t ArrayNum;
	int32_t ArrayMax;
};