namespace Inventory
{
    FFortItemEntry* FindItemEntry(AFortPlayerController* PlayerController, FGuid& ItemGuid, int* Idx = nullptr)
    {
        auto& Entries = PlayerController->WorldInventory->Inventory.ReplicatedEntries;
        for (int i = 0; i < Entries.Num(); i++)
        {
            auto& ItemEntry = Entries[i];
            if (UKismetGuidLibrary::EqualEqual_GuidGuid(ItemEntry.ItemGuid, ItemGuid))
            {
                if (Idx)
                    *Idx = i;

                return &ItemEntry;
            }
        }

        return nullptr;
    }

    FFortItemEntry* FindItemEntry(AFortPlayerController* PlayerController, UFortItemDefinition* ItemDef, int* Idx = nullptr)
    {
        auto& Entries = PlayerController->WorldInventory->Inventory.ReplicatedEntries;
        for (int i = 0; i < Entries.Num(); i++)
        {
            auto& ItemEntry = Entries[i];
            if (ItemEntry.ItemDefinition == ItemDef)
            {
                if (Idx)
                    *Idx = i;

                return &ItemEntry;
            }
        }

        return nullptr;
    }

    void Update(AFortPlayerController* PlayerController, FFortItemEntry* ItemEntry = nullptr)
    {
        PlayerController->WorldInventory->HandleInventoryLocalUpdate();
        if (ItemEntry)
        {
            Utils::MarkItemDirty(PlayerController->WorldInventory->Inventory, ItemEntry);
        }
        else
        {
            Utils::MarkArrayDirty(PlayerController->WorldInventory->Inventory);
        }
    }

    void RemoveIndex(AFortPlayerController* PlayerController, int Idx)
    {
        PlayerController->WorldInventory->Inventory.ReplicatedEntries.Remove(Idx);
        PlayerController->WorldInventory->Inventory.ItemInstances.Remove(Idx);
        Update(PlayerController);
    }

    void DumpInventory(AFortPlayerController* PlayerController, bool DropItems)
    {
        FVector Pos = { 0, 0, 0 };
        if (DropItems && PlayerController->Pawn)
            Pos = PlayerController->Pawn->K2_GetActorLocation();

        auto& Entries = PlayerController->WorldInventory->Inventory.ReplicatedEntries;
        for (int i = Entries.Num() - 1; i >= 0; i--)
        {
            auto& ItemEntry = Entries[i];
            auto ItemDef = (UFortWorldItemDefinition*)ItemEntry.ItemDefinition;
            if (ItemDef->bCanBeDropped)
            {
                if (DropItems)
                {
                    auto Pickup = Utils::SpawnActor<AFortPickupAthena>(Pos);
                    Pickup->PrimaryPickupItemEntry = ItemEntry;
                    Pickup->TossPickup(Pos, nullptr, 1, true, false, EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::Unset);
                }

                RemoveIndex(PlayerController, i);
            }
        }
    }

    UFortWorldItem* GiveItem(AFortPlayerController* PlayerController, UFortItemDefinition* ItemDef, int32 Count = -1)
    {
        if (!ItemDef || Count == 0)
            return nullptr;

        auto MaxStackSize = ItemDef->GetMaxStackSize(nullptr);

        if (Count == -1)
            Count = MaxStackSize;

        UFortWorldItem* Ret = nullptr;
        int SlotCount = 0;

        auto& Entries = PlayerController->WorldInventory->Inventory.ReplicatedEntries;
        for (int i = 0; i < Entries.Num(); i++)
        {
            auto& ItemEntry = Entries[i];

            if (ItemEntry.ItemDefinition != ItemDef || Count <= 0 || ItemEntry.Count >= MaxStackSize)
                continue;

            auto HowManyToAdd = std::min(MaxStackSize - ItemEntry.Count, Count);
            Count -= HowManyToAdd;

            ItemEntry.Count += HowManyToAdd;
            Update(PlayerController, &ItemEntry);

            Ret = PlayerController->WorldInventory->Inventory.ItemInstances[i];
        }

        int InitialAmmo = -1;
        if (ItemDef->IsA(UFortWorldItemDefinition::StaticClass()))
        {
            auto WorldItemDef = (UFortWorldItemDefinition*)ItemDef;
            InitialAmmo = WorldItemDef->GetInitialAmmo(1);
            // TODO bForceFocusWhenAdded
            //      bShouldShowItemToast
        }

        while (Count > 0)
        {
            if (!ItemDef->bAllowMultipleStacks && Ret)
                break;

            int ToAdd = std::min(Count, MaxStackSize);
            auto Item = (UFortWorldItem*)ItemDef->CreateTemporaryItemInstanceBP(ToAdd, 1);
            if (InitialAmmo != -1)
                Item->ItemEntry.LoadedAmmo = InitialAmmo;

            auto& Inv = PlayerController->WorldInventory->Inventory;
            Inv.ReplicatedEntries.Add(Item->ItemEntry);
            Inv.ItemInstances.Add(Item);
            Update(PlayerController);

            Count -= ToAdd;
            Ret = Item;
        }

        // TODO Drop leftovers

        return Ret;
    }

