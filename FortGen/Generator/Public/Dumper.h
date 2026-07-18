#pragma once
#include "framework.h"

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
private:
	static void ProcessEnums(const std::vector<UObject*>& Objects, const std::string& PackageName, std::ostream& File);
private:
	static void GenerateEnum(class UEnum* Enum, std::ostream& File);
private:
	inline static std::unordered_map<class UStruct*, int32_t> MinStructSize;
	inline static std::unordered_set<std::string> ClassesFullName;
	inline static std::unordered_set<std::string> GeneratedFiles;
	inline static std::unordered_set<std::string> GeneratedNamesInPackage;
};