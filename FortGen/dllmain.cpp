// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

DWORD MainThread(HMODULE Module)
{
    Utils::InitConsole();
    Utils::InitLogger();
    Finder::SetupAddress();
    Utils::Logger("Main", "FAHHH");
    for (int i = 0; i < GUObjectArray->ObjObjects.NumElements; i++)
        Utils::Logger("Main", std::format("i={}", i).c_str());
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, 0);
        break;
    }

    return TRUE;
}