    void EquipItemEntry(AFortPlayerController* PlayerController, FFortItemEntry* ItemEntry)
    {
        if (PlayerController->IsInAircraft())
            return;

        auto Pawn = (AFortPawn*)PlayerController->Pawn;
        Pawn->EquipWeaponDefinition((UFortWeaponItemDefinition*)ItemEntry->ItemDefinition, ItemEntry->ItemGuid, {}, false);
    }

    void ServerExecuteInventoryItem(AFortPlayerController* PlayerController, FGuid& ItemGuid)
    {
        if (auto ItemEntry = FindItemEntry(PlayerController, ItemGuid))
            EquipItemEntry(PlayerController, ItemEntry);
    }

    void ServerHandlePickupInfo(AFortPlayerPawn* Pawn, AFortPickup* Pickup, FFortPickupRequestInfo& Params)
    {
        Pickup->bForceDefaultFlyTime = true;
        Pickup->DefaultFlyTime = Params.FlyTime / 4; // TODO FlyTime still doesn't feel right, look more into this
        Pickup->BlueprintSetPickupTarget(Pawn, Params.bPlayPickupSound);
    }

    void GivePickupToPlayer(AFortPickup* Pickup, uintptr_t InventoryInterface, uint8 a3)
    {
        AFortPlayerController* Controller = (AFortPlayerController*)(InventoryInterface - 0x8B8);
        GiveItem(Controller, Pickup->PrimaryPickupItemEntry.ItemDefinition, Pickup->PrimaryPickupItemEntry.Count);
        Pickup->K2_DestroyActor();
    }

