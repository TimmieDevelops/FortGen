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
	static void InitMinStructSize();
	static std::string SanitizeName(std::string Name);
	static std::unordered_set<std::string> GetPackageDependencies(const std::string& PackageName, const std::vector<UObject*>& Objects, bool bStructuralOnly = false, bool bClassSuperOnly = false);
	static void CollectDependencies(class UProperty* Property, const std::string& PackageName, std::unordered_set<std::string>& Dependencies, bool bStructuralOnly = false);
	static void GeneratePropertyInfo(class UProperty* Property, PropertyInfo& Info);
	static std::string GetPropertyType(class UProperty* Property);
	static std::string GetSafeName(const std::string& Name, const std::string& Type);
private:
	static void ProcessEnums(const std::vector<UObject*>& Objects, const std::string& PackageName, std::ostream& File);
	static void ProcessScriptStructs(const std::vector<UObject*>& Objects, const std::string& PackageName, std::ostream& File);
private:
	static void GenerateEnum(class UEnum* Enum, std::ostream& File);
	static void GenerateScriptStructs(class UScriptStruct* ScriptStruct, const std::string& PackageName, std::ostream& File);
	static void GenerateScriptStruct(UScriptStruct* ScriptStruct, std::ostream& File);
private:
	inline static std::unordered_map<class UStruct*, int32_t> MinStructSize;
	inline static std::unordered_set<std::string> ClassesFullName;
	inline static std::unordered_set<std::string> GeneratedFiles;
	inline static std::unordered_set<std::string> GeneratedNamesInPackage;
	inline static std::unordered_set<std::string> ProcessingScriptStructs;
	inline static std::unordered_set<std::string> ScriptStructsFullName;
};