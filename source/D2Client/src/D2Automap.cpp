#include "D2Automap.h"
#include "Fog/include/Fog.h"

D2AutomapCellStrc *D2Client_AutomapDataPool_Alloc() {
    int blockIndex = g_nAutomapDataCount / 512;  // numéro du bloc cible
    int slotIndex  = g_nAutomapDataCount % 512;  // slot dans ce bloc
    g_nAutomapDataCount++;                        // réserve le slot

    // Parcourt la liste chainée jusqu'au bloc cible
    AutomapDataBlock * prev;
    AutomapDataBlock * current = g_pAutomapDataPool;

    for (int i = 0; i < blockIndex && current != nullptr; i++) {
        prev  = current;
        current = current->pNextBlock;
        if (!current)
            break; // pas assez de blocs, on sort pour en allouer un nouveau
    }

    // Si le bloc n'existe pas encore, on l'alloue et on le chaine
    if (!current) {
        current = D2_CALLOC(sizeof(AutomapDataBlock));
        if (prev)
            prev->pNextBlock = current;  // chaine au bloc précédent
        else
            g_pAutomapDataPool = current; // premier bloc
    }

    // Récupère le slot et l'initialise à zéro
    D2AutomapCellStrc *entry = &current->entries[slotIndex];
    entry->pRootCell       = nullptr;
    entry->automap_cell_no = 0;
    entry->unit_pixel_x    = 0;
    entry->unit_pixel_y    = 0;
    entry->unk1            = 0;
    entry->pLessNode       = 0;
    entry->pMoreNode       = 0;
    return entry;
}

// Retourne 1 si la hauteur du sous-arbre a augmenté, 0 sinon
// *objectsAutomapCells = pointeur vers le champ pNext/pPrev du parent (lien à mettre à jour)
int D2Client_AutomapAVL_Insert(D2AutomapCellStrc *newNode, D2AutomapCellStrc **link)
{
    D2AutomapCellStrc *current = *link;

    // ── Cas 1 : feuille atteinte → insertion ──────────────────────────
    if (!current) {
        newNode->pLessNode = nullptr;
        newNode->pMoreNode = nullptr;
        newNode->unk1      = 0;
        *link = (D2AutomapCellStrc*)newNode; // insère le nouveau nœud
        return 1; // hauteur a augmenté
    }

    // Comparaison pour trouver la position d'insertion : ordre de tri = 1. Y pixel, 2. X pixel, 3. numéro cellule
    int cmp = newNode->unit_y - current->yPixel;   // 1. par Y pixel
    if (cmp == 0) {
        cmp = newNode->unit_x - current->xPixel;   // 2. par X pixel
        if (cmp == 0) {
            int newGroup = g_AutomapCellGroupByNo[newNode->automap_cell_no];

            // Si le groupe vaut -1 → singleton : traité comme égal, on n'insère pas
            // Si même groupe que le nœud courant → traité comme égal, on n'insère pas
            // Seulement si groupes différents (et non-singleton) → on trie par cell_no
            if (newGroup != -1 
                && newGroup != g_AutomapCellGroupByNo[current->nCellNo]) {
                cmp = newNode->automap_cell_no - current->nCellNo;  // 3. par cell_no
            }
            // sinon cmp reste 0 → égalité → pas d'insertion
        }
    }

        

    // ── Cas 2 : égalité → nœud déjà présent ──────────────────────────
    if (cmp == 0) {
        *link = current;
        return 0;
    }

    // ── Cas 3 : insertion à droite (newNode > current) ────────────────
    if (cmp > 0) {
        int grew = D2Client_AutomapAVL_Insert(newNode, &current->pNext);
        if (!grew) { *link = current; return 0; }

        // Rééquilibrage AVL côté droit
        switch (current->wWeight) {
            case -1: current->wWeight = 0;  *link = current; return 0; // rééquilibré
            case  0: current->wWeight = +1; *link = current; return 1; // +1 niveau
            case +1: // déséquilibre → rotation(s)
                AVL_RotateLeft_orDoubleRotate(link, current); return 0;
        }
    }

    // ── Cas 4 : insertion à gauche (newNode < current) ────────────────
    if (cmp < 0) {
        int grew = D2Client_AutomapAVL_Insert(newNode, &current->pPrev);
        if (!grew) { *link = current; return 0; }

        // Rééquilibrage AVL côté gauche
        switch (current->wWeight) {
            case +1: current->wWeight = 0;  *link = current; return 0;
            case  0: current->wWeight = -1; *link = current; return 1;
            case -1: // déséquilibre → rotation(s)
                AVL_RotateRight_orDoubleRotate(link, current); return 0;
        }
    }
}

