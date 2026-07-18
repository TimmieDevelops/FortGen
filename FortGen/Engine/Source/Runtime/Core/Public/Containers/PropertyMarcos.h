#pragma once
#include "framework.h"

#define DEFINE_MEMBER(Name, Offset, ...) \
    __VA_ARGS__ Get##Name() const { return *reinterpret_cast<__VA_ARGS__*>(reinterpret_cast<uintptr_t>(this) + Offset); } \
    void Set##Name(__VA_ARGS__ Value) { *reinterpret_cast<__VA_ARGS__*>(reinterpret_cast<uintptr_t>(this) + Offset) = Value; }

#define DEFINE_MEMBER_REF(Name, Offset, ...) \
    __VA_ARGS__& Get##Name() const { return *reinterpret_cast<__VA_ARGS__*>(reinterpret_cast<uintptr_t>(this) + Offset); }

#define DEFINE_STRUCTSIZE(Size) \
    static size_t GetStructSize() { return Size; }

#define DEFINE_DATAINDEX(Name, Offset, Type, Structsize) \
    Type Get##Name(size_t Index) const { return reinterpret_cast<Type>(*reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(this) + Offset) + Structsize * Index); }

#define DEFINE_STATICCLASS(ClassPathName, ClassName) \
    static class UClass* StaticClass() \
    { \
        static class UClass* Clss = nullptr; \
        if (!Clss) \
            Clss = StaticFindObject<class UClass>(ClassPathName); \
        return Clss; \
    } \
    static class ClassName* GetDefaultObj() \
    { \
        static class ClassName* DefaultObj = nullptr; \
        if (!DefaultObj) \
            DefaultObj = (class ClassName*)StaticClass()->GetClassDefaultObject(); \
        return DefaultObj; \
    }