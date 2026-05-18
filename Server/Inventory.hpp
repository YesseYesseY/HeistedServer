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

    void Update(AFortPlayerController* PlayerController)
    {
        PlayerController->WorldInventory->HandleInventoryLocalUpdate();
        Utils::MarkArrayDirty(PlayerController->WorldInventory->Inventory);
    }

    void GiveItem(AFortPlayerController* PlayerController, UFortItemDefinition* ItemDef, int32 Count = -1)
    {
        if (!ItemDef || Count == 0)
            return;

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

    void Init()
    {
        Hook::VTable<AFortPlayerControllerAthena>(4456 / 8, ServerExecuteInventoryItem);
    }
}
