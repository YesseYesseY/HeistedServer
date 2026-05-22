namespace DataLayers
{
    UDataLayerManager* GetManager()
    {
        static auto Ret = Utils::FindFirstObjectOfClass<UDataLayerManager>();
        return Ret;
    }

    void Activate(UDataLayerAsset* DL)
    {
        if (auto Manager = GetManager())
            Manager->SetDataLayerRuntimeState(DL, EDataLayerRuntimeState::Activated, true);
    }

    void Deactivate(UDataLayerAsset* DL)
    {
        if (auto Manager = GetManager())
            Manager->SetDataLayerRuntimeState(DL, EDataLayerRuntimeState::Unloaded, true);
    }

    void Dump()
    {
        std::ofstream outfile("datalayers.txt");
        for (int i = 0; i < UObject::GObjects->Num(); ++i)
        {
            UObject* Object = UObject::GObjects->GetByIndex(i);

            if (!Object || Object->IsDefaultObject())
                continue;

            if (!Object->IsA(UDataLayerAsset::StaticClass()))
                continue;

            auto DL = (UDataLayerAsset*)Object;
            auto DLI = GetManager()->GetDataLayerInstanceFromAsset(DL);
            outfile << std::format("{} = {}\n", DL->GetName(), GetManager()->GetDataLayerInstanceRuntimeState(DLI) == EDataLayerRuntimeState::Activated ? "Activated" : "Deactivated");
        }
        outfile.close();
    }
}
