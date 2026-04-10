#include <UI/automap.h>

#include <climits>
#include <cstdlib>

#include <D2CMP.h>
#include <D2DataTbls.h>
#include <D2Dungeon.h>
#include <Drlg/D2DrlgDrlg.h>
#include <DataTbls/LevelsIds.h>
#include <DataTbls/LevelsTbls.h>
#include <DataTbls/ObjectsIds.h>
#include <DataTbls/ObjectsTbls.h>
#include <Units/Units.h>
#include <UNIT/CUnit.h>
#include <Fog.h>

namespace
{
constexpr int kAutomapCellsPerBlock = 512;

int CompareAutomapCells(const D2AutomapCellStrc* pLeft, const D2AutomapCellStrc* pRight)
{
    int nCmp = static_cast<int>(pLeft->nPixelY) - static_cast<int>(pRight->nPixelY);
    if (nCmp != 0)
    {
        return nCmp;
    }

    nCmp = static_cast<int>(pLeft->nPixelX) - static_cast<int>(pRight->nPixelX);
    if (nCmp != 0)
    {
        return nCmp;
    }

    if (g_AutomapCellGroupByNo)
    {
        const int nLeftGroup = static_cast<int>(g_AutomapCellGroupByNo[pLeft->nCellNo]);
        const int nRightGroup = static_cast<int>(g_AutomapCellGroupByNo[pRight->nCellNo]);
        if (nLeftGroup != -1 && nRightGroup != -1 && nLeftGroup != nRightGroup)
        {
            return static_cast<int>(pLeft->nCellNo) - static_cast<int>(pRight->nCellNo);
        }
        return 0;
    }

    return static_cast<int>(pLeft->nCellNo) - static_cast<int>(pRight->nCellNo);
}

int DivideBy10(const int nValue)
{
    return nValue / 10;
}

bool ShouldRevealTile(const D2DrlgTileDataStrc* pTileData, const int bClipFlag)
{
    if (pTileData->dwFlags & MAPTILE_HIDDEN)
    {
        return false;
    }

    if (pTileData->dwFlags & MAPTILE_EXPLORED)
    {
        return true;
    }

    return bClipFlag != 0;
}

bool ShouldRevealUnit(const D2UnitStrc* pUnit)
{
    if (!(pUnit->dwFlags & UNITFLAG_AUTOMAP_VISIBLE))
    {
        return false;
    }

    return (pUnit->dwFlags & UNITFLAG_AUTOMAP_REVEALED) == 0;
}

int GetObjectAutomapCellNumber(D2UnitStrc* pUnit, D2ActiveRoomStrc* pRoom)
{
    D2ObjectsTxt* pObjectsTxtRecord = DATATBLS_GetObjectsTxtRecord(pUnit->dwClassId);
    if (!pObjectsTxtRecord || !pObjectsTxtRecord->dwAutomap)
    {
        return 0;
    }

    switch (pUnit->dwClassId)
    {
    case OBJECT_VALLEY_WAYPOINT:
        if (DUNGEON_GetLevelIdFromRoom(pRoom) != LEVEL_TALRASHASTOMB4)
        {
            return 0;
        }
        break;

    case OBJECT_SEWER_STAIRS:
        if (pUnit->dwAnimMode != 2)
        {
            return 0;
        }
        break;

    case OBJECT_STASH:
    {
        const uint8_t nAct = DRLG_GetActNoFromLevelId(DUNGEON_GetLevelIdFromRoom(pRoom));
        if (nAct != ACT_II && nAct != ACT_III)
        {
            return 0;
        }
        break;
    }

    default:
        break;
    }

    return static_cast<int>(pObjectsTxtRecord->dwAutomap);
}

int GetMonsterAutomapCellNumber(D2UnitStrc* pUnit)
{
    if (pUnit->dwClassId < 0)
    {
        return 0;
    }

    D2MonStatsTxt* pMonStatsTxtRecord = DATATBLS_GetMonStatsTxtRecord(pUnit->dwClassId);
    if (!pMonStatsTxtRecord)
    {
        return 0;
    }

    const int nMonStats2RecordId = pMonStatsTxtRecord->wMonStatsEx;
    if (nMonStats2RecordId < 0 || nMonStats2RecordId >= sgptDataTables->nMonStats2TxtRecordCount)
    {
        return 0;
    }

    D2MonStats2Txt* pMonStats2TxtRecord = &sgptDataTables->pMonStats2Txt[nMonStats2RecordId];
    if (!pMonStats2TxtRecord)
    {
        return 0;
    }

    return static_cast<int>(pMonStats2TxtRecord->dwAutomapCel);
}

int GetUnitAutomapCellNumber(D2UnitStrc* pUnit, D2ActiveRoomStrc* pRoom)
{
    switch (pUnit->dwUnitType)
    {
    case UNIT_MONSTER:
        return GetMonsterAutomapCellNumber(pUnit);

    case UNIT_OBJECT:
        return GetObjectAutomapCellNumber(pUnit, pRoom);

    default:
        return 0;
    }
}

}
uint32_t* g_AutomapCellGroupByNo;
AutomapDataBlock* g_pAutomapDataPool;
int g_nAutomapDataCount;
// D2Client + 0x1119E4 -> 0x6FBB19E4
int g_nAutomapUpdateCounter;
// D2Client + 0x111A3C -> 0x6FBB1A3C
int g_CurrentUnitClientCoordX;
// D2Client + 0x111A40 -> 0x6FBB1A40
int g_CurrentUnitClientCoordY;
// D2Client + 0x111A34 -> 0x6FBB1A34
int g_PreviousUnitClientCoordX;
// D2Client + 0x111A38 -> 0x6FBB1A38
int g_PreviousUnitClientCoordY;
#if defined(D2_VERSION_110F)
// Direct aliases to original D2Client globals (1.10f):
//   0x6FBB19A0 -> g_pAutomapLayers
//   0x6FBB19A4 -> g_pCurrentAutomapLayer
// This avoids one-time value copies and keeps reads/writes synchronized with game memory.
D2AutomapLayerStrc*& g_pAutomapLayers = *reinterpret_cast<D2AutomapLayerStrc**>(0x6FBB19A0);
D2AutomapLayerStrc*& g_pCurrentAutomapLayer = *reinterpret_cast<D2AutomapLayerStrc**>(0x6FBB19A4);
#else
namespace
{
D2AutomapLayerStrc* g_pAutomapLayersStorage = nullptr;
D2AutomapLayerStrc* g_pCurrentAutomapLayerStorage = nullptr;
}

