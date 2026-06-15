#include <Windows.h>
#include <iostream>
#include <fstream>
#include <set>
#include <print>

#define MsgBox(...) MessageBoxA(NULL, std::format(__VA_ARGS__).c_str(), "HeistedServer", MB_OK)
#define JustLoadMeIntoAsteriaIWantToDumpAnSDK 0

#include <SDK/FortniteGame_classes.hpp>
#include <SDK/LagerRuntime_classes.hpp>
#include <SDK/GameplayTags_classes.hpp>
#include <SDK/SafeZoneIndicator_classes.hpp>
#include <SDK/BGA_KeysAndLocks_DisplayCase_classes.hpp>
using namespace SDK;

#include <Frame.hpp>
#include <Hook.hpp>
#include <Utils.hpp>
#include <Memcury.hpp>

#include "Inventory.hpp"
#include "GameFeatures.hpp"
#include "Loot.hpp"
#include "Vehicles.hpp"

#include "GamePhaseLogic.hpp"
#include "Net.hpp"
#include "DataLayers.hpp"
#include "Abilities.hpp"
#include "GameMode.hpp"
#include "Player.hpp"
#include "Building.hpp"

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
    FFrame::Init();
    Net::Init();
    GameMode::Init();
    Player::Init();
    Abilities::Init();
    Inventory::Init();
    Building::Init();

    Utils::FindObjectFast<UCurieGlobals>("Default__CurieGlobals")->bEnableCurie = false;

    Utils::ExecuteConsoleCommand(L"log LogFortUIDirector None");
    Utils::ExecuteConsoleCommand(L"log LogGarbage None");
    Utils::ExecuteConsoleCommand(L"log LogOnline None");
    Utils::ExecuteConsoleCommand(L"log LogOnlineParty None");
    Utils::ExecuteConsoleCommand(L"log LogOnlineIdentity None");
    Utils::ExecuteConsoleCommand(L"log LogFortChat None");
    Utils::ExecuteConsoleCommand(L"log LogFortLoadingScreen None");
    Utils::ExecuteConsoleCommand(L"log LogEOSMessaging None");

    // Utils::ExecuteConsoleCommand(L"log LogAbilitySystem VeryVerbose");
    // Utils::ExecuteConsoleCommand(L"log LogFort VeryVerbose");

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
