namespace Player
{
    void ServerAcknowledgePossession(APlayerController* Controller, APawn* Pawn)
    {
        Controller->AcknowledgedPawn = Pawn;

        auto PlayerState = (AFortPlayerStateAthena*)Controller->PlayerState;
        auto ASC = PlayerState->AbilitySystemComponent;

        auto AbilitySet = UObject::FindObject<UFortAbilitySet>("FortAbilitySet GAS_AthenaPlayer.GAS_AthenaPlayer");
        for (auto Ability : AbilitySet->GameplayAbilities)
        {
            ASC->K2_GiveAbility(Ability, 1, 1);
        }
        ASC->K2_GiveAbility(UObject::FindClassFast("GA_Athena_TacticalSprint_C"), 1, 1);
    }

    void ServerCheat(AFortPlayerControllerAthena* Controller, FString& FMsg)
    {
        auto Msg = FMsg.ToWString();

        if (Msg == L"test")
        {
            MsgBox("UwU");
        }
        else if (Msg == L"dumpobjects")
        {
            Utils::DumpObjects();
        }
    }

    void Init()
    {
        Hook::VTable<AFortPlayerControllerAthena>(2440 / 8, ServerAcknowledgePossession);
        Hook::VTable<AFortPlayerControllerAthena>(4024 / 8, ServerCheat);
    }
}
