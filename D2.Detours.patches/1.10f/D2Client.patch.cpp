#include <DetoursPatch.h>

#include <cwchar>

#include <GAME/SCmd.h>
#include <UI/automap.h>

// Defined in CUnit.cpp
extern struct D2UnitStrc* g_pCurrentUnit;
extern struct D2UnitStrc* D2Client_GetCurrentUnit();
extern struct D2ActiveRoomStrc* D2Client_GetCurrentUnitRoom();
// Defined in automap.cpp
extern int D2Client_InitAutomapLayer();
extern void D2Client_AutomapLayer_Load();
extern void D2Client_AutomapLayer_Save();

//#define DISABLE_ALL_PATCHES

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wmicrosoft-cast"
#endif

extern "C" {
constexpr int __cdecl GetBaseOrdinal() { return 10'001; }
constexpr int __cdecl GetLastOrdinal() { return 10'001; }
constexpr int GetOrdinalCount() { return GetLastOrdinal() - GetBaseOrdinal() + 1; }
}

static PatchAction patchActions[GetOrdinalCount()] = {
    PatchAction::Ignore,
};

extern "C" {

__declspec(dllexport)
PatchAction __cdecl GetPatchAction(int ordinal)
{
#ifdef DISABLE_ALL_PATCHES
    return PatchAction::Ignore;
#else
    if (ordinal < GetBaseOrdinal() || ordinal > GetLastOrdinal())
    {
        return PatchAction::FunctionReplacePatchByOriginal;
    }

    static_assert(GetOrdinalCount() == (sizeof(patchActions) / sizeof(*patchActions)), "Make sure we have the right number of ordinal patch entries");
    return patchActions[ordinal - GetBaseOrdinal()];
#endif
}

static const int D2ClientImageBase = 0x6FAA0000;

static ExtraPatchAction extraPatchActions[] = {
#ifdef D2_VERSION_110F
    { 0x6FAB50B0 - D2ClientImageBase, &D2Client_ParseGamePacket, PatchAction::FunctionReplaceOriginalByPatch },
    { 0x6FACBA40 - D2ClientImageBase, &D2Client_AutomapDataPool_Alloc, PatchAction::FunctionReplaceOriginalByPatch },
    { 0x6FACBAF0 - D2ClientImageBase, &D2Client_InitAutomapLayer, PatchAction::FunctionReplaceOriginalByPatch },
    { 0x6FACCD50 - D2ClientImageBase, &D2Client_AutomapAVL_Insert, PatchAction::FunctionReplaceOriginalByPatch },
    { 0x6FACD180 - D2ClientImageBase, &D2Client_AutomapRevealLayerRoom, PatchAction::FunctionReplaceOriginalByPatch },
    { 0x6FACD3C0 - D2ClientImageBase, &D2Client_AutomapAddTileCell, PatchAction::FunctionReplaceOriginalByPatch },
    { 0x6FACD560 - D2ClientImageBase, &D2Client_AutomapAddObjectCell, PatchAction::FunctionReplaceOriginalByPatch },
    { 0x6FACD660 - D2ClientImageBase, &D2Client_AutomapRevealRoom, PatchAction::FunctionReplaceOriginalByPatch },

    { 0x6FBAF990 - D2ClientImageBase, &g_AutomapCellGroupByNo, PatchAction::PointerReplacePatchByOriginal },
    { 0x6FBB1998 - D2ClientImageBase, &g_pAutomapDataPool, PatchAction::PointerReplacePatchByOriginal },
    { 0x6FBB199C - D2ClientImageBase, &g_nAutomapDataCount, PatchAction::PointerReplacePatchByOriginal },
    { 0x6FBB19E4 - D2ClientImageBase, &g_nAutomapUpdateCounter, PatchAction::PointerReplacePatchByOriginal },
    { 0x6FBB1A3C - D2ClientImageBase, &g_CurrentUnitClientCoordX, PatchAction::PointerReplacePatchByOriginal },
    { 0x6FBB1A40 - D2ClientImageBase, &g_CurrentUnitClientCoordY, PatchAction::PointerReplacePatchByOriginal },
    { 0x6FBB1A34 - D2ClientImageBase, &g_PreviousUnitClientCoordX, PatchAction::PointerReplacePatchByOriginal },
    { 0x6FBB1A38 - D2ClientImageBase, &g_PreviousUnitClientCoordY, PatchAction::PointerReplacePatchByOriginal },
    { 0x6FBBC200 - D2ClientImageBase, &g_pCurrentUnit, PatchAction::PointerReplacePatchByOriginal },
    { 0x6FB283D0 - D2ClientImageBase, &D2Client_GetCurrentUnit, PatchAction::FunctionReplacePatchByOriginal },
    { 0x6FB29370 - D2ClientImageBase, &D2Client_GetCurrentUnitRoom, PatchAction::FunctionReplacePatchByOriginal },
    { 0x6FACC610 - D2ClientImageBase, &D2Client_AutomapLayer_Load, PatchAction::FunctionReplacePatchByOriginal },
    { 0x6FACBCD0 - D2ClientImageBase, &D2Client_AutomapLayer_Save, PatchAction::FunctionReplacePatchByOriginal },
#endif
    { 0, 0, PatchAction::Ignore },
};

__declspec(dllexport)
constexpr int __cdecl GetExtraPatchActionsCount()
{
#ifdef DISABLE_ALL_PATCHES
    return 0;
#else
    return sizeof(extraPatchActions) / sizeof(ExtraPatchAction);
#endif
}

__declspec(dllexport)
ExtraPatchAction* __cdecl GetExtraPatchAction(int index)
{
    return &extraPatchActions[index];
}

__declspec(dllexport)
PatchInformationFunctions __cdecl GetPatchInformationFunctions(const wchar_t* dllName)
{
    (void)dllName;
    return { &GetBaseOrdinal, &GetLastOrdinal, &GetPatchAction, &GetExtraPatchActionsCount, &GetExtraPatchAction };
}

__declspec(dllexport)
uint32_t __cdecl DllPreLoadHook(HookContext* ctx, const wchar_t* dllName)
{
    if (_wcsicmp(dllName, L"D2Client.dll") == 0)
    {
        D2Client_SetOriginalModuleBase(ctx->hOriginalModule);
        D2Client_LoadOffsets();
    }

    return 0;
}

}

#include <type_traits>

static_assert(std::is_same<decltype(GetBaseOrdinal)*, GetIntegerFunctionType>::value, "Ensure calling convention doesn't change");
static_assert(std::is_same<decltype(GetLastOrdinal)*, GetIntegerFunctionType>::value, "Ensure calling convention doesn't change");
static_assert(std::is_same<decltype(GetPatchAction)*, GetPatchActionType>::value, "Ensure calling convention doesn't change");

static_assert(std::is_same<decltype(GetExtraPatchActionsCount)*, GetIntegerFunctionType>::value, "Ensure calling convention doesn't change");
static_assert(std::is_same<decltype(GetExtraPatchAction)*, GetExtraPatchActionType>::value, "Ensure calling convention doesn't change");

static_assert(std::is_same<decltype(GetPatchInformationFunctions)*, GetPatchInformationFunctionsType>::value, "Ensure calling convention doesn't change");
static_assert(std::is_same<decltype(DllPreLoadHook)*, DllPreLoadHookType>::value, "Ensure calling convention doesn't change");
