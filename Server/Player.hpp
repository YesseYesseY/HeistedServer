namespace Player
{
    void ServerAcknowledgePossession(AFortPlayerControllerAthena* PlayerController, APawn* Pawn)
    {
        PlayerController->AcknowledgedPawn = Pawn;
    }

    void ServerCheat(AFortPlayerControllerAthena* Controller, FString& FMsg)
    {
        auto Msg = FMsg.ToWString();

        if (Msg.starts_with(L"server "))
        {
            Utils::ExecuteConsoleCommand(Msg.substr(7).c_str());
        }
        else if (Msg.starts_with(L"client "))
        {
            if (!Controller->CheatManager)
                Controller->CheatManager = Utils::SpawnObject<UCheatManager>(Controller);
            Utils::ExecuteConsoleCommand(Msg.substr(7).c_str(), Controller);
        }
        else if (Msg == L"test")
        {
            MsgBox("{} {}", UKismetSystemLibrary::IsServer(UWorld::GetWorld()), UKismetSystemLibrary::IsDedicatedServer(UWorld::GetWorld()));
            // MsgBox("{}", UFortCurieBlueprintFunctionLibrary::IsCurieEnabled());
        }
        else if (Msg == L"dumpobjects")
        {
            Utils::DumpObjects();
        }
        else if (Msg == L"startaircraft")
        {
            // Nope :(
        }
        else if (Msg == L"spawnrockbr")
        {
            static auto VID = Utils::FindObjectFast<UFortVehicleItemDefinition>("VID_Rock_Vehicle_BR");
            static auto VehicleClass = Utils::GetSoftPtr(VID->VehicleActorClass);
            auto Pos = Controller->Pawn->K2_GetActorLocation();
            Pos.Z += 500.0f;
            Utils::SpawnActor(VehicleClass, Pos);
        }
        else if (Msg == L"spawnrock")
        {
            static auto VID = Utils::FindObjectFast<UFortVehicleItemDefinition>("VID_Rock_Vehicle");
            static auto VehicleClass = Utils::GetSoftPtr(VID->VehicleActorClass);
            auto Pos = Controller->Pawn->K2_GetActorLocation();
            Pos.Z += 500.0f;
            Utils::SpawnActor(VehicleClass, Pos);
        }
        else if (Msg == L"dl")
        {
            DataLayers::Dump();
        }
        else if (Msg == L"og")
        {
            // Mega City Signs
            DataLayers::Activate(Utils::FindObjectFast<UDataLayerAsset>("Asteria_DL_RTSign_Active"));
            DataLayers::Deactivate(Utils::FindObjectFast<UDataLayerAsset>("Asteria_DL_RTSign_DeActivated"));

            // Remove the time machine from kado thornes basement
            DataLayers::Deactivate(Utils::FindObjectFast<UDataLayerAsset>("Asteria_DL_TM_01"));
            // Put the time machine in frenzy field with chapter 1 props
            DataLayers::Activate(Utils::FindObjectFast<UDataLayerAsset>("Asteria_DL_TM_02"));
            // There is Asteria_DL_TM_03 but that's just Asteria_DL_TM_02 but without the props and time machine
        }
    }

    void Init()
    {
        Hook::VTable<AFortPlayerControllerAthena>(2440 / 8, ServerAcknowledgePossession);
        Hook::VTable<AFortPlayerControllerAthena>(4024 / 8, ServerCheat);
    }
}
