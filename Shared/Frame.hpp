struct FFrame
{
    uint8 pad[16];
    UFunction* Node; // 16
    UObject* Object; // 24
    uint8* Code; // 32
    uint8* Locals; // 40

    FProperty* MostRecentProperty; // 48
    uint8* MostRecentPropertyAddress; // 56
    uint8* MostRecentPropertyContainer; // 64
    uint8 pad2[64]; // 72

    FField* PropertyChainForCompiledIn; // 136

    static inline void (*sep)(void*, void*, void*) = nullptr;
    static inline void (*step)(void* a1, void* a2, void* a3) = nullptr;

    static void Init()
    {
        auto ImageBase = InSDKUtils::GetImageBase();
        sep = decltype(sep)(ImageBase + 0x129AAC0);
        step = decltype(step)(ImageBase + 0xFB2034);
    }

    void Step(void* a1)
    {
        if (Code)
        {
            step(this, Object, a1);
        }
        else
        {
            FProperty* Property = (FProperty*)PropertyChainForCompiledIn;
            PropertyChainForCompiledIn = Property->Next;
            sep(this, a1, Property);
        }
    }

    template <typename T>
    T& StepRef(void* a1)
    {
        MostRecentPropertyAddress = nullptr;
        MostRecentPropertyContainer = nullptr;

        if (Code)
        {
            step(this, Object, a1);
        }
        else
        {
            FProperty* Property = (FProperty*)PropertyChainForCompiledIn;
            PropertyChainForCompiledIn = Property->Next;
            sep(this, a1, Property);
        }

        return (MostRecentPropertyAddress != nullptr) ? *(T*)(MostRecentPropertyAddress) : *(T*)a1;
    }

    void End()
    {
        *(uint64*)(uint64(this) + 32) += *(uint64*)(uint64(this) + 32) != 0i64;
    }
};
static_assert(offsetof(FFrame, Code) == 32, "FFrame::Code wrong offset");
static_assert(offsetof(FFrame, PropertyChainForCompiledIn) == 136, "FFrame::PropertyChainForCompiledIn wrong offset");

#define FRAME_PROP(Type, Name) \
    Type Name; \
    Stack->Step(&Name);

#define FRAME_PROP_REF(Type, Name) \
    Type T_##Name; \
    Type& Name = Stack->StepRef<Type>(&T_##Name);

#define FRAME_END() Stack->End();
