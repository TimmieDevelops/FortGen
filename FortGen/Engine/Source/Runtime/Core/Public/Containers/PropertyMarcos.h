#pragma once
#include "framework.h"

#define DEFINE_MEMBER(Name, Offset, Type) \
    Type Get##Name() const { return *reinterpret_cast<Type*>(reinterpret_cast<uintptr_t>(this) + Offset); } \
    void Set##Name(Type Value) { *reinterpret_cast<Type*>(reinterpret_cast<uintptr_t>(this) + Offset) = Value; }
