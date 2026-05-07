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
            GameState->OnRep_CurrentPlaylistInfo();

            auto Logic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());
            Logic->GamePhase = EAthenaGamePhase::None;
            Logic->OnRep_GamePhase(EAthenaGamePhase::Setup);

            Net::Listen();

            GameMode->WarmupRequiredPlayerCount = 1;

            UWorld::GetWorld()->ServerStreamingLevelsVisibility = Utils::SpawnActor<AServerStreamingLevelsVisibility>();
        }

        if (GameMode->NumPlayers > 0)
        {
            GameMode->bWorldIsReady = true;

            return true;
        }

        return false;
    }

    APawn* SpawnDefaultPawnForHook(AFortGameModeBR* GameMode, AController* NewPlayer, AActor* StartSpot)
    {
        FTransform translivesmatter; // = StartSpot->GetTransform();
        translivesmatter.Translation = { 0, 0, 10000 };
        translivesmatter.Scale3D = { 1, 1, 1 };
        auto Pawn = (AFortPlayerPawn*)GameMode->SpawnDefaultPawnAtTransform(NewPlayer, translivesmatter);
        Pawn->bCanBeDamaged = false;
        static void (*ApplyCharacterCustomization)(AFortPlayerState*, AFortPlayerPawn*) = decltype(ApplyCharacterCustomization)(InSDKUtils::GetImageBase() + 0x890425C);
        ApplyCharacterCustomization((AFortPlayerState*)NewPlayer->PlayerState, Pawn);
        return Pawn;
    }

    void Init()
    {
        Hook::VTable<AFortGameModeBR>(2328 / 8, ReadyToStartMatchHook);
        Hook::VTable<AFortGameModeBR>(1832 / 8, SpawnDefaultPawnForHook);
    }
}
