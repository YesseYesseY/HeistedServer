#include "Patches.hpp"

namespace Net
{
    bool (*InitHost)(UObject*);
    void (*PauseBeaconRequests)(UObject*, bool);
    bool (*InitListen)(UNetDriver*, UObject*, void*, bool, FString&);
    void (*SetWorld)(UNetDriver*, UWorld*);

    int64 GetNetMode(int64 a1)
    {
        return 1;
    }

    enum class EReplicationSystemSendPass : unsigned
    {
        Invalid,
        PostTickDispatch,
        TickFlush = 2
    };

    struct yesuwu
    {
        EReplicationSystemSendPass a1;
        float a2;
    };

    void SendClientMoveAdjustments(UNetDriver* NetDriver)
    {
        for (UNetConnection* Connection : NetDriver->ClientConnections)
        {
            if (Connection == nullptr || Connection->ViewTarget == nullptr)
            {
                continue;
            }

            static void (*SendClientAdjustment)(APlayerController*) = decltype(SendClientAdjustment)(InSDKUtils::GetImageBase() + 0x5EA9F18);

            if (APlayerController* PC = Connection->PlayerController)
            {
                SendClientAdjustment(PC);
            }

            for (UNetConnection* ChildConnection : Connection->Children)
            {
                if (ChildConnection == nullptr)
                {
                    continue;
                }

                if (APlayerController* PC = ChildConnection->PlayerController)
                {
                    SendClientAdjustment(PC);
                }
            }
        }
    }

    void (*TickFlushOriginal)(UNetDriver* NetDriver, float DeltaSeconds);
    void TickFlushHook(UNetDriver* NetDriver, float DeltaSeconds)
    {
        auto ReplicationSystem = *(UReplicationSystem**)(int64(NetDriver) + 0x738);

        if (NetDriver->ClientConnections.Num() > 0 && ReplicationSystem)
        {
            static void (*UpdateReplicationViews)(UNetDriver*) = decltype(UpdateReplicationViews)(InSDKUtils::GetImageBase() + 0x5DD1C2C);
            static void (*PreSendUpdate)(UReplicationSystem*, yesuwu&) = decltype(PreSendUpdate)(InSDKUtils::GetImageBase() + 0x3E9BF70);

            UpdateReplicationViews(NetDriver);
            SendClientMoveAdjustments(NetDriver);
            yesuwu thing;
            thing.a1 = EReplicationSystemSendPass::TickFlush;
            thing.a2 = DeltaSeconds;
            PreSendUpdate(ReplicationSystem, thing);
        }

        TickFlushOriginal(NetDriver, DeltaSeconds);
    }

    void Listen()
    {
        auto Beacon = Utils::SpawnActor<AFortOnlineBeaconHost>();
        Beacon->ListenPort = 7777;

        if (!InitHost(Beacon))
        {
            MsgBox("InitHost Failed");
            return;
        }

        PauseBeaconRequests(Beacon, false);

        auto World = UWorld::GetWorld();
        auto NetDriver = Beacon->NetDriver;

        *(bool*)(int64(NetDriver) + 0x0730 + 8 + 8 + 1) = true; // bIsUsingIris

        NetDriver->World = World;
        World->NetDriver = NetDriver;
        NetDriver->NetDriverName = UKismetStringLibrary::Conv_StringToName(L"GameNetDriver");

        FURL Url = {};
        Url.Port = 7776;
        FString Error;
        if (!InitListen(NetDriver, World, &Url, false, Error))
        {
            MsgBox("InitListen Failed");
            return;
        }

        SetWorld(NetDriver, World);
        World->LevelCollections[0].NetDriver = NetDriver;
        World->LevelCollections[1].NetDriver = NetDriver;
    }

    void Init()
    {
        auto ImageBase = InSDKUtils::GetImageBase();
        InitHost = decltype(InitHost)(ImageBase + 0x60b9cd8);
        PauseBeaconRequests = decltype(PauseBeaconRequests)(ImageBase + 0x85d8a60);
        SetWorld = decltype(SetWorld)(ImageBase + 0x1d802ac);
        InitListen = decltype(InitListen)(ImageBase + 0x60ba70c);

        Hook::Function(ImageBase + 0x1D7D9E8, GetNetMode); // GetNetMode??????
        Hook::Function(ImageBase + 0x3819AB4, Hook::ReturnFalseHook); // ULevelStreaming::IsConcernedByNetVisibilityTransactionAck
        Hook::Function(ImageBase + 0x33B7968, Hook::ReturnHook); // GameSession Crash
        Hook::Function(ImageBase + 0x5C1EFEC, Hook::ReturnHook); // KickPlayer
        Hook::Function(ImageBase + 0x381B060, TickFlushHook, &TickFlushOriginal);

        *(bool*)(ImageBase + 0xE981F30) = true; // UseIrisReplication
        *(bool*)(ImageBase + 0xE81611D) = true; // GIsServer
        *(bool*)(ImageBase + 0xE83E1B2) = false; // GIsClient
        UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);

