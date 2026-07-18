#include "pch.h"

void Address::SetupAddress()
{
	uintptr_t ImageBase = Scanner::GetModuleBase();

	FMemory_Realloc = Finder::FindFMemory_Realloc() - ImageBase;
	Logger::Log(LogLevel::Info, std::format("FMemory::ReallocExternal: 0x{:X}", FMemory_Realloc).c_str());

	UKismetSystemLibrary_GetEngineVersion = Finder::FindUKismetSystemLibrary_GetEngineVersion() - ImageBase;
	Logger::Log(LogLevel::Info, std::format("UKismetSystemLibrary::GetEngineVersion: 0x{:X}", UKismetSystemLibrary_GetEngineVersion).c_str());
	UKismetSystemLibrary_GetPathName = Finder::FindUKismetSystemLibrary_GetPathName() - ImageBase;
	Logger::Log(LogLevel::Info, std::format("UKismetSystemLibrary::GetPathName: 0x{:X}", UKismetSystemLibrary_GetPathName).c_str());

	GUObjectArray = Finder::FindGUObjectArray() - ImageBase;
	Logger::Log(LogLevel::Info, std::format("GUObjectArray: 0x{:X}", GUObjectArray).c_str());

	StaticLoadObject = Finder::FindStaticLoadObject() - ImageBase;
	Logger::Log(LogLevel::Info, std::format("StaticLoadObject: 0x{:X}", StaticLoadObject).c_str());
	StaticFindObject = Finder::FindStaticFindObject() - ImageBase;
	Logger::Log(LogLevel::Info, std::format("StaticFindObject: 0x{:X}", StaticFindObject).c_str());

	FName_ToString = Finder::FindFName_ToString() - ImageBase;
	Logger::Log(LogLevel::Info, std::format("FName::ToString: 0x{:X}", FName_ToString).c_str());
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

		UObjectBase_ObjectFlags = 0x4;
		Logger::Log(LogLevel::Info, std::format("UObjectBase::ObjectFlags: 0x{:X}", UObjectBase_ObjectFlags).c_str());
		UObjectBase_InternalIndex = 0x8;
		Logger::Log(LogLevel::Info, std::format("UObjectBase::InternalIndex: 0x{:X}", UObjectBase_InternalIndex).c_str());
		UObjectBase_ClassPrivate = 0xC;
		Logger::Log(LogLevel::Info, std::format("UObjectBase::ClassPrivate: 0x{:X}", UObjectBase_ClassPrivate).c_str());
		UObjectBase_NamePrivate = 0x10;
		Logger::Log(LogLevel::Info, std::format("UObjectBase::NamePrivate: 0x{:X}", UObjectBase_NamePrivate).c_str());
		UObjectBase_OuterPrivate = 0x18;
		Logger::Log(LogLevel::Info, std::format("UObjectBase::OuterPrivate: 0x{:X}", UObjectBase_OuterPrivate).c_str());

		UField_Next = 0x1C;
		Logger::Log(LogLevel::Info, std::format("UField::Next: 0x{:X}", UField_Next).c_str());

		UStruct_SuperStruct = 0x20;
		Logger::Log(LogLevel::Info, std::format("UStruct::SuperStruct: 0x{:X}", UStruct_SuperStruct).c_str());
		UStruct_Children = 0x24;
		Logger::Log(LogLevel::Info, std::format("UStruct::Children: 0x{:X}", UStruct_Children).c_str());
		UStruct_PropertiesSize = 0x28;
		Logger::Log(LogLevel::Info, std::format("UStruct::PropertiesSize: 0x{:X}", UStruct_PropertiesSize).c_str());
		UStruct_MinAlignment = 0x2C;
		Logger::Log(LogLevel::Info, std::format("UStruct::MinAlignment: 0x{:X}", UStruct_MinAlignment).c_str());

		UClass_ClassDefaultObject = 0x2A; // some reason in IDA it's says this is the correct offset (i prob need to change the UStruct offsets soon)
		Logger::Log(LogLevel::Info, std::format("UClass::ClassDefaultObject: 0x{:X}", UClass_ClassDefaultObject).c_str());

		UProperty_ArrayDim = 0x20;
		Logger::Log(LogLevel::Info, std::format("UProperty::ArrayDim: 0x{:X}", UProperty_ArrayDim).c_str());
		UProperty_ElementSize = 0x24;
		Logger::Log(LogLevel::Info, std::format("UProperty::ElementSize: 0x{:X}", UProperty_ElementSize).c_str());
		UProperty_PropertyFlags = 0x28;
		Logger::Log(LogLevel::Info, std::format("UProperty::PropertyFlags: 0x{:X}", UProperty_PropertyFlags).c_str());
		UProperty_OffsetInternal = 0x3C;
		Logger::Log(LogLevel::Info, std::format("UProperty::OffsetInternal: 0x{:X}", UProperty_OffsetInternal).c_str());

		UFunction_FunctionFlags = 0x58;
		Logger::Log(LogLevel::Info, std::format("UFunction:LFunctionFlags: 0x{:X}", UFunction_FunctionFlags).c_str());
		UFunction_Func = 0x6C;
		Logger::Log(LogLevel::Info, std::format("UFunction::Func: 0x{:X}", UFunction_Func).c_str());

		UStructProperty_Struct = 0x50;
		Logger::Log(LogLevel::Info, std::format("UStructProperty::Struct: 0x{:X}", UStructProperty_Struct).c_str());

		UByteProperty_Enum = 0x50;
		Logger::Log(LogLevel::Info, std::format("UByteProperty::Enum: 0x{:X}", UByteProperty_Enum).c_str());

		UArrayProperty_Inner = 0x50;
		Logger::Log(LogLevel::Info, std::format("UArrayProperty::Inner: 0x{:X}", UArrayProperty_Inner).c_str());

		UMapProperty_KeyProp = 0x50;
		Logger::Log(LogLevel::Info, std::format("UMapProperty::KeyProp: 0x{:X}", UMapProperty_KeyProp).c_str());
		UMapProperty_ValueProp = 0x54;
		Logger::Log(LogLevel::Info, std::format("UMapProperty::ValueProp: 0x{:X}", UMapProperty_ValueProp).c_str());

		UDelegateProperty_SignatureFunction = 0x50;
		Logger::Log(LogLevel::Info, std::format("UDelegateProperty::SignatureFunction: 0x{:X}", UDelegateProperty_SignatureFunction).c_str());

		UMulticastDelegateProperty_SignatureFunction = 0x50;
		Logger::Log(LogLevel::Info, std::format("UMulticastDelegateProperty::SignatureFunction: 0x{:X}", UMulticastDelegateProperty_SignatureFunction).c_str());

		UEnum_Names = 0x2C; // IDA
		Logger::Log(LogLevel::Info, std::format("UEnum::Names: 0x{:X}", UEnum_Names).c_str());

		UBoolProperty_FieldSize = 0x50;
		Logger::Log(LogLevel::Info, std::format("UBoolProperty::FieldSize: 0x{:X}", UBoolProperty_FieldSize).c_str());
		UBoolProperty_ByteOffset = 0x51;
		Logger::Log(LogLevel::Info, std::format("UBoolProperty::ByteOffset: 0x{:X}", UBoolProperty_ByteOffset).c_str());
		UBoolProperty_ByteMask = 0x52;
		Logger::Log(LogLevel::Info, std::format("UBoolProperty::ByteMask: 0x{:X}", UBoolProperty_ByteMask).c_str());
		UBoolProperty_FieldMask = 0x53;
		Logger::Log(LogLevel::Info, std::format("UBoolProperty::FieldMask: 0x{:X}", UBoolProperty_FieldMask).c_str());

		UObjectPropertyBase_PropertyClass = 0x50;
		Logger::Log(LogLevel::Info, std::format("UObjectPropertyBase::PropertyClass: 0x{:X}", UObjectPropertyBase_PropertyClass).c_str());

		UMapProperty_KeyProp = 0x50;
		Logger::Log(LogLevel::Info, std::format("UMapProperty::KeyProp: 0x{:X}", UMapProperty_KeyProp).c_str());
		UMapProperty_ValueProp = 0x54;
		Logger::Log(LogLevel::Info, std::format("UMapProperty::ValueProp: 0x{:X}", UMapProperty_ValueProp).c_str());
	}
}