D2AutomapLayerStrc*& g_pAutomapLayers = g_pAutomapLayersStorage;
D2AutomapLayerStrc*& g_pCurrentAutomapLayer = g_pCurrentAutomapLayerStorage;
#endif

// D2Client.0x6FACBA40 (RVA: 0x2BA40)
D2AutomapCellStrc* D2Client_AutomapDataPool_Alloc()
{
    const int nBlockIndex = g_nAutomapDataCount / kAutomapCellsPerBlock;
    const int nSlotIndex = g_nAutomapDataCount % kAutomapCellsPerBlock;
    ++g_nAutomapDataCount;

    AutomapDataBlock* pPrevious = nullptr;
    AutomapDataBlock* pCurrent = g_pAutomapDataPool;
    for (int i = 0; i < nBlockIndex; ++i)
    {
        pPrevious = pCurrent;
        pCurrent = pCurrent ? pCurrent->pNextBlock : nullptr;
    }

    if (!pCurrent)
    {
        pCurrent = static_cast<AutomapDataBlock*>(D2_CALLOC(sizeof(AutomapDataBlock)));
        pCurrent->pEntries = static_cast<D2AutomapCellStrc*>(D2_CALLOC(sizeof(D2AutomapCellStrc) * kAutomapCellsPerBlock));
        if (pPrevious)
        {
            pPrevious->pNextBlock = pCurrent;
        }
        else
        {
            g_pAutomapDataPool = pCurrent;
        }
    }

    D2AutomapCellStrc* pCell = &pCurrent->pEntries[nSlotIndex];
    pCell->pCell = nullptr;
    pCell->nCellNo = 0;
    pCell->nPixelX = 0;
    pCell->nPixelY = 0;
    pCell->nBalance = 0;
    pCell->pLess = nullptr;
    pCell->pMore = nullptr;
    return pCell;
}

