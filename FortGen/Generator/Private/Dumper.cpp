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
			if (!Buffer.str().empty())
			{
				std::ofstream File(Path);
				File << Buffer.str();
			}
			else
			{
				if (std::filesystem::exists(Path))
					std::filesystem::remove(Path);
			}
		}

		std::string StructFileName = "FN_" + PackageName + "_structs.h";
		if (GeneratedFiles.count(StructFileName))
		{
			std::ostringstream Buffer;
			ProcessScriptStructs(Objects, PackageName, Buffer);
			std::filesystem::path Path = FolderPath / StructFileName;
			if (!Buffer.str().empty())
			{
				std::ofstream File(Path);
				File << Buffer.str();
			}
			else
			{
				if (std::filesystem::exists(Path))
					std::filesystem::remove(Path);
			}
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

void Dumper::GeneratePropertyInfo(UProperty* Property, PropertyInfo& Info)
{
	if (!Property) return;

	Info.Type = GetPropertyType(Property);
	Info.Name = GetSafeName(SanitizeName(Property->GetName()), Info.Type);
	Info.Offset = Property->GetOffset_Internal();
	Info.Size = Property->GetElementSize() * Property->GetArrayDim();
	Info.ArrayDim = Property->GetArrayDim();

	Info.bIsBool = Property->IsA(UBoolProperty::StaticClass());
	Info.bIsBitField = false;
	Info.ByteOffset = 0;
	Info.ByteMask = 0;

	if (Info.bIsBool)
	{
		UBoolProperty* BoolProperty = Property->Cast<UBoolProperty>();
		if (!BoolProperty) return;

		Info.ByteOffset = BoolProperty->GetByteOffset();
		Info.ByteMask = BoolProperty->GetByteMask();

		if (BoolProperty->GetFieldMask() != 0xFF)
			Info.bIsBitField = true;
	}
}

std::string Dumper::GetPropertyType(UProperty* Property)
{
	if (!Property)
		return "Unknown";

	if (Property->IsA(UBoolProperty::StaticClass()))
		return "bool";

	if (Property->IsA(UIntProperty::StaticClass()))
		return "int32_t";

	if (Property->IsA(UFloatProperty::StaticClass()))
		return "float";

	if (Property->IsA(UStructProperty::StaticClass()))
	{
		UStructProperty* StructProperty = Property->Cast<UStructProperty>();
		if (!StructProperty) return "Unknown";
		if (StructProperty->GetStruct()) return SanitizeName(StructProperty->GetStruct()->GetNameCPP());
		return "Unknown";
	}

	if (Property->IsA(UByteProperty::StaticClass()))
	{
		UByteProperty* ByteProperty = Property->Cast<UByteProperty>();
		if (!ByteProperty) return "Unknown";
		if (ByteProperty->GetEnum()) return SanitizeName(ByteProperty->GetEnum()->GetName());
		return "Unknown";
	}

	if (Property->IsA(UObjectProperty::StaticClass()))
	{
		UObjectProperty* ObjectProperty = Property->Cast<UObjectProperty>();
		if (!ObjectProperty) return "Unknown";
		if (ObjectProperty->GetPropertyClass()) return "class " + SanitizeName(ObjectProperty->GetPropertyClass()->GetNameCPP()) + "*";
		return "Unknown";
	}

	if (Property->IsA(UArrayProperty::StaticClass()))
	{
		UArrayProperty* ArrayProperty = Property->Cast<UArrayProperty>();
		if (!ArrayProperty) return "Unknown";
		if (ArrayProperty->GetInner()) return "TArray<" + GetPropertyType(ArrayProperty->GetInner()) + ">";
		return "Unknown";
	}

	if (Property->IsA(UStrProperty::StaticClass()))
		return "FString";

	if (Property->IsA(UWeakObjectProperty::StaticClass()))
	{
		UWeakObjectProperty* WeakObjectProperty = Property->Cast<UWeakObjectProperty>();
		if (!WeakObjectProperty) return "Unknown";
		if (WeakObjectProperty->GetPropertyClass()) return "TWeakObjectPtr<class " + SanitizeName(WeakObjectProperty->GetPropertyClass()->GetNameCPP()) + ">";
		return "TWeakObjectPtr<class UObject>";
	}

	if (Property->IsA(UNameProperty::StaticClass()))
		return "FName";

	if (Property->IsA(UTextProperty::StaticClass()))
		return "FText";

	if (Property->IsA(UDoubleProperty::StaticClass()))
		return "double";

	if (Property->IsA(UMapProperty::StaticClass()))
	{
		UMapProperty* MapProperty = Property->Cast<UMapProperty>();
		if (!MapProperty) return "Unknown";
		return "TMap<" + GetPropertyType(MapProperty->GetKeyProp()) + ", " + GetPropertyType(MapProperty->GetValueProp()) + ">";
	}

	if (Property->IsA(UUInt32Property::StaticClass()))
		return "uint32_t";

	if (Property->IsA(UUInt64Property::StaticClass()))
		return "uint64_t";

	if (Property->IsA(UInt16Property::StaticClass()))
		return "int16_t";

	if (Property->IsA(UInt8Property::StaticClass()))
		return "int8_t";

	if (Property->IsA(UInt64Property::StaticClass()))
		return "int64_t";

	if (Property->IsA(UUInt16Property::StaticClass()))
		return "uint16_t";

	if (Property->IsA(UAssetObjectProperty::StaticClass()))
	{
		UAssetObjectProperty* AssetObjectProperty = Property->Cast<UAssetObjectProperty>();
		if (!AssetObjectProperty) return "Unknown";
		return "TAssetPtr<" + SanitizeName(AssetObjectProperty->GetPropertyClass()->GetNameCPP()) + ">";
	}

	if (Property->IsA(UMulticastDelegateProperty::StaticClass()))
	{
		UMulticastDelegateProperty* DelegateProperty = Property->Cast<UMulticastDelegateProperty>();
		if (!DelegateProperty) return "FMulticastScriptDelegate";
		if (DelegateProperty->GetSignatureFunction()) return "struct F" + SanitizeName(DelegateProperty->GetSignatureFunction()->GetOuterPrivate()->GetNameCPP()) + "_" + SanitizeName(DelegateProperty->GetSignatureFunction()->GetName());
		return "FMulticastScriptDelegate";
	}

	if (Property->IsA(UDelegateProperty::StaticClass()))
	{
		UDelegateProperty* DelegateProperty = Property->Cast<UDelegateProperty>();
		if (!DelegateProperty) return "FScriptDelegate";
		if (DelegateProperty->GetSignatureFunction()) return "struct F" + SanitizeName(DelegateProperty->GetSignatureFunction()->GetOuterPrivate()->GetNameCPP()) + "_" + SanitizeName(DelegateProperty->GetSignatureFunction()->GetName());
		return "FScriptDelegate";
	}

	if (Property->IsA(ULazyObjectProperty::StaticClass()))
	{
		ULazyObjectProperty* ObjectProperty = Property->Cast<ULazyObjectProperty>();
		if (ObjectProperty) return "Unknown";
		if (ObjectProperty->GetPropertyClass()) return "TLazyObjectPtr<class " + SanitizeName(ObjectProperty->GetPropertyClass()->GetNameCPP()) + ">";
		return "TLazyObjectPtr<class UObject>";
	}

	Logger::Log(LogLevel::Info, std::format("[Dumper::GetPropertyType]: Property Class is not found: {}", Property->GetFullName()).c_str());
	return "Unknown";
}

std::string Dumper::GetSafeName(const std::string& Name, const std::string& Type)
{
	std::string CleanName = Name;
	std::string CleanType = Type;

	size_t LastSpace = CleanType.find_last_of(' ');
	if (LastSpace != std::string::npos)
		CleanType = CleanType.substr(LastSpace + 1);

	size_t Marker = CleanType.find_last_of("*<>");
	if (Marker != std::string::npos)
		CleanType = CleanType.substr(0, Marker);

	if (CleanName == CleanType)
		CleanName += "_";

	return CleanName;
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

void Dumper::ProcessScriptStructs(const std::vector<UObject*>& Objects, const std::string& PackageName, std::ostream& File)
{
	for (UObject* Object : Objects)
	{
		if (!Object)
			continue;

		if (Object->IsA(UScriptStruct::StaticClass()))
		{
			UScriptStruct* Struct = Object->Cast<UScriptStruct>();
			if (!Struct) continue;
			GenerateScriptStructs(Struct, PackageName, File);
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

void Dumper::GenerateScriptStructs(UScriptStruct* ScriptStruct, const std::string& PackageName, std::ostream& File)
{
	if (!ScriptStruct)
		return;

	if (ProcessingScriptStructs.count(ScriptStruct->GetFullName()))
		return;

	ProcessingScriptStructs.insert(ScriptStruct->GetFullName());

	UStruct* SuperStruct = ScriptStruct->GetSuperStruct();
	if (SuperStruct && SuperStruct->IsA(UScriptStruct::StaticClass()) && SanitizeName(SuperStruct->GetOutermost()->GetPackageName()) == PackageName)
	{
		UScriptStruct* Struct = SuperStruct->Cast<UScriptStruct>();
		if (!Struct) return;
		if (ScriptStructsFullName.find(Struct->GetFullName()) == ScriptStructsFullName.end())
			GenerateScriptStructs(Struct, PackageName, File);
	}

	auto ProcessProperty = [&](UObject* Object)
		{
			if (Object->IsA(UStructProperty::StaticClass()))
			{
				UStructProperty* StructProperty = Object->Cast<UStructProperty>();
				if (!StructProperty) return;
				if (StructProperty->GetStruct() && StructProperty->GetStruct()->IsA(UScriptStruct::StaticClass()))
				{
					UScriptStruct* Struct = StructProperty->GetStruct()->Cast<UScriptStruct>();
					if (!Struct) return;
					if (SanitizeName(Struct->GetOutermost()->GetPackageName()) == PackageName)
						GenerateScriptStructs(Struct, PackageName, File);
				}
			}
			else if (Object->IsA(UArrayProperty::StaticClass()))
			{
				UArrayProperty* ArrayProperty = Object->Cast<UArrayProperty>();
				if (!ArrayProperty) return;
				if (ArrayProperty->GetInner() && ArrayProperty->GetInner()->IsA(UStructProperty::StaticClass()))
				{
					UStructProperty* Inner = ArrayProperty->GetInner()->Cast<UStructProperty>();
					if (!Inner) return;
					if (Inner->GetStruct() && Inner->GetStruct()->IsA(UScriptStruct::StaticClass()))
					{
						UScriptStruct* Struct = Inner->GetStruct()->Cast<UScriptStruct>();
						if (!Struct) return;
						if (SanitizeName(Struct->GetOutermost()->GetPackageName()) == PackageName)
							GenerateScriptStructs(Struct, PackageName, File);
					}
				}
			}
			else if (Object->IsA(UMapProperty::StaticClass()))
			{
				UMapProperty* MapProperty = Object->Cast<UMapProperty>();
				if (!MapProperty) return;

				auto CheckProp = [&](UProperty* Property)
					{
						if (Property && Property->IsA(UStructProperty::StaticClass()))
						{
							UStructProperty* StructProperty = Property->Cast<UStructProperty>();
							if (!StructProperty) return;
							if (StructProperty->GetStruct() && StructProperty->GetStruct()->IsA(UScriptStruct::StaticClass()))
							{
								UScriptStruct* Struct = StructProperty->GetStruct()->Cast<UScriptStruct>();
								if (!Struct) return;
								if (SanitizeName(Struct->GetOutermost()->GetPackageName()) == PackageName)
									GenerateScriptStructs(Struct, PackageName, File);
							}
						}
					};

				CheckProp(MapProperty->GetKeyProp());
				CheckProp(MapProperty->GetValueProp());
			}
			// TODO: USetProperty
		};

	for (UField* Child = ScriptStruct->GetChildren(); Child; Child = Child->GetNext())
	{
		if (!Child)
			continue;

		if (Child->IsA(UProperty::StaticClass()))
			ProcessProperty(Child);
	}

	if (SanitizeName(ScriptStruct->GetOutermost()->GetPackageName()) == PackageName)
	{
		std::string FullName = ScriptStruct->GetFullName();
		GenerateScriptStruct(ScriptStruct, File);
		ScriptStructsFullName.insert(FullName);
	}
}

void Dumper::GenerateScriptStruct(UScriptStruct* ScriptStruct, std::ostream& File)
{
	if (!ScriptStruct)
		return;

	if (ScriptStructsFullName.find(ScriptStruct->GetFullName()) != ScriptStructsFullName.end())
		return;

	std::string StructName = SanitizeName(ScriptStruct->GetName());
	if (GeneratedNamesInPackage.count(StructName))
		return;

	GeneratedNamesInPackage.insert(StructName);

	std::ostringstream Buffer;
	std::ostringstream SupportBuffer;

	std::string SuperStructName = (ScriptStruct->GetSuperStruct() && ScriptStruct->GetSuperStruct()->IsA(UScriptStruct::StaticClass())) ? SanitizeName(ScriptStruct->GetSuperStruct()->GetNameCPP()) : "";
	std::string StructFullName = ScriptStruct->GetFullName();

	if (StructFullName.find("Default__") != std::string::npos)
		return;

	Buffer << "// " << StructFullName << "\n";

	int32_t FullSize = MinStructSize.count(ScriptStruct) ? MinStructSize[ScriptStruct] : ScriptStruct->GetPropertiesSize();
	int32_t SuperSize = (ScriptStruct->GetSuperStruct() && (ScriptStruct->GetSuperStruct()->IsA(UScriptStruct::StaticClass()) || ScriptStruct->GetSuperStruct()->IsA(UClass::StaticClass()))) ? (MinStructSize.count(ScriptStruct->GetSuperStruct()) ? MinStructSize[ScriptStruct->GetSuperStruct()] : ScriptStruct->GetSuperStruct()->GetPropertiesSize()) : 0;

	Buffer << "// " << SDKMC_SSHEX(FullSize - SuperSize, 4) << " (" << SDKMC_SSHEX(FullSize, 4) << ")\n";

	if (!SuperStructName.empty())
		Buffer << "struct " << StructName << " : public " << SuperStructName << "\n{\n";
	else
		Buffer << "struct " << StructName << "\n{\n";

	std::vector<PropertyInfo> Properties;

	for (UField* Child = ScriptStruct->GetChildren(); Child; Child = Child->GetNext())
	{
		if (!Child) continue;
		if (Child->IsA(UProperty::StaticClass()))
		{
			UProperty* Property = Child->Cast<UProperty>();
			if (!Property) continue;
			PropertyInfo Info;
			GeneratePropertyInfo(Property, Info);
			Properties.push_back(Info);
		}
	}

	std::sort(Properties.begin(), Properties.end(), PropertyInfo::Sort);

	int32_t CurrentOffset = SuperSize;

	if (!Properties.empty() || FullSize > CurrentOffset)
	{
		int32_t LastBitfieldByteOffset = -1;

		for (const PropertyInfo& Property : Properties)
		{
			int32_t Offset = Property.Offset;
			int32_t Size = Property.Size;

			if (Offset > CurrentOffset)
			{
				int32_t Padding = Offset - CurrentOffset;
				std::ostringstream PadName;

				PadName << "UnknownData_" << std::hex << std::uppercase << CurrentOffset << "[" << SDKMC_SSHEX(Padding, 0) << "];";
				Buffer << "\t" << SDKMC_SSCOL("uint8_t", 50) << " " << SDKMC_SSCOL(PadName.str(), 50) << " // " << SDKMC_SSHEX(CurrentOffset, 4) << " (" << SDKMC_SSHEX(Padding, 4) << ") MISSED OFFSET\n";
			}

			std::string PropertyType = Property.Type;
			std::string PropertyName = Property.Name;

			if (Property.ArrayDim > 1)
				PropertyName += "[" + std::to_string(Property.ArrayDim) + "]";

			if (Property.bIsBool && Property.bIsBitField)
			{
				if (LastBitfieldByteOffset != -1 && LastBitfieldByteOffset != Property.ByteOffset)
					Buffer << "\t" << SDKMC_SSCOL("uint8_t", 50) << " " << SDKMC_SSCOL(": 0;", 50) << "\n";

				Buffer << "\t" << SDKMC_SSCOL("uint8_t", 50) << " " << SDKMC_SSCOL(PropertyName + " : 1;", 50) << " // " << SDKMC_SSHEX(Offset, 4) << " (" << SDKMC_SSHEX(Size, 4) << ") [BITFIELD] [ByteOffset: " << SDKMC_SSHEX(Property.ByteOffset, 2) << "] [Mask: " << SDKMC_SSHEX(Property.ByteMask, 2) << "]\n";

				LastBitfieldByteOffset = Property.ByteOffset;
			}
			else
			{
				Buffer << "\t" << SDKMC_SSCOL(PropertyType, 50) << " " << SDKMC_SSCOL(PropertyName + ";", 50) << " // " << SDKMC_SSHEX(Offset, 4) << " (" << SDKMC_SSHEX(Size, 4) << ")\n";
				LastBitfieldByteOffset = -1;
			}

			CurrentOffset = max(CurrentOffset, Offset + Size);
		}

		if (FullSize > CurrentOffset)
		{
			int32_t Padding = FullSize - CurrentOffset;
			std::ostringstream PadName;

			PadName << "UnknownData_" << std::hex << std::uppercase << CurrentOffset << "[" << SDKMC_SSHEX(Padding, 0) << "];";
			Buffer << "\t" << SDKMC_SSCOL("uint8_t", 50) << " " << SDKMC_SSCOL(PadName.str(), 50) << " // " << SDKMC_SSHEX(CurrentOffset, 4) << " (" << SDKMC_SSHEX(Padding, 4) << ") MISSED OFFSET\n";
		}
	}

	Buffer << "\n\tstatic class UScriptStruct* StaticStruct();\n";

	Buffer << "};\n\n";

	File << Buffer.str();
}
