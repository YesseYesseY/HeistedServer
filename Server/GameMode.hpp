#define SkipAircraft 0

namespace GameMode
{
    bool ReadyToStartMatchHook(AFortGameModeBR* GameMode)
    {
        auto GameState = (AFortGameStateBR*)GameMode->GameState;

        static bool Started = false;
        if (!Started)
        {
            Started = true;
            auto Playlist = UObject::FindObject<UFortPlaylistAthena>("FortPlaylistAthena Playlist_DefaultSolo.Playlist_DefaultSolo");

            GameState->CurrentPlaylistInfo.BasePlaylist = Playlist;
            GameState->CurrentPlaylistInfo.PlaylistReplicationKey++;
            GameState->OnRep_CurrentPlaylistInfo();

            auto Logic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());
#if SkipAircraft
            Logic->GamePhase = EAthenaGamePhase::None;
#else
            Logic->GamePhase = EAthenaGamePhase::Warmup;
            Logic->GamePhaseStep = EAthenaGamePhaseStep::Warmup;
#endif
            Logic->OnRep_GamePhase(EAthenaGamePhase::Setup);

            Net::Listen();

            GameMode->WarmupRequiredPlayerCount = 1;

            UWorld::GetWorld()->ServerStreamingLevelsVisibility = Utils::SpawnActor<AServerStreamingLevelsVisibility>();
        }

        if (GameMode->NumPlayers > 0)
        {
            GameFeatures::Init();
            Vehicles::Init();
            Loot::Init();

            // Put time machine in kado thornes basement
            DataLayers::Activate(Utils::FindObjectFast<UDataLayerAsset>("Asteria_DL_TM_01"));

            int32 RTs[3] = { 1, 1, 1 };
            RTs[0] = UKismetMathLibrary::RandomIntegerInRange(1, 6);
            while (RTs[1] == RTs[0])
                RTs[1] = UKismetMathLibrary::RandomIntegerInRange(1, 6);
            while (RTs[2] == RTs[0] || RTs[2] == RTs[1])
                RTs[1] = UKismetMathLibrary::RandomIntegerInRange(1, 6);

            for (int i = 0; i < 3; i++)
                DataLayers::Activate(Utils::FindObjectFast<UDataLayerAsset>(std::format("Asteria_DL_RT_{}", RTs[i])));

            auto MapInfo = GameState->MapInfo; // Utils::FindObjectFast<AFortAthenaMapInfo>("DefaultMapInfo_UAID_E04F43E629FE0A2D01_1437486460");
            auto Logic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());

            FAircraftFlightConstructionInfo FCI = { EAirCraftBehavior::Default };
            FCI.AircraftCount = 1;
            FAircraftFlightInfo& (*IFP)(AFortAthenaMapInfo*, AFortGameState*, UFortGameStateComponent_BattleRoyaleGamePhaseLogic*, FAircraftFlightConstructionInfo&) = decltype(IFP)(InSDKUtils::GetImageBase() + 0x787F2F8);
            auto FI = IFP(MapInfo, GameState, Logic, FCI);

            TArray<AFortAthenaAircraft*> Aircrafts;
            Aircrafts.Add(AFortAthenaAircraft::SpawnAircraft(UWorld::GetWorld(), MapInfo->AircraftClass, FI));
            Logic->SetAircrafts(Aircrafts);

            auto TimeSeconds = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
            Logic->WarmupCountdownStartTime = TimeSeconds;
            Logic->WarmupCountdownEndTime = TimeSeconds + 60.0f;
            Logic->AircraftStartTime = Logic->WarmupCountdownEndTime;
            Logic->AircraftRealStartTime = Logic->WarmupCountdownEndTime;

            GameMode->bWorldIsReady = true;

            return true;
        }

        return false;
    }

    APawn* SpawnDefaultPawnForHook(AFortGameModeBR* GameMode, AController* NewPlayer, AActor* StartSpot)
    {
        FTransform translivesmatter = StartSpot->GetTransform();
#if SkipAircraft
        translivesmatter.Translation = { 0, 0, 10000 };
#endif
        auto Pawn = (AFortPlayerPawn*)GameMode->SpawnDefaultPawnAtTransform(NewPlayer, translivesmatter);
        Pawn->bCanBeDamaged = false;
        return Pawn;
    }

    void (*HandleStartingNewPlayerOriginal)(AFortGameModeBR* GameMode, APlayerController* PlayerController);
    void HandleStartingNewPlayer(AFortGameModeBR* GameMode, AFortPlayerControllerAthena* PlayerController)
    {
        HandleStartingNewPlayerOriginal(GameMode, PlayerController);

        auto Pawn = (AFortPlayerPawnAthena*)PlayerController->Pawn;

        static void (*ApplyCharacterCustomization)(AFortPlayerState*, AFortPlayerPawn*) = decltype(ApplyCharacterCustomization)(InSDKUtils::GetImageBase() + 0x890425C);
        ApplyCharacterCustomization((AFortPlayerState*)PlayerController->PlayerState, Pawn);

        auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
        auto ASC = PlayerState->AbilitySystemComponent;

        static auto PlayerAbilitySet = Utils::FindObjectFast<UFortAbilitySet>("GAS_AthenaPlayer");
        static auto TacSprintAbilitySet = Utils::FindObjectFast<UFortAbilitySet>("AS_TacticalSprint");
        // static auto AscenderAbilitySet = Utils::FindObjectFast<UFortAbilitySet>("AS_Ascender");

        Abilities::GiveAbilitySet(PlayerState->AbilitySystemComponent, PlayerAbilitySet);
        Abilities::GiveAbilitySet(PlayerState->AbilitySystemComponent, TacSprintAbilitySet);
        // Abilities::GiveAbilitySet(PlayerState->AbilitySystemComponent, AscenderAbilitySet);

        // TODO Look into this not working
        // ASC->K2_GiveAbility(UObject::FindClassFast("GA_Athena_Player_DoorBash_C"), 1, 1);

        Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortItemDefinition>("WID_Harvest_Pickaxe_Athena_C_T01"));

        for (int i = 0; i < 5; i++) // 5 = The 4 main builds + EditTool, after 5 there is just smartbuilds
            Inventory::GiveItem(PlayerController, GameMode->StartingItems[i].Item, GameMode->StartingItems[i].Count);

        Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortItemDefinition>("WoodItemData"));
        Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortItemDefinition>("StoneItemData"));
        Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortItemDefinition>("MetalItemData"));
        Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortItemDefinition>("WID_Melee_Katana_R"));
        Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortItemDefinition>("WID_GrenadeLauncher_Hopscotch_Athena_SR"));
        Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortItemDefinition>("AmmoDataRockets"));
        Inventory::GiveItem(PlayerController, Utils::FindObjectFast<UFortItemDefinition>("AGID_KeysAndLocks_Key"));
    }

    void Init()
    {
        Hook::VTable<AFortGameModeBR>(2328 / 8, ReadyToStartMatchHook);
        Hook::VTable<AFortGameModeBR>(1832 / 8, SpawnDefaultPawnForHook);
        Hook::VTable<AFortGameModeBR>(1880 / 8, HandleStartingNewPlayer, &HandleStartingNewPlayerOriginal);
    }
}
