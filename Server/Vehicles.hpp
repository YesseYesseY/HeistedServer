namespace Vehicles
{
    void ServerMove(AFortPhysicsPawn* Pawn, FReplicatedPhysicsPawnState* InState)
    {
        auto Component = (UPrimitiveComponent*)Pawn->GetComponentByClass(UPrimitiveComponent::StaticClass());

        FTransform translivesmatter;
        translivesmatter.Translation = InState->Translation;
        translivesmatter.Rotation = InState->Rotation;
        translivesmatter.Scale3D = { 1, 1, 1 };
        Component->K2_SetWorldTransform(translivesmatter, false, nullptr, true);
        Component->SetAllPhysicsLinearVelocity(InState->LinearVelocity, false);
        Component->SetAllPhysicsAngularVelocityInDegrees(InState->AngularVelocity, false);
    }

    struct VehicleInfo
    {
        std::string VIDName;
        UFortVehicleItemDefinition* VID;
        float Weight;

        VehicleInfo()
        {
            VIDName = "";
            Weight = 0.0f;
            VID = nullptr;
        }

        VehicleInfo(const std::string& vidname, float weight)
        {
            VIDName = vidname;
            Weight = weight;
            VID = nullptr;
        }

        UFortVehicleItemDefinition* GetVID()
        {
            if (!VID)
                VID = Utils::FindObjectFast<UFortVehicleItemDefinition>(VIDName);

            return VID;
        }
    };

    // I have decided i don't want to get all this at runtime
    std::unordered_map<FName, VehicleInfo> VehicleSpawnInfo =
    {
        // Dirtbike
        { UKismetStringLibrary::Conv_StringToName(L"Athena.Vehicle.SpawnLocation.Motorcycle.Dirtbike"), VehicleInfo("VID_Motorcycle_DirtBike_Vehicle", 1.0f) },

        // Sportbike
        { UKismetStringLibrary::Conv_StringToName(L"Athena.Vehicle.SpawnLocation.Motorcycle.Sportbike"), VehicleInfo("VID_Motorcycle_Sport_Vehicle", 1.0f) },

        // Motorboat
        { UKismetStringLibrary::Conv_StringToName(L"Athena.Vehicle.SpawnLocation.Motorboat"), VehicleInfo("VID_MeatballVehicle_L", 1.0f) },

        // GetAwayCar
        { UKismetStringLibrary::Conv_StringToName(L"Athena.Vehicle.SpawnLocation.Valet.BasicCar.Drift.GetAway.Civilian"), VehicleInfo("VID_Valet_BasicCar_Vehicle_GetAway", 100.0f) },
        { UKismetStringLibrary::Conv_StringToName(L"Athena.Vehicle.SpawnLocation.Valet.BasicCar.Drift.GetAway.Pizza"), VehicleInfo("VID_Valet_BasicCar_Vehicle_GetAway_Pizza", 100.0f) },
        { UKismetStringLibrary::Conv_StringToName(L"Athena.Vehicle.SpawnLocation.Valet.BasicCar.Drift.GetAway.VIP"), VehicleInfo("VID_Valet_BasicCar_Vehicle_GetAway_VIP", 100.0f) },

        // Valet - BasicCar
        { UKismetStringLibrary::Conv_StringToName(L"Athena.Vehicle.SpawnLocation.Valet.BasicCar.Base"), VehicleInfo("VID_Valet_BasicCar_Vehicle", 3.0f) },
        { UKismetStringLibrary::Conv_StringToName(L"Athena.Vehicle.SpawnLocation.Valet.BasicCar.Taxi"), VehicleInfo("VID_Valet_TaxiCab_Vehicle", 4.0f) },
        { UKismetStringLibrary::Conv_StringToName(L"Athena.Vehicle.SpawnLocation.Valet.BasicCar.Upgraded"), VehicleInfo("VID_Valet_BasicCar_Vehicle_Upgrade", 25.0f) },

        // Valet - BasicTruck
        { UKismetStringLibrary::Conv_StringToName(L"Athena.Vehicle.SpawnLocation.Valet.BasicTruck.Base"), VehicleInfo("VID_Valet_BasicTruck_Vehicle", 6.0f) },
        { UKismetStringLibrary::Conv_StringToName(L"Athena.Vehicle.SpawnLocation.Valet.BasicTruck.Upgraded"), VehicleInfo("VID_Valet_BasicTruck_Vehicle_Upgrade", 100.0f) },

        // Valet - BigRig
        { UKismetStringLibrary::Conv_StringToName(L"Athena.Vehicle.SpawnLocation.Valet.BigRig.Base"), VehicleInfo("VID_Valet_BigRig_Vehicle", 6.0f) },
        { UKismetStringLibrary::Conv_StringToName(L"Athena.Vehicle.SpawnLocation.Valet.BigRig.Upgraded"), VehicleInfo("VID_Valet_BigRig_Vehicle_Upgrade", 100.0f) },

        // Valet - SportsCar
        { UKismetStringLibrary::Conv_StringToName(L"Athena.Vehicle.SpawnLocation.Valet.SportsCar.Base"), VehicleInfo("VID_Valet_SportsCar_Vehicle", 6.0f) },
        { UKismetStringLibrary::Conv_StringToName(L"Athena.Vehicle.SpawnLocation.Valet.SportsCar.Upgraded"), VehicleInfo("VID_Valet_SportsCar_Vehicle_Upgrade", 100.0f) },
    };

    struct WeightedIndexRandomizer
    {
        std::vector<float> Weights;

        int32 Random(float TotalWeight = -1.0f)
        {
            if (TotalWeight == -1.0f)
            {
                TotalWeight = 0.0f;
                for (float Weight : Weights)
                    TotalWeight += Weight;
            }

            float Randy = UKismetMathLibrary::RandomFloatInRange(0, TotalWeight);
            float Total = 0.0f;

            for (int32 i = 0; i < Weights.size(); i++)
            {
                Total += Weights[i];
                if (Total >= Randy)
                {
                    return i;
                }
            }

            return 0;
        }

        void Add(float Weight)
        {
            Weights.push_back(Weight);
        }
    };

    void Init()
    {
        auto Spawners = Utils::GetAllActorsOfClass<AFortAthenaLivingWorldVehiclePointProvider>();
        for (auto Spawner : Spawners)
        {
            WeightedIndexRandomizer WIR;
            std::vector<VehicleInfo*> SpawnerVehicleInfo;

            for (auto& Tag : Spawner->FiltersTags.GameplayTags)
            {
                if (VehicleSpawnInfo.contains(Tag.TagName))
                {
                    SpawnerVehicleInfo.push_back(&VehicleSpawnInfo[Tag.TagName]);
                    WIR.Add(VehicleSpawnInfo[Tag.TagName].Weight);
                }
            }

            if (SpawnerVehicleInfo.size() <= 0)
            {
                MsgBox("Nothing for {}", Spawner->GetFullName());
                continue;
            }

            auto VID = SpawnerVehicleInfo[WIR.Random()]->GetVID();
            if (!VID)
                continue;

            auto VehicleClass = Utils::GetSoftPtr(VID->VehicleActorClass);
            if (!VehicleClass)
                continue;

            // TODO Multiple SpawnPoints? Maybe?
            //      SpawnChance
            FTransform translivesmatter = UKismetMathLibrary::ComposeTransforms(Spawner->GetTransform(), Spawner->SpawnPoints[0]);;
            Utils::SpawnActor(VehicleClass, translivesmatter);
        }

        Hook::AllVTables<AFortPhysicsPawn>(2192 / 8, ServerMove);
    }
}
