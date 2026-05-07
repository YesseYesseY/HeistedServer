namespace Utils
{
    template <typename T>
    T* SpawnActor(FVector Pos = {}, FRotator Rot = {}, FVector Size = { 1, 1, 1 })
    {
        FTransform translivesmatter = UKismetMathLibrary::MakeTransform(Pos, Rot, Size);
        auto Ret = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), T::StaticClass(), translivesmatter, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, nullptr, ESpawnActorScaleMethod::MultiplyWithRoot);

        if (Ret)
            Ret = UGameplayStatics::FinishSpawningActor(Ret, translivesmatter, ESpawnActorScaleMethod::MultiplyWithRoot);

        return (T*)Ret;
    }

    template <typename T>
    T* SpawnObject(UObject* Outer)
    {
        return (T*)UGameplayStatics::SpawnObject(T::StaticClass(), Outer);
    }

    void ExecuteConsoleCommand(const wchar_t* Cmd)
    {
        UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), Cmd, nullptr);
    }
}

