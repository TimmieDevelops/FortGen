#include "pch.h"

std::string UObject::GetPackageName() const
{
	if (!this)
		return "None";

    std::string Name = this->GetName();

	if (!Name.empty() && Name.back() == '/')
		Name.pop_back();

	size_t LastSlash = Name.find_last_of('/');
	if (LastSlash != std::string::npos)
		Name = Name.substr(LastSlash + 1);

    return Name;
}

std::string UObject::GetNameCPP() const
{
	if (!this)
		return "None";

	std::string Name = "";

	if (IsA(UEnum::StaticClass()))
	{
		Name += "E";
		Name += GetName();
		return Name;
	}
	else if (UScriptStruct::StaticClass())
	{
		Name += "F";
		Name += GetName();
		return Name;
	}
	else if (UClass::StaticClass())
	{
		for (UClass* ClassPrivate = (UClass*)this; ClassPrivate; ClassPrivate = (UClass*)ClassPrivate->GetSuperStruct())
		{
			if (!ClassPrivate)
				continue;

			std::string ClassName = ClassPrivate->GetName();

			if (ClassName == "Actor")
			{
				Name += "A";
				break;
			}
			else if (ClassName == "Object")
			{
				Name += "U";
				break;
			}
		}

		Name += GetName();
		return Name;
	}

	return "None";
}
