#include "pch.h"

#define SDKFN_PRINT( file, stream )		fprintf ( file, "%s", stream.str().c_str() ); stream.str ( std::string() );
#define SDKFN_EMPTY( stream )			stream.str ( std::string() );
#define SDKMC_SSDEC( value, width )		std::dec << std::setfill ( '0' ) << std::setw ( width ) << std::right << (uint32_t)(value) << std::setfill ( ' ' )
#define SDKMC_SSHEX( value, width )		"0x" << std::hex << std::uppercase << std::setfill ( '0' ) << std::setw ( width ) << std::right << (uint32_t)(value) << std::nouppercase << std::setfill ( ' ' )
#define SDKMC_SSCOL( string, width )	std::setw(width) << std::setfill(' ') << std::left << string

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

	std::unordered_map<std::string, std::vector<UObject*>> PackageMap;

	for (int i = 0; i < GUObjectArray->GetObjObjects().GetNumElements(); i++)
	{
		UObject* Object = (UObject*)GUObjectArray->GetObjObjects().GetObjects(i)->GetObjectW();
		if (!Object) continue;
		UPackage* Package = Object->GetOutermost();
		if (!Package) continue;
		PackageMap[SanitizeName(Package->GetPackageName())].push_back(Object);
	}

	for (auto& [PackageName, Objects] : PackageMap)
	{
		bool bHasEnum = false;
		bool bHasStruct = false;
		bool bHasClass = false;
		bool bHasParam = false;

		for (UObject* Object : Objects)
		{
			if (!Object)
				continue;

			if (Object->IsA(UEnum::StaticClass()))
			{
				bHasEnum = true;
			}

			if (Object->IsA(UScriptStruct::StaticClass()))
				bHasStruct = true;

			if (Object->IsA(UClass::StaticClass()))
				bHasClass = true;

			if (Object->IsA(UFunction::StaticClass()))
			{
				UFunction* Function = Object->Cast<UFunction>();
				if (!Function) continue;
				bHasParam = true;
				if (Function->GetFunctionFlags() & (FUNC_Delegate | FUNC_MulticastDelegate))
					bHasStruct = true;
			}
		}

		if (bHasEnum)
			GeneratedFiles.insert("FN_" + PackageName + "_enums.h");

		if (bHasStruct)
			GeneratedFiles.insert("FN_" + PackageName + "_structs.h");

		if (bHasClass)
			GeneratedFiles.insert("FN_" + PackageName + "_classes.h");

		if (bHasParam)
			GeneratedFiles.insert("FN_" + PackageName + "_parameters.h");
	}

	for (auto& [PackageName, Objects] : PackageMap)
	{
		GeneratedNamesInPackage.clear();

		std::unordered_set<std::string> StructuralDependencies = GetPackageDependencies(PackageName, Objects, true);
		std::unordered_set<std::string> FullDependencies = GetPackageDependencies(PackageName, Objects, false);
		std::unordered_set<std::string> InheritanceDependencies = GetPackageDependencies(PackageName, Objects, false, true);

		std::string EnumFileName = "FN_" + PackageName + "_enums.h";
		if (GeneratedFiles.count(EnumFileName))
		{
			std::ostringstream Buffer;
			ProcessEnums(Objects, PackageName, Buffer);
			std::filesystem::path Path = FolderPath / EnumFileName;
			std::ofstream File(Path);
			File << Buffer.str();
		}
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
							UProperty* Property = Child->Cast<UProperty>();
							if (MinOffset == -1 || Property->GetOffset_Internal() < MinOffset)
								MinOffset = Property->GetOffset_Internal();
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

std::string Dumper::SanitizeName(std::string Name)
{
	static const std::string InvalidChars = " /?:;.,'\"!@#$%^&*()+-=<>[]";

	for (char& Char : Name)
	{
		if (InvalidChars.find(Char) != std::string::npos)
			Char = '_';
	}

	if (!Name.empty() && isdigit(Name[0]))
		Name = "_" + Name;

	static const std::unordered_set<std::string> Keywords = {
		"int", "float", "double", "char", "bool", "virtual", "volatile", "class", "struct", "true", "false", "unsigned", "signed", "long", "short", "auto", "template", "typename", "public", "protected", "private"
	};

	if (Keywords.count(Name))
		Name = "_" + Name;

	return Name;
}

std::unordered_set<std::string> Dumper::GetPackageDependencies(const std::string& PackageName, const std::vector<UObject*>& Objects, bool bStructuralOnly, bool bClassSuperOnly)
{
	std::unordered_set<std::string> Dependencies;

	for (UObject* Object : Objects)
	{
		if (!Object)
			continue;

		if (Object->IsA(UScriptStruct::StaticClass()) || Object->IsA(UClass::StaticClass()))
		{
			UStruct* Struct = Object->Cast<UStruct>();
			if (!Struct)
				continue;

			if (Struct->GetSuperStruct())
			{
				bool bIsClassInheritance = Object->IsA(UClass::StaticClass());
				bool bShouldAdd = !bClassSuperOnly || bIsClassInheritance;
				UObject* SuperPackage = Struct->GetSuperStruct()->GetOutermost();
				if (bShouldAdd && SuperPackage && SanitizeName(SuperPackage->GetPackageName()) != PackageName)
					Dependencies.insert(SanitizeName(SuperPackage->GetPackageName()));
			}

			if (bClassSuperOnly)
				continue;

			for (UField* Child = Struct->GetChildren(); Child; Child = Child->GetNext())
			{
				if (!Child)
					continue;

				if (Child->IsA(UProperty::StaticClass()))
				{
					UProperty* Property = Child->Cast<UProperty>();
					if (!Property) continue;
					CollectDependencies(Property, PackageName, Dependencies, bStructuralOnly);
				}
			}
		}
	}

	return Dependencies;
}

void Dumper::CollectDependencies(UProperty* Property, const std::string& PackageName, std::unordered_set<std::string>& Dependencies, bool bStructuralOnly)
{
	if (!Property)
		return;

	auto AddDependency = [&](UObject* Object)
		{
			if (!Object)
				return;

			UObject* Outer = Object->GetOutermost();
			if (Outer && SanitizeName(Outer->GetPackageName()) != PackageName)
				Dependencies.insert(SanitizeName(Outer->GetPackageName()));
		};

	// TODO: SetProperty

	if (Property->IsA(UStructProperty::StaticClass()))
	{
		UStructProperty* Struct = Property->Cast<UStructProperty>();
		if (!Struct) return;
		AddDependency(Struct->GetStruct());
	}
	else if (Property->IsA(UByteProperty::StaticClass()))
	{
		UByteProperty* Byte = Property->Cast<UByteProperty>();
		if (!Byte) return;
		UEnum* Enum = Byte->GetEnum();
		if (!Enum) return;
		AddDependency(Enum);
	}
	else if (Property->IsA(UArrayProperty::StaticClass()))
	{
		UArrayProperty* Array = Property->Cast<UArrayProperty>();
		if (!Array) return;
		UProperty* Inner = Array->GetInner();
		if (!Inner) return;
		CollectDependencies(Inner, PackageName, Dependencies, bStructuralOnly);
	}
	else if (UMapProperty::StaticClass())
	{
		UMapProperty* Map = Property->Cast<UMapProperty>();
		if (!Map) return;
		UProperty* KeyProp = Map->GetKeyProp();
		if (!KeyProp) return;
		UProperty* ValueProp = Map->GetValueProp();
		if (!ValueProp) return;
		CollectDependencies(KeyProp, PackageName, Dependencies, bStructuralOnly);
		CollectDependencies(ValueProp, PackageName, Dependencies, bStructuralOnly);
	}
	else if (UDelegateProperty::StaticClass())
	{
		UDelegateProperty* Delegate = Property->Cast<UDelegateProperty>();
		if (!Delegate) return;
		UFunction* SignatureFunction = Delegate->GetSignatureFunction();
		AddDependency(SignatureFunction);
	}
	else if (UMulticastDelegateProperty::StaticClass())
	{
		UMulticastDelegateProperty* Delegate = Property->Cast<UMulticastDelegateProperty>();
		if (!Delegate) return;
		UFunction* SignatureFunction = Delegate->GetSignatureFunction();
		if (!SignatureFunction) return;
		AddDependency(SignatureFunction);
	}
}

void Dumper::ProcessEnums(const std::vector<UObject*>& Objects, const std::string& PackageName, std::ostream& File)
{
	for (UObject* Object : Objects)
	{
		if (!Object)
			continue;

		if (Object->IsA(UEnum::StaticClass()))
		{
			UEnum* Enum = Object->Cast<UEnum>();
			if (!Enum) continue;
			GenerateEnum(Enum, File);
		}
	}
}

void Dumper::GenerateEnum(UEnum* Enum, std::ostream& File)
{
	if (!Enum)
		return;

	std::string EnumName = SanitizeName(Enum->GetName());
	if (GeneratedNamesInPackage.count(EnumName))
		return;

	GeneratedNamesInPackage.insert(EnumName);

	std::ostringstream Buffer;
	std::ostringstream SupportBuffer;

	std::string OuterNameCPP = SanitizeName(Enum->GetNameCPP());
	std::string FullName = Enum->GetFullName();

	if (EnumName.find("Default__") != std::string::npos)
		return;

	Buffer << "// " << FullName << "\n";
	Buffer << "enum class " << EnumName << " : uint8_t\n{\n";

	std::map<std::string, int> Names;

	for (int i = 0; i < Enum->GetNames().Num(); i++)
	{
		std::string Name = Enum->GetNames()[i].Key.ToString();

		size_t Pos = Name.find("::");
		std::string EnumName = SanitizeName((Pos != std::string::npos) ? Name.substr(Pos + 2) : Name);

		if (Names.count(EnumName) == 0)
		{
			Names[EnumName] = 1;
			SupportBuffer << EnumName;
		}
		else
		{
			SupportBuffer << EnumName << SDKMC_SSDEC(Names[EnumName], 2);
			Names[EnumName]++;
		}

		if (i != Enum->GetNames().Num() - 1)
			Buffer << "\t" << SDKMC_SSCOL(SupportBuffer.str(), 50) << " = " << i << ",\n";
		else
			Buffer << "\t" << SDKMC_SSCOL(SupportBuffer.str(), 50) << " = " << i << "\n";

		SDKFN_EMPTY(SupportBuffer);
	}

	Buffer << "};\n\n";

	File << Buffer.str().c_str();
	Buffer.str("");
}
