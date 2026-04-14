#pragma once

#include <cstdint>

struct D2ActiveRoomStrc;
struct D2DrlgRoomStrc;
struct D2DrlgTileDataStrc;
struct D2UnitStrc;

struct D2AutomapCellStrc;

// Backing pool used by AUTOMAP_AllocCell.
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

// D2Client.dll + 0x10F990 (0x6FBAF990)
extern uint32_t*& g_AutomapCellGroupByNo;
// D2Client.dll + 0x111998 (0x6FBB1998)
extern AutomapDataBlock*& g_pAutomapDataPool;
// D2Client.dll + 0x11199C (0x6FBB199C)
extern int& g_nAutomapDataCount;
// D2Client.dll + 0x1119E4 (0x6FBB19E4)
extern int& g_nAutomapUpdateCounter;
// D2Client.dll + 0x111A3C (0x6FBB1A3C)
extern int& g_CurrentUnitClientCoordX;
// D2Client.dll + 0x111A40 (0x6FBB1A40)
extern int& g_CurrentUnitClientCoordY;
// D2Client.dll + 0x111A34 (0x6FBB1A34)
extern int& g_PreviousUnitClientCoordX;
// D2Client.dll + 0x111A38 (0x6FBB1A38)
extern int& g_PreviousUnitClientCoordY;
// D2Client.dll + 0x1119A0 (0x6FBB19A0)
extern D2AutomapLayerStrc*& g_pAutomapLayers;
// D2Client.dll + 0x1119A4 (0x6FBB19A4)
extern D2AutomapLayerStrc*& g_pCurrentAutomapLayer;

// D2Client.dll + 0x2BA40 (0x6FACBA40)
D2AutomapCellStrc* AUTOMAP_AllocCell();
// D2Client.dll + 0x2BAF0 (0x6FACBAF0)
int AUTOMAP_Update();
// D2Client.dll + 0x2CD50 (0x6FACCD50)
int __fastcall AUTOMAP_AVL_Insert(D2AutomapCellStrc* pNewCell, D2AutomapCellStrc** ppTreeRoot);
// D2Client.dll + 0x2D3C0 (0x6FACD3C0)
int __fastcall AUTOMAP_AVL_AddTile(D2DrlgTileDataStrc* pTileData, D2DrlgRoomStrc* pDrlgRoom, D2AutomapCellStrc** ppCellTree);
// D2Client.dll + 0x2D560 (0x6FACD560)
int __fastcall AUTOMAP_AVL_AddObject(D2UnitStrc* pUnit, int nAutomapCellNumber, D2AutomapCellStrc** ppCellTree);
// D2Client.dll + 0x2D180 (0x6FACD180)
void __fastcall AUTOMAP_RevealLayerRoom(D2ActiveRoomStrc* pRoom, int bClipFlag, D2AutomapLayerStrc* pLayer);
// D2Client.dll + 0x2D660 (0x6FACD660)
void __stdcall AUTOMAP_RevealRoom(D2ActiveRoomStrc* pRoom);
void __fastcall AUTOMAP_DrawDiamond(int nX, int nY, uint8_t nColor);
// D2Client.dll + 0x2C610 (0x6FACC610)
void AUTOMAP_Layer_Load();
// D2Client.dll + 0x2BCD0 (0x6FACBCD0)
void AUTOMAP_Layer_Save();
