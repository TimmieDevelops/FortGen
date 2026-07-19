#pragma once
#include "framework.h"

struct PropertyInfo
{
	std::string Name;
	std::string Type;
	int32_t Offset;
	int32_t Size;
	int32_t ArrayDim;
	bool bIsBool;
	bool bIsBitField;
	uint8_t ByteOffset;
	uint8_t ByteMask;

	static bool Sort(const PropertyInfo& A, const PropertyInfo& B)
	{
		if (A.Offset == B.Offset)
		{
			if (A.bIsBool && B.bIsBool)
			{
				if (A.ByteOffset != B.ByteOffset)
					return A.ByteOffset < B.ByteOffset;

				return A.ByteMask < B.ByteMask;
			}

			return A.Name < B.Name;
		}

		return A.Offset < B.Offset;
	}
};

class Dumper
{
public:
	static void Initialize();
private:
	static void DumpObjects(std::filesystem::path& FolderPath);
	static void ProcessPackages(std::filesystem::path& FolderPath);
	static void BuildMinStructSize();
	static std::string SanitizeName(std::string Name);
	static std::unordered_set<std::string> GetPackageDependencies(const std::string& PackageName, const std::vector<class UObject*>& Objects, bool bStructuralOnly = false, bool bClassSuperOnly = false);
	static void CollectDependencies(class UProperty* Property, const std::string& PackageName, std::unordered_set<std::string>& Dependencies, bool bStructuralOnly = false);
	static void GeneratePropertyInfo(class UProperty* Property, PropertyInfo& Info);
	static std::string GetPropertyType(class UProperty* Property);
	static std::string GetSafeName(const std::string& Name, const std::string& Type);
	static void PrintFileHeader(std::ostream& File, const std::string& PackageName, const std::unordered_set<std::string>& Dependencies = {}, const std::string& Type = "None", const std::unordered_set<std::string>& InheritanceDependencies = {});
	static void BuildValidStructPackages();
	static std::string GetFunctionSignature(class UFunction* Function, bool bWithScope = false);
	static std::string GetFunctionBody(class UFunction* Function);
	static void GenerateSDKHeader(std::filesystem::path& HeaderPath);
	static void GenerateBasicHeader(std::filesystem::path& HeaderPath);
	static bool IsDelegateSignature(class UFunction* Function);
private:
	static void ProcessEnums(const std::vector<class UObject*>& Objects, const std::string& PackageName, std::ostream& File);
	static void ProcessScriptStructs(const std::vector<class UObject*>& Objects, const std::string& PackageName, std::ostream& File);
	static void ProcessClasses(const std::vector<class UObject*>& Objects, const std::string& PackageName, std::ostream& File);
	static void ProcessFunctions(const std::vector<class UObject*>& Objects, std::ostream& File);
	static void ProcessParameters(const std::vector<class UObject*>& Objects, const std::string& PackageName, std::ostream& File);
	static void ProcessDelegates(const std::vector<class UObject*>& Objects, const std::string& PackageName, std::ostream& File);
private:
	static void GenerateEnum(class UEnum* Enum, std::ostream& File);
	static void GenerateScriptStructs(class UScriptStruct* ScriptStruct, const std::string& PackageName, std::ostream& File);
	static void GenerateScriptStruct(class UScriptStruct* ScriptStruct, std::ostream& File);
	static void GenerateClasses(const std::vector<class UObject*>& Objects, class UClass* Class, const std::string& PackageName, std::ostream& File);
	static void GenerateClass(const std::vector<class UObject*>& Objects, class UClass* Class, std::ostream& File);
	static void GenerateStaticClass(class UClass* Class, std::ostream& File);
	static void GenerateStaticStruct(class UScriptStruct* Struct, std::ostream& File);
	static void GenerateFunction(const std::vector<class UObject*>& Objects, class UClass* Class, std::ostream& File);
	static void GenerateParameters(class UFunction* Function, std::ostream& File);
private:
	inline static std::unordered_map<class UStruct*, int32_t> MinStructSize;
	inline static std::unordered_set<std::string> ClassesFullName;
	inline static std::unordered_set<std::string> GeneratedFiles;
	inline static std::unordered_set<std::string> GeneratedNamesInPackage;
	inline static std::unordered_set<std::string> ProcessingScriptStructs;
	inline static std::unordered_set<std::string> ScriptStructsFullName;
	inline static std::unordered_set<std::string> ValidStructPackages;
};