    int32 RemoveItem(AFortPlayerController* PlayerController, UFortWorldItemDefinition* ItemDefinition, int32 Amount)
    {
        int32 Ret = 0;

        for (int i = 0; i < PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
        {
            auto& ItemEntry = PlayerController->WorldInventory->Inventory.ReplicatedEntries[i];
            if (ItemEntry.ItemDefinition != ItemDefinition)
                continue;

            if (Amount >= ItemEntry.Count)
            {
                Ret = ItemEntry.Count;
                PlayerController->WorldInventory->Inventory.ReplicatedEntries.Remove(i);
                PlayerController->WorldInventory->Inventory.ItemInstances.Remove(i);
                Update(PlayerController);
                Amount -= Ret;
            }
            else
            {
                Ret = Amount;
                ItemEntry.Count -= Amount;
                Update(PlayerController, &ItemEntry);
                Amount = 0;
            }

            if (Amount <= 0)
                break;
        }

        return Ret;
    }

    void RemoveItem(AFortPlayerController* PlayerController, FGuid& ItemGuid, int32 Count)
    {
        int Idx = -1;
        if (auto ItemEntry = FindItemEntry(PlayerController, ItemGuid, &Idx))
        {
            if (Count >= ItemEntry->Count)
            {
                RemoveIndex(PlayerController, Idx);
            }
            else
            {
                ItemEntry->Count -= Count;
                Update(PlayerController, ItemEntry);
            }
        }
    }

    void K2_RemoveItemFromPlayer(UObject* Object, FFrame* Stack, int32* Ret)
    {
        FRAME_PROP(AFortPlayerController*, PlayerController);
        FRAME_PROP(UFortWorldItemDefinition*, ItemDefinition);
        FRAME_PROP(FGuid, ItemVariantGuid);
        FRAME_PROP(int32, AmountToRemove);
        FRAME_PROP(bool, bForceRemoval);
        FRAME_END();

        *Ret = RemoveItem(PlayerController, ItemDefinition, AmountToRemove);
    }

    void IML_AddItem(UObject* Object, FFrame* Stack, UFortWorldItem** Ret)
    {
        FRAME_PROP(TScriptInterface<IFortInventoryOwnerInterface>, InventoryInterface);
        FRAME_PROP(UFortItemDefinition*, ItemDefinition);
        FRAME_PROP(int32, Count);
        FRAME_PROP(bool, bShouldFireCollectStatEvent);
        FRAME_END();

        auto Controller = (AFortPlayerController*)InventoryInterface.ObjectPointer;
        GiveItem(Controller, ItemDefinition, Count);
    }

    void (*WeaponModAmmoOriginal)(AFortWeapon* Weapon, int a2);
    void WeaponModAmmo(AFortWeapon* Weapon, int a2)
    {
        WeaponModAmmoOriginal(Weapon, a2);

        auto Pawn = (AFortPlayerPawnAthena*)Weapon->GetOwner();
        auto Controller = (AFortPlayerControllerAthena*)Pawn->Controller;
        if (auto ItemEntry = FindItemEntry(Controller, Weapon->ItemEntryGuid))
        {
            ItemEntry->LoadedAmmo = Weapon->AmmoCount;
            Update(Controller, ItemEntry);
        }
    }

    void ServerAttemptInventoryDrop(AFortPlayerController* PlayerController, FGuid& ItemGuid, int32 Count, bool bTrash)
    {
        if (Count <= 0)
            return;

        int Idx = -1;
        if (auto ItemEntry = FindItemEntry(PlayerController, ItemGuid, &Idx))
        {
            auto Pawn = (AFortPlayerPawn*)PlayerController->Pawn;
            auto Pos = Pawn->K2_GetActorLocation();

            AFortPickup* Pickup = nullptr;
            if (!bTrash)
            {
                Pickup = Utils::SpawnActor<AFortPickupAthena>(Pos);
                Pickup->PrimaryPickupItemEntry = *ItemEntry;
            }

            int32 Removed = 0;
            if (Count >= ItemEntry->Count)
            {
                Removed = ItemEntry->Count;
                RemoveIndex(PlayerController, Idx);
            }
            else
            {
                Removed = Count;
                ItemEntry->Count -= Count;
                Update(PlayerController, ItemEntry);
            }

            if (Pickup)
            {
                Pickup->PrimaryPickupItemEntry.Count = Removed;
                // TODO real tossing
                Pickup->TossPickup(Pos, Pawn, Removed, true, false, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::TossedByPlayer);
            }
        }
    }

    void InterfaceRemoveItem(int64 InventoryInterface, FGuid& ItemGuid, int32 Count, bool bForceRemoval)
    {
        AFortPlayerController* Controller = (AFortPlayerController*)(InventoryInterface - 0x8B8);
        RemoveItem(Controller, ItemGuid, Count);
    }

    void Init()
    {
        Hook::VTable<AFortPlayerControllerAthena>(4456 / 8, ServerExecuteInventoryItem);
        Hook::VTable<AFortPlayerControllerAthena>(4544 / 8, ServerAttemptInventoryDrop);
        Hook::VTable<AFortPlayerPawnAthena>(4576 / 8, ServerHandlePickupInfo);

        Hook::Function(InSDKUtils::GetImageBase() + 0x838A8A0, GivePickupToPlayer);
        Hook::Function(InSDKUtils::GetImageBase() + 0x8887A18, InterfaceRemoveItem);

        Hook::Function(InSDKUtils::GetImageBase() + 0x8CA4A00, WeaponModAmmo, &WeaponModAmmoOriginal);

        Hook::UFunc("Function FortniteGame.FortKismetLibrary.K2_RemoveItemFromPlayer", K2_RemoveItemFromPlayer);
        Hook::UFunc("Function FortniteGame.InventoryManagementLibrary.AddItem", IML_AddItem);
    }
}
