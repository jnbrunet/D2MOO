#include "D2Common/include/Drlg/D2DrlgDrlg.h"
#include "D2Common/include/Units/Units.h"
#include <ENGINE/Cursor.h>
#include <GAME/Game.h>
#include <GAME/SCmd.h>
#include <GAME/Select.h>
#include <Skills/Skills.h>
#include <UNIT/CUnit.h>
#include <UI/automap.h>
#include <UI/showitems.h>
#include <UI/ui.h>

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


// D2Client.dll + 0xD40EC (0x6FB740EC)
extern int g_nScreenWidth;

// D2Client.dll + 0xD40F0 (0x6FB740F0)
extern int g_nScreenHeight;

// D2Client.dll + 0xFA980 (0x6FB9A980)
extern BOOL g_DisplayPerformanceStats;

// D2Client.dll + 0x1077D4 (0x6FBA77D4)
extern _RTL_CRITICAL_SECTION D2Client_CriticalSection_A;

// D2Client.dll + 0x107810 (0x6FBA7810)
extern char * g_pCurrentCharacterName;

// D2Client.dll + 0x10B9C4 (0x6FBAB9C4)
extern D2C_PanelConfig g_nOpenPanels;

// D2Client.dll + 0x10B9C0 (0x6FBAB9C0)
extern int16_t g_nPanelCameraOffsetX;

// D2Client.dll + 0x117028 (0x6FBB7028)
extern ItemLabel g_ItemLabels[32];

// D2Client.dll + 0x10B9BC (0x6FBAB9BC)
extern int g_nCameraOriginX;

// D2Client.dll + 0x10B9B8 (0x6FBAB9B8)
extern int g_nCameraOriginY;

// D2Client.dll + 0x11C200 (0x6FBBC200)
extern D2UnitStrc * g_pCurrentUnit;

// D2Client.dll + 0x111A3C (0x6FBB1A3C)
extern int g_CurrentUnitClientCoordX;

// D2Client.dll + 0x111A40 (0x6FBB1A40)
extern int g_CurrentUnitClientCoordY;

// D2Client.dll + 0x111A34 (0x6FBB1A34)
extern int g_PreviousUnitClientCoordX;

// D2Client.dll + 0x111A38 (0x6FBB1A38)
extern int g_PreviousUnitClientCoordY;

// D2Client.dll + 0x11AA00 (0x6FBBAA00)
extern D2UnitStrc *g_D2Client_GlobalUnitTables[128];

// D2Client.dll + 0x10B9E8 (0x6FBAB9E8)
extern BOOL g_bHasSelectedUnit;

// D2Client.dll + 0x119428 (0x6FBB9428)
extern int sgnNumShowItems;

// D2Client.dll + 0x116FDC (0x6FBB6FDC)
extern D2MenuInfoStrc g_D2Client_MenuInfo;

// D2Client.dll + 0xD7BC0 (0x6FB77BC0)
extern int g_D2Client_divisor;

// D2Client.dll + 0x1119D8 (0x6FBB19D8)
extern POINT g_D2CLIENT_offset;

// D2Client.dll + 0x111990 (0x6FBB1990)
extern BOOL p_D2Client_Automap_Is_Mini;

// D2Client.dll + 0xBF80 (0x6FAABF80)
int GAME_GetFrame();

// D2Client.dll + 0xC0805 (0x6FB60805)
int D2CLIENT_sprintf(_BYTE *buffer, const char *format, ...);

// D2Client.dll + 0xC2EBE (0x6FB62EBE)
int __cdecl D2CLIENT_buf_overflow(unsigned __int8 c, _DWORD *stream_ctx);

// D2Client.dll + 0xC2FD6 (0x6FB62FD6)
int __cdecl D2CLIENT_vsnprintf(int *stream_ctx, const char *format, va_list args);

// D2Client.dll + 0xC3717 (0x6FB63717)
_DWORD *__cdecl D2CLIENT_putchar_buf(unsigned __int8 c, _DWORD *stream_ctx, _DWORD *pCount);

// D2Client.dll + 0xC37B5 (0x6FB637B5)
int __cdecl D2CLIENT_va_arg_int(va_list *ap);

// D2Client.dll + 0xC42A0 (0x6FB642A0)
char *__cdecl D2CLIENT_strlen(_DWORD *str);

// D2Client.dll + 0xC2E48 (0x6FB62E48)
int __cdecl D2CLIENT_format_float(int value, int buffer, int fmt_char, int precision, int flags);

