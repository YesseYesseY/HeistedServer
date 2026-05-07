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
        Hook::Function(ImageBase + 0x3819AB4, GetNetMode); // GetNetMode??????
        Hook::Function(ImageBase + 0x33B7968, Hook::ReturnHook); // GameSession Crash
        Hook::Function(ImageBase + 0x5C1EFEC, Hook::ReturnHook); // KickPlayer
        Hook::Function(ImageBase + 0x381B060, TickFlushHook, &TickFlushOriginal);

        *(bool*)(ImageBase + 0xE981F30) = true; // UseIrisReplication
        *(bool*)(ImageBase + 0xE81611D) = true; // GIsServer
        *(bool*)(ImageBase + 0xE83E1B2) = false; // GIsClient
        UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);
    }
}
