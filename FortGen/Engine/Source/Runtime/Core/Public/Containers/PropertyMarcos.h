#pragma once
#include "framework.h"

#define DEFINE_MEMBER(Name, Offset, Type) \
    Type Get##Name() const { return *reinterpret_cast<Type*>(reinterpret_cast<uintptr_t>(this) + Offset); } \
    void Set##Name(Type Value) { *reinterpret_cast<Type*>(reinterpret_cast<uintptr_t>(this) + Offset) = Value; }

#define DEFINE_STRUCTSIZE(Size) \
    static int32_t GetStructSize() { return Size; }

#define DEFINE_DATAINDEX(Name, Offset, Type, StructSize) \
    Type Get##Name(int32_t Index) const { return reinterpret_cast<Type>(*reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(this) + Offset) + StructSize * Index); }
