#pragma once

#include <D2Unicode.h>

struct D2UnitStrc;
struct D2ActiveRoomStrc;
enum D2C_UnitTypes : int;

// D2Client + 0x11C200 -> 0x6FBBC200
extern D2UnitStrc*& g_pCurrentUnit;

// D2Client + 0x11AA00 -> 0x6FBBAA00
// Pointer to the GlobalUnitTables array (set by D2Client_LoadOffsets)
extern D2UnitStrc** g_GlobalUnitTables;

// D2Client.0x6FB283D0 (RVA: 0x883D0)
D2UnitStrc* UNIT_GetCurrentUnit();

// D2Client.0x6FB29370 (RVA: 0x89370)
D2ActiveRoomStrc* UNIT_GetCurrentUnitRoom();

// D2Client + 0x869F0 -> 0x6FB269F0
D2UnitStrc *__fastcall UNIT_GetUnitFromIndex(int dwUnitId, D2C_UnitTypes unitType);

// D2Client + 0x86C90 -> 0x6FB26C90
D2ActiveRoomStrc *__fastcall UNIT_GetRoomAtSubtileCoords(int32_t x, int32_t y);

// D2Client + 0x886F0 -> 6FB286F0
int __fastcall UNIT_TestSelect(D2UnitStrc *pUnit, int a2, int a3, int a4);

// D2Client + 0x88020 -> 6FB28020
int UNIT_AllocateGlobalTables();

// D2Client + 0x897F0 -> 6FB297F0
Unicode *__fastcall UNIT_GetUnitName(D2UnitStrc *pUnit);
