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

    void ServerBeginEditingBuildingActor(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToEdit)
    {
        BuildingActorToEdit->EditingPlayer = (AFortPlayerStateAthena*)PlayerController->PlayerState;
        BuildingActorToEdit->OnRep_EditingPlayer();

        static auto EditToolItemDef = Utils::FindObjectFast<UFortEditToolItemDefinition>("EditTool");
        if (auto ItemEntry = Inventory::FindItemEntry(PlayerController, EditToolItemDef))
        {
            BuildingActorToEdit->EditingPlayer = (AFortPlayerStateAthena*)PlayerController->PlayerState;
            BuildingActorToEdit->OnRep_EditingPlayer();

            Inventory::EquipItemEntry(PlayerController, ItemEntry);
        }
        else
        {
            PlayerController->ClientFailedToBeginEditingBuildingActor(BuildingActorToEdit);
        }
    }

    void ServerEndEditingBuildingActor(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToStopEditing)
    {
        BuildingActorToStopEditing->EditingPlayer = nullptr;
        BuildingActorToStopEditing->OnRep_EditingPlayer();
    }

    void ServerEditBuildingActor(AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToEdit, UClass* NewBuildingClass, uint8 RotationIterations, bool bMirrored)
    {
        static ABuildingSMActor* (*RBA)(ABuildingSMActor*, uint8, UClass*, int, int, bool, AFortPlayerController*) = decltype(RBA)(InSDKUtils::GetImageBase() + 0x7CAF9AC);
        RBA(BuildingActorToEdit, 1, NewBuildingClass, 0, RotationIterations, bMirrored, PlayerController);
    }

    void Init()
    {
        Hook::VTable<AFortPlayerControllerAthena>(4712 / 8, ServerCreateBuildingActor);
        Hook::VTable<AFortPlayerControllerAthena>(4728 / 8, ServerEditBuildingActor);
        Hook::VTable<AFortPlayerControllerAthena>(4752 / 8, ServerEndEditingBuildingActor);
        Hook::VTable<AFortPlayerControllerAthena>(4768 / 8, ServerBeginEditingBuildingActor);

        // "FortEditToolItemDefinition EditTool.EditTool"
    }
}
