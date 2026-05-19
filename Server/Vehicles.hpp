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

    void Init()
    {
        // TODO Don't do this. Do it through GameFeatures
#define FV(obj) Utils::FindObjectFast<UFortVehicleItemDefinition>(obj)
#define VTB(tag, obj) { Utils::MakeGameplayTag(TEXT(tag)), { FV(obj) } },
#define VTS(tag) { Utils::MakeGameplayTag(TEXT(tag)), { 
#define VTE()  } },

        std::vector<std::pair<FGameplayTag, std::vector<UFortVehicleItemDefinition*>>> VehicleTags =
        {
            VTB("Athena.Vehicle.SpawnLocation.Motorcycle.Dirtbike", "VID_Motorcycle_DirtBike_Vehicle")
            VTB("Athena.Vehicle.SpawnLocation.Motorcycle.Sportbike", "VID_Motorcycle_Sport_Vehicle")
            VTB("Athena.Vehicle.SpawnLocation.Motorboat", "VID_MeatballVehicle_L")
            VTS("Athena.Vehicle.SpawnLocation.Valet.BasicCar.Taxi") FV("VID_Valet_BasicCar_Vehicle"), FV("VID_Valet_TaxiCab_Vehicle"), FV("VID_Valet_BasicCar_Vehicle_Upgrade") VTE()
            VTS("Athena.Vehicle.SpawnLocation.Valet.BasicCar.Base") FV("VID_Valet_BasicCar_Vehicle"), FV("VID_Valet_BasicCar_Vehicle_Upgrade") VTE()
            VTS("Athena.Vehicle.SpawnLocation.Valet.BasicTruck.Base") FV("VID_Valet_BasicTruck_Vehicle"), FV("VID_Valet_BasicTruck_Vehicle_Upgrade") VTE()
            VTS("Athena.Vehicle.SpawnLocation.Valet.BigRig.Base") FV("VID_Valet_BigRig_Vehicle"), FV("VID_Valet_BigRig_Vehicle_Upgrade") VTE()
            VTS("Athena.Vehicle.SpawnLocation.Valet.SportsCar.Base") FV("VID_Valet_SportsCar_Vehicle"), FV("VID_Valet_SportsCar_Vehicle_Upgrade") VTE()
        };
#undef FV
#undef VTB
#undef VTS
#undef VTE

        auto Spawners = Utils::GetAllActorsOfClass<AFortAthenaLivingWorldVehiclePointProvider>();
        for (auto Spawner : Spawners)
        {
            for (auto thing : VehicleTags)
            {
                if (!UBlueprintGameplayTagLibrary::HasTag(Spawner->FiltersTags, thing.first, true))
                    continue;

                auto VehicleClass = Utils::GetSoftPtr(thing.second[UKismetMathLibrary::RandomInteger(thing.second.size())]->VehicleActorClass);
                if (!VehicleClass)
                    continue;

                FTransform translivesmatter = UKismetMathLibrary::ComposeTransforms(Spawner->GetTransform(), Spawner->SpawnPoints[0]);;
                Utils::SpawnActor(VehicleClass, translivesmatter);
                break;
            }
        }

        Hook::AllVTables<AFortPhysicsPawn>(2192 / 8, ServerMove);
    }
}