// D2Client.0x6FACBAF0 (RVA: 0x2BAF0)
// Callee: D2Client_AutomapRevealLayerRoom
// Called from the main game-tick update path whenever the player's visible room set changes.
//
// Responsibilities:
//   1. Read the player unit + current room from client globals.
//   2. Navigate to the D2DrlgLevelStrc to check DRLGLEVELFLAG_AUTOMAP_REVEAL (full-reveal mode).
//   3. Find or create a D2AutomapLayerStrc for the current level in g_pAutomapLayers.
//   4. Normal mode  : reveal only rooms adjacent to the player (DUNGEON_GetAdjacentRoomsListFromRoom).
//      Full-reveal  : iterate the entire D2DrlgLevelStrc::pFirstRoomEx chain and reveal all
//                     rooms that currently have an active D2ActiveRoomStrc attached.
//      In both modes, DRLGROOMFLAG_AUTOMAP_REVEAL is stamped on each revealed room so it is
//      not processed again on subsequent ticks.
//   5. Invoke pDrlg->pfAutomap(pRoom) if non-null — this callback (registered by D2Client at
//      act init time) adds warp-exit icon cells (the triangular arrows) to the automap tree.
//
// Globals written:
//   g_pCurrentAutomapLayer — updated to the layer for the player's current level so the
//   drawing code always targets the right AVL trees.
//
int D2Client_InitAutomapLayer()
{
    if (g_nAutomapUpdateCounter)
    {
        return --g_nAutomapUpdateCounter;
    }

    D2UnitStrc* pUnit = D2Client_GetCurrentUnit();
    if (!pUnit)
    {
        FOG_DisplayAssert("hUnit", __FILE__, __LINE__);
        std::exit(-1);
    }

    g_CurrentUnitClientCoordX = UNITS_GetClientCoordX(pUnit);
    g_CurrentUnitClientCoordY = UNITS_GetClientCoordY(pUnit);

    int nDx = g_PreviousUnitClientCoordX - g_CurrentUnitClientCoordX;
    if (nDx < 0)
    {
        nDx = -nDx;
    }

    int nDy = g_PreviousUnitClientCoordY - g_CurrentUnitClientCoordY;
    if (nDy < 0)
    {
        nDy = -nDy;
    }

    const int nDistance = (nDx <= nDy) ? ((nDx + 2 * nDy) / 2) : ((nDy + 2 * nDx) / 2);
    int nResult = nDistance;
    if (nDistance < 80)
    {
        return nResult;
    }

    g_PreviousUnitClientCoordX = g_CurrentUnitClientCoordX;
    g_PreviousUnitClientCoordY = g_CurrentUnitClientCoordY;

    D2ActiveRoomStrc* pRoom = D2Client_GetCurrentUnitRoom();
    if (!pRoom)
    {
        return nResult;
    }

    D2ActiveRoomStrc** ppRoomList = nullptr;
    int nAdjacentRooms = 0;
    DUNGEON_GetAdjacentRoomsListFromRoom(pRoom, &ppRoomList, &nAdjacentRooms);

    const int nLevelId = DUNGEON_GetLevelIdFromRoom(pRoom);
    D2LevelDefBin* pLevelDef = DATATBLS_GetLevelDefRecord(nLevelId);
    if (!pLevelDef)
    {
        return nResult;
    }

    const uint32_t dwLayer = pLevelDef->dwLayer;
    D2AutomapLayerStrc* pLayer = g_pAutomapLayers;
    while (pLayer && pLayer->nLayerNo != dwLayer)
    {
        pLayer = pLayer->pNext;
    }

    if (!pLayer)
    {
        pLayer = static_cast<D2AutomapLayerStrc*>(D2_CALLOC(sizeof(D2AutomapLayerStrc)));
        pLayer->nLayerNo = dwLayer;
        pLayer->pNext = g_pAutomapLayers;
        g_pAutomapLayers = pLayer;
    }

    if (pLayer != g_pCurrentAutomapLayer)
    {
        D2Client_AutomapLayer_Save();

        if (g_pCurrentAutomapLayer)
        {
            AutomapDataBlock* pBlock = g_pAutomapDataPool;
            while (pBlock)
            {
                AutomapDataBlock* pNext = pBlock->pNextBlock;
                D2_FREE(pBlock);
                pBlock = pNext;
            }

            g_pAutomapDataPool = nullptr;
            g_nAutomapDataCount = 0;
            g_pCurrentAutomapLayer->pFloors = nullptr;
            g_pCurrentAutomapLayer->pWalls = nullptr;
            g_pCurrentAutomapLayer->pObjects = nullptr;
            g_pCurrentAutomapLayer->pExtras = nullptr;
        }

        g_pCurrentAutomapLayer = pLayer;
        D2Client_AutomapLayer_Load();
    }

    nResult = nAdjacentRooms;
    for (int i = 0; i < nAdjacentRooms; ++i)
    {
        D2ActiveRoomStrc* pAdjacentRoom = ppRoomList[i];
        const int nAdjacentLevelId = DUNGEON_GetLevelIdFromRoom(pAdjacentRoom);
        D2LevelDefBin* pAdjacentLevelDef = DATATBLS_GetLevelDefRecord(nAdjacentLevelId);
        if (pAdjacentLevelDef && pAdjacentLevelDef->dwLayer == dwLayer)
        {
            D2Client_AutomapRevealLayerRoom(pAdjacentRoom, 0, pLayer);
        }
    }

    return nResult;
}

