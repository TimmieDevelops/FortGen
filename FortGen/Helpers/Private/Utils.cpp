#include "pch.h"

void Utils::InitConsole()
{
    /* Code to open a console window */
    AllocConsole();
    FILE* Dummy;
    freopen_s(&Dummy, "CONOUT$", "w", stdout);
    freopen_s(&Dummy, "CONIN$", "r", stdin);
    SetConsoleTitle(L"FortGen");
}
