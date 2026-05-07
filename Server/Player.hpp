namespace Player
{
    void ServerAcknowledgePossession(APlayerController* Controller, APawn* Pawn)
    {
        Controller->AcknowledgedPawn = Pawn;
    }

    void Init()
    {
        Hook::VTable<AFortPlayerControllerAthena>(2440 / 8, ServerAcknowledgePossession);
    }
}