// D2Client.0x6FACCD50 (RVA: 0x2CD50)
int __fastcall D2Client_AutomapAVL_Insert(D2AutomapCellStrc* pNewCell, D2AutomapCellStrc** ppTreeRoot)
{
    D2AutomapCellStrc** ppLink = ppTreeRoot;
    while (*ppLink)
    {
        D2AutomapCellStrc* pCurrent = *ppLink;
        const int nCmp = CompareAutomapCells(pNewCell, pCurrent);
        if (nCmp == 0)
        {
            return 0;
        }
        ppLink = (nCmp < 0) ? &pCurrent->pLess : &pCurrent->pMore;
    }

    *ppLink = pNewCell;
    return 1;
}

// D2Client.0x6FACD3C0 (RVA: 0x2D3C0)
int __fastcall D2Client_AutomapAddTileCell(D2DrlgTileDataStrc* pTileData, D2DrlgRoomStrc* pDrlgRoom, D2AutomapCellStrc** ppCellTree)
{
    if (!pTileData || !pDrlgRoom || !pDrlgRoom->pLevel)
    {
        return 0;
    }

    if (pTileData->dwFlags & MAPTILE_PROCESSED)
    {
        return 0;
    }

    pTileData->dwFlags |= MAPTILE_PROCESSED;

    const int nAutomapLevelType = DRLG_GetLevelTypeFromLevelId(pDrlgRoom->pLevel->nLevelId);
    const int nAutomapTileType = D2CMP_10077_GetTileType(pTileData->pTile);
    const int nTileStyle = D2CMP_10078_GetTileStyle(pTileData->pTile);
    const int nTileSequence = D2CMP_10082_GetTileSequence(pTileData->pTile);
    const int nAutomapCellNumber = DATATBLS_GetAutomapCellId(nAutomapLevelType, nAutomapTileType, nTileStyle, nTileSequence);
    if (nAutomapCellNumber == -1)
    {
        return 0;
    }

    D2AutomapCellStrc* pNewCell = D2Client_AutomapDataPool_Alloc();

    int nTileX = pTileData->nPosX + pDrlgRoom->nTileXPos;
    int nTileY = pTileData->nPosY + pDrlgRoom->nTileYPos;
    DUNGEON_GameTileToClientCoords(&nTileX, &nTileY);

    const int nPixelX = DivideBy10(nTileX);
    int nPixelY = DivideBy10(nTileY);
    if (pTileData->nTileType >= TILETYPE_LEFT_WALL_DOWN)
    {
        nPixelY += 24;
    }

    D2_ASSERTM(nPixelX >= SHRT_MIN && nPixelX <= SHRT_MAX, "nPixelX >= SHRT_MIN && nPixelX <= SHRT_MAX");
    D2_ASSERTM(nPixelY >= SHRT_MIN && nPixelY <= SHRT_MAX, "nPixelY >= SHRT_MIN && nPixelY <= SHRT_MAX");
    D2_ASSERTM(nAutomapCellNumber >= SHRT_MIN && nAutomapCellNumber <= SHRT_MAX, "nCelNum >= SHRT_MIN && nCelNum <= SHRT_MAX");

    pNewCell->nCellNo = static_cast<uint16_t>(nAutomapCellNumber);
    pNewCell->nPixelX = static_cast<int16_t>(nPixelX);
    pNewCell->nPixelY = static_cast<int16_t>(nPixelY);
    return D2Client_AutomapAVL_Insert(pNewCell, ppCellTree);
}

