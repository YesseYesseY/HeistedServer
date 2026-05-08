namespace Player
{
    void ServerAcknowledgePossession(AFortPlayerControllerAthena* PlayerController, APawn* Pawn)
    {
        PlayerController->AcknowledgedPawn = Pawn;
    }

    void ServerCheat(AFortPlayerControllerAthena* Controller, FString& FMsg)
    {
        auto Msg = FMsg.ToWString();

        if (Msg.starts_with(L"server "))
        {
            Utils::ExecuteConsoleCommand(Msg.substr(7).c_str());
        }
        else if (Msg == L"test")
        {
            MsgBox("{} {}", UKismetSystemLibrary::IsServer(UWorld::GetWorld()), UKismetSystemLibrary::IsDedicatedServer(UWorld::GetWorld()));
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
