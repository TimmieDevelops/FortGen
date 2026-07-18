#include "pch.h"

FUObjectArray* GUObjectArray = nullptr;

std::string UObject::GetName() const
{
	return GetNamePrivate().ToString();
}

void UObject::GetPathName(const UObject* StopOuter, std::string& ResultString) const
{
	if (this != StopOuter)
	{
		UObject* Outer = (UObject*)GetOuterPrivate();
		if (Outer && Outer != StopOuter)
		{
			Outer->GetPathName(StopOuter, ResultString);
			ResultString += ".";
		}
		ResultString += GetName();
	}
}

std::string UObject::GetPathName() const
{
	std::string PathName;
	GetPathName(nullptr, PathName);
	return PathName;
}

std::string UObject::GetFullName() const
{
	std::string FullName;
	if (GetClassPrivate())
	{
		FullName += ((UObject*)GetClassPrivate())->GetName();
		FullName += " ";
	}
	FullName += GetPathName();
	return FullName;
}

class UPackage* UObject::GetOutermost() const
{
	UObject* Outermost = (UObject*)this;
	while (Outermost->GetOuterPrivate() != nullptr)
	{
		Outermost = (UObject*)Outermost->GetOuterPrivate();
	}
	return (class UPackage*)Outermost;
}

class UObject* UClass::GetDefaultObject() const
{
	std::string CdoName = "Default__" + GetName();
	std::wstring WideCdoName(CdoName.begin(), CdoName.end());
	return StaticFindObject((class UClass*)this, nullptr, WideCdoName.c_str());
}

class UObject* StaticFindObject(class UClass* Class, class UObject* InOuter, const wchar_t* Name, bool ExactClass)
{
	if (!GUObjectArray || !Name) return nullptr;

	std::wstring WideName(Name);
	std::string NameStr(WideName.begin(), WideName.end());

	for (int i = 0; i < GUObjectArray->GetObjObjects().GetNumElements(); i++)
	{
		UObjectBase* ObjectBase = GUObjectArray->GetObjObjects().GetObjects(i)->GetObjectW();
		if (!ObjectBase) continue;

		class UObject* Object = (class UObject*)ObjectBase;

		if (InOuter && Object->GetOuterPrivate() != InOuter) continue;

		if (Class && Object->GetClassPrivate() != Class) continue;

		std::string ObjectName = Object->GetName();
		std::string ObjectPathName = Object->GetPathName();
		std::string ObjectFullName = Object->GetFullName();

		if (ObjectName == NameStr || ObjectPathName == NameStr || ObjectFullName == NameStr)
		{
			return Object;
		}
	}

	return nullptr;
}

class UObject* StaticFindObject(class UClass* Class, class UObject* InOuter, const char* Name, bool ExactClass)
{
	if (!GUObjectArray || !Name) return nullptr;

	std::string NameStr(Name);

	for (int i = 0; i < GUObjectArray->GetObjObjects().GetNumElements(); i++)
	{
		UObjectBase* ObjectBase = GUObjectArray->GetObjObjects().GetObjects(i)->GetObjectW();
		if (!ObjectBase) continue;

		class UObject* Object = (class UObject*)ObjectBase;

		if (InOuter && Object->GetOuterPrivate() != InOuter) continue;

		if (Class && Object->GetClassPrivate() != Class) continue;

		std::string ObjectName = Object->GetName();
		std::string ObjectPathName = Object->GetPathName();
		std::string ObjectFullName = Object->GetFullName();

		if (ObjectName == NameStr || ObjectPathName == NameStr || ObjectFullName == NameStr)
		{
			return Object;
		}
	}

	return nullptr;
}