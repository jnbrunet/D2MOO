#include <GAME/SCmd.h>

#include <Windows.h>


static HMODULE g_D2ClientOriginalModuleBase = nullptr;

HMODULE delayedD2ClientDllBaseGet()
{
    if (g_D2ClientOriginalModuleBase)
    {
        return g_D2ClientOriginalModuleBase;
    }

    if (HMODULE hModule = GetModuleHandleW(L"D2Client.dll"))
    {
        g_D2ClientOriginalModuleBase = hModule;
        return g_D2ClientOriginalModuleBase;
    }

    // Never force-load D2Client here: loading a second module instance would produce
    // a different .data/.rdata view and invalid runtime pointers.
    return nullptr;
}

void D2Client_SetOriginalModuleBase(void* hOriginalModule)
{
    g_D2ClientOriginalModuleBase = reinterpret_cast<HMODULE>(hOriginalModule);
}

void D2Client_LoadOffsets()
{
    D2Client_SCmd_LoadOffsets();
}
