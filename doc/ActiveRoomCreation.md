# Création et Cycle de Vie d'une D2ActiveRoomStrc

## Table des matières

1. [Vue d'Ensemble](#vue-densemble)
2. [État des Rooms](#état-des-rooms)
3. [Processus Détaillé](#processus-détaillé)
4. [Déclencheurs](#déclencheurs)
5. [Structures Créées](#structures-créées)
6. [Populations et Units](#populations-et-units)
7. [Timeline Complète](#timeline-complète)

---

## Vue d'Ensemble

Une `D2ActiveRoomStrc` (salle active) passe par plusieurs **états** (statuts) avant d'être pleinement fonctionnelle.

```
┌─────────────────────────────────────────────────────────────────┐
│ LIFECYCLE: D2DrlgRoomStrc → D2ActiveRoomStrc                   │
└─────────────────────────────────────────────────────────────────┘

Level Load (D2Common)
    ↓
D2DrlgRoomStrc created (blueprint)
    ↓
Player enters dungeon
    ↓
    ├─→ ROOMSTATUS_CLIENT_IN_ROOM (Player present)
    │   ├─→ DRLGACTIVATE_RoomEx_EnsureHasRoom()
    │   ├─→ DUNGEON_AllocRoom() ← D2ActiveRoomStrc created
    │   └─→ sub_6FC385A0() ← Spawn units
    │
    ├─→ ROOMSTATUS_CLIENT_IN_SIGHT (Adjacent rooms)
    │   └─→ DRLGACTIVATE_RoomEx_EnsureHasRoom() (on demand)
    │
    ├─→ ROOMSTATUS_CLIENT_OUT_OF_SIGHT (Far rooms)
    │   └─→ Keep D2ActiveRoomStrc for efficiency
    │
    └─→ ROOMSTATUS_UNTILE (On demand loading)
        ├─→ Load tile data (DT1 files)
        └─→ Spawn hardcoded preset units

Player leaves
    ↓
Room state changes → eventually freed
```

---

## État des Rooms

### États Possibles

```cpp
enum D2DrlgRoomStatus
{
    ROOMSTATUS_CLIENT_IN_ROOM = 0,    // ← Player is HERE (highest priority)
    ROOMSTATUS_CLIENT_IN_SIGHT = 1,   // Adjacent/visible rooms
    ROOMSTATUS_CLIENT_OUT_OF_SIGHT = 2, // Far but loaded
    ROOMSTATUS_UNTILE = 3,            // On-demand tile loading
    ROOMSTATUS_COUNT = 4
};
```

### Machine d'État

```
CLIENT_IN_ROOM (Joueur présent)
    ↑ ↓
CLIENT_IN_SIGHT (Rooms adjacentes)
    ↑ ↓
CLIENT_OUT_OF_SIGHT (Rooms loin)
    ↑ ↓
UNTILE (Pas encore chargées)

Priorité: Lower value = Higher priority
```

**Règles Transitions**:
- Une room peut avoir **PLUSIEURS statuts** (ref count)
- L'état actif = **statut avec plus faible priorité** parmi tous les statuts actifs
- Changement d'état = appel fonction callback `gRoomExSetStatus[status](pRoom)`

---

## Processus Détaillé

### 1️⃣ Déclencheur Initial: Joueur Entre une Salle

**Appelant**: `DUNGEON_ChangeClientRoom()` (D2Dungeon.cpp:540)

```cpp
void __stdcall DUNGEON_ChangeClientRoom(D2ActiveRoomStrc* pRoom1, D2ActiveRoomStrc* pRoom2)
{
    // pRoom1 = ancienne salle (peut être NULL)
    // pRoom2 = nouvelle salle (où le joueur arrive)
    
    DRLGACTIVATE_ChangeClientRoom(
        pRoom1 ? pRoom1->pDrlgRoom : nullptr,
        pRoom2 ? pRoom2->pDrlgRoom : nullptr
    );
}
```

**Appelée par**: `UNITS_` ou `GAME_` quand un joueur se déplace

---

### 2️⃣ Changement d'État: Salle Passe à CLIENT_IN_ROOM

**Fonction**: `DRLGACTIVATE_ChangeClientRoom()` (DrlgActivate.cpp:295)

```cpp
void __fastcall DRLGACTIVATE_ChangeClientRoom(
    D2DrlgRoomStrc* pPreviousRoom,
    D2DrlgRoomStrc* pNewRoom)
{
    if (pPreviousRoom == pNewRoom)
        return;  // Pas de changement
    
    // Nouvelle salle = CLIENT_IN_ROOM
    if (pNewRoom)
    {
        DRLGACTIVATE_RoomSetAndPropagateStatus(
            pNewRoom,
            ROOMSTATUS_CLIENT_IN_ROOM  // ← État prioritaire
        );
    }
    
    // Ancienne salle = perte du statut CLIENT_IN_ROOM
    if (pPreviousRoom)
    {
        DRLGACTIVATE_RoomUnsetAndPropagateStatus(
            pPreviousRoom,
            ROOMSTATUS_CLIENT_IN_ROOM
        );
    }
}
```

---

### 3️⃣ Allocation de D2ActiveRoomStrc

Déclenché par `DRLGACTIVATE_RoomExSetStatus_ClientInRoom()`:

```cpp
void __fastcall DRLGACTIVATE_RoomExSetStatus_ClientInRoom(D2DrlgRoomStrc* pDrlgRoom)
{
    // Change l'état à ROOMSTATUS_CLIENT_IN_ROOM
    DRLGACTIVATE_UpdateRoomExStatusImpl(pDrlgRoom, ROOMSTATUS_CLIENT_IN_ROOM);
    // Si c'est le premier statut → appelle DRLGACTIVATE_RoomEx_EnsureHasRoom()
}
```

Principal: `DRLGACTIVATE_RoomEx_EnsureHasRoom()` (DrlgActivate.cpp:95)

```cpp
void DRLGACTIVATE_RoomEx_EnsureHasRoom(D2DrlgRoomStrc* pDrlgRoom, bool bInitTimeoutCounter)
{
    // Si D2ActiveRoomStrc n'existe pas:
    if (pDrlgRoom->pRoom == nullptr && 
        !(pDrlgRoom->dwFlags & DRLGROOMFLAG_HAS_ROOM))
    {
        D2DrlgStrc* pDrlg = pDrlgRoom->pLevel->pDrlg;
        
        // ⭐ ÉTAPE 1: Compute nearby rooms
        if (!pDrlgRoom->nRoomsNear)
        {
            sub_6FD77BB0(pDrlg->pMempool, pDrlgRoom);
        }
        
        // ⭐ ÉTAPE 2: Initialize room grids (walls, floors, etc)
        DRLGROOMTILE_InitRoomGrids(pDrlgRoom);
        
        // ⭐ ÉTAPE 3: Add map tiles
        DRLGROOMTILE_AddRoomMapTiles(pDrlgRoom);
        
        // ⭐ ÉTAPE 4: CREATE D2ActiveRoomStrc + collision grid
        DRLG_CreateRoomForRoomEx(pDrlg, pDrlgRoom);
        
        // Stats tracking
        ++pDrlg->nRoomsInitSinceLastUpdate;
        ++pDrlg->nAllocatedRooms;
        
        // Timeout pour l'initialisation progressive
        if (bInitTimeoutCounter)
        {
            DRLGACTIVATE_InitRoomsInitTimeout(pDrlg);
        }
    }
}
```

---

### 4️⃣ Création Réelle: DRLG_CreateRoomForRoomEx()

**Location**: DrlgDrlg.cpp:505

Cette fonction crée `D2ActiveRoomStrc` ET initialise ses données:

```cpp
void __fastcall DRLG_CreateRoomForRoomEx(D2DrlgStrc* pDrlg, D2DrlgRoomStrc* pDrlgRoom)
{
    // Prépare les coordonnées (tuiles → subtiles)
    D2DrlgCoordsStrc pDrlgCoords = {};
    pDrlgCoords.nTileXPos = pDrlgRoom->nTileXPos;      // Copie tuiles
    pDrlgCoords.nSubtileX = pDrlgRoom->nTileXPos;
    pDrlgCoords.nTileYPos = pDrlgRoom->nTileYPos;
    pDrlgCoords.nSubtileY = pDrlgRoom->nTileYPos;
    pDrlgCoords.nTileWidth = pDrlgRoom->nTileWidth;
    pDrlgCoords.nSubtileWidth = pDrlgRoom->nTileWidth;
    pDrlgCoords.nTileHeight = pDrlgRoom->nTileHeight;
    pDrlgCoords.nSubtileHeight = pDrlgRoom->nTileHeight;
    
    // ⭐ CONVERSION: tuiles → subtiles (multiplier par 5)
    DUNGEON_GameTileToSubtileCoords(&pDrlgCoords.nSubtileX, &pDrlgCoords.nSubtileY);
    DUNGEON_GameTileToSubtileCoords(&pDrlgCoords.nSubtileWidth, &pDrlgCoords.nSubtileHeight);
    
    uint32_t dwFlags = 0;
    if (pDrlgRoom->pTileGrid->pTiles.nWalls || pDrlgRoom->pTileGrid->pTiles.nFloors)
    {
        // Détermine les flags d'automap
        if (pDrlgRoom->dwFlags & DRLGROOMFLAG_AUTOMAP_REVEAL)
            dwFlags = 4;  // Automap revealed
        else if (pDrlgRoom->dwOtherFlags & 1)
            dwFlags = 1;  // Automap not revealed
        else
            dwFlags = 0;
        
        // ⭐ CREATION: Appelle DUNGEON_AllocRoom()
        pDrlgRoom->pRoom = DUNGEON_AllocRoom(
            pDrlg->pAct,
            pDrlgRoom,
            &pDrlgCoords,                        // Coordonnées converties
            &pDrlgRoom->pTileGrid->pTiles,       // Tuiles
            (int)SEED_RollRandomNumber(&pDrlgRoom->pSeed),  // Seed
            dwFlags                              // Automap flags
        );
    }
}
```

---

### 5️⃣ Allocation Mémoire: DUNGEON_AllocRoom()

**Location**: D2Dungeon.cpp:178

```cpp
D2ActiveRoomStrc* __fastcall DUNGEON_AllocRoom(
    D2DrlgActStrc* pAct,
    D2DrlgRoomStrc* pDrlgRoom,
    D2DrlgCoordsStrc* pDrlgCoords,
    D2DrlgRoomTilesStrc* pRoomTiles,
    int nLowSeed,
    uint32_t dwFlags)
{
    // ⭐ ALLOCATION: Créer la structure
    D2ActiveRoomStrc* pRoom = D2_CALLOC_STRC_POOL(pAct->pMemPool, D2ActiveRoomStrc);
    
    // Configuration de base
    pRoom->dwFlags = dwFlags;
    SEED_InitLowSeed(&pRoom->pSeed, nLowSeed);
    
    // Chaînage
    pRoom->pDrlgRoom = pDrlgRoom;              // ← Backref
    
    // ⭐ COPY COORDINATES
    memcpy(&pRoom->tCoords, pDrlgCoords, sizeof(D2DrlgCoordsStrc));
    
    pRoom->pRoomTiles = pRoomTiles;            // Tuiles graphiques
    
    // Chaînage dans la liste des rooms de l'acte
    pRoom->pRoomNext = pAct->pRoom;
    pAct->pRoom = pRoom;
    pAct->bHasPendingRoomsUpdates = TRUE;
    pRoom->pAct = pAct;
    
    // Linking avec le D2DrlgRoomStrc
    DRLGROOM_SetRoom(pDrlgRoom, pRoom);
    
    // ⭐ ALLOCATE NEARBY ROOMS LIST
    pRoom->ppRoomList = (D2ActiveRoomStrc**)D2_ALLOC_POOL(
        pAct->pMemPool,
        sizeof(D2ActiveRoomStrc*) * pRoom->pDrlgRoom->nRoomsNear
    );
    
    // ⭐ REORDER AND INITIALIZE NEARBY ROOMS
    pRoom->nNumRooms = DRLGROOM_ReorderNearRoomList(
        pRoom->pDrlgRoom,
        pRoom->ppRoomList
    );
    
    // Initialiser les salles adjacentes
    for (int i = 0; i < pRoom->nNumRooms; ++i)
    {
        D2ActiveRoomStrc* pAdjacentRoom = pRoom->ppRoomList[i];
        
        if (pAdjacentRoom != pRoom)
        {
            // Mettre à jour la liste de salles adjacentes
            pAdjacentRoom->nNumRooms = DRLGROOM_ReorderNearRoomList(
                pAdjacentRoom->pDrlgRoom,
                pAdjacentRoom->ppRoomList
            );
        }
    }
    
    // ⭐ ALLOCATE COLLISION GRID
    COLLISION_AllocRoomCollisionGrid(pAct->pMemPool, pRoom);
    
    // Callback (optional)
    if (pAct->pfnActCallBack)
    {
        pAct->pfnActCallBack(pRoom);
    }
    
    return pRoom;
}
```

---

### 6️⃣ Initialisation des Units: sub_6FC385A0()

**Location**: Game.cpp:1715

```cpp
void __fastcall sub_6FC385A0(D2GameStrc* pGame, D2ActiveRoomStrc* pRoom)
{
    // sub_6FC679F0 = (peut être désactivé ou être un no-op)
    sub_6FC679F0(pGame, pRoom);
    
    // Vérifier si la salle a déjà été initialisée
    if (pRoom->dwFlags & 1)  // ← Bit 0 = initialized flag
    {
        // Room déjà initialisée: restore les unités inactives
        if (!D2Common_10074(pRoom))
        {
            SUNITINACTIVE_RestoreInactiveUnits(pGame, pRoom);
            D2Common_10075(pRoom, 1);
        }
    }
    else  // ← PREMIÈRE INITIALISATION
    {
        // ⭐ STEP 1: Spawn all preset units (objects + monsters)
        SUNIT_SpawnPresetUnitsInRoom(pGame, pRoom);
        
        // ⭐ STEP 2: Restore any inactive units (saved from previous visit)
        SUNITINACTIVE_RestoreInactiveUnits(pGame, pRoom);
        
        // ⭐ STEP 3: Population dynamique (monstres procéduraux)
        OBJECTS_PopulationHandler(pGame, pRoom);
        
        // ⭐ STEP 4: Additional population
        D2GAME_PopulateRoom_6FC67190(pGame, pRoom);
        
        // ⭐ Mark room as initialized
        pRoom->dwFlags |= 1u;
        
        // Callback
        D2Common_10075(pRoom, 1);
    }
}
```

---

## Déclencheurs

### Déclencheur Principal: Mouvement du Joueur

**Séquence**:

1. **Joueur se déplace** → Collision avec une frontière de salle
2. **Code mouvement détecte changement de room**
3. **Appel `DUNGEON_ChangeClientRoom(oldRoom, newRoom)`**
4. **Cascade d'initialisations** (voir processus ci-dessus)

### Déclencheurs Secondaires

**Beam/Teleport**:
- Même processus, juste instantané au lieu de progressif

**Scrolling/Viewing Adjacent Rooms**:
- Rooms dans `ROOMSTATUS_CLIENT_IN_SIGHT` initialisées on-demand
- Pas aussi prioritaire que `CLIENT_IN_ROOM`

**Level Up/Difficulty Change**:
- Toutes les rooms du dungeon peuvent être réinitialisées

---

## Structures Créées

### 1. D2ActiveRoomStrc

```cpp
D2ActiveRoomStrc {
    D2DrlgCoordsStrc tCoords;              // Coordonnées (subtiles + tuiles)
    D2DrlgRoomTilesStrc* pRoomTiles;       // Tuiles graphiques
    D2ActiveRoomStrc** ppRoomList;         // List of nearby rooms
    int32_t nNumRooms;                     // Count
    
    D2UnitStrc* pUnitFirst;                // ← LIST STARTS HERE (units added)
    D2UnitStrc* pUnitUpdate;               // Update cursor
    
    D2RoomCollisionGridStrc* pCollisionGrid; // Collision data
    D2DrlgRoomStrc* pDrlgRoom;             // Backref
    
    D2SeedStrc pSeed;                      // RNG seed
    
    D2DrlgActStrc* pAct;                   // Parent act
    D2ActiveRoomStrc* pRoomNext;           // Linked list
    
    uint32_t dwFlags;                      // Init state + automap
    
    // ... autres champs ...
};
```

### 2. D2RoomCollisionGridStrc

Créé par `COLLISION_AllocRoomCollisionGrid()` (D2Collision.cpp)

- Grille de collision pour pathfinding
- Données topologiques de la salle

### 3. D2UnitStrc pour Preset Units

Créé par `SUNIT_CreatePresetUnit()` (SUnit.cpp:1002)

- 1 D2UnitStrc par objet/monstre prédéfini
- Chaîné dans `pRoom->pUnitFirst`

---

## Populations et Units

### Phase 1: Preset Units

**Fonction**: `SUNIT_SpawnPresetUnitsInRoom()` (SUnit.cpp:1084)

```cpp
// Parcourt D2PresetUnitStrc -> D2UnitStrc

// Spawn objectives FIRST (objects):
for (D2PresetUnitStrc* i = pPresetUnit; i; i = i->pNext)
{
    if (i->nUnitType != UNIT_MONSTER && !(i->bSpawned & 1))
    {
        SUNIT_SpawnPresetUnit(pGame, pRoom, i);
        // ⭐ Créé D2UnitStrc, linked à pRoom->pUnitFirst
    }
}

// THEN spawn monsters:
for (D2PresetUnitStrc* i = pPresetUnit; i; i = i->pNext)
{
    if (i->nUnitType == UNIT_MONSTER && !(i->bSpawned & 1))
    {
        SUNIT_SpawnPresetUnit(pGame, pRoom, i);
    }
}
```

### Phase 2: Inactive Units Restoration

**Fonction**: `SUNITINACTIVE_RestoreInactiveUnits()` (SUnitInactive.cpp:50)

- Restaure les unités sauvegardées de visites précédentes
- Units non sauvegardés normalement disparaissent

### Phase 3: Dynamic Population

**Functions**:
- `OBJECTS_PopulationHandler()` - Population procédurale d'objets
- `D2GAME_PopulateRoom_6FC67190()` - Autres populations

---

## Timeline Complète

```
────────────────────────────────────────────────────────────────────

LEVEL LOAD (D2Common Init)
├─ Load level .ds1/.wld files
├─ Create D2DrlgRoomStrc per room (blueprint, no units)
└─ Create D2DrlgLevelStrc (container)

────────────────────────────────────────────────────────────────────

GAME START (D2Game when player enters)
├─ Player spawns in starting salle
├─ Calls DUNGEON_ChangeClientRoom(nullptr, startingRoom)
│
└─> STATUS CHANGE: CLIENT_IN_ROOM
    ├─ Calls DRLGACTIVATE_RoomExSetStatus_ClientInRoom()
    │
    └─> DRLGACTIVATE_RoomEx_EnsureHasRoom()
        ├─ DRLGROOMTILE_InitRoomGrids()
        │  └─ Initialize wall/floor grids from D2DrlgTileGridStrc
        │
        ├─ DRLGROOMTILE_AddRoomMapTiles()
        │  └─ Add tile data to D2DrlgRoomStrc
        │
        └─ DRLG_CreateRoomForRoomEx()
           ├─ Create D2DrlgCoordsStrc (tuiles → subtiles)
           │
           └─> DUNGEON_AllocRoom()
               ├─ D2_CALLOC_STRC_POOL() ← D2ActiveRoomStrc allocated
               ├─ memcpy(&pRoom->tCoords, ...) ← Coordinates copied
               ├─ COLLISION_AllocRoomCollisionGrid() ← Collision data
               └─ Return pRoom

    Then IMMEDIATELY called from Game.cpp:

────────────────────────────────────────────────────────────────────

UNIT INITIALIZATION (D2Game)
│
└─> sub_6FC385A0(pGame, pRoom) [First time]
    │
    ├─ Check pRoom->dwFlags & 1
    │  If not set, FIRST TIME initialization:
    │
    ├─> SUNIT_SpawnPresetUnitsInRoom()
    │   │
    │   ├─ For each D2PresetUnitStrc in pDrlgRoom->pPresetUnits:
    │   │  ├─ If not UNIT_MONSTER and not spawned:
    │   │  │  └─ SUNIT_SpawnPresetUnit()
    │   │  │     └─ SUNIT_CreatePresetUnit()  ← Create D2UnitStrc
    │   │  │        └─ Link to pRoom->pUnitFirst
    │   │  │
    │   │  └─ If UNIT_MONSTER and not spawned:
    │   │     └─ SUNIT_SpawnPresetUnit()
    │   │        └─ SUNIT_CreatePresetUnit()  ← Create D2UnitStrc
    │   │
    │   └─ Set pPresetUnit->bSpawned |= 1  ← Mark spawned
    │
    ├─> SUNITINACTIVE_RestoreInactiveUnits()
    │   └─ Restore units from save (if any)
    │
    ├─> OBJECTS_PopulationHandler()
    │   └─ Spawn procedural objects
    │
    ├─> D2GAME_PopulateRoom_6FC67190()
    │   └─ Additional procedural population
    │
    ├─ pRoom->dwFlags |= 1  ← Mark as initialized
    │
    └─ D2Common_10075(pRoom, 1)  ← Callback

────────────────────────────────────────────────────────────────────

ROOM ACTIVE AND READY
│
├─ All units in pRoom->pUnitFirst chain active
├─ Collision grid ready for pathfinding
├─ Coordinates system active
│
└─ Game loop processes units normally

────────────────────────────────────────────────────────────────────

PLAYER LEAVES ROOM
│
├─ Calls DUNGEON_ChangeClientRoom(currentRoom, newRoom)
│
├─ currentRoom status → CLIENT_IN_SIGHT or lower
│  └─ Room stays resident in memory (for efficiency)
│
├─ Units may be deactivated/saved depending on distance
│
└─ Event loop continues for other rooms

────────────────────────────────────────────────────────────────────

ROOM CLEANUP (when freed)
│
└─ DRLGROOM_FreeDrlgRoom() or similar
   ├─ Free collision grid
   ├─ Free unit list
   ├─ Free nearby rooms list
   └─ pRoom = nullptr

────────────────────────────────────────────────────────────────────
```

---

## Résumé des Appels Clés

| Étape | Fonction | Fichier | Rôle |
|-------|----------|---------|------|
| 1 | `DUNGEON_ChangeClientRoom()` | D2Dungeon.cpp | Entry point |
| 2 | `DRLGACTIVATE_ChangeClientRoom()` | DrlgActivate.cpp | Status transition |
| 3 | `DRLGACTIVATE_RoomEx_EnsureHasRoom()` | DrlgActivate.cpp | Ensure room grids |
| 4 | `DRLG_CreateRoomForRoomEx()` | DrlgDrlg.cpp | Prepare coordinates |
| 5 | `DUNGEON_AllocRoom()` | D2Dungeon.cpp | Allocate D2ActiveRoomStrc |
| 6 | `COLLISION_AllocRoomCollisionGrid()` | D2Collision.cpp | Collision system |
| 7 | `sub_6FC385A0()` | Game.cpp | Initialize units |
| 8 | `SUNIT_SpawnPresetUnitsInRoom()` | SUniT.cpp | Spawn preset units |
| 9 | `SUNITINACTIVE_RestoreInactiveUnits()` | SUnitInactive.cpp | Restore saved units |
| 10 | `OBJECTS_PopulationHandler()` | Objects.cpp | Dynamic population |

---

## Dépendances de Structures

```
D2DrlgRoomStrc (statique)
    │
    ├─ pPresetUnits → D2PresetUnitStrc chain
    │  └─ Converted to D2UnitStrc during spawn
    │
    ├─ pTileGrid → D2DrlgTileGridStrc
    │  └─ Contains graphic tiles data
    │
    └─ pRoom ← D2ActiveRoomStrc (dynamique)
       │
       ├─ tCoords (D2DrlgCoordsStrc)
       │  ├─ nSubtileX/Y/Width/Height (en subtiles)
       │  └─ nTileXPos/Y/Width/Height (en tuiles)
       │
       ├─ pUnitFirst → D2UnitStrc chain
       │  ├─ pUnitNext (à l'intérieur de room)
       │  └─ pDynamicPath (position data)
       │
       ├─ pCollisionGrid → D2RoomCollisionGridStrc
       │  └─ Collision/walkability data
       │
       ├─ ppRoomList → D2ActiveRoomStrc* array
       │  └─ Adjacent rooms for efficiency
       │
       └─ pDrlgRoom ← Backref to D2DrlgRoomStrc
```

---

## Notes Importantes

### Flags d'Initialisation

```cpp
pRoom->dwFlags & 1      // Bit 0: Room initialized (units spawned)
pRoom->dwFlags & 4      // Bit 2: Automap revealed

// Autres flags possibles stockés dans D2DrlgRoomStrc
DRLGROOMFLAG_HAS_ROOM   // Room had D2ActiveRoomStrc created
DRLGROOMFLAG_TILELIB_LOADED  // Tile graphics loaded
DRLGROOMFLAG_PRESET_UNITS_ADDED  // Preset units spawned
```

### Coordonnées

- **D2DrlgRoomStrc**: Coordonnées en **tuiles** (5 subtiles = 1 tuile)
- **D2DrlgCoordsStrc**: Coordonnées converties en **tuiles + en subtiles**
- **D2ActiveRoomStrc.tCoords**: Copie de D2DrlgCoordsStrc après conversion
- **Formule**: 1 tuile = 5 subtiles, donc `subtile = tuile * 5`

### Order Important: Objects Before Monsters

Quand on spawne les preset units, les **objets** sont créés AVANT les **monstres**:

```cpp
// Objects first
for (i in pPresetUnit) {
    if (i->nUnitType != UNIT_MONSTER)  ← Not monsters
        SUNIT_SpawnPresetUnit(...);
}

// Monsters second
for (i in pPresetUnit) {
    if (i->nUnitType == UNIT_MONSTER)  ← Only monsters
        SUNIT_SpawnPresetUnit(...);
}
```

Raison: Les objets peuvent être des quests, et les monstres pourraient en dépendre.

---

## Optimisations et Considérations

### Memory Pooling

- All allocations go through `D2_CALLOC_STRC_POOL(pMemPool, ...)`
- Memory pool = act-scoped persistent allocator
- Allows quick deallocation of entire act

### Lazy Loading

- Rooms in `CLIENT_IN_SIGHT` created on-demand
- `ROOMSTATUS_UNTILE` for on-demand tile loading
- Prevents memory exhaustion for large levels

### State Machine

- Rooms tracked per status (linked lists)
- Efficient traversal of only active rooms
- Easy to iterate "rooms player can see", etc.

### Collision Grid

- Allocated after room, uses room coordinates
- Essential for pathfinding and movement validation

