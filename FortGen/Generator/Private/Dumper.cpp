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

	std::filesystem::path BasicHeader = SDKPath / "FN_Basic.h";
	std::filesystem::path SDKHeader = FolderPath / "SDK.h";

	BuildMinStructSize();
	BuildValidStructPackages();

	DumpObjects(FolderPath);
	ProcessPackages(SDKPath);
	GenerateSDKHeader(SDKHeader);
	GenerateBasicHeader(BasicHeader);

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
	ScriptStructsFullName.clear();
	GeneratedFiles.clear();

	std::unordered_map<std::string, std::vector<UObject*>> PackageMap;

	for (int i = 0; i < GUObjectArray->GetObjObjects().GetNumElements(); i++)
	{
		UObject* Object = (UObject*)GUObjectArray->GetObjObjects().GetObjects(i)->GetObjectW();
		if (!Object) continue;
		UPackage* Package = Object->GetOutermost();
		if (!Package) continue;
		PackageMap[SanitizeName(Package->GetPackageName())].push_back(Object);
	}

	std::unordered_map<std::string, std::string> EnumBuffers;
	std::unordered_map<std::string, std::string> StructBuffers;
	std::unordered_map<std::string, std::string> ClassBuffers;
	std::unordered_map<std::string, std::string> FunctionBuffers;
	std::unordered_map<std::string, std::string> ParamBuffers;
	std::unordered_map<std::string, std::string> DelegateBuffers;

	for (auto& [PackageName, Objects] : PackageMap)
	{
		GeneratedNamesInPackage.clear();

		{
			std::ostringstream Buffer;
			ProcessEnums(Objects, PackageName, Buffer);
			std::string Content = Buffer.str();
			if (!Content.empty())
			{
				EnumBuffers[PackageName] = Content;
				GeneratedFiles.insert("FN_" + PackageName + "_enums.h");
			}
		}

		{
			std::ostringstream Buffer;
			ProcessingScriptStructs.clear();
			ProcessScriptStructs(Objects, PackageName, Buffer);
			std::string Content = Buffer.str();
			if (!Content.empty())
			{
				StructBuffers[PackageName] = Content;
				GeneratedFiles.insert("FN_" + PackageName + "_structs.h");
			}
		}

		{
			std::ostringstream Buffer;
			ProcessClasses(Objects, PackageName, Buffer);
			std::string Content = Buffer.str();
			if (!Content.empty())
			{
				ClassBuffers[PackageName] = Content;
				GeneratedFiles.insert("FN_" + PackageName + "_classes.h");
			}
		}

		{
			std::ostringstream Buffer;
			ProcessFunctions(Objects, Buffer);
			std::string Content = Buffer.str();
			if (!Content.empty())
			{
				FunctionBuffers[PackageName] = Content;
				GeneratedFiles.insert("FN_" + PackageName + "_functions.cpp");
			}
		}

		{
			std::ostringstream Buffer;
			ProcessParameters(Objects, PackageName, Buffer);
			std::string Content = Buffer.str();
			if (!Content.empty())
			{
				DelegateBuffers[PackageName] = Content;
				GeneratedFiles.insert("FN_" + PackageName + "_parameters.h");
			}
		}

		{
			std::ostringstream Buffer;
			ProcessDelegates(Objects, PackageName, Buffer);
			std::string Content = Buffer.str();
			if (!Content.empty())
			{
				DelegateBuffers[PackageName] = Content;
				GeneratedFiles.insert("FN_" + PackageName + "_delegates.h");
			}
		}
	}

	for (auto& [PackageName, Objects] : PackageMap)
	{
		GeneratedNamesInPackage.clear();

		std::unordered_set<std::string> StructuralDependencies = GetPackageDependencies(PackageName, Objects, true);
		std::unordered_set<std::string> FullDependencies = GetPackageDependencies(PackageName, Objects, false);
		std::unordered_set<std::string> InheritanceDependencies = GetPackageDependencies(PackageName, Objects, false, true);

		std::string EnumFileName = "FN_" + PackageName + "_enums.h";
		std::filesystem::path EnumPath = FolderPath / EnumFileName;
		if (GeneratedFiles.count(EnumFileName))
		{
			std::ofstream File(EnumPath);
			PrintFileHeader(File, PackageName, {}, "enums");
			File << EnumBuffers[PackageName];
		}
		else
		{
			if (std::filesystem::exists(EnumPath))
				std::filesystem::remove(EnumPath);
		}

		std::string StructFileName = "FN_" + PackageName + "_structs.h";
		std::filesystem::path StructPath = FolderPath / StructFileName;
		if (GeneratedFiles.count(StructFileName))
		{
			std::ofstream File(StructPath);
			PrintFileHeader(File, PackageName, StructuralDependencies, "structs");
			File << StructBuffers[PackageName];
		}
		else
		{
			if (std::filesystem::exists(StructPath))
				std::filesystem::remove(StructPath);
		}

		std::string ClassFileName = "FN_" + PackageName + "_classes.h";
		std::filesystem::path ClassPath = FolderPath / ClassFileName;
		if (GeneratedFiles.count(ClassFileName))
		{
			std::ofstream File(ClassPath);
			PrintFileHeader(File, PackageName, FullDependencies, "classes", InheritanceDependencies);
			File << ClassBuffers[PackageName];
		}
		else
		{
			if (std::filesystem::exists(ClassPath))
				std::filesystem::remove(ClassPath);
		}

		std::string FunctionFileName = "FN_" + PackageName + "_functions.cpp";
		std::filesystem::path FunctionPath = FolderPath / FunctionFileName;
		if (GeneratedFiles.count(FunctionFileName))
		{
			std::ostringstream Buffer;
			std::ofstream File(FunctionPath);
			if (Settings::bInclude_pch_Header) File << "#include \"pch.h\"\n\n";
			std::string Header = "FN_" + PackageName + "_classes.h";
			if (GeneratedFiles.count(Header))
				File << "#include \"" << Header << "\"\n\n";
			File << FunctionBuffers[PackageName];
		}
		else
		{
			if (std::filesystem::exists(FunctionPath))
				std::filesystem::remove(FunctionPath);
		}

		std::string ParamsFileName = "FN_" + PackageName + "_parameters.h";
		std::filesystem::path ParamsPath = FolderPath / ParamsFileName;
		if (GeneratedFiles.count(ParamsFileName))
		{
			std::ofstream File(ParamsPath);
			PrintFileHeader(File, PackageName, FullDependencies, "parameters");
			File << ParamBuffers[PackageName];
		}
		else
		{
			if (std::filesystem::exists(ParamsPath))
				std::filesystem::remove(ParamsPath);
		}

		std::string DelegatesFileName = "FN_" + PackageName + "_delegates.h";
		std::filesystem::path DelegatesPath = FolderPath / DelegatesFileName;
		if (GeneratedFiles.count(DelegatesFileName))
		{
			std::ofstream File(DelegatesPath);
			PrintFileHeader(File, PackageName, FullDependencies, "delegates");
			File << DelegateBuffers[PackageName];
		}
		else
		{
			if (std::filesystem::exists(DelegatesPath))
				std::filesystem::remove(DelegatesPath);
		}
	}
}

