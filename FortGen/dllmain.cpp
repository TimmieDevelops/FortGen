// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

DWORD MainThread(HMODULE Module)
{
    Utils::InitConsole();
    if (Logger::Init()) Logger::Log(LogLevel::Info, "FortGen DLL loaded and initialized successfully.");
    Address::SetupAddress();
    VersionInfo::InitParseVersion();
    Address::SetupOffsets();
    GUObjectArray = reinterpret_cast<FUObjectArray*>(Scanner::GetModuleBase() + Address::GUObjectArray);
    for (int i = 0; i < GUObjectArray->GetObjObjects().GetNumElements(); i++)
        Logger::Log(LogLevel::Info, std::format("fmgnio gay={}", i).c_str());
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
        break;
    case DLL_PROCESS_DETACH:
        Logger::Shutdown();
        break;
    }

    return TRUE;
}