int D2Client_RevealAutomapRoom(D2ActiveRoomStrc *room1, DWORD clip_flag, D2AutomapLayerStrc *layer) {
    D2DrlgRoomStrc * room2 = DUNGEON_GetRoomExFromRoom(room1);

    // Add floor tiles
    int tilesCount;
    D2DrlgTileDataStrc *pTile    = DUNGEON_GetFloorTilesFromRoom(room, &tilesCount);
    D2DrlgTileDataStrc *pTileEnd = pTile + tilesCount;

    for (; pTile < pTileEnd; ++pTile) {
        bool bExplored  = (pTile->dwFlags & D2MapTileFlags::MAPTILE_EXPLORED) != 0;
        bool bHidden    = (pTile->dwFlags & D2MapTileFlags::MAPTILE_HIDDEN) != 0;

        if (!bHidden && (bExplored || g_bAutomapRevealAll || clip))
            D2Client_AutomapAddTileCell(pTile, room2, &layer->pFloors);
    }

    // Add wall tiles
    pTile    = DUNGEON_GetWallTilesFromRoom(room, &tilesCount);
    pTileEnd = pTile + tilesCount;

    for (; pTile < pTileEnd; ++pTile) {
        bool bExplored  = (pTile->dwFlags & D2MapTileFlags::MAPTILE_EXPLORED) != 0;
        bool bHidden    = (pTile->dwFlags & D2MapTileFlags::MAPTILE_HIDDEN) != 0;

        if (!bHidden && (bExplored || g_bAutomapRevealAll || clip))
            D2Client_AutomapAddTileCell(pTile, room2, &layer->pWalls);
    }

    // Add objects/monsters
    for (D2UnitStrc * pUnit = room->pUnitFirst; pUnit; pUnit = pUnit->pRoomNext) {
        bool bVisible   = (pUnit->dwFlags & UNITFLAG_AUTOMAP_VISIBLE) != 0;
        bool bRevealed  = (pUnit->dwFlags & UNITFLAG_AUTOMAP_REVEALED) != 0;

        if (!bRevealAll && (!bVisible || bRevealed))
            continue;
        
        pUnit->dwFlags |= UNITFLAG_AUTOMAP_REVEALED;
        int automapCellNumber = 0;
        if ( pUnit->dwUnitType == UNIT_MONSTER ) {
            if (pUnit->dwClassId < 0 || pUnit->dwClassId >= sgptDataTables->nMonStatsTxtRecordCount)
                continue;
            D2MonStatsTxt *pMonStatsTxtRecord = &sgptDataTables->pMonStatsTxt[pUnit->dwClassId];
            if (pMonStatsTxtRecord->wMonStatsEx < 0 || pMonStatsTxtRecord->wMonStatsEx >= sgptDataTables->nMonStats2TxtRecordCount)
                continue;

            D2MonStats2Txt *pMonStats2TxtRecord = &sgptDataTables->pMonStats2Txt[pMonStatsTxtRecord->wMonStatsEx];

            if (!pMonStats2TxtRecord)
                continue;
            
            automapCellNumber = pMonStats2TxtRecord->dwAutomapCel;
        } else if (pUnit->dwUnitType == UNIT_OBJECT) {
            if (pUnit->dwClassId == OBJECT_STASH) {
                int actNo = DRLG_GetActNoFromLevelId(DUNGEON_GetLevelIdFromRoom(room1));
                if ( ActNoFromLevelId != 2 && ActNoFromLevelId != 3 )
                    continue; // stashes are only revealed in act 2 and 3
            } else if (pUnit->dwClassId == OBJECT_SEWER_STAIRS) {
                if (pUnit->dwAnimMode != OBJMODE_OPENED)
                    continue; // sewer stairs are only revealed when open
            } else if (pUnit->dwClassId == OBJECT_WAYPOINT) {
                if (DUNGEON_GetLevelIdFromRoom(room1) != LEVEL_ARCANESANCTUARY)
                    continue; // waypoints are only revealed in Arcane Sanctuary
            }

            D2ObjectsTxt *objectTxt = DATATBLS_GetObjectsTxtRecord(pUnit->dwClassId);
            automapCellNumber = objectTxt->dwAutomap;
        } else {
            continue; // only monsters and objects are revealed on automap
        }

        if (!automapCellNumber)
            continue;

        D2Client_AutomapAddObjectCell(pUnit, automapCellNumber, &layer->pObjects);
    }
}

int __fastcall D2Client_AutomapAddObjectCell(D2UnitStrc *unit, int automapCellNumber, D2AutomapCellStrc **pCellTree) {
    D2AutomapCellStrc *pNewCell = D2Client_AutomapDataPool_Alloc();
    int x = UNITS_GetClientCoordX(unit) / 10 + 1; // +1 for visual centering
    int y = UNITS_GetClientCoordY(unit) / 10 - 3; // -3 for visual centering (amplified by the isometric projection)

    D2_ASSERTM(x >= -32768 && x <= 32767, "nPixelX >= SHRT_MIN && nPixelX <= SHRT_MAX");
    D2_ASSERTM(y >= -32768 && y <= 32767, "nPixelY >= SHRT_MIN && nPixelY <= SHRT_MAX");
    D2_ASSERTM(automapCellNumber >= -32768 && automapCellNumber <= 32767, "nCelNum >= SHRT_MIN && nCelNum <= SHRT_MAX");

    pNewCell->nCellNo = automapCellNumber;
    pNewCell->xPixel = x;
    pNewCell->yPixel = y;

    return D2Client_AutomapAVL_Insert(pNewCell, pCellTree);
}