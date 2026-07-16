// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

DWORD MainThread(HMODULE Module)
{
    Utils::InitConsole();
    int i = 0x4;
    if (Logger::Init()) Logger::Log(LogLevel::Info, "FortGen DLL loaded and initialized successfully.");
    Logger::Log(LogLevel::Info, std::format("0x{:X}", Finder::FindUKismetSystemLibrary_GetEngineVersion() - Scanner::GetModuleBase()));
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
