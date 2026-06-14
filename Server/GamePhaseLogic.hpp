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

                    // AFortPlayerPawn::EquipWeapon(nullptr);
                    EW(Pawn, nullptr);
                    // AFortPlayerPawn::SetIsInAnyStorm(false);
                    SIIAS(Pawn, false);

                    // It calls a null func here

                    // ProcessMulticastDelegate(OnEnteredAircraft);
                    Utils::ProcessMulticastDelegate(&Pawn->OnEnteredAircraft, 0);
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

    void StartSafeZonesPhase()
    {
        SetGamePhase(EAthenaGamePhase::SafeZones);
    }

    void SpawnInitialSafeZone(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* Logic)
    {
        // I've tried to literally copy paste the code from the "storm" command in Player.hpp. Doesn't work. But the command works... :)
    }

    void UpdateSafeZonesPhase(UFortGameStateComponent_BattleRoyaleGamePhaseLogic* Logic)
    {
        auto TimeSeconds = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

        if (!Logic->SafeZoneIndicator && TimeSeconds >= Logic->SafeZonesStartTime)
            SpawnInitialSafeZone(Logic);
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
            }
        }
        else if (Logic->GamePhase == EAthenaGamePhase::Aircraft)
        {
            auto Aircraft = Logic->Aircrafts_GameState[0].Get();
            if (Logic->bAircraftIsLocked && TimeSeconds >= Aircraft->DropStartTime)
            {
                Logic->bAircraftIsLocked = false;
            }
            else if (TimeSeconds >= Aircraft->DropEndTime)
            {
                // Doesn't work.
                // for (auto& WeakPtr : Utils::GetPlayerControllerList())
                // {
                //     auto Controller = WeakPtr.Get();
                //     if (!Controller || !Controller->IsA(AFortPlayerControllerAthena::StaticClass()))
                //         continue;

                //     auto PlayerController = (AFortPlayerControllerAthena*)Controller;
                //     auto AircraftComponent = (UFortControllerComponent_Aircraft*)PlayerController->GetComponentByClass(UFortControllerComponent_Aircraft::StaticClass());
                //     if (!AircraftComponent || !AircraftComponent->CurrentAircraft)
                //         continue;

                //     *(bool*)(int64(AircraftComponent) + 0x13D) = true;
                //     GameMode->RestartPlayer(Controller);
                //     *(bool*)(int64(AircraftComponent) + 0x13D) = false;
                // }

                StartSafeZonesPhase();
                Logic->GamePhaseStep = EAthenaGamePhaseStep::StormForming;
                Logic->SafeZonesStartTime = TimeSeconds + 15.0f;
            }
        }
        else if (Logic->GamePhase == EAthenaGamePhase::SafeZones)
        {
            UpdateSafeZonesPhase(Logic);

            if (Logic->Aircrafts_GameState.Num() > 0)
            {
                for (int i = Logic->Aircrafts_GameState.Num() - 1; i >= 0; i--)
                {
                    auto Aircraft = Logic->Aircrafts_GameState[i].Get();
                    if (!Aircraft)
                    {
RemoveAircraft:
                        Logic->Aircrafts_GameState.Remove(i);
                        continue;
                    }

                    if (TimeSeconds >= Aircraft->FlightEndTime)
                    {
                        Aircraft->K2_DestroyActor();
                        goto RemoveAircraft;
                    }
                }
            }
        }
    }
}