// D2Client.0x6FACD560 (RVA: 0x2D560)
int __fastcall D2Client_AutomapAddObjectCell(D2UnitStrc* pUnit, int nAutomapCellNumber, D2AutomapCellStrc** ppCellTree)
{
    D2AutomapCellStrc* pNewCell = D2Client_AutomapDataPool_Alloc();

    const int nPixelX = DivideBy10(UNITS_GetClientCoordX(pUnit)) + 1;
    const int nPixelY = DivideBy10(UNITS_GetClientCoordY(pUnit)) - 3;

    D2_ASSERTM(nPixelX >= SHRT_MIN && nPixelX <= SHRT_MAX, "nPixelX >= SHRT_MIN && nPixelX <= SHRT_MAX");
    D2_ASSERTM(nPixelY >= SHRT_MIN && nPixelY <= SHRT_MAX, "nPixelY >= SHRT_MIN && nPixelY <= SHRT_MAX");
    D2_ASSERTM(nAutomapCellNumber >= SHRT_MIN && nAutomapCellNumber <= SHRT_MAX, "nCelNum >= SHRT_MIN && nCelNum <= SHRT_MAX");

    pNewCell->nCellNo = static_cast<uint16_t>(nAutomapCellNumber);
    pNewCell->nPixelX = static_cast<int16_t>(nPixelX);
    pNewCell->nPixelY = static_cast<int16_t>(nPixelY);
    return D2Client_AutomapAVL_Insert(pNewCell, ppCellTree);
}

// D2Client.0x6FACD660 (RVA: 0x2D660)
void __stdcall D2Client_AutomapRevealRoom(D2ActiveRoomStrc* pRoom)
{
    if (!pRoom)
    {
        return;
    }

    const uint32_t nPreviousLayerNo = g_pCurrentAutomapLayer ? g_pCurrentAutomapLayer->nLayerNo : static_cast<uint32_t>(-1);

    const int nLevelId = DUNGEON_GetLevelIdFromRoom(pRoom);
    D2LevelDefBin* pLevelDef = DATATBLS_GetLevelDefRecord(nLevelId);
    if (!pLevelDef)
    {
        return;
    }

    const uint32_t dwLayer = pLevelDef->dwLayer;
    D2AutomapLayerStrc* pLayer = g_pAutomapLayers;
    while (pLayer && pLayer->nLayerNo != dwLayer)
    {
        pLayer = pLayer->pNext;
    }

    if (!pLayer)
    {
        pLayer = static_cast<D2AutomapLayerStrc*>(D2_CALLOC(sizeof(D2AutomapLayerStrc)));
        pLayer->nLayerNo = dwLayer;
        pLayer->pNext = g_pAutomapLayers;
        g_pAutomapLayers = pLayer;
    }

    if (pLayer != g_pCurrentAutomapLayer)
    {
        D2Client_AutomapLayer_Save();

        if (g_pCurrentAutomapLayer)
        {
            AutomapDataBlock* pBlock = g_pAutomapDataPool;
            while (pBlock)
            {
                AutomapDataBlock* pNext = pBlock->pNextBlock;
                D2_FREE(pBlock);
                pBlock = pNext;
            }

            g_pAutomapDataPool = nullptr;
            g_nAutomapDataCount = 0;
            g_pCurrentAutomapLayer->pFloors = nullptr;
            g_pCurrentAutomapLayer->pWalls = nullptr;
            g_pCurrentAutomapLayer->pObjects = nullptr;
            g_pCurrentAutomapLayer->pExtras = nullptr;
        }

        g_pCurrentAutomapLayer = pLayer;
        D2Client_AutomapLayer_Load();
    }

    D2Client_AutomapRevealLayerRoom(pRoom, 1, pLayer);

    if (nPreviousLayerNo != static_cast<uint32_t>(-1))
    {
        D2AutomapLayerStrc* pPreviousLayer = g_pAutomapLayers;
        while (pPreviousLayer && pPreviousLayer->nLayerNo != nPreviousLayerNo)
        {
            pPreviousLayer = pPreviousLayer->pNext;
        }

        if (!pPreviousLayer)
        {
            pPreviousLayer = static_cast<D2AutomapLayerStrc*>(D2_CALLOC(sizeof(D2AutomapLayerStrc)));
            pPreviousLayer->nLayerNo = nPreviousLayerNo;
            pPreviousLayer->pNext = g_pAutomapLayers;
            g_pAutomapLayers = pPreviousLayer;
        }

        if (pPreviousLayer != g_pCurrentAutomapLayer)
        {
            D2Client_AutomapLayer_Save();

            if (g_pCurrentAutomapLayer)
            {
                AutomapDataBlock* pBlock = g_pAutomapDataPool;
                while (pBlock)
                {
                    AutomapDataBlock* pNext = pBlock->pNextBlock;
                    D2_FREE(pBlock);
                    pBlock = pNext;
                }

                g_pAutomapDataPool = nullptr;
                g_nAutomapDataCount = 0;
                g_pCurrentAutomapLayer->pFloors = nullptr;
                g_pCurrentAutomapLayer->pWalls = nullptr;
                g_pCurrentAutomapLayer->pObjects = nullptr;
                g_pCurrentAutomapLayer->pExtras = nullptr;
            }

            g_pCurrentAutomapLayer = pPreviousLayer;
            D2Client_AutomapLayer_Load();
        }
    }
}

