#pragma once

#include <cstdint>

struct D2ActiveRoomStrc;
struct D2DrlgRoomStrc;
struct D2DrlgTileDataStrc;
struct D2UnitStrc;

struct D2AutomapCellStrc;

// Backing pool used by D2Client_AutomapDataPool_Alloc.
struct AutomapDataBlock
{
    D2AutomapCellStrc* pEntries;
    AutomapDataBlock* pNextBlock;
};

// Matches D2Client 1.10f layout used by the automap AVL routines.
struct D2AutomapCellStrc
{
    D2AutomapCellStrc* pCell;
    uint16_t nCellNo;
    int16_t nPixelX;
    int16_t nPixelY;
    int16_t nBalance;
    D2AutomapCellStrc* pLess;
    D2AutomapCellStrc* pMore;
};

struct D2AutomapLayerStrc
{
    uint32_t nLayerNo;
    uint32_t fSaved;
    D2AutomapCellStrc* pFloors;
    D2AutomapCellStrc* pWalls;
    D2AutomapCellStrc* pObjects;
    D2AutomapCellStrc* pExtras;
    D2AutomapLayerStrc* pNext;
};

extern uint32_t* g_AutomapCellGroupByNo;
extern AutomapDataBlock* g_pAutomapDataPool;
extern int g_nAutomapDataCount;
// D2Client + 0x1119E4 -> 0x6FBB19E4
extern int g_nAutomapUpdateCounter;
// D2Client + 0x111A3C -> 0x6FBB1A3C
extern int g_CurrentUnitClientCoordX;
// D2Client + 0x111A40 -> 0x6FBB1A40
extern int g_CurrentUnitClientCoordY;
// D2Client + 0x111A34 -> 0x6FBB1A34
extern int g_PreviousUnitClientCoordX;
// D2Client + 0x111A38 -> 0x6FBB1A38
extern int g_PreviousUnitClientCoordY;
// D2Client + 0x1119A0 -> 0x6FBB19A0
extern D2AutomapLayerStrc*& g_pAutomapLayers;
// D2Client + 0x1119A4 -> 0x6FBB19A4
extern D2AutomapLayerStrc*& g_pCurrentAutomapLayer;

// D2Client.0x6FACBA40 (RVA: 0x2BA40)
D2AutomapCellStrc* D2Client_AutomapDataPool_Alloc();
// D2Client.0x6FACBAF0 (RVA: 0x2BAF0)
int D2Client_AutomapUpdate();
// D2Client.0x6FACCD50 (RVA: 0x2CD50)
int __fastcall D2Client_AutomapAVL_Insert(D2AutomapCellStrc* pNewCell, D2AutomapCellStrc** ppTreeRoot);
// D2Client.0x6FACD3C0 (RVA: 0x2D3C0)
int __fastcall D2Client_AutomapAddTileCell(D2DrlgTileDataStrc* pTileData, D2DrlgRoomStrc* pDrlgRoom, D2AutomapCellStrc** ppCellTree);
// D2Client.0x6FACD560 (RVA: 0x2D560)
int __fastcall D2Client_AutomapAddObjectCell(D2UnitStrc* pUnit, int nAutomapCellNumber, D2AutomapCellStrc** ppCellTree);
// D2Client.0x6FACD180 (RVA: 0x2D180)
void __fastcall D2Client_AutomapRevealLayerRoom(D2ActiveRoomStrc* pRoom, int bClipFlag, D2AutomapLayerStrc* pLayer);
void __fastcall D2Client_AutomapDrawDiamond(int nX, int nY, uint8_t nColor);
// D2Client.0x6FACC610 (RVA: 0x2C610)
void D2Client_AutomapLayer_Load();
// D2Client.0x6FACBCD0 (RVA: 0x2BCD0)
void D2Client_AutomapLayer_Save();
