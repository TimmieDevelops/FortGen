#include "pch.h"

void Dumper::Initialize()
{
	if (!GUObjectArray)
	{
		Logger::Log(LogLevel::Error, "GUObjectArray is nullptr!");
		return;
	}

	Logger::Log(LogLevel::Info, "FortGen is dumping the SDK...");

	std::string EngineVersion = UKismetSystemLibrary::GetEngineVersion().ToString();

	std::filesystem::path FolderPath = Settings::FolderPath;
	FolderPath /= EngineVersion;
	if (!std::filesystem::exists(FolderPath))
		std::filesystem::create_directories(FolderPath);

	std::filesystem::path SDKPath = FolderPath / "SDK";
	if (!std::filesystem::exists(SDKPath))
		std::filesystem::create_directories(SDKPath);

	InitMinStructSize();
	DumpObjects(FolderPath);
	ProcessPackages(SDKPath);

	Logger::Log(LogLevel::Info, "FortGen is finished dumping the SDK.");
}

void Dumper::DumpObjects(std::filesystem::path& FolderPath)
{
	std::filesystem::path PathFile = FolderPath / "Objects.txt";
	std::ofstream File(PathFile);
	if (!File.is_open())
	{
		Logger::Log(LogLevel::Error, "[Dumper::DumpObjects]: Failed to create Objects.txt"); 
		return;
	}

	int TotalObjects = GUObjectArray->GetObjObjects().GetNumElements();

	File << "Total Objects: " << TotalObjects << "\n\n";

	for (int i = 0; i < TotalObjects; i++)
	{
		UObject* Object = (UObject*)GUObjectArray->GetObjObjects().GetObjects(i)->GetObjectW();
		if (!Object) continue;
		File << "[" << i << "]: " << Object->GetFullName() << std::endl;
	}

	File.close();
}

void Dumper::ProcessPackages(std::filesystem::path& FolderPath)
{
	ClassesFullName.clear();

	std::unordered_map<std::string, std::vector<UPackage*>> PackageMap;

	for (int i = 0; i < GUObjectArray->GetObjObjects().GetNumElements(); i++)
	{
		UObject* Object = (UObject*)GUObjectArray->GetObjObjects().GetObjects(i)->GetObjectW();
		if (!Object) continue;
		UPackage* Package = Object->GetOutermost();
		if (!Package) continue;
		// Logger::Log(LogLevel::Info, Package->GetPackageName());
	}
}

void Dumper::InitMinStructSize()
{
	bool bChanged = true;

	while (bChanged)
	{
		bChanged = false;

		for (int i = 0; i < GUObjectArray->GetObjObjects().GetNumElements(); i++)
		{
			UObject* Object = (UObject*)GUObjectArray->GetObjObjects().GetObjects(i)->GetObjectW();
			if (!Object) continue;
			if (Object->IsA(UClass::StaticClass()))
			{
				UStruct* Struct = Object->Cast<UStruct>();
				if (!Struct) continue;
				UStruct* Super = Struct->GetSuperStruct();
				if (Super)
				{
					int32_t MinOffset = -1;

					for (UField* Child = Struct->GetChildren(); Child; Child = Child->GetNext())
					{
						if (!Child)
							continue;

						if (Child->IsA(UProperty::StaticClass()))
						{
							UProperty* Property = (UProperty*)Child;
							/*if (MinOffset == -1 || Property->GetOffset_Internal() < MinOffset)
								MinOffset = Property->GetOffset_Internal();*/
						}
					}

					if (MinOffset != -1 && MinStructSize.count(Struct->GetSuperStruct()))
					{
						if (MinOffset < MinStructSize[Struct->GetSuperStruct()])
						{
							MinStructSize[Struct->GetSuperStruct()] = MinOffset;
							bChanged = true;
						}
					}
				}
			}
		}
	}
}
