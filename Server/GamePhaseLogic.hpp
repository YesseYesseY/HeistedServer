namespace GamePhaseLogic
{
    void SetGamePhase(EAthenaGamePhase NewPhase)
    {
        static void (*nfunc)(UFortGameStateComponent_BattleRoyaleGamePhaseLogic*, EAthenaGamePhase) = decltype(nfunc)(InSDKUtils::GetImageBase() + 0x8D15B1C);
        static auto Logic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());
        nfunc(Logic, NewPhase);
    }

    // NOTE: The camera bug on the battle bus is not my fault, it was actually happning during that time
    //       https://youtu.be/FJEvSW1Wxpw?t=17
    void StartAircraftPhase(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* Logic, bool AircraftAlreadySpawned = false)
    {
        auto GameState = (AFortGameStateAthena*)Logic->GetOwner();
        auto Playlist = GameState->CurrentPlaylistInfo.BasePlaylist;

        if (!GameState || Logic->bGameModeWillSkipAircraft)
        {
            std::println("Skipping aircraft phase: {}", (GameState ? "GameState == nullptr" : "bGameModeWillSkipAircraft"));
            return;
        }

        if (Logic->GamePhase >= EAthenaGamePhase::Aircraft)
        {
            std::println("Already past aircraft phase");
            return;
        }

        std::println("Initiating aircraft phase");

        // It checks for this but is it really needed, it's already checked on the if statement before this
        // if (Logic->GamePhase != EAthenaGamePhase::Aircraft)
        //     return;

        if (!Playlist)
        {
            std::println("Playlist is Null");
            return;
        }

        auto World = UWorld::GetWorld();
        if (!AircraftAlreadySpawned && Logic->AirCraftBehavior != EAirCraftBehavior::NoAircraft)
        {
            int TeamCount = 1;
            if (Logic->AirCraftBehavior != EAirCraftBehavior::OpposingAirCraftForEachTeam)
                TeamCount = GameState->TeamCount;

            // TODO
        }

        if (Playlist->MaxTeamSize > 1)
        {
            // TODO
        }

        if (World)
        {
            for (auto& WeakPtr : Utils::GetPlayerControllerList(World))
            {
                auto Controller = WeakPtr.Get(); // Utils::GetWeakPtr(WeakPtr);
                if (!Controller || !Controller->IsA(AFortPlayerControllerAthena::StaticClass()))
                    continue;

                auto PlayerController = (AFortPlayerControllerAthena*)Controller;
                PlayerController->LastDamager = nullptr;
                PlayerController->LastFallInstigator = nullptr;
                // TODO Does something with EmoteUsageCounts, probably clears it

                if (Controller->Pawn && Controller->Pawn->IsA(AFortPlayerPawnAthena::StaticClass()))
                {
                    auto Pawn = (AFortPlayerPawnAthena*)Controller->Pawn;

                    static void (*EW)(AFortPlayerPawn*, AFortWeapon*) = decltype(EW)(InSDKUtils::GetImageBase() + 0x87951AC);
                    static void (*SIIAS)(AFortPlayerPawn*, bool) = decltype(SIIAS)(InSDKUtils::GetImageBase() + 0x87E50BC);
                    static void (*PMD)(void*, void*) = decltype(PMD)(InSDKUtils::GetImageBase() + 0x10957B0);

                    // AFortPlayerPawn::EquipWeapon(nullptr);
                    EW(Pawn, nullptr);
                    // AFortPlayerPawn::SetIsInAnyStorm(false);
                    SIIAS(Pawn, false);

                    // It calls a null func here

                    // ProcessMulticastDelegate(OnEnteredAircraft);
                    PMD(&Pawn->OnEnteredAircraft, 0);
                }

                PlayerController->ClientActivateSlot(EFortQuickBars::Primary, 0, 0.0f, true, false);

                if (Controller->Pawn)
                    Controller->Pawn->K2_DestroyActor();

                bool DumpInventoryWhenStartingAircraft = true;
                if (DumpInventoryWhenStartingAircraft) // Athena.DumpInventoryWhenStartingAircraft
                {
                    // TODO
                }

                // Controller->Reset();
                void (*reset)(AActor*) = decltype(reset)(((void**)Controller->VTable)[1704 / 8]);
                reset(Controller);

                static auto SpectatingState = UKismetStringLibrary::Conv_StringToName(L"Spectating");
                Controller->ClientGotoState(SpectatingState);
            }
        }

        // TODO

        SetGamePhase(EAthenaGamePhase::Aircraft);

        auto TimeSeconds = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
        for (auto Aircraft : Logic->Aircrafts_GameState)
            Aircraft.Get()->StartFlightPath(TimeSeconds);

        // TODO
    }

    void Tick()
    {
        static auto Logic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());

        if (!Logic)
            Logic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());

        if (!Logic)
            return;

        static auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

        if (!GameMode->bWorldIsReady)
            return;

        auto TimeSeconds = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

        if (Logic->GamePhase == EAthenaGamePhase::Warmup)
        {
            if (TimeSeconds >= Logic->AircraftRealStartTime)
            {
                StartAircraftPhase(Logic);

                // TODO KeysAndLock_DisplayCase calls PickLootDrops somehow, idk how because FModels decompiler isn't good enough so i gotta port my decompiler to 26.30 to check :/
                auto KeysAndLocks = Utils::GetAllActorsOfClass<ABGA_KeysAndLocks_DisplayCase_C>();
                for (auto Case : KeysAndLocks)
                {
                    auto GenLoot = Loot::Get(Case->Tier_Group_Name);
                    for (auto thing : GenLoot)
                        Case->Loot.Add(UFortKismetLibrary::CreateItemEntry(thing.first, thing.second, 0));

                    Case->ItemDefinition = (UFortWorldItemDefinition*)Case->Loot[0].ItemDefinition;
                    Case->SpawnCount = Case->Loot[0].Count;
                    Case->OnRep_ItemDefinition();
                    Case->ForceNetUpdate();
                }
            }
        }
    }
}
