namespace Inventory
{
    FFortItemEntry* FindItemEntry(AFortPlayerController* PlayerController, FGuid& ItemGuid)
    {
        for (auto& ItemEntry : PlayerController->WorldInventory->Inventory.ReplicatedEntries)
        {
            if (UKismetGuidLibrary::EqualEqual_GuidGuid(ItemEntry.ItemGuid, ItemGuid))
                return &ItemEntry;
        }

        return nullptr;
    }

    FFortItemEntry* FindItemEntry(AFortPlayerController* PlayerController, UFortItemDefinition* ItemDef)
    {
        for (auto& ItemEntry : PlayerController->WorldInventory->Inventory.ReplicatedEntries)
        {
            if (ItemEntry.ItemDefinition == ItemDef)
                return &ItemEntry;
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

    UFortWorldItem* GiveItem(AFortPlayerController* PlayerController, UFortItemDefinition* ItemDef, int32 Count = -1)
    {
        if (!ItemDef || Count == 0)
            return nullptr;

        if (Count == -1)
            Count = ItemDef->GetMaxStackSize(nullptr);

        auto Item = (UFortWorldItem*)ItemDef->CreateTemporaryItemInstanceBP(Count, 1);
        auto& Inv = PlayerController->WorldInventory->Inventory;
        if (ItemDef->IsA(UFortWorldItemDefinition::StaticClass()))
        {
            auto WorldItemDef = (UFortWorldItemDefinition*)ItemDef;
            Item->ItemEntry.LoadedAmmo = WorldItemDef->GetInitialAmmo(1);
        }
        Inv.ReplicatedEntries.Add(Item->ItemEntry);
        Inv.ItemInstances.Add(Item);
        Update(PlayerController);

        return Item;
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
        Utils::SetWeakPtr(Pickup->PickupLocationData.PickupTarget, Pawn);
        Pickup->PickupLocationData.FlyTime = Params.FlyTime / 4;
        Pickup->PickupLocationData.bPlayPickupSound = Params.bPlayPickupSound;
        Pickup->OnRep_PickupLocationData();
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

    void Init()
    {
        Hook::VTable<AFortPlayerControllerAthena>(4456 / 8, ServerExecuteInventoryItem);
        Hook::VTable<AFortPlayerPawnAthena>(4576 / 8, ServerHandlePickupInfo);

        Hook::Function(InSDKUtils::GetImageBase() + 0x838A8A0, GivePickupToPlayer);
        Hook::Function(InSDKUtils::GetImageBase() + 0x8CA4A00, WeaponModAmmo, &WeaponModAmmoOriginal);

        Hook::UFunc("Function FortniteGame.FortKismetLibrary.K2_RemoveItemFromPlayer", K2_RemoveItemFromPlayer);
        Hook::UFunc("Function FortniteGame.InventoryManagementLibrary.AddItem", IML_AddItem);
    }
}
