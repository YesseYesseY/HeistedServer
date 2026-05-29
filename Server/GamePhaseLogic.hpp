namespace GamePhaseLogic
{
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
                void (*EnterAircraft)(void*, void*) = decltype(EnterAircraft)(InSDKUtils::GetImageBase() + 0x7DCEC1C);

                Logic->GamePhase = EAthenaGamePhase::Aircraft;
                Logic->GamePhaseStep = EAthenaGamePhaseStep::BusLocked;
                Logic->OnRep_GamePhase(EAthenaGamePhase::Warmup);

                auto Aircraft = Logic->Aircrafts_GameState[0].Get();
                Aircraft->StartFlightPath(TimeSeconds);

                for (auto Player : UWorld::GetWorld()->NetDriver->ClientConnections)
                {
                    auto Controller = (AFortPlayerControllerAthena*)Player->PlayerController;
                    auto Component = (UFortControllerComponent_Aircraft*)Controller->GetComponentByClass(UFortControllerComponent_Aircraft::StaticClass());
                    EnterAircraft(Component, Aircraft);
                    Controller->Pawn->K2_DestroyActor();
                }
            }
        }
    }
}