void Dumper::BuildMinStructSize()
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
		else if (Object->IsA(UFunction::StaticClass()))
		{
			UFunction* Function = Object->Cast<UFunction>();
			if (!Function)
				continue;

			EFunctionFlags FunctionFlags = Function->GetFunctionFlags();
			if (IsDelegateSignature(Function))
			{
				for (UField* Param = Function->GetChildren(); Param; Param = Param->GetNext())
				{
					if (!Param)
						continue;

					if (Param->IsA(UProperty::StaticClass()))
					{
						UProperty* Property = Param->Cast<UProperty>();
						if (!Property)
							continue;

						CollectDependencies(Property, PackageName, Dependencies, bStructuralOnly);
					}
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
	Info.bIsDelegate = Property->IsA(UDelegateProperty::StaticClass()) || Property->IsA(UMulticastDelegateProperty::StaticClass());

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

	// Logger::Log(LogLevel::Info, Property->GetFullName());

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
		if (!ByteProperty) return "uint8_t";
		if (ByteProperty->GetEnum()) return SanitizeName(ByteProperty->GetEnum()->GetName());
		return "uint8_t";
	}

	if (Property->IsA(UObjectProperty::StaticClass()))
	{
		UObjectProperty* ObjectProperty = Property->Cast<UObjectProperty>();
		if (!ObjectProperty) return "Unknown";
		if (ObjectProperty->GetPropertyClass()) return "class " + SanitizeName(ObjectProperty->GetPropertyClass()->GetNameCPP()) + "*";
		return "class UObject*";
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
		if (!ObjectProperty) return "Unknown";
		if (ObjectProperty->GetPropertyClass()) return "TLazyObjectPtr<class " + SanitizeName(ObjectProperty->GetPropertyClass()->GetNameCPP()) + ">";
		return "TLazyObjectPtr<class UObject>";
	}

	if (Property->IsA(UInterfaceProperty::StaticClass()))
	{
		UInterfaceProperty* InterfaceProperty = Property->Cast<UInterfaceProperty>();
		if (!InterfaceProperty) return "Unknown";
		if (InterfaceProperty->GetInterfaceClass()) return "TScriptInterface<class " + SanitizeName(InterfaceProperty->GetInterfaceClass()->GetNameCPP()) + ">";
		return "TScriptInterface<class UObject>";
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

void Dumper::PrintFileHeader(std::ostream& File, const std::string& PackageName, const std::unordered_set<std::string>& Dependencies, const std::string& Type, const std::unordered_set<std::string>& InheritanceDependencies)
{
	File << "#pragma once\n";
	File << "#include \"FN_Basic.h\"\n\n";

	if (Type == "structs" || Type == "classes" || Type == "parameters" || Type == "delegates")
	{
		std::string EnumHeader = "FN_" + PackageName + "_enums.h";
		if (GeneratedFiles.count(EnumHeader))
			File << "#include \"" << EnumHeader << "\"\n";

		if (Type == "classes" || Type == "parameters" || Type == "delegates")
		{
			std::string StructHeader = "FN_" + PackageName + "_structs.h";
			if (GeneratedFiles.count(StructHeader))
				File << "#include \"" << StructHeader << "\"\n";
		}

		if (Type == "classes")
		{
			std::string ParamHeader = "FN_" + PackageName + "_parameters.h";
			if (GeneratedFiles.count(ParamHeader))
				File << "#include \"" << ParamHeader << "\"\n";
		}

		std::vector<std::string> SortedDependencies(Dependencies.begin(), Dependencies.end());
		std::sort(SortedDependencies.begin(), SortedDependencies.end());

		for (const std::string& Dependency : SortedDependencies)
		{
			if (Dependency == PackageName)
				continue;

			std::string DepEnum = "FN_" + Dependency + "_enums.h";
			if (GeneratedFiles.count(DepEnum))
				File << "#include \"" << DepEnum << "\"\n";

			std::string DepDelegates = "FN_" + Dependency + "_delegates.h";
			if (GeneratedFiles.count(DepDelegates))
				File << "#include \"" << DepDelegates << "\"\n";

			std::string DepStruct = "FN_" + Dependency + "_structs.h";
			if (GeneratedFiles.count(DepStruct))
				File << "#include \"" << DepStruct << "\"\n";

			if (Type == "classes")
			{
				if (InheritanceDependencies.count(Dependency))
				{
					std::string DepClass = "FN_" + Dependency + "_classes.h";
					if (GeneratedFiles.count(DepClass))
						File << "#include \"" << DepClass << "\"\n";
				}
			}
		}
	}

	File << "\n/*\n";
	File << "* =========================================================================================\n";
	File << "* Generated by FortGen\n";
	File << "* =========================================================================================\n";
	File << "* PackageName: " << PackageName << "\n";
	File << "*/\n\n";
}

void Dumper::BuildValidStructPackages()
{
	ValidStructPackages.clear();

	for (int i = 0; i < GUObjectArray->GetObjObjects().GetNumElements(); i++)
	{
		UObject* Object = (UObject*)GUObjectArray->GetObjObjects().GetObjects(i)->GetObjectW();
		if (!Object) continue;
		if (!Object->IsA(UScriptStruct::StaticClass())) continue;
		UScriptStruct* Struct = Object->Cast<UScriptStruct>();
		if (!Struct) continue;
		if (Object->GetOutermost()->GetPackageName().empty()) continue;
		bool bValid = (Struct->GetChildren() != nullptr) || (Struct->GetPropertiesSize() > 0);
		if (bValid) ValidStructPackages.insert(SanitizeName(Object->GetOutermost()->GetPackageName()));
	}
}

std::string Dumper::GetFunctionSignature(UFunction* Function, bool bWithScope)
{
	if (!Function)
		return "void Unknown()";

	std::string Prefix = "";
	EFunctionFlags FunctionFlags = Function->GetFunctionFlags();
	if (FunctionFlags & FUNC_Static && !bWithScope)
		Prefix = "static ";

	std::string Suffix = "";
	if (FunctionFlags & FUNC_Const && !(FunctionFlags & FUNC_Static))
		Suffix = " const";

	std::string ReturnValue = "void";
	std::vector<std::string> Params;

	for (UField* Child = Function->GetChildren(); Child; Child = Child->GetNext())
	{
		if (!Child)
			continue;

		if (Child->IsA(UProperty::StaticClass()))
		{
			UProperty* Property = Child->Cast<UProperty>();
			if (!Property)
				continue;

			EPropertyFlags PropertyFlags = Property->GetPropertyFlags();
			if (PropertyFlags & CPF_Parm)
			{
				std::string Type = GetPropertyType(Property);
				std::string Name = GetSafeName(SanitizeName(Property->GetName()), Type);

				if (PropertyFlags & CPF_ReturnParm)
					ReturnValue = Type;
				else
					Params.push_back(Type + " " + Name);
			}
		}
	}

	std::string Scope = "";

	if (bWithScope)
		Scope = SanitizeName(Function->GetOuterPrivate()->GetNameCPP()) + "::";

	std::string Signature = Prefix + ReturnValue + " " + Scope + SanitizeName(Function->GetName()) + "(";

	for (size_t i = 0; i < Params.size(); i++)
	{
		Signature += Params[i];
		if (i != Params.size() - 1)
			Signature += ", ";
	}

	Signature += ")" + Suffix;

	return Signature;
}

std::string Dumper::GetFunctionBody(UFunction* Function)
{
	if (!Function)
		return "";

	std::string OuterName = SanitizeName(Function->GetOuterPrivate()->GetNameCPP());
	std::string FunctionName = SanitizeName(Function->GetName());
	std::string ParamStruct = "F" + OuterName + "_" + FunctionName + "_Params";

	std::ostringstream Body;
	
	Body << "{\n";

	bool bHasParams = false;
	bool bHasReturn = false;

	std::string ReturnValue = "void";
	std::string ReturnProperty = "";

	for (UField* Child = Function->GetChildren(); Child; Child = Child->GetNext())
	{
		if (!Child)
			continue;

		if (Child->IsA(UProperty::StaticClass()))
		{
			UProperty* Property = Child->Cast<UProperty>();
			if (!Property) continue;
			EPropertyFlags PropertyFlags = Property->GetPropertyFlags();
			if (PropertyFlags & CPF_Parm)
			{
				bHasParams = true;
				if (PropertyFlags & CPF_ReturnParm)
				{
					bHasReturn = true;
					ReturnValue = GetPropertyType(Property);
					ReturnProperty = GetSafeName(SanitizeName(Property->GetName()), ReturnValue);
				}
			}
		}
	}

	Body << "\tstatic UFunction* FN = nullptr;\n";
	Body << "\tif (!FN)\n";
	Body << "\t\tFN = UObject::StaticFindObject<UFunction>(\"" << Function->GetPathName() << "\");\n\n";

	if (bHasParams)
	{
		Body << "\t" << ParamStruct << " Parms;\n\n";

		for (UField* Child = Function->GetChildren(); Child; Child = Child->GetNext())
		{
			if (!Child)
				continue;

			if (Child->IsA(UProperty::StaticClass()))
			{
				UProperty* Property = Child->Cast<UProperty>();
				if (!Property) continue;
				EPropertyFlags PropertyFlags = Property->GetPropertyFlags();
				if ((PropertyFlags & CPF_Parm) && !(PropertyFlags & CPF_ReturnParm))
				{
					std::string PropertyName = GetSafeName(SanitizeName(Property->GetName()), GetPropertyType(Property));
					Body << "\tParms." << PropertyName << " = " << PropertyName << ";\n";
				}
			}
		}

		Body << "\n";
	}

	if (Function->GetFunctionFlags() & FUNC_Static)
		Body << "\tStaticClass()->ProcessEvent(FN, " << (bHasParams ? "&Parms" : "nullptr") << ");\n";
	else
		Body << "\tProcessEvent(FN, " << (bHasParams ? "&Parms" : "nullptr") << ");\n";

	Body << "}\n";

	return Body.str();
}

void Dumper::GenerateSDKHeader(std::filesystem::path& HeaderPath)
{
	std::ostringstream Buffer;
	std::ofstream File(HeaderPath);

	std::vector<std::string> SortedFiles(GeneratedFiles.begin(), GeneratedFiles.end());
	
	std::sort(SortedFiles.begin(), SortedFiles.end(), 
		[](const std::string& A, const std::string& B) {

			auto Split = [](const std::string& File)
				{
					size_t Pos = File.find_last_of('_');

					if (Pos == std::string::npos)
						return std::pair<std::string, std::string>{File, ""};

					return std::pair<std::string, std::string>{File.substr(0, Pos), File.substr(Pos + 1)};
				};

			auto [PackageA, TypeA] = Split(A);
			auto [PackageB, TypeB] = Split(B);

		if (PackageA != PackageB)
			return PackageA < PackageB;

		auto Priority = [](const std::string& Type)
			{
				if (Type == "enums.h")
					return 0;

				if (Type == "structs.h")
					return 1;

				if (Type == "delegates.h")
					return 2;

				if (Type == "classes.h")
					return 3;

				if (Type == "parameters")
					return 4;

				return 5;
			};

		return Priority(TypeA) < Priority(TypeB);
		});

	for (const std::string& File : SortedFiles)
	{
		if (File.find(".cpp") != std::string::npos)
			continue;

		Buffer << "#include \"SDK/" << File << "\"\n";
	}

	File << Buffer.str();
	File.close();
}

void Dumper::GenerateBasicHeader(std::filesystem::path& HeaderPath)
{
	std::ostringstream Buffer;
	std::ofstream File(HeaderPath);

	Buffer << "#pragma once\n";
	Buffer << "#include <Windows.h>\n";
	Buffer << "#include <iostream>\n";

	File << Buffer.str();
	File.close();
}

bool Dumper::IsDelegateSignature(UFunction* Function)
{
	if (!Function)
		return false;

	EFunctionFlags FunctionFlags = Function->GetFunctionFlags();
	return (FunctionFlags & (FUNC_Delegate | FUNC_MulticastDelegate)) || Function->GetName().find("__Delegate") != std::string::npos;
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

void Dumper::ProcessClasses(const std::vector<UObject*>& Objects, const std::string& PackageName, std::ostream& File)
{
	for (UObject* Object : Objects)
	{
		if (!Object)
			continue;

		if (Object->IsA(UClass::StaticClass()))
		{
			UClass* Class = Object->Cast<UClass>();
			if (!Class) continue;
			GenerateClasses(Objects, Class, PackageName, File);
		}
	}
}

void Dumper::ProcessFunctions(const std::vector<UObject*>& Objects, std::ostream& File)
{
	for (UObject* Object : Objects)
	{
		if (!Object)
			continue;

		if (Object->IsA(UClass::StaticClass()))
		{
			UClass* Class = Object->Cast<UClass>();
			if (!Class) continue;
			GenerateStaticClass(Class, File);
			GenerateFunction(Objects, Class, File);
		}
		else if (Object->IsA(UScriptStruct::StaticClass()))
		{
			UScriptStruct* Struct = Object->Cast<UScriptStruct>();
			if (!Struct) continue;
			GenerateStaticStruct(Struct, File);
		}
	}
}

void Dumper::ProcessParameters(const std::vector<class UObject*>& Objects, const std::string& PackageName, std::ostream& File)
{
	for (UObject* Object : Objects)
	{
		if (!Object)
			continue;

		if (Object->IsA(UFunction::StaticClass()))
		{
			UFunction* Function = Object->Cast<UFunction>();
			if (!Function) continue;
			if (IsDelegateSignature(Function)) continue;
			GenerateParameters(Function, File);
		}
	}
}

void Dumper::ProcessDelegates(const std::vector<class UObject*>& Objects, const std::string& PackageName, std::ostream& File)
{
	for (UObject* Object : Objects)
	{
		if (!Object)
			continue;

		if (Object->IsA(UFunction::StaticClass()))
		{
			UFunction* Function = Object->Cast<UFunction>();
			if (!Function) continue;
			if (!IsDelegateSignature(Function)) continue;
			GenerateDelegates(Function, File);
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

	std::string StructName = SanitizeName(ScriptStruct->GetNameCPP());
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
				Buffer << "\t" << SDKMC_SSCOL("uint8_t", 100) << " " << SDKMC_SSCOL(PadName.str(), 50) << " // " << SDKMC_SSHEX(CurrentOffset, 4) << " (" << SDKMC_SSHEX(Padding, 4) << ") MISSED OFFSET\n";
			}

			std::string PropertyType = Property.Type;
			std::string PropertyName = Property.Name;

			if (Property.ArrayDim > 1)
				PropertyName += "[" + std::to_string(Property.ArrayDim) + "]";

			if (Property.bIsBool && Property.bIsBitField)
			{
				if (LastBitfieldByteOffset != -1 && LastBitfieldByteOffset != Property.ByteOffset)
					Buffer << "\t" << SDKMC_SSCOL("uint8_t", 100) << " " << SDKMC_SSHEX(": 0;", 50) << "\n";

				Buffer << "\t" << SDKMC_SSCOL("uint8_t", 100) << " " << SDKMC_SSCOL(PropertyName + " : 1;", 50) << " // " << SDKMC_SSHEX(Offset, 4) << " (" << SDKMC_SSHEX(Size, 4) << ") [BITFIELD] [ByteOffset: " << SDKMC_SSHEX(Property.ByteOffset, 2) << "] [Mask: " << SDKMC_SSHEX(Property.ByteMask, 2) << "]\n";

				LastBitfieldByteOffset = Property.ByteOffset;
			}
			else
			{
				Buffer << "\t" << SDKMC_SSCOL(PropertyType, 100) << " " << SDKMC_SSCOL(PropertyName + ";", 50) << " // " << SDKMC_SSHEX(Offset, 4) << " (" << SDKMC_SSHEX(Size, 4) << ")\n";
				LastBitfieldByteOffset = -1;
			}

			CurrentOffset = max(CurrentOffset, Offset + Size);
		}

		if (FullSize > CurrentOffset)
		{
			int32_t Padding = FullSize - CurrentOffset;
			std::ostringstream PadName;

			PadName << "UnknownData_" << std::hex << std::uppercase << CurrentOffset << "[" << SDKMC_SSHEX(Padding, 0) << "];";
			Buffer << "\t" << SDKMC_SSCOL("uint8_t", 100) << " " << SDKMC_SSCOL(PadName.str(), 50) << " // " << SDKMC_SSHEX(CurrentOffset, 4) << " (" << SDKMC_SSHEX(Padding, 4) << ") MISSED OFFSET\n";
		}
	}

	Buffer << "\n\tstatic class UScriptStruct* StaticStruct();\n";

	Buffer << "};\n\n";

	File << Buffer.str();
}

void Dumper::GenerateClasses(const std::vector<class UObject*>& Objects, UClass* Class, const std::string& PackageName, std::ostream& File)
{
	if (!Class)
		return;

	if (ClassesFullName.find(Class->GetFullName()) != ClassesFullName.end())
		return;

	UStruct* SuperStruct = Class->GetSuperStruct();
	if (SuperStruct && SuperStruct->IsA(UClass::StaticClass()) && SanitizeName(SuperStruct->GetOutermost()->GetPackageName()) == PackageName)
	{
		UClass* Struct = SuperStruct->Cast<UClass>();
		if (!Struct) return;
		GenerateClasses(Objects, Struct, PackageName, File);
	}

	if (SanitizeName(Class->GetOutermost()->GetPackageName()) == PackageName)
	{
		std::string FullName = Class->GetFullName();
		GenerateClass(Objects, Class, File);
		ClassesFullName.insert(FullName);
	}
}

void Dumper::GenerateClass(const std::vector<UObject*>& Objects, UClass* Class, std::ostream& File)
{
	if (!Class)
		return;

	if (ClassesFullName.find(Class->GetFullName()) != ClassesFullName.end())
		return;

	std::string StructName = SanitizeName(Class->GetNameCPP());
	if (GeneratedNamesInPackage.count(StructName))
		return;

	GeneratedNamesInPackage.insert(StructName);

	std::ostringstream Buffer;

	std::string SuperStructName = (Class->GetSuperStruct() && Class->GetSuperStruct()->IsA(UClass::StaticClass())) ? SanitizeName(Class->GetSuperStruct()->GetNameCPP()) : "";
	std::string ClassFullName = Class->GetFullName();

	if (ClassFullName.find("Default__") != std::string::npos)
		return;

	Buffer << "// " << ClassFullName << "\n";

	int32_t FullSize = MinStructSize.count(Class) ? MinStructSize[Class] : Class->GetPropertiesSize();
	int32_t SuperSize = (Class->GetSuperStruct() && (Class->GetSuperStruct()->IsA(UClass::StaticClass()) || Class->GetSuperStruct()->IsA(UScriptStruct::StaticClass()))) ? (MinStructSize.count(Class->GetSuperStruct()) ? MinStructSize[Class->GetSuperStruct()] : Class->GetSuperStruct()->GetPropertiesSize()) : 0;

	Buffer << "// " << SDKMC_SSHEX(FullSize - SuperSize, 4) << " (" << SDKMC_SSHEX(FullSize, 4) << ")\n";

	if (!SuperStructName.empty())
		Buffer << "class " << StructName << " : public " << SuperStructName << "\n{\n";
	else
		Buffer << "class " << StructName << "\n{\n";

	std::vector<PropertyInfo> Properties;

	for (UField* Child = Class->GetChildren(); Child; Child = Child->GetNext())
	{
		if (!Child) 
			continue;

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

	std::vector<std::string> Functions;
	std::unordered_set<UFunction*> ProcessedFunctions;

	for (UField* Child = Class->GetChildren(); Child; Child = Child->GetNext())
	{
		if (!Child) continue;
		if (Child->IsA(UFunction::StaticClass()))
		{
			UFunction* Function = Child->Cast<UFunction>();
			if (!Function) continue;
			Functions.push_back(GetFunctionSignature(Function) + "; // " + Child->GetFullName());
			ProcessedFunctions.insert(Function);
		}
	}

	for (UObject* Object : Objects)
	{
		if (!Object || !Object->IsA(UFunction::StaticClass()))
			continue;

		UFunction* Function = Object->Cast<UFunction>();
		if (!Function)
			continue;

		UObject* OuterPrivate = Function->GetOuterPrivate();
		if (!OuterPrivate || OuterPrivate != Class)
			continue;

		if (ProcessedFunctions.find(Function) != ProcessedFunctions.end())
			continue;

		Functions.push_back(GetFunctionSignature(Function) + "; // " + Function->GetFullName());
		ProcessedFunctions.insert(Function);
	}

	if (!Properties.empty() || FullSize > CurrentOffset)
	{
		Buffer << "public:\n";

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
				Buffer << "\t" << SDKMC_SSCOL("uint8_t", 100) << " " << SDKMC_SSCOL(PadName.str(), 50) << " // " << SDKMC_SSHEX(CurrentOffset, 4) << " (" << SDKMC_SSHEX(Padding, 4) << ") MISSED OFFSET\n";
			}

			std::string PropertyType = Property.Type;
			std::string PropertyName = Property.Name;

			if (Property.ArrayDim > 1)
				PropertyName += "[" + std::to_string(Property.ArrayDim) + "]";

			if (Property.bIsBool && Property.bIsBitField)
			{
				if (LastBitfieldByteOffset != -1 && LastBitfieldByteOffset != Property.ByteOffset)
					Buffer << "\t" << SDKMC_SSCOL("uint8_t", 100) << " " << SDKMC_SSHEX(": 0;", 50) << "\n";

				Buffer << "\t" << SDKMC_SSCOL("uint8_t", 100) << " " << SDKMC_SSCOL(PropertyName + " : 1;", 50) << " // " << SDKMC_SSHEX(Offset, 4) << " (" << SDKMC_SSHEX(Size, 4) << ") [BITFIELD] [ByteOffset: " << SDKMC_SSHEX(Property.ByteOffset, 2) << "] [Mask: " << SDKMC_SSHEX(Property.ByteMask, 2) << "]\n";

				LastBitfieldByteOffset = Property.ByteOffset;
			}
			else
			{
				Buffer << "\t" << SDKMC_SSCOL(PropertyType, 100) << " " << SDKMC_SSCOL(PropertyName + ";", 50) << " // " << SDKMC_SSHEX(Offset, 4) << " (" << SDKMC_SSHEX(Size, 4) << ")\n";
				LastBitfieldByteOffset = -1;
			}

			CurrentOffset = max(CurrentOffset, Offset + Size);
		}

		if (FullSize > CurrentOffset)
		{
			int32_t Padding = FullSize - CurrentOffset;
			std::ostringstream PadName;

			PadName << "UnknownData_" << std::hex << std::uppercase << CurrentOffset << "[" << SDKMC_SSHEX(Padding, 0) << "];";
			Buffer << "\t" << SDKMC_SSCOL("uint8_t", 100) << " " << SDKMC_SSCOL(PadName.str(), 50) << " // " << SDKMC_SSHEX(CurrentOffset, 4) << " (" << SDKMC_SSHEX(Padding, 4) << ") MISSED OFFSET\n";
		}
	}

	if (!Functions.empty())
	{
		Buffer << "public:\n";

		for (const std::string& Function : Functions)
			Buffer << "\t" << Function << "\n";
	}

	Buffer << "public:\n";

	Buffer << "\tstatic class UClass* StaticClass();\n";

	Buffer << "};\n\n";

	File << Buffer.str();
}

void Dumper::GenerateStaticClass(UClass* Class, std::ostream& File)
{
	if (!Class)
		return;

	std::string ClassName = SanitizeName(Class->GetNameCPP());
	if (ClassName.find("Default__") != std::string::npos)
		return;

	File << "UClass* " << ClassName << "::StaticClass()\n{\n";
	File << "\tstatic UClass* Class = nullptr;\n";
	File << "\tif (!Class)\n";
	File << "\t\tClass = UObject::StaticFindObject<UClass>(\"" << Class->GetPathName() << "\");\n\n";
	File << "\treturn Class;\n";
	File << "}\n\n";
}

void Dumper::GenerateStaticStruct(UScriptStruct* Struct, std::ostream& File)
{
	if (!Struct)
		return;

	std::string StructName = SanitizeName(Struct->GetNameCPP());
	if (StructName.find("Default__") != std::string::npos)
		return;

	File << "UScriptStruct* " << StructName << "::StaticStruct()\n{\n";
	File << "\tstatic UScriptStruct* Struct = nullptr;\n";
	File << "\tif (!Struct)\n";
	File << "\t\tStruct = UObject::StaticFindObject<UScriptStruct>(\"" << Struct->GetPathName() << "\");\n\n";
	File << "\treturn Struct;\n";
	File << "}\n\n";
}

void Dumper::GenerateFunction(const std::vector<UObject*>& Objects, UClass* Class, std::ostream& File)
{
	if (!Class)
		return;

	std::vector<UFunction*> Functions;
	std::unordered_set<UFunction*> ProcessedFunctions;

	for (UField* Child = Class->GetChildren(); Child; Child = Child->GetNext())
	{
		if (!Child) 
			continue;

		if (Child->IsA(UFunction::StaticClass()))
		{
			UFunction* Function = Child->Cast<UFunction>();
			if (!Function) continue;
			Functions.push_back(Function);
			ProcessedFunctions.insert(Function);
		}
	}

	for (UObject* Object : Objects)
	{
		if (!Object || !Object->IsA(UFunction::StaticClass()))
			continue;

		UFunction* Function = Object->Cast<UFunction>();
		if (!Function)
			continue;

		UObject* OuterPrivate = Function->GetOuterPrivate();
		if (!OuterPrivate || OuterPrivate != Class)
			continue;

		if (ProcessedFunctions.find(Function) != ProcessedFunctions.end())
			continue;

		Functions.push_back(Function);
		ProcessedFunctions.insert(Function);
	}

	if (Functions.empty())
		return;

	for (UFunction* Function : Functions)
	{
		if (!Function)
			continue;

		File << "// " << Function->GetFullName() << "\n";
		File << GetFunctionSignature(Function, true) << "\n";
		File << GetFunctionBody(Function) << "\n";
	}
}

void Dumper::GenerateParameters(UFunction* Function, std::ostream& File)
{
	if (!Function)
		return;

	std::ostringstream Buffer;

	std::string FunctionName = Function->GetName();
	std::string FunctionFullName = Function->GetFullName();
	if (FunctionFullName.find("Default__") != std::string::npos)
		return;

	std::string OuterName = SanitizeName(Function->GetOuterPrivate()->GetNameCPP());
	std::string StructName = "F" + OuterName + "_" + SanitizeName(FunctionName) + "_Params";

	Buffer << "// " << FunctionFullName << "\n";
	Buffer << "struct " << StructName << "\n{\n";

	std::vector<PropertyInfo> Properties;

	for (UField* Child = Function->GetChildren(); Child; Child = Child->GetNext())
	{
		if (!Child)
			continue;

		if (Child->IsA(UProperty::StaticClass()))
		{
			UProperty* Property = Child->Cast<UProperty>();
			if (!Property) continue;
			if (Property->GetPropertyFlags() & CPF_Parm)
			{
				PropertyInfo Info;
				GeneratePropertyInfo(Property, Info);
				Properties.push_back(Info);
			}
		}
	}

	if (Properties.empty())
		return;

	std::sort(Properties.begin(), Properties.end(), PropertyInfo::Sort);

	int32_t CurrentOffset = 0;

	int32_t LastBitfieldByteOffset = 0;

	for (const PropertyInfo& Property : Properties)
	{
		int32_t Offset = Property.Offset;
		int32_t Size = Property.Size;

		if (Offset > CurrentOffset)
		{
			int32_t Padding = Offset - CurrentOffset;
			std::ostringstream PadName;

			PadName << "UnknownData_" << std::hex << std::uppercase << CurrentOffset << "[" << SDKMC_SSHEX(Padding, 0) << "];";
			Buffer << "\t" << SDKMC_SSCOL("uint8_t", 100) << " " << SDKMC_SSCOL(PadName.str(), 50) << " // " << SDKMC_SSHEX(CurrentOffset, 4) << " (" << SDKMC_SSHEX(Padding, 4) << ") MISSED OFFSET\n";
		}

		std::string PropertyType = Property.Type;
		std::string PropertyName = Property.Name;

		if (Property.ArrayDim > 1)
			PropertyName += "[" + std::to_string(Property.ArrayDim) + "]";

		if (Property.bIsBool && Property.bIsBitField)
		{
			if (LastBitfieldByteOffset != -1 && LastBitfieldByteOffset != Property.ByteOffset)
				Buffer << "\t" << SDKMC_SSCOL("uint8_t", 100) << " " << SDKMC_SSHEX(": 0;", 50) << "\n";

			Buffer << "\t" << SDKMC_SSCOL("uint8_t", 100) << " " << SDKMC_SSCOL(PropertyName + " : 1;", 50) << " // " << SDKMC_SSHEX(Offset, 4) << " (" << SDKMC_SSHEX(Size, 4) << ") [BITFIELD] [ByteOffset: " << SDKMC_SSHEX(Property.ByteOffset, 2) << "] [Mask: " << SDKMC_SSHEX(Property.ByteMask, 2) << "]\n";

			LastBitfieldByteOffset = Property.ByteOffset;
		}
		else
		{
			Buffer << "\t" << SDKMC_SSCOL(PropertyType, 100) << " " << SDKMC_SSCOL(PropertyName + ";", 50) << " // " << SDKMC_SSHEX(Offset, 4) << " (" << SDKMC_SSHEX(Size, 4) << ")\n";
			LastBitfieldByteOffset = -1;
		}

		CurrentOffset = max(CurrentOffset, Offset + Size);
	}

	Buffer << "};\n\n";

	File << Buffer.str();
}

void Dumper::GenerateDelegates(UFunction* Function, std::ostream& File)
{
	if (!Function)
		return;

	std::ostringstream Buffer;

	std::string FunctionName = Function->GetName();
	std::string FunctionFullName = Function->GetFullName();
	if (FunctionFullName.find("Default__") != std::string::npos)
		return;

	std::string OuterName = SanitizeName(Function->GetOuterPrivate()->GetNameCPP());
	std::string StructName = "F" + OuterName + "_" + SanitizeName(FunctionName);

	Buffer << "// " << FunctionFullName << "\n";

	bool bIsMulticast = (Function->GetFunctionFlags() & FUNC_MulticastDelegate) != 0;
	std::string BaseClass = bIsMulticast ? "FMulticastScriptDelegate" : "FScriptDelegate";

	Buffer << "struct " << StructName << " : public " << BaseClass << "\n{\n};\n\n";

	File << Buffer.str();
}