        // NOTE: How this works is it scans for all AttemptDeriveFromURL calls
        //       When it finds a call it searches for the first part of UWorld::GetNetMode, "if (NetDriver != nullptr)"
        //       If NetDriver is not null it returns NM_Client, technically you could try to find all NM_Client in assembly and change it to 1
        //       But that is way too complicated when you concider for example "mov ebx, 1;mov eax, ebx"
        //       Instead of doing that it's simpler to replace the jz to jnz in the assembly, this basically inverts the if statement and goes to the next one
        //       The next one is "if (DemoNetDriver && ...)" which will always be false, and because of that it goes to the last step
        //       Calling AttemptDeriveFromURL and returns that, so that function is all that need to be hooked instead of going around all the assembly scanning for 3s.
        //       This works for 99% of the GetNetMode calls, the rest of them i just did manually
        constexpr bool ScanForPatches = false;
        
        if constexpr (!ScanForPatches)
        {
            ApplyPatches();
        }
        else
        {
            const auto sizeOfImage = Memcury::PE::GetNTHeaders()->OptionalHeader.SizeOfImage;
            const auto scanBytes = reinterpret_cast<std::uint8_t*>(Memcury::PE::GetModuleBase());

            auto ADFU = (uintptr_t)(ImageBase + 0x1D7D9E8);

            std::ofstream patch74("patch74.txt");
            std::ofstream patch75("patch75.txt");
            std::ofstream patch84("patch84.txt");
            std::ofstream patch85("patch85.txt");
            for (auto i = 0ul; i < sizeOfImage - 1; ++i)
            {
                if (scanBytes[i] == 0xE8)
                {
                    uintptr_t currAddr = reinterpret_cast<uintptr_t>(&scanBytes[i]);
                    uintptr_t relPtr = currAddr + 1 + 4 + *reinterpret_cast<int32_t*>(currAddr + 1);
                    if (relPtr != ADFU)
                        continue;

                    if (currAddr == 0x141DCD67A)
                        continue;

                    auto Idx = -1;
                    auto Scanner = Memcury::Scanner(currAddr).ScanForEitherPattern({ "39 ? 38", "83 ? 38 00" }, false, 0, &Idx);

                    if (Idx == 1)
                        Scanner.AbsoluteOffset(1);

                    if (Scanner.Get() != currAddr)
                    {
                        Scanner.AbsoluteOffset(3);
meow:
                        uintptr_t PatchAddr = Scanner.Get(); // 0x74/0x75
                        if (*(uint8*)(PatchAddr) == 0x0F)
                        {
                            if (*(uint8*)(PatchAddr + 1) == 0x85)
                            {
                                DWORD yes;
                                VirtualProtect((LPVOID)(PatchAddr + 1), 1, PAGE_EXECUTE_READWRITE, &yes);
                                *(uint8*)(PatchAddr + 1) = 0x84;
                                VirtualProtect((LPVOID)(PatchAddr + 1), 1, yes, &yes);
                                patch84 << std::format("0x{:X}, ", PatchAddr - ImageBase + 1);
                            }
                            else if (*(uint8*)(PatchAddr + 1) == 0x84)
                            {
                                DWORD yes;
                                VirtualProtect((LPVOID)(PatchAddr + 1), 1, PAGE_EXECUTE_READWRITE, &yes);
                                *(uint8*)(PatchAddr + 1) = 0x85;
                                VirtualProtect((LPVOID)(PatchAddr + 1), 1, yes, &yes);
                                patch85 << std::format("0x{:X}, ", PatchAddr - ImageBase + 1);
                            }
                        }
                        else if (*(uint8*)(PatchAddr) == 0x74)
                        {
                            DWORD yes;
                            VirtualProtect((LPVOID)PatchAddr, 1, PAGE_EXECUTE_READWRITE, &yes);
                            *(uint8*)(PatchAddr) = 0x75;
                            VirtualProtect((LPVOID)PatchAddr, 1, yes, &yes);
                            patch75 << std::format("0x{:X}, ", PatchAddr - ImageBase);
                        }
                        else if (*(uint8*)(PatchAddr) == 0x75)
                        {
                            if (PatchAddr - InSDKUtils::GetImageBase() != 0x3818215 /*0x143818215*/)
                            {
                                DWORD yes;
                                VirtualProtect((LPVOID)PatchAddr, 1, PAGE_EXECUTE_READWRITE, &yes);
                                *(uint8*)(PatchAddr) = 0x74;
                                VirtualProtect((LPVOID)PatchAddr, 1, yes, &yes);
                                patch74 << std::format("0x{:X}, ", PatchAddr - ImageBase);
                            }
                        }
                    }
                }
            }
            patch74.close();
            patch75.close();
            patch84.close();
            patch85.close();
        }
    }
}
