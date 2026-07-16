#include "pch.h"

void Address::SetupAddress()
{
	uintptr_t ImageBase = Scanner::GetModuleBase();

	FMemory_Realloc = Finder::FindFMemory_Realloc() - ImageBase;
	Logger::Log(LogLevel::Info, std::format("FMemory::ReallocExternal: 0x{:X}", FMemory_Realloc).c_str());

	UKismetSystemLibrary_GetEngineVersion = Finder::FindUKismetSystemLibrary_GetEngineVersion() - ImageBase;
	Logger::Log(LogLevel::Info, std::format("UKismetSystemLibrary::GetEngineVersion: 0x{:X}", UKismetSystemLibrary_GetEngineVersion).c_str());

	GUObjectArray = Finder::FindGUObjectArray() - ImageBase;
	Logger::Log(LogLevel::Info, std::format("GUObjectArray: 0x{:X}", GUObjectArray).c_str());
}

void Address::SetupOffsets()
{
	double EngineVersion = VersionInfo::EngineVersion;
	int CL = VersionInfo::CL; // just incase we need this i guess

	if (EngineVersion == 4.12)
	{
		FUObjectItem_Object = 0x0;
		Logger::Log(LogLevel::Info, std::format("FUObjectItem::Object: 0x{:X}", FUObjectItem_Object).c_str());
		FUObjectItem_ClusterAndFlags = 0x4;
		Logger::Log(LogLevel::Info, std::format("FUObjectItem::ClusterAndFlags: 0x{:X}", FUObjectItem_ClusterAndFlags).c_str());
		FUObjectItem_SerialNumber = 0x8;
		Logger::Log(LogLevel::Info, std::format("FUObjectItem::SerialNumber: 0x{:X}", FUObjectItem_SerialNumber).c_str());
		FUObjectItem_StructSize = 0xC;
		Logger::Log(LogLevel::Info, std::format("FUObjectItem::StructSize: 0x{:X}", FUObjectItem_StructSize).c_str());

		FFixedUObjectArray_Objects = 0x0;
		Logger::Log(LogLevel::Info, std::format("FFixedUObjectArray::Objects: 0x{:X}", FFixedUObjectArray_Objects).c_str());
		FFixedUObjectArray_MaxElements = 0x4;
		Logger::Log(LogLevel::Info, std::format("FFixedUObjectArray::MaxElements: 0x{:X}", FFixedUObjectArray_MaxElements).c_str());
		FFixedUObjectArray_NumElements = 0x8;
		Logger::Log(LogLevel::Info, std::format("FFixedUObjectArray::NumElements: 0x{:X}", FFixedUObjectArray_NumElements).c_str());

		FUObjectItem_Object = 0x0;
		Logger::Log(LogLevel::Info, std::format("FUObjectItem::Object: 0x{:X}", FUObjectItem_Object).c_str());
	}
}
