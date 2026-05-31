#include <Minhook.h>

namespace Hook
{
    template <typename T = void*>
    void Function(uintptr_t Addr, void* Hook, T* Original = nullptr)
    {
        MH_CreateHook((LPVOID)Addr, Hook, (LPVOID*)Original);
        MH_EnableHook((LPVOID)Addr);
    }

    template <typename T = void*>
    void VTable(void** vTable, int32 Index, void* Hook, T* Original = nullptr)
    {
        auto Addr = (LPVOID)(int64(vTable) + (Index * 8));
        if (Original)
            *Original = (T)vTable[Index];
        DWORD yes;
        VirtualProtect(Addr, 8, PAGE_EXECUTE_READWRITE, &yes);
        vTable[Index] = Hook;
        VirtualProtect(Addr, 8, yes, &yes);
    }

    template <typename T, typename T2 = void*>
    void VTable(int32 Index, void* Hook, T2* Original = nullptr)
    {
        VTable((void**)T::GetDefaultObj()->VTable, Index, Hook, Original);
    }

    template <typename T, typename T2 = void*>
    void AllVTables(int32 Index, void* Hook, T2* Original = nullptr)
    {
        for (int i = 0; i < UObject::GObjects->Num(); i++)
        {
            auto Object = UObject::GObjects->GetByIndex(i);
            if (!Object || !Object->HasTypeFlag(EClassCastFlags::Class)) continue;

            if (((UClass*)Object)->IsSubclassOf(T::StaticClass()))
            {
                Hook::VTable((void**)((UClass*)Object)->ClassDefaultObject->VTable, Index, Hook, Original);
            }
        }
    }

    template <typename T = void*>
    void UFunc(UFunction* Func, void* Hook, T* Original = nullptr)
    {
        if (Original)
            *Original = *(T*)(int64(Func) + 0xD8);

        *(void**)(int64(Func) + 0xD8) = Hook;
    }

    template <typename T = void*>
    void UFunc(const std::string& FuncName, void* Hook, T* Original = nullptr)
    {
        auto Func = UObject::FindObject<UFunction>(FuncName, EClassCastFlags::Function);
        if (Func)
            UFunc(Func, Hook, Original);
    }

    bool ReturnTrueHook()
    {
        return true;
    }

    bool ReturnFalseHook()
    {
        return false;
    }

    void ReturnHook()
    {
        return;
    }
}
