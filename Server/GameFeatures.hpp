namespace GameFeatures
{
    std::vector<UGameFeatureData*> Active;

    void Init()
    {
        auto SubsystemClass = UGameFeaturesSubsystem::StaticClass();
        if (!SubsystemClass)
            return;

        auto Subsystem = UObject::FindObjectFast<UGameFeaturesSubsystem>("GameFeaturesSubsystem_2147482646");
        if (!Subsystem)
            return;

        for (auto& thing : Subsystem->GameFeaturePluginStateMachines)
        {
            auto& StateProps = thing.Value()->StateProperties;
            if (*(uint8*)(int64(thing.Value()) + 40) != 28) // Active
                continue;

            Active.push_back(StateProps.GameFeatureData);
        }
    }
}
