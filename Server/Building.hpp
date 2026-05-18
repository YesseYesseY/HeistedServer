namespace Building
{
    void ServerCreateBuildingActor(AFortPlayerController* PlayerController, FCreateBuildingActorData& CreateBuildingData)
    {
        UBuildingStructuralSupportSystem* BSSS = nullptr;
        if (!BSSS)
            UFortKismetLibrary::GetBuildingStructuralSupportSystem(PlayerController, &BSSS);

        static auto GameState = UFortKismetLibrary::GetGameStateAthena(UWorld::GetWorld());
        auto BuildingClass = GameState->AllPlayerBuildableClasses[CreateBuildingData.BuildingClassHandle];

        auto BuildLoc = CreateBuildingData.BuildLoc;
        auto BuildRot = CreateBuildingData.BuildRot;

        TArray<ABuildingActor*> Existing;
        EFortBuildPreviewMarkerOptionalAdjustment MOA;
        auto CanAdd = BSSS->CanAddBuildingActorClassToGrid(PlayerController, BuildingClass, BuildLoc, BuildRot, CreateBuildingData.bMirrored, &Existing, &MOA, false);
        if (CanAdd == EFortStructuralGridQueryResults::CanAdd)
        {
            auto Build = Utils::SpawnActor<ABuildingSMActor>(BuildingClass, CreateBuildingData.BuildLoc, CreateBuildingData.BuildRot);
            Build->InitializeKismetSpawnedBuildingActor(Build, PlayerController, true, nullptr);
        }

        for (auto Actor : Existing)
            Actor->K2_DestroyActor();

        Existing.Free();
    }

    void Init()
    {
        Hook::VTable<AFortPlayerControllerAthena>(4712 / 8, ServerCreateBuildingActor);
    }
}
