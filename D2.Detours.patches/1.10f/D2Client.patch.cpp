#include <DetoursPatch.h>

#include <cwchar>

#include <GAME/SCmd.h>
#include <UI/automap.h>

// Forward declarations for UNIT functions defined in CUnit.cpp
extern struct D2UnitStrc* UNIT_GetCurrentUnit();
extern struct D2ActiveRoomStrc* UNIT_GetCurrentUnitRoom();
extern struct D2UnitStrc* __fastcall UNIT_GetUnitFromIndex(int dwUnitId, D2C_UnitTypes unitType);

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
    { 0x6FAB50B0 - D2ClientImageBase, &SCMD_ParseGamePacket, PatchAction::FunctionReplaceOriginalByPatch },
    { 0x6FACBA40 - D2ClientImageBase, &AUTOMAP_AllocCell, PatchAction::FunctionReplaceOriginalByPatch },
    { 0x6FACBAF0 - D2ClientImageBase, &AUTOMAP_Update, PatchAction::FunctionReplaceOriginalByPatch },
    { 0x6FACCD50 - D2ClientImageBase, &AUTOMAP_AVL_Insert, PatchAction::FunctionReplaceOriginalByPatch },
    { 0x6FACD180 - D2ClientImageBase, &AUTOMAP_RevealLayerRoom, PatchAction::FunctionReplaceOriginalByPatch },
    { 0x6FACD3C0 - D2ClientImageBase, &AUTOMAP_AVL_AddTile, PatchAction::FunctionReplaceOriginalByPatch },
    { 0x6FACD560 - D2ClientImageBase, &AUTOMAP_AVL_AddObject, PatchAction::FunctionReplaceOriginalByPatch },
    { 0x6FACD660 - D2ClientImageBase, &AUTOMAP_RevealRoom, PatchAction::FunctionReplaceOriginalByPatch },

    { 0x6FB283D0 - D2ClientImageBase, &UNIT_GetCurrentUnit, PatchAction::FunctionReplacePatchByOriginal },
    { 0x6FB29370 - D2ClientImageBase, &UNIT_GetCurrentUnitRoom, PatchAction::FunctionReplacePatchByOriginal },
    { 0x6FB269F0 - D2ClientImageBase, &UNIT_GetUnitFromIndex, PatchAction::FunctionReplaceOriginalByPatch },
    { 0x6FACC610 - D2ClientImageBase, &AUTOMAP_Layer_Load, PatchAction::FunctionReplacePatchByOriginal },
    { 0x6FACBCD0 - D2ClientImageBase, &AUTOMAP_Layer_Save, PatchAction::FunctionReplacePatchByOriginal },
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
