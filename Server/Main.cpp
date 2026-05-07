#include <Windows.h>
#include <iostream>
#include <fstream>

#define MsgBox(...) MessageBoxA(NULL, std::format(__VA_ARGS__).c_str(), "HeistedServer", MB_OK)
#define JustLoadMeIntoAsteriaIWantToDumpAnSDK 0

#include <SDK/FortniteGame_classes.hpp>
using namespace SDK;

#include <Hook.hpp>
#include <Utils.hpp>

#include "Net.hpp"
#include "Inventory.hpp"
#include "GameMode.hpp"
#include "Player.hpp"
#include "Abilities.hpp"

DWORD MainThread(HMODULE Module)
{
    AllocConsole();
    FILE* Dummy;
    freopen_s(&Dummy, "CONOUT$", "w", stdout);
    freopen_s(&Dummy, "CONIN$", "r", stdin);

    MH_Initialize();

    auto ImageBase = InSDKUtils::GetImageBase();
    Hook::Function(ImageBase + 0x31E946C, Hook::ReturnHook); // RequestExit

#if JustLoadMeIntoAsteriaIWantToDumpAnSDK
    Utils::ExecuteConsoleCommand(L"open Asteria_Terrain");
    return 0;
#endif

    FMemory::Init((void*)(InSDKUtils::GetImageBase() + 0x3E54B6C));
    Net::Init();
    GameMode::Init();
    Player::Init();
    Abilities::Init();
    Inventory::Init();

    Utils::ExecuteConsoleCommand(L"log LogFortUIDirector None");
    Utils::ExecuteConsoleCommand(L"log LogAbilitySystem VeryVerbose");
    Utils::ExecuteConsoleCommand(L"open Asteria_Terrain");

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
