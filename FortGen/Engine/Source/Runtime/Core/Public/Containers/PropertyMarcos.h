#pragma once
#include "framework.h"

#define UE_UPROPERTY_OFFSET(Type, Name, Offset) \
public: \
    Type Get##Name() const { return *reinterpret_cast<Type*>(reinterpret_cast<uintptr_t>(this) + Offset); } \
    void Set##Name(Type Value) { *reinterpret_cast<Type*>(reinterpret_cast<uintptr_t>(this) + Offset) = Value; } \
public: \
    __declspec(property(get = Get##Name, put = Set##Name)) Type Name;