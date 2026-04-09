#include "D2Common/include/Drlg/D2DrlgDrlg.h"
#include "D2Common/include/Units/Units.h"

//D2Client.0x6FAA0000

enum D2C_PanelConfig {
    PANEL_NONE,
    PANEL_RIGHT_OPEN,
    PANEL_LEFT_OPEN,
    PANLE_BOTH_OPEN
};

struct ItemLabel
{
    uint32_t nLeft;
    uint32_t nTop;
    uint32_t nRight;
    uint32_t nBottom;
    D2UnitStrc *pItem;
    Unicode wszText[127];
    D2C_StringColorCodes bgColor;
    int32_t flags;
    D2C_StringColorCodes textColor;
};

struct D2MenuInfoStrc
{
    int32_t  nItemCount;      // +0x00  [0]  nombre d'items dans le menu
    int32_t  nItemHeight;     // +0x04  [1]  hauteur de chaque item (px)
    int32_t  nItemWidth;      // +0x08  [2]  largeur de chaque item (px)
    int32_t  nTorchOffsetY;   // +0x0C  [3]  offset Y pour les torches animées
    int32_t  nSliderOffsetY;  // +0x10  [4]  offset Y pour le slider (type 2)
};


// D2Client + 0xD40EC -> 6FB740EC
extern int g_nScreenWidth;

// D2Client + 0xD40F0 -> 6FB740F0
extern int g_nScreenHeight;

// D2Client + 0xFA980 -> 6FB9A980
extern BOOL g_DisplayPerformanceStats;

// D2Client + 0x1077D4 -> 6FBA77D4
extern _RTL_CRITICAL_SECTION D2Client_CriticalSection_A;

// D2Client + 0x107810 -> 0x6FBA7810
extern char * g_pCurrentCharacterName;

// D2Client + 0x10B9C4 -> 6FBAB9C4
extern D2C_PanelConfig g_nOpenPanels;

// D2Client + 0x10B9C0 -> 6FBAB9C0
extern int16_t g_nPanelCameraOffsetX;

// D2Client + 0x117028 -> 6FBB7028
extern ItemLabel g_ItemLabels[32];

// D2Client + 0x10B9BC -> 6FBAB9BC
extern int g_nCameraOriginX;

// D2Client + 0x10B9B8 -> 6FBAB9B8
extern int g_nCameraOriginY;

// D2Client + 0x11C200 -> D2Client.0x6FBBC200
extern D2UnitStrc * g_pCurrentUnit;

// D2Client + 0x111A3C -> 6FBB1A3C
extern int g_CurrentUnitClientCoordX;

// D2Client + 0x111A40 -> 6FBB1A40
extern int g_CurrentUnitClientCoordY;

// D2Client + 0x111A34 -> 6FBB1A34
extern int g_PreviousUnitClientCoordX;

// D2Client + 0x111A38 -> 6FBB1A38
extern int g_PreviousUnitClientCoordY;

// D2Client + 0x11AA00 -> 6FBBAA00
extern D2UnitStrc *g_D2Client_GlobalUnitTables[128];

// D2Client + 0x10B9E8 -> 6FBAB9E8
extern BOOL g_bHasSelectedUnit;

// D2Client + 0x119428 -> 6FBB9428
extern int sgnNumShowItems;

// D2Client + 0x116FDC -> 6FBB6FDC
extern D2MenuInfoStrc g_D2Client_MenuInfo;

// D2Client + 0xD7BC0 -> 6FB77BC0
extern int g_D2Client_divisor;

// D2Client + 0x1119D8 -> 6FBB19D8
extern POINT g_D2CLIENT_offset;

// D2Client + 0x111990 -> 6FBB1990
extern BOOL p_D2Client_Automap_Is_Mini;

// D2Client + 0x12970 -> D2Client.0x6FAB2970
D2UnitStrc *__fastcall D2Client_sSCmd_UnitWarpXY(UnitWarpXY *arg);

// D2Client + 0x1D330 -> 6FABD330
void __fastcall D2Client_HandleDualWieldAlternateSwing(D2UnitStrc *pUnit, _DWORD *swingState);

// D2Client + 0xA3B0 -> 6FAAA3B0
DWORD __stdcall D2Client_OpenServerThread(LPVOID a1);

// D2Client + 0xB370 -> 6FAAB370
int D2Client_Main();

// D2Client + 0x9AF0 -> 6FAA9AF0
int __stdcall D2Client_InGame_Tick(); // Arguments not sure

// D2Client + 0x9450 -> 0x6FAA9450
int D2Client_AllocateGlobals();

// D2Client + 0x883D0 -> D2Client.0x6FB283D0
D2UnitStrc *D2Client_GetCurrentUnit();

// D2Client + 0x15A20 -> 6FAB5A20
D2UnitStrc *D2Client_GetSelectedItem();

