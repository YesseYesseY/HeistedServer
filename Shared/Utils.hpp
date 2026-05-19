namespace Utils
{
    template <typename T = AActor>
    T* SpawnActor(UClass* ActorClass, FTransform translivesmatter)
    {
        auto Ret = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), ActorClass, translivesmatter, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, nullptr, ESpawnActorScaleMethod::MultiplyWithRoot);

        if (Ret)
            Ret = UGameplayStatics::FinishSpawningActor(Ret, translivesmatter, ESpawnActorScaleMethod::MultiplyWithRoot);

        return (T*)Ret;
    }

    template <typename T = AActor>
    T* SpawnActor(UClass* ActorClass, FVector Pos = {}, FRotator Rot = {}, FVector Size = { 1, 1, 1 })
    {
        FTransform translivesmatter = UKismetMathLibrary::MakeTransform(Pos, Rot, Size);
        return SpawnActor<T>(ActorClass, translivesmatter);
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

    int32 GetSerialNumber(FUObjectItem* ObjectItem)
    {
        return *(int32*)(int64(ObjectItem) + 16);
    }

    FUObjectItem* GetUObjectItemByIndex(int32 Index)
    {
        auto& ObjectArr = UObject::GObjects;
        const int32 ChunkIndex = Index / ObjectArr->ElementsPerChunk;
        const int32 InChunkIdx = Index % ObjectArr->ElementsPerChunk;
        
        if (Index < 0 || ChunkIndex >= ObjectArr->NumChunks || Index >= ObjectArr->NumElements)
            return nullptr;
        
        FUObjectItem* ChunkPtr = ObjectArr->GetDecrytedObjPtr()[ChunkIndex];
        if (!ChunkPtr) return nullptr;
        
        return &ChunkPtr[InChunkIdx];
    }

    UObject* GetWeakPtr(FWeakObjectPtr& WeakPtr)
    {
        if (WeakPtr.ObjectSerialNumber == 0 || WeakPtr.ObjectIndex < 0)
            return nullptr;

        auto ObjectItem = GetUObjectItemByIndex(WeakPtr.ObjectIndex);
        if (!ObjectItem || GetSerialNumber(ObjectItem) != WeakPtr.ObjectSerialNumber)
            return nullptr;

        return ObjectItem->Object;
    }

    void SetWeakPtr(FWeakObjectPtr& WeakPtr, UObject* Obj)
    {
        WeakPtr.ObjectIndex = Obj->Index;
        WeakPtr.ObjectSerialNumber = GetSerialNumber(GetUObjectItemByIndex(Obj->Index));
    }

    template <typename T>
    T* GetWeakPtr(TWeakObjectPtr<T>& WeakPtr)
    {
        return (T*)GetWeakPtr(WeakPtr);
    }

    template <typename T>
    T* GetSoftPtr(TSoftObjectPtr<T>& SoftPtr)
    {
        if (auto WeakObj = GetWeakPtr(SoftPtr.WeakPtr))
            return WeakObj;

        if (auto Obj = (T*)UKismetSystemLibrary::LoadAsset_Blocking(SoftPtr))
            return Obj;

        return nullptr;
    }

    UClass* GetSoftPtr(TSoftClassPtr<UClass>& SoftPtr)
    {
        if (auto WeakObj = (UClass*)GetWeakPtr(SoftPtr.WeakPtr))
            return WeakObj;

        if (auto Obj = UKismetSystemLibrary::LoadClassAsset_Blocking(SoftPtr))
            return Obj;

        return nullptr;
    }

    FGameplayTag MakeGameplayTag(const wchar_t* Tag)
    {
        return { UKismetStringLibrary::Conv_StringToName(Tag) };
    }

    template <typename T>
    TArray<T*> GetAllActorsOfClass()
    {
        TArray<AActor*> Ret;
        UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), T::StaticClass(), &Ret);

        return *(TArray<T*>*)&Ret;
    }

    template <typename T>
    T* FindObjectFast(const std::string& Name, EClassCastFlags RequiredType = EClassCastFlags::None)
    {
        for (int i = 0; i < UObject::GObjects->Num(); ++i)
        {
            UObject* Object = UObject::GObjects->GetByIndex(i);
        
            if (!Object || Object->HasTypeFlag(EClassCastFlags::Package))
                continue;
            
            if (Object->HasTypeFlag(RequiredType) && Object->GetName() == Name)
                return (T*)Object;
        }

        return nullptr;
    }
}

