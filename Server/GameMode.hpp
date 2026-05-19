#define SkipAircraft 1

namespace GameMode
{
    bool ReadyToStartMatchHook(AFortGameModeBR* GameMode)
    {
        // if (UFortKismetLibrary::GetNumActorsOfClass(UWorld::GetWorld(), AFortPlayerStartWarmup::StaticClass()))
        //     return false;

        static bool Started = false;
        if (!Started)
        {
            Started = true;
            auto Playlist = UObject::FindObject<UFortPlaylistAthena>("FortPlaylistAthena Playlist_DefaultSolo.Playlist_DefaultSolo");

            auto GameState = (AFortGameStateBR*)GameMode->GameState;
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
            Vehicles::Init();
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

        static auto PlayerAbilitySet = UObject::FindObject<UFortAbilitySet>("FortAbilitySet GAS_AthenaPlayer.GAS_AthenaPlayer");
        static auto TacSprintAbilitySet = UObject::FindObject<UFortAbilitySet>("FortAbilitySet AS_TacticalSprint.AS_TacticalSprint");
        Abilities::GiveAbilitySet(PlayerState->AbilitySystemComponent, PlayerAbilitySet);
        Abilities::GiveAbilitySet(PlayerState->AbilitySystemComponent, TacSprintAbilitySet);

        // TODO Look into this not working
        // ASC->K2_GiveAbility(UObject::FindClassFast("GA_Athena_Player_DoorBash_C"), 1, 1);

        for (int i = 0; i < 5; i++) // 5 = The 4 main builds + EditTool, after 5 there is just smartbuilds
            Inventory::GiveItem(PlayerController, GameMode->StartingItems[i].Item, GameMode->StartingItems[i].Count);

        Inventory::GiveItem(PlayerController, UObject::FindObject<UFortItemDefinition>("FortWeaponMeleeItemDefinition WID_Harvest_Pickaxe_Athena_C_T01.WID_Harvest_Pickaxe_Athena_C_T01"));
        Inventory::GiveItem(PlayerController, UObject::FindObject<UFortItemDefinition>("FortResourceItemDefinition WoodItemData.WoodItemData"));
        Inventory::GiveItem(PlayerController, UObject::FindObject<UFortItemDefinition>("FortResourceItemDefinition StoneItemData.StoneItemData"));
        Inventory::GiveItem(PlayerController, UObject::FindObject<UFortItemDefinition>("FortResourceItemDefinition MetalItemData.MetalItemData"));
        Inventory::GiveItem(PlayerController, UObject::FindObject<UFortItemDefinition>("FortWeaponMeleeItemDefinition WID_Melee_Katana_R.WID_Melee_Katana_R"));
    }

    void Init()
    {
        Hook::VTable<AFortGameModeBR>(2328 / 8, ReadyToStartMatchHook);
        Hook::VTable<AFortGameModeBR>(1832 / 8, SpawnDefaultPawnForHook);
        Hook::VTable<AFortGameModeBR>(1880 / 8, HandleStartingNewPlayer, &HandleStartingNewPlayerOriginal);
    }
}
