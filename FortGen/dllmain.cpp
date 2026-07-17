// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

DWORD MainThread(HMODULE Module)
{
    Utils::InitConsole();
    if (Logger::Init()) Logger::Log(LogLevel::Info, "FortGen DLL loaded and initialized successfully.");
    Address::SetupAddress();
    VersionInfo::InitParseVersion();
    Address::SetupOffsets();
    // GUObjectArray = reinterpret_cast<FUObjectArray*>(Scanner::GetModuleBase() + Address::GUObjectArray);
    GUObjectArray = decltype(GUObjectArray)(Scanner::GetModuleBase() + Address::GUObjectArray);
    Logger::Log(LogLevel::Info, std::format("NumElements={}", GUObjectArray->GetObjObjects().GetNumElements()).c_str());
    for (int i = 0; i < GUObjectArray->GetObjObjects().GetNumElements(); i++)
    {
        Logger::Log(LogLevel::Info, std::format("fmgnio gay={}", i).c_str());
        UObjectBase* Object = GUObjectArray->GetObjObjects().GetObjects(i)->GetObjectW();
        if (!Object)
        {
            Logger::Log(LogLevel::Info, "Object is null!");
            continue;
        }

        Logger::Log(LogLevel::Info, Object->GetNamePrivate().ToString());
    }
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
