namespace Player
{
    void ServerAcknowledgePossession(AFortPlayerControllerAthena* PlayerController, APawn* Pawn)
    {
        PlayerController->AcknowledgedPawn = Pawn;
    }

    void ServerCheat(AFortPlayerControllerAthena* Controller, FString& FMsg)
    {
        auto Msg = FMsg.ToWString();

        if (Msg == L"test")
        {
            MsgBox("{}", UKismetSystemLibrary::IsDedicatedServer(UWorld::GetWorld()));
            // UWorld::GetWorld()->NetDriver = nullptr;
            // MsgBox("{}", UKismetSystemLibrary::IsDedicatedServer(UWorld::GetWorld()));
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
