namespace Utils
{
    template <typename T = AActor>
    T* SpawnActor(UClass* ActorClass, FVector Pos = {}, FRotator Rot = {}, FVector Size = { 1, 1, 1 })
    {
        FTransform translivesmatter = UKismetMathLibrary::MakeTransform(Pos, Rot, Size);
        auto Ret = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), ActorClass, translivesmatter, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, nullptr, ESpawnActorScaleMethod::MultiplyWithRoot);

        if (Ret)
            Ret = UGameplayStatics::FinishSpawningActor(Ret, translivesmatter, ESpawnActorScaleMethod::MultiplyWithRoot);

        return (T*)Ret;
    }

    template <typename T>
    T* SpawnActor(FVector Pos = {}, FRotator Rot = {}, FVector Size = { 1, 1, 1 })
    {
        return SpawnActor<T>(T::StaticClass(), Pos, Rot, Size);
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

    void MarkArrayDirty(FFastArraySerializer& Arr)
    {
        static void (*MarkDirty)(FFastArraySerializer& Arr) = decltype(MarkDirty)(InSDKUtils::GetImageBase() + 0x189EF88);
        MarkDirty(Arr);
    }

    std::string GetName(UObject* Obj)
    {
        return Obj ? Obj->Name.GetRawString() : "None";
    }

    std::string GetFullName(UObject* Obj)
    {
        if (Obj && Obj->Class)
        {
            std::string Temp;

            for (UObject* NextOuter = Obj->Outer; NextOuter; NextOuter = NextOuter->Outer)
            {
                Temp = GetName(NextOuter) + "." + Temp;
            }

            std::string Name = GetName(Obj->Class);
            Name += " ";
            Name += Temp;
            Name += GetName(Obj);

            return Name;
        }

        return "None";
    }

    void DumpObjects()
    {
        std::ofstream outfile("objects.txt");
        std::ofstream outfile2("unrealobjects.txt");
        for (int i = 0; i < UObject::GObjects->Num(); i++)
        {
            auto Obj = UObject::GObjects->GetByIndex(i);
            if (!Obj)
                continue;

            outfile << Obj->GetFullName() << '\n';
            outfile2 << Utils::GetFullName(Obj) << '\n';
        }
        outfile.close();
        outfile2.close();
    }
}

