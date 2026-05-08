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

#define PATCH_BYTE(Offset, Val) \
        { \
        auto Addr = (LPVOID)(ImageBase + Offset); \
        DWORD yes; \
        VirtualProtect(Addr, 1, PAGE_EXECUTE_READWRITE, &yes); \
        *(uint8*)(Addr) = Val; \
        VirtualProtect(Addr, 1, yes, &yes); \
        }

        // Context: On this build alot of functions that call GetNetMode get messed up and decide to hardcode NetMode to be 3
        //          There are only 2 builds i've seen this on, 24.40 and 26.30. Even 28.30 which is after those 2 doesn't have this.
        //          I noticed it on UKismetSystemLibrary::IsServer/IsDedicatedServer but ALOT of funcs have it ...
        //          I suspect one of these messed up functions is why the pickaxe doesn't work but it could also just be because im bad at making a server
        // PATCH_BYTE(0x15576D3, 1); // UKismetSystemLibrary::IsServer
        // PATCH_BYTE(0x178302A, 1); // UKismetSystemLibrary::IsDedicatedServer
        // PATCH_BYTE(0xECE82D, 1);

        std::vector<uintptr_t> Patches = {
            // "B8 03 00 00 00 83 F8 01"
            0x140ECE82C, 0x140ED5F58, 0x140ED66CE, 0x140EDB909, 0x1410773D5, 0x1410B867F, 0x1410E05EC, 0x1411021D7, 0x1411C4127, 0x1411C44C3,
            0x14129B997, 0x14129D6A8, 0x14129DB97, 0x14129DE3D, 0x1412A6B21, 0x14137A9FB, 0x1413941DF, 0x1414B65F8, 0x1414C2F15, 0x1414EE0BD,
            0x1414FCB58, 0x1415017B5, 0x141555B9B, 0x14155D69A, 0x1415BF00D, 0x1415BF0C3, 0x1415F1575, 0x141617247, 0x141714E2E, 0x141783029,
            0x1417FEA14, 0x1418F493E, 0x1418FB810, 0x141916D23, 0x1419FE7E4, 0x141A35CEE, 0x141A36516, 0x141AB56F5, 0x141AC9809, 0x141ADCD06,
            0x141AE1639, 0x141C14C21, 0x141C15C3C, 0x141C165A0, 0x141C8E25D, 0x141C910F2, 0x141D029D6, 0x141D31B82, 0x141EAC348, 0x141EFA6D7,
            0x141F61FBF, 0x14208DADE, 0x1420CE34E, 0x1420D11C4, 0x1420D1344, 0x1420D1A56, 0x142200250, 0x142229331, 0x14227D9B8, 0x14229C5EA,
            0x14230BD24, 0x142361BA4, 0x142417764, 0x14241AE48, 0x14241BD49, 0x1424C7287, 0x142527028, 0x1425271D8, 0x1425DC15A, 0x14268F284,
            0x1426E8003, 0x1427BDDA7, 0x14285F20B, 0x142A5EACC, 0x142B5F6EE, 0x142E44B94, 0x142E47FE2, 0x142EC6010, 0x142FABE30, 0x143037F8A,
            0x143121540, 0x1432FF8E1, 0x14339B9A1, 0x1433AEE03, 0x14341CF2F, 0x14382A9A3, 0x14386869B, 0x143869B30,

            // "B8 03 00 00 00 83 F8 03"
            0x140ECF9BE, 0x14132AB7F, 0x1414BCFFE, 0x1415576D2, 0x141835463, 0x14183B663, 0x141D7EB5B, 0x1420CA77D, 0x14229C1FE, 0x14229C318, 
            0x1422CD717, 0x14241BEF1, 0x142810B6A, 0x14296C0E1, 0x1431BE389, 0x1432C29E2, 
        };
        for (auto Patch : Patches)
        {
            PATCH_BYTE(Patch - 0x140000000 + 1, 1);
        }
    }
}
