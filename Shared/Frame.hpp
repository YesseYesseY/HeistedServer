struct FFrame
{
    void Step(void* a1)
    {
        static void (*sep)(void*, void*, void*) = nullptr;
        static void (*step)(void* a1, void* a2, void* a3) = nullptr;
        if (!sep)
        {
            auto ImageBase = InSDKUtils::GetImageBase();
            sep = decltype(sep)(ImageBase + 0x129AAC0);
            step = decltype(step)(ImageBase + 0xFB2034);
        }

        auto This = (int64)this;
        if (*(uint64*)(This + 32))
        {
            step(this, *(void**)(This + 24), a1);
        }
        else
        {
            void* v5 = *(void**)(This + 136);
            *(FProperty**)(This + 136) = (FProperty*)(*(FProperty**)(This + 128))->Next;

            sep(this, a1, v5);
        }
    }

    void End()
    {
        *(uint64*)(uint64(this) + 32) += *(uint64*)(uint64(this) + 32) != 0i64;
    }
};

#define FRAME_PROP(Type, Name) \
    Type Name; \
    Stack->Step(&Name);

#define FRAME_END() Stack->End();
