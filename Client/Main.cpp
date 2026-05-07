#include <Windows.h>
#include <iostream>
#include <fstream>

#include <SDK/FortniteGame_classes.hpp>
using namespace SDK;

#include <Hook.hpp>

bool (*UWorldExecOriginal)(UWorld* World, int64 a2, const wchar_t* Cmd, int64 a4);
bool UWorldExecHook(UWorld* World, int64 a2, const wchar_t* Cmd, int64 a4)
{
    if (wcscmp(Cmd, L"givemecheats") == 0)
    {
        auto PlayerController = World->OwningGameInstance->LocalPlayers[0]->PlayerController;
        PlayerController->CheatManager = (UCheatManager*)UGameplayStatics::SpawnObject(UCheatManager::StaticClass(), PlayerController);
        return true;
    }
    else if (wcscmp(Cmd, L"dumpobjects") == 0)
    {
        return true;
    }

    return UWorldExecOriginal(World, a2, Cmd, a4);
}

// void crashit()
// {
//     ((AFortPickup*)UWorld::GetWorld())->TossPickup({}, (AFortPawn*)UWorld::GetWorld(), INT32_MAX, true, UINT8_MAX, UINT8_MAX);
// }

DWORD MainThread(HMODULE Module)
{
    AllocConsole();
    FILE* Dummy;
    freopen_s(&Dummy, "CONOUT$", "w", stdout);
    freopen_s(&Dummy, "CONIN$", "r", stdin);

    MH_Initialize();

    auto ImageBase = InSDKUtils::GetImageBase();
    Hook::Function(ImageBase + 0x31E946C, Hook::ReturnHook); // RequestExit
    Hook::Function(ImageBase + 0x26F10E0, UWorldExecHook, &UWorldExecOriginal); // UWorld::Exec

    auto Engine = UEngine::GetEngine();
    auto GameViewport = Engine->GameViewport;
    GameViewport->ViewportConsole = (UConsole*)UGameplayStatics::SpawnObject(UConsole::StaticClass(), GameViewport);

    *(bool*)(ImageBase + 0xE981F30) = true; // UseIrisReplication

    while (!(GetAsyncKeyState(VK_F5) & 0x8000)) Sleep(100);

    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"open 127.0.0.1", nullptr);

    // UKismetSystemLibrary::ExecuteConsoleCommand(L"log LogFortInventory VeryVerbose");
    // UKismetSystemLibrary::ExecuteConsoleCommand(L"open 127.0.0.1");

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
