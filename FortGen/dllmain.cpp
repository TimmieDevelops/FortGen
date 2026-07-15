// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "Helpers/Public/Utils.h"
#include "Helpers/Public/Logger.h"

DWORD MainThread(HMODULE Module)
{
    Utils::InitConsole();

    if (Logger::Init("fortgen_log.txt"))
    {
        Logger::Log(LogLevel::Info, "FortGen DLL loaded and initialized successfully.");
        Logger::Log(LogLevel::Warning, "This is a test warning log message.");
        Logger::Log(LogLevel::Error, "This is a test error log message.");
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
