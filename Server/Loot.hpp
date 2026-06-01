namespace Loot
{
    struct LootTierData
    {
        float Weight;
        FName LootPackage;
        float NumLootPackageDrops;
        int32 LootTier;
    
        LootTierData(FFortLootTierData* Data)
        {
            Weight = Data->Weight;
            LootPackage = Data->LootPackage;
            NumLootPackageDrops = Data->NumLootPackageDrops;
            LootTier = Data->LootTier;
    
            // TODO Whyy is this needed
            if (LootPackage.ToString().contains("WorldPKG.AthenaLoot.Weapon") && Data->TierGroup.ToString().contains("FloorLoot")) 
            {
                NumLootPackageDrops = 2.0f;
            }
        }
    };
    
    struct LootPackageData
    {
        float Weight;
        FName LootPackageCall;
        UFortItemDefinition* ItemDef;
        int32 Count;
    
        LootPackageData(FFortLootPackageData* Data)
        {
            Weight = Data->Weight;
            LootPackageCall = UKismetStringLibrary::Conv_StringToName(Data->LootPackageCall);
            ItemDef = Utils::GetSoftPtr(Data->ItemDefinition);
            Count = Data->Count;
        }
    };
    
    template <typename T>
    struct WeightedType
    {
        T Value;
        float Weight;
    };
    
    template <typename T>
    struct WeightedContainerSimple
    {
        float TotalWeight = 0.0f;
        std::vector<WeightedType<T>> Items;
    
        T& GetRandomItem()
        {
            float Randy = UKismetMathLibrary::RandomFloatInRange(0, TotalWeight);
            float Total = 0.0f;
    
            for (auto& Item : Items)
            {
                Total += Item.Weight;
                if (Total >= Randy)
                {
                    return Item.Value;
                }
            }
    
            return Items[0].Value;
        }
    
        void Add(T Item, float Weight)
        {
            if (Weight <= 0.0f)
                return;
    
            TotalWeight += Weight;
            Items.push_back(WeightedType<T>(Item, Weight));
        }
    };
    
    template <typename T>
    struct WeightedContainer
    {
        float TotalWeight = 0.0f;
        std::vector<T> Items;
    
        T& GetRandomItem()
        {
            float Randy = (float)UKismetMathLibrary::RandomFloatInRange(0, TotalWeight);
            float Total = 0.0f;
    
            for (auto& Item : Items)
            {
                Total += Item.Weight;
                if (Total >= Randy)
                {
                    return Item;
                }
            }
    
            return Items[0];
        }
    };
    
    struct LootTierDataContainer : WeightedContainer<LootTierData>
    {
        std::unordered_map<int32, float> LootTierTotalWeight;
    
        float GetTotalWeight(int32 LootTier)
        {
            if (LootTierTotalWeight.contains(LootTier))
                return LootTierTotalWeight[LootTier];
    
            MsgBox("TotalWeight not found for LootTier {} {}", LootTier, LootTierTotalWeight.size());
    
            return 0.0f;
        }
    
        void Add(FFortLootTierData* Data)
        {
            auto weight = Data->Weight;
            if (weight <= 0.0f)
                return;
    
            auto LootTier = Data->LootTier;
    
            if (!LootTierTotalWeight.contains(LootTier))
                LootTierTotalWeight[LootTier] = 0.0f;
    
            LootTierTotalWeight[LootTier] += weight;
    
            TotalWeight += weight;
            Items.push_back(LootTierData(Data));
        }
    
        LootTierData& GetRandomItemWithLootTier(int32 LootTier)
        {
            float Randy = (float)UKismetMathLibrary::RandomFloatInRange(0, GetTotalWeight(LootTier));
            float Total = 0.0f;
    
            for (auto& Item : Items)
            {
                if (Item.LootTier != LootTier)
                    continue;
    
                Total += Item.Weight;
                if (Total >= Randy)
                {
                    return Item;
                }
            }
    
            return Items[0];
        }
    };
    
    struct LootPackageDataContainer : WeightedContainer<LootPackageData>
    {
        void Add(FFortLootPackageData* Data)
        {
            auto weight = Data->Weight;
            if (weight <= 0.0f)
                return;
    
            TotalWeight += weight;
            Items.push_back(LootPackageData(Data));
        }
    };

    std::unordered_map<FName, LootTierDataContainer> LootTiers;
    std::unordered_map<FName, LootPackageDataContainer> LootPackages;

    std::vector<std::pair<UFortItemDefinition*, int32>> Get(FName TierGroup, int32 InLootTier = -1)
    {
        static auto GameMode = (AFortGameModeAthena*)UGameplayStatics::GetGameMode(UWorld::GetWorld());
        static std::unordered_map<FName, FName> TierGroupRedirect = {
        };
        static bool InitedTierGroupRedirect = false;
        if (!InitedTierGroupRedirect)
        {
            InitedTierGroupRedirect = true;
            for (auto& thing : GameMode->RedirectAthenaLootTierGroups)
                TierGroupRedirect[thing.Key()] = thing.Value();
        }

        if (TierGroupRedirect.contains(TierGroup))
            TierGroup = TierGroupRedirect[TierGroup];

        std::vector<std::pair<UFortItemDefinition*, int32>> Ret;

        if (!LootTiers.contains(TierGroup))
        {
            MsgBox("TierGroup {} not in LootTiers", TierGroup.ToString());
            return Ret;
        }

        auto LootTier = LootTiers[TierGroup];
        auto LTI = InLootTier == -1 ? LootTier.GetRandomItem() : LootTier.GetRandomItemWithLootTier(InLootTier);

        if (!LootPackages.contains(LTI.LootPackage))
        {
            MsgBox("LootPackage {} not in LootPackages", LTI.LootPackage.ToString());
            return Ret;
        }

        auto BaseLootPackage = LootPackages[LTI.LootPackage];
        auto IsWorldList = LTI.LootPackage.ToString().starts_with("WorldList"); // Is there a better way?
        if (IsWorldList)
        {
            for (int i = 0; i < LTI.NumLootPackageDrops; i++)
            {
                if (BaseLootPackage.Items.size() <= 0)
                {
                    MsgBox("Invalid Size");
                    continue;
                }
                auto toadd = BaseLootPackage.GetRandomItem();
                Ret.push_back({ toadd.ItemDef, toadd.Count });
            }
        }
        else
        {
            for (int i = 0; i < LTI.NumLootPackageDrops; i++)
            {
                if (i >= BaseLootPackage.Items.size())
                    continue;

                if (!LootPackages.contains(BaseLootPackage.Items[i].LootPackageCall))
                {
                    MsgBox("LootPackageCall {} not in LootPackages", BaseLootPackage.Items[i].LootPackageCall.ToString());
                    return Ret;
                }

                auto RealLootPackage = LootPackages[BaseLootPackage.Items[i].LootPackageCall];
                if (RealLootPackage.Items.size() <= 0)
                {
                    MsgBox("{}", BaseLootPackage.Items[i].LootPackageCall.ToString());
                    continue;
                }
                auto toadd = RealLootPackage.GetRandomItem();
                Ret.push_back({ toadd.ItemDef, toadd.Count });
            }
        }

        return Ret;
    }

    bool SpawnLoot(ABuildingContainer* Container)
    {
        if (!Container)
            return false;

        float a2 = Container->LootTossConeHalfAngle_Athena;

        // Probably faster to just do int v13 = 4; it's been 4 on every container ive tried, which is: chests, ammo boxes, produce boxes, coolers, fishing rod barrels
        int v13 = -((int)(float)(-0.5f - (float)((float)(a2 * 0.055556f) + (float)(a2 * 0.055556f))) >> 1);

        auto Drops = Get(Container->SearchLootTierGroup);
        auto DropLocation = UKismetMathLibrary::TransformLocation(Container->GetTransform(), { Container->LootSpawnLocation_Athena.X, Container->LootSpawnLocation_Athena.Y, Container->LootSpawnLocation_Athena.Z });
        for (auto& Drop : Drops)
        {
            if (!Drop.first || Drop.second <= 0)
                continue;

            auto Pickup = Utils::SpawnActor<AFortPickupAthena>(DropLocation);
            Pickup->PrimaryPickupItemEntry.ItemDefinition = Drop.first;
            Pickup->PrimaryPickupItemEntry.Count = Drop.second;
            Pickup->OnRep_PrimaryPickupItemEntry();
            auto Step = UKismetMathLibrary::RandomInteger(v13);
            FRotator Dir = { (double)Container->LootTossDirection_Athena.Pitch, (double)Container->LootTossDirection_Athena.Yaw, (double)Container->LootTossDirection_Athena.Roll };
            UFortKismetLibrary::TossPickupFromContainer(UWorld::GetWorld(), Container, Pickup, v13, Step, Container->LootTossConeHalfAngle_Athena, Dir, Container->LootTossSpeed_Athena, Container->bForceHidePickupMinimapIndicator);
        }

        // Container->bAlreadySearched = true;
        // Container->OnRep_bAlreadySearched();
        // Container->RaiseTreasure();

        return true;
    }

    void AddLTD(UDataTable* LTD)
    {
        if (!LTD)
            return;

        for (auto& thing : LTD->RowMap)
        {
            auto Data = (FFortLootTierData*)thing.Value();
            LootTiers[Data->TierGroup].Add(Data);
        }
    }

    void AddLPD(UDataTable* LPD)
    {
        if (!LPD)
            return;

        for (auto& thing : LPD->RowMap)
        {
            auto Data = (FFortLootPackageData*)thing.Value();
            LootPackages[Data->LootPackageID].Add(Data);
        }
    }

    void K2_SpawnPickupInWorld(UObject* Obj, FFrame* Stack, AFortPickup** Ret)
    {
        FRAME_PROP(UObject*, WorldContextObject);
        FRAME_PROP(UFortWorldItemDefinition*, ItemDefinition);
        FRAME_PROP(int32, NumberToSpawn);
        FRAME_PROP(FVector, Position);
        FRAME_PROP(FVector, Direction);
        FRAME_PROP(int32, OverrideMaxStackCount);
        FRAME_PROP(bool, bToss);
        FRAME_PROP(bool, bRandomRotation);
        FRAME_PROP(bool, bBlockedFromAutoPickup);
        FRAME_PROP(int32, PickupInstigatorHandle);
        FRAME_PROP(EFortPickupSourceTypeFlag, SourceType);
        FRAME_PROP(EFortPickupSpawnSource, Source);
        FRAME_PROP(AFortPlayerController*, OptionalOwnerPC);
        FRAME_PROP(bool, bPickupOnlyRelevantToOwner);
        FRAME_END();

        auto Pickup = Utils::SpawnActor<AFortPickupAthena>(Position);
        Pickup->bRandomRotation = bRandomRotation;
        Pickup->PrimaryPickupItemEntry.ItemDefinition = ItemDefinition;
        Pickup->PrimaryPickupItemEntry.Count = NumberToSpawn;
        Pickup->OnRep_PrimaryPickupItemEntry();
        if (bToss)
            Pickup->TossPickup(Position, OptionalOwnerPC ? OptionalOwnerPC->MyFortPawn : nullptr, OverrideMaxStackCount, bToss, true, SourceType, Source);

        *Ret = Pickup;
    }

    // TODO K2_RemoveItemFromPlayer
    //      PickLootDrops

    void Init()
    {
        Hook::Function(InSDKUtils::GetImageBase() + 0x7C4C0A4, SpawnLoot);
        Hook::UFunc("Function FortniteGame.FortKismetLibrary.K2_SpawnPickupInWorld", K2_SpawnPickupInWorld);

        auto GameState = (AFortGameStateAthena*)UGameplayStatics::GetGameState(UWorld::GetWorld());
        auto CurrentPlaylist = GameState->CurrentPlaylistInfo.BasePlaylist;
        auto& PlaylistTags = CurrentPlaylist->GameplayTagContainer;

        auto LTD = Utils::GetSoftPtr(CurrentPlaylist->LootTierData);
        if (!LTD)
            LTD = Utils::FindObjectFast<UDataTable>("AthenaLootTierData_Client");

        auto LPD = Utils::GetSoftPtr(CurrentPlaylist->LootPackages);
        if (!LPD)
            LPD = Utils::FindObjectFast<UDataTable>("AthenaLootPackages_Client");

        AddLTD(LTD);
        AddLPD(LPD);

        for (auto GameFeatureData : GameFeatures::Active)
        {
            if (!GameFeatureData->IsA(UFortGameFeatureData::StaticClass()))
                continue;

            auto Data = (UFortGameFeatureData*)GameFeatureData;

            bool AddedLoot = false;
            for (auto& thing : Data->PlaylistOverrideLootTableData)
            {
                if (UBlueprintGameplayTagLibrary::HasTag(PlaylistTags, thing.Key(), true))
                {
                    AddedLoot = true;
                    AddLTD(Utils::GetSoftPtr(thing.Value().LootTierData));
                    AddLPD(Utils::GetSoftPtr(thing.Value().LootPackageData));
                    break;
                }
            }

            if (!AddedLoot)
            {
                AddLTD(Utils::GetSoftPtr(Data->DefaultLootTableData.LootTierData));
                AddLPD(Utils::GetSoftPtr(Data->DefaultLootTableData.LootPackageData));
            }
        }

        auto FLC1 = UObject::FindClassFast("Tiered_Athena_FloorLoot_01_C");
        auto FLC2 = UObject::FindClassFast("Tiered_Athena_FloorLoot_Warmup_C");
        auto FloorLootContainers = Utils::GetAllActorsOfClass<ABuildingContainer>(FLC1);
        for (auto Container : FloorLootContainers)
            SpawnLoot(Container);
        FloorLootContainers.Free();

        FloorLootContainers = Utils::GetAllActorsOfClass<ABuildingContainer>(FLC2);
        for (auto Container : FloorLootContainers)
            SpawnLoot(Container);
        FloorLootContainers.Free();

    }
}
