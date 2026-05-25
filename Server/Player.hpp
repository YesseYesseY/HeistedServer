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
                Controller->CheatManager = Utils::SpawnObject<UFortCheatManager>(Controller);
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
        else if (Msg == L"dragon") // Doesn't work :(
        {
            static auto DragonClass = UObject::FindClassFast("BP_CyberDragon_A_C");
            static auto Dragon = Utils::FindFirstObjectOfClass<AActor>(DragonClass);
            static auto PlaySound = DragonClass->GetFunction("BP_CyberDragon_A_C", "PlaySound");
            static auto DragonBreath = (UNiagaraComponent*)Dragon->GetComponentByClass(UNiagaraComponent::StaticClass());
            if (DragonBreath)
                DragonBreath->Activate(false);
            Dragon->ProcessEvent(PlaySound, nullptr);
        }
        else if (Msg == L"tpalltome")
        {
            auto Pos = Controller->Pawn->K2_GetActorLocation();
            auto Rot = Controller->GetControlRotation();
            for (auto Player : UWorld::GetWorld()->NetDriver->ClientConnections)
            {
                Player->PlayerController->Pawn->K2_TeleportTo(Pos, Rot);
            }
        }
        else if (Msg == L"aircraft")
        {
            static auto MapInfo = Utils::FindObjectFast<AFortAthenaMapInfo>("DefaultMapInfo_UAID_E04F43E629FE0A2D01_1437486460");
            MsgBox("{}\n{}", MapInfo->GetFullName(), MapInfo->FlightInfos.Num());
        }
        else if (Msg == L"storm")
        {
            auto TimeSeconds = (float)UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());
            auto SZ = (AFortSafeZoneIndicator*)Utils::SpawnActor<ASafeZoneIndicator_C>();
            SZ->PreviousRadius = 100000.0f;
            SZ->NextRadius = 10000.0f;
            SZ->SafeZoneStartShrinkTime = TimeSeconds + 120.0f;
            SZ->SafeZoneFinishShrinkTime = TimeSeconds + 240.0f;
            SZ->CurrentPhase = 0;
            SZ->OnRep_CurrentPhase();
            FFortSafeZonePhaseInfo info;
            info.Radius = 10000.0f;
            info.WaitTime = 120.0f;
            info.ShrinkTime = 120.0f;
            SZ->SafeZonePhases.Add(info);
            SZ->PhaseCount = 1;
            SZ->OnRep_PhaseCount();
            auto Logic = UFortGameStateComponent_BattleRoyaleGamePhaseLogic::Get(UWorld::GetWorld());
            Logic->SafeZoneIndicator = SZ;
            Logic->OnRep_SafeZoneIndicator();

            Logic->SafeZonesStartTime = TimeSeconds + 120.0f;
            Logic->GamePhase = EAthenaGamePhase::SafeZones;
            Logic->GamePhaseStep = EAthenaGamePhaseStep::StormHolding;
            Logic->OnRep_GamePhase(EAthenaGamePhase::None);

            Logic->OnSafeZonePhaseChanged(); // This gets called on OnRep_SafeZoneIndicator
        }
    }

    void GetPlayerViewPoint(APlayerController* PlayerController, FVector* Location, FRotator* Rotation)
    {
        static FName NAME_Spectating = UKismetStringLibrary::Conv_StringToName(L"Spectating");
        if (PlayerController->StateName == NAME_Spectating)
        {
            *Location = PlayerController->LastSpectatorSyncLocation;
            *Rotation = PlayerController->LastSpectatorSyncRotation;
        }
        else if (PlayerController->Pawn)
        {
            *Location = PlayerController->Pawn->K2_GetActorLocation();
            *Rotation = PlayerController->GetControlRotation();
        }
        else
        {
            *Location = PlayerController->K2_GetActorLocation();
            *Rotation = PlayerController->K2_GetActorRotation();
        }
    }

    void Init()
    {
        Hook::VTable<AFortPlayerControllerAthena>(2440 / 8, ServerAcknowledgePossession);
        Hook::VTable<AFortPlayerControllerAthena>(4024 / 8, ServerCheat);
        Hook::AllVTables<APlayerController>(2032 / 8, GetPlayerViewPoint);
    }
}
