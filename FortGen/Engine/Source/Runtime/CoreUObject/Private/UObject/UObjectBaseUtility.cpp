#include "pch.h"

std::string UObjectBaseUtility::GetName() const
{
    return GetNamePrivate().ToString();
}

std::string UObjectBaseUtility::GetFullName() const
{
    std::string Result;

    UClass* ClassPrivate = GetClassPrivate();

    if (ClassPrivate)
        Result += ClassPrivate->GetName();
    else
        Result += "None";

    Result += " ";
    Result += GetPathName();

    return Result;
}

std::string UObjectBaseUtility::GetPathName() const
{
    return UKismetSystemLibrary::GetPathName((UObject*)this).ToString();
}

bool UObjectBaseUtility::IsA(const UClass* SomeBase) const
{
    if (!this)
        return false;

    UClass* ClassPrivate = GetClassPrivate();
    if (!ClassPrivate)
        return false;

    for (UStruct* SuperStruct = ClassPrivate; SuperStruct; SuperStruct = SuperStruct->GetSuperStruct())
    {
        if (SuperStruct == SomeBase)
            return true;
    }

    return false;
}

UPackage* UObjectBaseUtility::GetOutermost() const
{
    if (!this)
        return nullptr;

    UObject* Outermost = (UObject*)this;

    while (Outermost->GetOuterPrivate() != nullptr)
        Outermost = (UObject*)Outermost->GetOuterPrivate();

    return (class UPackage*)Outermost;
}
