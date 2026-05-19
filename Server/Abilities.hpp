namespace Abilities
{
    static inline bool (*InternalTryActivateAbility)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, const FPredictionKey&, UGameplayAbility**, void* OnGameplayAbilityEndedDelegate, void* TriggerEventData);

    FGameplayAbilitySpec* FindAbilitySpecFromHandle(UAbilitySystemComponent* Component, FGameplayAbilitySpecHandle Handle)
    {
        auto& Items = Component->ActivatableAbilities.Items;
        for (auto& Spec : Items)
        {
            if (Spec.Handle.Handle == Handle.Handle)
                return &Spec;
        }

        return nullptr;
    }

    void InternalServerTryActivateAbility(UAbilitySystemComponent* Component, FGameplayAbilitySpecHandle Handle, bool InputPressed, FPredictionKey& PredictionKey, FGameplayEventData* TriggerEventData)
    {
        FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Component, Handle);
        if (!Spec)
        {
            Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
            return;
        }

        UGameplayAbility* AbilityToActivate = Spec->Ability;
        if (!AbilityToActivate)
        {
            Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
            return;
        }

        if (AbilityToActivate->NetSecurityPolicy == EGameplayAbilityNetSecurityPolicy::ServerOnlyExecution ||
            AbilityToActivate->NetSecurityPolicy == EGameplayAbilityNetSecurityPolicy::ServerOnly)
        {
            Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
            return;
        }

        UGameplayAbility* InstancedAbility = nullptr;
        Spec->InputPressed = true;

        if (InternalTryActivateAbility(Component, Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
        {
        }
        else
        {
            Component->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
            Spec->InputPressed = false;
            Utils::MarkArrayDirty(Component->ActivatableAbilities);
        }
    }

    void GiveAbilitySet(UAbilitySystemComponent* ASC, UFortAbilitySet* AbilitySet)
    {
        for (auto Ability : AbilitySet->GameplayAbilities)
            ASC->K2_GiveAbility(Ability, 1, 1);

        for (auto Effect : AbilitySet->GrantedGameplayEffects)
            ASC->BP_ApplyGameplayEffectToSelf(Effect.GameplayEffect, Effect.Level, {});
    }

    void Init()
    {
        InternalTryActivateAbility = decltype(InternalTryActivateAbility)(InSDKUtils::GetImageBase() + 0x68e7084);

        Hook::AllVTables<UAbilitySystemComponent>(2224 / 8, InternalServerTryActivateAbility);
    }
}
