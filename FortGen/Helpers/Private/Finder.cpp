#include "pch.h"

void Finder::SetupAddress()
{
    uintptr_t ImageBase = Utils::GetImageBase();

    // --- INITIALIZE ADDRESS HERE ---
    Address::GUObjectArray = FindGUObjectArray() - ImageBase;
    Utils::Logger("Finder::SetupAddress", std::format("GUObjectArray: 0x{:X}", Address::GUObjectArray).c_str());

    Address::Memory_Realloc = FindMemory_Realloc() - ImageBase;
    Utils::Logger("Finder::SetupAddress", std::format("FMemory::Realloc: 0x{:X}", Address::Memory_Realloc).c_str());
    // --- END ADDRESS LINE ---

    // --- INITIALIZE OFFSETS HERE ---
    Address::UObjectArray_ObjObjects = 0x0;
    Utils::Logger("Finder::SetupAddress", std::format("FUObjectArray::ObjObjects: 0x{:X}", Address::UObjectArray_ObjObjects).c_str());

    Address::FixedUObjectArray_Objects = 0x0;
    Utils::Logger("Finder::SetupAddress", std::format("FFixedUObjectArray::ObjObjects: 0x{:X}", Address::FixedUObjectArray_Objects).c_str());
    Address::FixedUObjectArray_MaxElements = 0x4;
    Utils::Logger("Finder::SetupAddress", std::format("FFixedUObjectArray::MaxElements: 0x{:X}", Address::FixedUObjectArray_MaxElements).c_str());
    Address::FixedUObjectArray_NumElements = 0x8;
    Utils::Logger("Finder::SetupAddress", std::format("FFixedUObjectArray::NumElements: 0x{:X}", Address::FixedUObjectArray_NumElements).c_str());

    Address::UObjectItem_Object = 0x0;
    Utils::Logger("Finder::SetupAddress", std::format("FUObjectItem::Object: 0x{:X}", Address::UObjectItem_Object).c_str());
    Address::UObjectItem_ClusterAndFlags = 0x4;
    Utils::Logger("Finder::SetupAddress", std::format("FUObjectItem::ClusterAndFlags: 0x{:X}", Address::UObjectItem_ClusterAndFlags).c_str());
    Address::UObjectItem_SerialNumber = 0x8;
    Utils::Logger("Finder::SetupAddress", std::format("FUObjectItem::SerialNumber: 0x{:X}", Address::UObjectItem_SerialNumber).c_str());
    // --- END OFFSETS LINE ---
}

uintptr_t Finder::FindGUObjectArray()
{
    uintptr_t StringRef = Scanner::FindString("SHOWPENDINGKILLS", true);
    if (!Scanner::IsValid(StringRef))
    {
        Utils::Logger("Finder::FindGUObjectArray", "StringRef is invalid!", Error);
        return -1;
    }

    for (int i = 0; i < 0x1000; i++)
    {
        uint8_t* Bytes = reinterpret_cast<uint8_t*>(StringRef + i);

        /* OT6.5
        * A1 AC 3A DC 02
        */
        if (Bytes[0] == 0xA1 && Bytes[1] == 0xAC && Bytes[2] == 0x3A)
            return GetResolveInstructionAddress(Bytes, Mov);

    }

    Utils::Logger("Finder::FindGUObjectArray", "Address is not found!", Error);
    return -1;
}

uintptr_t Finder::FindMemory_Realloc()
{
    uintptr_t AddressRef = Scanner::FindPattern("8B 0D ? ? ? ? 85 C9 75 ? E8 ? ? ? ? 8B 0D ? ? ? ? FF 74 24 ? 8B 01 FF 74 24 ? FF 74 24"); // OT6.5 (UE4.12)
    if (!Scanner::IsValid(AddressRef))
    {
        Utils::Logger("Finder::FindGUObjectArray", "AddressRef is invalid!", Error);
        return -1;
    }

    return AddressRef;
}

uintptr_t Finder::GetResolveInstructionAddress(uint8_t* Address, EInstructionType Type)
{
    switch (Type)
    {
    case EInstructionType::Mov:
    {
        uintptr_t Offset = *(uintptr_t*)(Address + 1);
        return Offset;
    }
    }

    return -1;
}
