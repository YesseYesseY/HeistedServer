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
            void (*SetCurrentPlaylistName)(AFortGameMode*, FName) = decltype(SetCurrentPlaylistName)(InSDKUtils::GetImageBase() + 0x814654C);
            SetCurrentPlaylistName(GameMode, Playlist->PlaylistName);

            auto Logic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());
#if 0
#if SkipAircraft
            Logic->GamePhase = EAthenaGamePhase::None;
#else
            Logic->GamePhase = EAthenaGamePhase::Warmup;
            Logic->GamePhaseStep = EAthenaGamePhaseStep::Warmup;
#endif
            Logic->OnRep_GamePhase(EAthenaGamePhase::Setup);
#endif

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

            auto Logic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());
            auto TimeSeconds = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
            Logic->WarmupCountdownStartTime = TimeSeconds;
            Logic->WarmupCountdownEndTime = TimeSeconds + 60.0f;
            Logic->AircraftStartTime = Logic->WarmupCountdownEndTime;
            Logic->AircraftRealStartTime = Logic->WarmupCountdownEndTime;
            Logic->bAircraftIsLocked = true;

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

    }

    // Gets called before StartWarmupPhase gets called in HandleMatchHasStarted
    bool (*sub_1479229DCOriginal)(AFortGameModeAthena* GameMode, int64 a2);
    bool sub_1479229DC(AFortGameModeAthena* GameMode, int64 a2)
    {
        if (sub_1479229DCOriginal(GameMode, a2))
        {
            int64 (*sub_140FAEAF4)(void*, char) = decltype(sub_140FAEAF4)(InSDKUtils::GetImageBase() + 0xFAEAF4);
            int64 v34 = sub_140FAEAF4(UWorld::GetWorld(), 0);
            if (v34)
                (*(void (__fastcall **)(int64))(*(uint64*)v34 + 1872))(v34);
        }

        auto Logic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());
        GamePhaseLogic::StartWarmupPhase(Logic);

        return false; // Returning true or sub_1479229DCOriginal will just do the same thing a second time
    }

    void OnPlaylistDataLoadedPatch()
    {
        int64 (*IFP)(void* MapInfo, void* GameState, void* Logic, char, int, int, float) = decltype(IFP)(InSDKUtils::GetImageBase() + 0x787FF4C);

        auto Logic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());
        auto GameState = (AFortGameStateAthena*)Logic->GetOwner();
        auto GameMode = (AFortGameModeAthena*)GameState->AuthorityGameMode;

        AFortAthenaMapInfo* (*GetMapInfo)(void* GameMode, void* GameState) = decltype(GetMapInfo)(InSDKUtils::GetImageBase() + 0x792289C);
        auto MapInfo = GetMapInfo(GameMode, GameState);

        if (MapInfo)
        {
            // TODO Stuff
            IFP(MapInfo, GameState, Logic, 0, 0, 0, 360.0f);
            // TODO InitializeSafeZoneLocations
        }
    }

    void Init()
    {
        Hook::VTable<AFortGameModeBR>(2328 / 8, ReadyToStartMatchHook);
        Hook::VTable<AFortGameModeBR>(1832 / 8, SpawnDefaultPawnForHook);
        Hook::VTable<AFortGameModeBR>(1880 / 8, HandleStartingNewPlayer, &HandleStartingNewPlayerOriginal);

        Hook::Function(InSDKUtils::GetImageBase() + 0x79229DC, sub_1479229DC, &sub_1479229DCOriginal);

        // OnPlaylistDataLoaded patch
        {
            auto PatchAddr = InSDKUtils::GetImageBase() + 0x791C907;

            DWORD yes;
            VirtualProtect((LPVOID)PatchAddr, 26, PAGE_EXECUTE_READWRITE, &yes);
            *(uint8*)(PatchAddr) = 0x00;

            *(uint8*)(PatchAddr) = 0x48;
            *(uint8*)(PatchAddr + 1) = 0xB8;
            *(uintptr_t*)(PatchAddr + 2) = (uintptr_t)OnPlaylistDataLoadedPatch;
            *(uint8*)(PatchAddr + 10) = 0xFF;
            *(uint8*)(PatchAddr + 11) = 0xD0;
            for (int i = 0; i < 14; i++)
            {
                *(uint8*)(PatchAddr + 12 + i) = 0x90;
            }

            VirtualProtect((LPVOID)PatchAddr, 26, yes, &yes);
        }
    }
}
