#pragma once
struct D2CellFileStrc;
struct Unicode;

// D2Client.dll + 0x10F990 (0x6FBAF990)
extern D2CellFileStrc *& g_sghLoadScreenProgress;

// D2Client.dll + 0xF03A8 (0x6FB903A8)
extern int & g_sgNumStateTbl;

// D2Client.dll + 0x11A778 (0x6FBBA778)
void __fastcall UI_GetDisplayItemText(D2UnitStrc *a1, Unicode *szItemName, int nSize);