// D2Client + 0xBF80 -> 6FAABF80
int D2Client_GameGetFrame();

// D2Client + 0x15960 -> 6FAB5960
BOOL D2Client_UpdateCameraOriginFromOpenPanels();

// D2Client + 0x89370 -> 0x6FB29370
D2ActiveRoomStrc *D2Client_GetCurrentUnitRoom();

// D2Client + 0x80740 -> 6FB20740
void __fastcall D2Client_GetDisplayItemText(D2UnitStrc *a1, Unicode *szItemName, int nSize);

// D2Client + 0x869F0 -> 0x6FB269F0
D2UnitStrc *__fastcall D2Client_GetUnitFromIndex(int dwUnitId, D2C_UnitTypes unitType);

// D2Client + 0x86C90 -> 0x6FB26C90
D2ActiveRoomStrc *__fastcall D2Client_GetRoomAtSubtileCoords(int32_t x, int32_t y);

// D2Client + 0x886F0 -> 6FB286F0
int __fastcall D2Client_UnitTestSelect(D2UnitStrc *pUnit, int a2, int a3, int a4);

// D2Client + 0xB7BC0 -> 6FB57BC0
int D2Client_GetMouseX();

// D2Client + 0xB7BD0 -> 6FB57BD0
int D2Client_GetMouseY();

// D2Client + 0x88020 -> 6FB28020
int D2Client_AllocateGlobalUnitTables();

// D2Client + 0xC0805 -> 0x6FB60805
int D2Client_sprintf(_BYTE *buffer, const char *format, ...);

// D2Client + 0xC2EBE -> 0x6FB62EBE
int __cdecl D2Client_buf_overflow(unsigned __int8 c, _DWORD *stream_ctx);

// D2Client + 0xC2FD6 -> 0x6FB62FD6
int __cdecl D2Client_vsnprintf(int *stream_ctx, const char *format, va_list args);

// D2Client + 0xC3717 -> 0x6FB63717
_DWORD *__cdecl D2Client_putchar_buf(unsigned __int8 c, _DWORD *stream_ctx, _DWORD *pCount);

// D2Client + 0xC37B5 -> 0x6FB637B5
int __cdecl D2Client_va_arg_int(va_list *ap);

// D2Client + 0xC42A0 -> 0x6FB642A0
char *__cdecl D2Client_strlen(_DWORD *str);

// D2Client + 0xC2E48 -> 0x6FB62E48
int __cdecl D2Client_format_float(int value, int buffer, int fmt_char, int precision, int flags);

// D2Client + 0x28E0 -> 6FAA28E0
void D2Client_DisplayPerformanceStatistics();

// D2Client + 0x150B0 -> 6FAB50B0
void D2Client_ParseGamePacket(uint8_t *pPacketBuffer, uint32_t nPacketSize);

// D2Client + 0x897F0 -> 6FB297F0
Unicode *__thiscall D2Client_GetUnitName(D2UnitStrc *this);

// D2Client + 0x80740 -> 6FB20740
ent_DisplayItemText(D2UnitStrc *a1, wchar_t *szItemName, int nSize);

// D2Client + 0x69F60 -> 6FB09F60
void __usercall D2Client_DrawGroundItemLabels(DrawMode drawMode@<edi>);

// D2Client + 0x2ECF0 -> 6FACECF0
void __fastcall D2Client_AutomapDrawDiamond(int nX, int nY, uint8_t nColor);

// 1.13C :

int D2Client_strcasecmp_locale(unsigned char* s1, unsigned char* s2)
// sub_6FAB39C9
// strcmp case-insensitive avec support locale (MBCS/Unicode)

locale_t* D2Client_GetCurrentThreadLocale()
// sub_6FAB72F7
// Récupère/initialise la locale du thread via FlsGetValue/FlsSetValue

locale_t* D2Client_GetGlobalLocale()
// sub_6FAB7283
// Récupère la locale globale du module (fallback)

int D2Client_CharToLower_Locale(locale_t* pLocale, int codepoint)
// sub_6FAB7604
// toLower avec support MBCS et LCMapString Windows

int D2Client_strcasecmp_ascii(char* s1, char* s2)
// sub_6FAB76F0
// strcmp case-insensitive ASCII pur (A-Z uniquement, pas de locale)

void* D2Client_Alloc(size_t elementSize, size_t count)
// sub_6FAB8797
// Allocateur mémoire custom avec pool aligné 16 bytes + HeapAlloc fallback

void D2Client_FatalError(int errorCode)
// sub_6FAB46DA
// Gestionnaire d'erreur fatale : log + exit 0xFF


// D2Client + 0x707F0
int __userpurge D2Client_ProcessUserMessage@<eax>(Unicode *str@<eax>, char *prompt@<edx>, int index);