// D2Client.0x6FACD180 (RVA: 0x2D180)
void __fastcall D2Client_AutomapRevealLayerRoom(D2ActiveRoomStrc* pRoom, int bClipFlag, D2AutomapLayerStrc* pLayer)
{
    if (!pRoom || !pLayer)
    {
        return;
    }

    D2DrlgRoomStrc* pDrlgRoom = DUNGEON_GetRoomExFromRoom(pRoom);
    if (!pDrlgRoom)
    {
        return;
    }

    int nFloorTiles = 0;
    D2DrlgTileDataStrc* pFloorTiles = DUNGEON_GetFloorTilesFromRoom(pRoom, &nFloorTiles);
    for (int i = 0; i < nFloorTiles; ++i)
    {
        D2DrlgTileDataStrc* pTileData = &pFloorTiles[i];
        if (ShouldRevealTile(pTileData, bClipFlag))
        {
            D2Client_AutomapAddTileCell(pTileData, pDrlgRoom, &pLayer->pFloors);
        }
    }

    int nWallTiles = 0;
    D2DrlgTileDataStrc* pWallTiles = DUNGEON_GetWallTilesFromRoom(pRoom, &nWallTiles);
    for (int i = 0; i < nWallTiles; ++i)
    {
        D2DrlgTileDataStrc* pTileData = &pWallTiles[i];
        if (ShouldRevealTile(pTileData, bClipFlag))
        {
            D2Client_AutomapAddTileCell(pTileData, pDrlgRoom, &pLayer->pWalls);
        }
    }

    for (D2UnitStrc* pUnit = pRoom->pUnitFirst; pUnit; pUnit = pUnit->pRoomNext)
    {
        if (!ShouldRevealUnit(pUnit))
        {
            continue;
        }

        pUnit->dwFlags |= UNITFLAG_AUTOMAP_REVEALED;

        const int nAutomapCellNumber = GetUnitAutomapCellNumber(pUnit, pRoom);
        if (nAutomapCellNumber)
        {
            D2Client_AutomapAddObjectCell(pUnit, nAutomapCellNumber, &pLayer->pObjects);
        }
    }
}

// D2Client.0x6FACC610 (RVA: 0x2C610)
// Loads previously saved automap cell data for a layer from the .d2s save game.
// Called in D2Client_InitAutomapLayer right after the layer is found/created.
void D2Client_AutomapLayer_Load()
{
}

// D2Client.0x6FACBCD0 (RVA: 0x2BCD0)
// Persists a layer's cell data back to the .d2s save game.
// Called in D2Client_InitAutomapLayer after all rooms for this tick have been revealed.
void D2Client_AutomapLayer_Save()
{
}