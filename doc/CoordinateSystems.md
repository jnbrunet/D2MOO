# Système de Coordonnées Diablo 2: Guide Complet

## Introduction

Diablo 2 utilise plusieurs systèmes de coordonnées interconnectés pour représenter les positions des objets, tuiles, et unités à différents niveaux de précision et pour différents usages (logique de jeu, rendu, minimap, etc.). Ce document mapppe complètement ces systèmes et montre comment naviguer entre eux.

## Table des Matières

1. [Systèmes de Coordonnées de Base](#systèmes-de-coordonnées-de-base)
2. [Structures de Données Principales](#structures-de-données-principales)
3. [Précisions des Coordonnées](#précisions-des-coordonnées)
4. [Navigation entre les Systèmes](#navigation-entre-les-systèmes)
5. [Cas d'Usage Pratiques](#cas-dusage-pratiques)
6. [Minimap (Automap)](#minimap-automap)

---

## Systèmes de Coordonnées de Base

### 1. Coordonnées de Jeu (Game Coordinates)

Les coordonnées de jeu représentent les positions **logiques** dans Diablo 2, alignées avec la grille.

- **Précision utilisée**: Subtiles (game coords)
- **Unité de base**: 1 subtile = 1/5 d'une tuile = 1 case du grid de collision
- **Origine**: Coin supérieur gauche (nord-ouest)
- **Axes**: X vers l'est, Y vers le sud
- **Utilisation**: Collision, AI, logique de jeu

```
Diablo 2 Game Grid (Subtiles)
┌─────────────────┐
│  North-West     │
│  Area           │
├─────────────────┤
│   X → East      │
│                 │
│   Y ↓ South     │
└─────────────────┘
```

### 2. Coordonnées Client (Client/Screen Coordinates)

Les coordonnées client représentent les positions **affichées** à l'écran après projection isométrique.

- **Projection**: Dimetric (2:1 aspect ratio)
- **Formules de projection**:
  - `clientX = (gameX - gameY) / 2`
  - `clientY = (gameX + gameY) / 4`
- **Inverse**:
  - `gameX = 2 * clientY + clientX`
  - `gameY = 2 * clientY - clientX`
- **Precision**: Pixels à l'écran
- **Utilisation**: Rendu graphique, affichage

### 3. Coordonnées de Minimap (Automap)

Les coordonnées de minimap sont dérivées des coordonnées client.

- **Formule**: `minimap_coord = client_coord / 10`
- **Precision**: Pixels de minimap (1/10 des pixels client)
- **Arbre de données**: AVL tree (binary search tree avec équilibrage)
- **Utilisation**: Affichage de la minimap

### 4. Coordonnées Fractionnelles (Fractional/Fixed-Point)

Pour les mouvements et les calculs de pathfinding.

- **Format**: 16.16 fixed point (16 bits entier, 16 bits fraction)
- **Précision**: 1/65536 de subtile
- **Structure**: `D2FP32_16` (union avec `dwPrecisionX/Y` et `wPosX/Y`)
- **Utilisation**: Pathfinding, mouvement fluide

---

## Structures de Données Principales

### D2DrlgCoordsStrc - Coordonnées de Salle (Room)

Définit l'emplacement et la taille d'une salle dans le dungeon.

```cpp
struct D2DrlgCoordsStrc  // sizeof 0x20
{
    int32_t nSubtileX;           // +0x00: Position X en subtiles (game coords)
    int32_t nSubtileY;           // +0x04: Position Y en subtiles (game coords)
    int32_t nSubtileWidth;       // +0x08: Largeur en subtiles
    int32_t nSubtileHeight;      // +0x0C: Hauteur en subtiles
    int32_t nTileXPos;           // +0x10: Position X en tuiles (floor tiles)
    int32_t nTileYPos;           // +0x14: Position Y en tuiles (floor tiles)
    int32_t nTileWidth;          // +0x18: Largeur en tuiles
    int32_t nTileHeight;         // +0x1C: Hauteur en tuiles
};
```

**Utilisation**: 
- Obtenir les limites d'une salle en deux précisions (subtiles ET tuiles)
- Checker si une position est dans une salle
- Grouper les tuiles et unités dans une salle

**Important**:
Cette structure est remplie dans `DRLG_CreateRoomForRoomEx()` avec une logique de conversion:
1. Les valeurs `nTile*` sont copiées depuis D2DrlgRoomStrc
2. Les valeurs `nSubtile*` sont **calculées** (tuiles × 5) via `DUNGEON_GameTileToSubtileCoords()`
3. Ensuite la structure est copiée (via `memcpy`) dans `D2ActiveRoomStrc.tCoords`

**Relations**:
- 1 tuile = 5 × 5 subtiles
- Relation mathématique: `nSubtileX = nTileXPos * 5` (etc pour Y et dimensions)
- Stockée dans `D2ActiveRoomStrc.tCoords`

### D2DrlgCoordStrc - Rectangle de Coordonnées

Représente un rectangle dans l'espace des tuiles (ou game coords).

```cpp
struct D2DrlgCoordStrc  // sizeof 0x0C
{
    int32_t nPosX;       // +0x00: Position X
    int32_t nPosY;       // +0x04: Position Y
    int32_t nWidth;      // +0x08: Largeur
    int32_t nHeight;     // +0x0C: Hauteur
};
```

**Utilisation**: 
- Boîtes de définition de rooms
- Rectangles de collision
- Zones d'intéressement

### D2DrlgRoomStrc - Définition de Salle (Design-Time)

Définie au chargement du dungeon, contient la description statique d'une salle.

```cpp
struct D2DrlgRoomStrc  // Room2
{
    D2DrlgLevelStrc* pLevel;              // +0x00: Niveau parent
    
    // Union des positions (3 façons de représenter la même chose):
    union {
        struct {
            int32_t nTileXPos;            // +0x04: Position X en tuiles
            int32_t nTileYPos;            // +0x08: Position Y en tuiles
            int32_t nTileWidth;           // +0x0C: Largeur en tuiles
            int32_t nTileHeight;          // +0x10: Hauteur en tuiles
        };
        D2DrlgCoordStrc pDrlgCoord;       // +0x04: Même données, structure compacte
    };
    
    uint32_t dwFlags;                     // +0x14: Flags de salle
    uint32_t dwOtherFlags;                // +0x18: Autres flags
    int32_t nType;                        // +0x1C: Type de salle (preset/outdoor/etc)
    
    union {
        D2DrlgPresetRoomStrc* pMaze;      // +0x20: Données de salle preset
        D2DrlgOutdoorRoomStrc* pOutdoor;  // +0x20: Données de salle outdoor
    };
    
    uint32_t dwDT1Mask;                   // +0x24: Tile caching mask
    D2TileLibraryHashStrc* pTiles[32];    // +0x28: Cache des tuiles
    D2DrlgTileGridStrc* pTileGrid;        // +0xA8: Grid de tuiles
    
    // ...autres champs...
    
    D2ActiveRoomStrc* pRoom;              // +0xE4: Pointeur vers la salle runtime
    
    // +0xE8 et au-delà : autres champs
};
```

**Utilisation**:
- Définition statique (blueprint) d'une salle
- Liée à une salle runtime via `pRoom`
- Contient les grilles de tuiles et les unités prédéfinies

### D2ActiveRoomStrc - Salle Active (Runtime)

La représentation active et dynamique d'une salle pendant le jeu.

```cpp
struct D2ActiveRoomStrc  // Room1
{
    D2DrlgCoordsStrc tCoords;             // +0x00: Coordonnées (calculées depuis D2DrlgRoomStrc)
    D2DrlgRoomTilesStrc* pRoomTiles;      // +0x20: Tuiles de la salle
    D2ActiveRoomStrc** ppRoomList;        // +0x24: Liste des salles
    int32_t nNumRooms;                    // +0x28: Nombre de salles
    
    D2UnitStrc* pUnitFirst;               // +0x2C: Premier item dans la liste des unités
    D2UnitStrc* pUnitUpdate;              // +0x30: Unité à mettre à jour
    D2RoomCollisionGridStrc* pCollisionGrid; // +0x34: Grid de collision
    
    D2DrlgRoomStrc* pDrlgRoom;            // +0x38: Backref vers D2DrlgRoomStrc
    
    D2SeedStrc pSeed;                     // +0x3C: Seed
    
    // +0x44 et au-delà : autres champs (clients, unités mortes, etc)
};
```

**Utilisation**:
- Représente une salle pendant la partie - créée depuis `DUNGEON_AllocRoom()`
- Contient la liste des unités actuellement dans la salle
- Contient les données de collision
- Lié à sa définition via `pDrlgRoom` et `tCoords`

**⚠️ Important sur les coordonnées**:
Les coordonnées `tCoords` ne sont PAS simplement copiées de D2DrlgRoomStrc. Elles sont transformées:
- Les champs `nTile*` viennent directement de D2DrlgRoomStrc
- Les champs `nSubtile*` sont **calculés**: `tuile * 5` (conversion via `DUNGEON_GameTileToSubtileCoords()`)
- Cette transformation se fait dans `DRLG_CreateRoomForRoomEx()` avant l'appel à `DUNGEON_AllocRoom()`

### D2UnitStrc - Unité (Unit/Object/Missile)

Représente tout ce qui bouge ou bouge: joueurs, monstres, objets, missiles.

```cpp
struct D2UnitStrc
{
    uint32_t dwUnitType;                  // +0x00: Type (D2C_UnitTypes)
    int32_t dwClassId;                    // +0x04: Class ID
    void* pMemoryPool;                    // +0x08: mémoire pool
    D2UnitGUID dwUnitId;                  // +0x0C: ID unique
    
    uint32_t dwAnimMode;                  // +0x10: Mode d'animation
    union {
        D2PlayerDataStrc* pPlayerData;
        D2ItemDataStrc* pItemData;
        D2MonsterDataStrc* pMonsterData;
        D2ObjectDataStrc* pObjectData;
        D2MissileDataStrc* pMissileData;   // +0x14: Données spécifiques au type
    };
    
    uint8_t nAct;                         // +0x18: Acte (0-4)
    D2DrlgActStrc* pDrlgAct;              // +0x1C: Acte DRLG parent
    
    D2SeedStrc pSeed;                     // +0x20: Seed
    
    union {
        D2DynamicPathStrc* pDynamicPath;  // Chemin dynamique (calculé)
        D2StaticPathStrc* pStaticPath;    // Chemin statique (prédéfini)
    };                                    // +0x2C: CONTIENT LES COORDONNÉES !
    
    // Animation/Séquence
    struct D2AnimSeqTxt* pAnimSeq;        // +0x30
    uint32_t dwSeqFrameCount;             // +0x34
    int32_t dwSeqFrame;                   // +0x38
    
    // ... beaucoup d'autres champs ...
    
    D2UnitStrc* pRoomNext;                // Chaînage dans la salle
    // ... etc ...
};
```

**Utilisation**:
- Représente les joueurs, monstres, objets, missiles
- Les coordonnées sont dans `pDynamicPath->tGameCoords` (ou pStaticPath)
- Fonctions accesseurs: `UNITS_GetClientCoordX/Y()`

### D2DynamicPathStrc - Chemin Dynamique

Contient les véritables coordonnées d'une unité dynamique.

```cpp
struct D2DynamicPathStrc
{
    D2FP32_16 tGameCoords;                // +0x00: Coordonnées game en fixed-point 16.16
    int32_t dwClientCoordX;               // +0x08: Coordonnées client (écran) en pixels X
    int32_t dwClientCoordY;               // +0x0C: Coordonnées client (écran) en pixels Y
    
    D2PathPointStrc tTargetCoord;         // +0x10+: Coordonnée cible
    D2PathPointStrc tPrevTargetCoord;
    D2PathPointStrc tFinalTargetCoord;
    
    D2ActiveRoomStrc* pRoom;              // Salle courante
    D2ActiveRoomStrc* pPreviousRoom;      // Salle précédente
    
    int32_t dwCurrentPointIdx;            // Index dans le chemin
    int32_t dwPathPoints;                 // Nombre de points du chemin
    
    // ... pathfinding data ...
    
    D2UnitStrc* pUnit;                    // Pointeur vers l'unité
    uint32_t dwFlags;                     // Flags de chemin
    uint32_t dwPathType;                  // Type de chemin (AStar, straight, etc)
};
```

**Utilisation**:
- Contient les coordonnées actuelles d'une unité
- `tGameCoords`: Position en fixed-point (fractional) pour le calcul
- `dwClientCoordX/Y`: Position écran (après dimetric projection)

### D2FP32_16 - Coordonnées Fractionnelles (Fixed-Point 16.16)

```cpp
union D2FP32_16
{
    struct {
        uint16_t wOffsetX;                // +0x00: Partie fractionnaire X (16 bits)
        uint16_t wPosX;                   // +0x02: Partie entière X (16 bits)
        uint16_t wOffsetY;                // +0x04: Partie fractionnaire Y (16 bits)
        uint16_t wPosY;                   // +0x06: Partie entière Y (16 bits)
    };
    
    struct {
        uint32_t dwPrecisionX;            // +0x00: Coordonnée complète X (32-bit fixed)
        uint32_t dwPrecisionY;            // +0x04: Coordonnée complète Y (32-bit fixed)
    };
    
    D2PathPointStrc ToPathPoint() const;  // Conversion en entier
};
```

**Utilisation**:
- Représente une position avec précision 1/65536 de subtile
- Utilisé pour le pathfinding et le mouvement fluide
- Peut être converti en coordonnées entières (subtiles)

### D2DrlgTileDataStrc - Données de Tuile

```cpp
struct D2DrlgTileDataStrc
{
    int32_t nWidth;                       // +0x00: Largeur en subtiles
    int32_t nHeight;                      // +0x04: Hauteur en subtiles
    int32_t nPosX;                        // +0x08: Position X en subtiles (game coords)
    int32_t nPosY;                        // +0x0C: Position Y en subtiles (game coords)
    
    int32_t unk0x10;                      // +0x10
    uint32_t dwFlags;                     // +0x14: Flags de tuile
    D2TileLibraryEntryStrc* pTile;        // +0x18: Référence graphique
    int32_t nTileType;                    // +0x1C: Type de tuile
    
    // Couleurs et intesité
    uint8_t nRed;                         // +0x28
    uint8_t nGreen;                       // +0x29
    uint8_t nBlue;                        // +0x2A
    uint8_t nIntensity;                   // +0x2B
};
```

**Utilisation**:
- Représente une tuile du dungeon
- `nPosX/Y`: Position en coordonnées game (subtiles)
- Utilisé lors du rendu et pour la minimap

### D2PresetUnitStrc - Unité Prédéfinie

```cpp
struct D2PresetUnitStrc
{
    int32_t nUnitType;                    // +0x00: Type (UNIT_PLAYER, UNIT_MONSTER, etc)
    int32_t nIndex;                       // +0x04: Index du preset
    int32_t nMode;                        // +0x08: Mode initial
    int32_t nXpos;                        // +0x0C: Position X en subtiles (game coords)
    int32_t nYpos;                        // +0x10: Position Y en subtiles (game coords)
    BOOL bSpawned;                        // +0x14: Spawned ou non
    D2MapAIStrc* pMapAI;                  // +0x18: AI data
    D2PresetUnitStrc* pNext;              // +0x1C: Chaînage
};
```

**Utilisation**:
- Représente les objets/monstres placés statiquement dans une salle du preset
- Position en coordonnées game (subtiles)
- Utilisé lors de la génération du niveau

### D2DrlgPresetRoomStrc - Salle Preset

```cpp
struct D2DrlgPresetRoomStrc
{
    int32_t nLevelPrest;                  // +0x00: Level preset ID
    int32_t nPickedFile;                  // +0x04: Fichier de preset choisi
    D2DrlgMapStrc* pMap;                  // +0x08: Map data
    
    uint32_t dwFlags;                     // +0x0C
    
    D2DrlgGridStrc pWallGrid[4];          // +0x10: Grilles de murs
    D2DrlgGridStrc pTileTypeGrid[4];      // +0x60: Grilles d'orientation
    D2DrlgGridStrc pFloorGrid[2];         // +0xB0: Grilles de sol
    D2DrlgGridStrc pCellGrid;             // +0xD8: Grille de cellules
    
    D2DrlgGridStrc* pMazeGrid;            // +0xEC: Grille du labyrinthe (optionnel)
    
    D2CoordStrc* pTombStoneTiles;         // +0xF0: Tuiles de tomb stone
    int32_t nTombStoneTiles;              // +0xF4
};
```

**Utilisation**:
- Données pour une salle prédéfinie (par exemple, une salle avec un layout fixe)
- Contient les grilles de tuiles

### D2DrlgOutdoorRoomStrc - Salle Extérieure

```cpp
struct D2DrlgOutdoorRoomStrc
{
    D2DrlgGridStrc pTileTypeGrid;         // +0x00
    D2DrlgGridStrc pWallGrid;             // +0x14
    D2DrlgGridStrc pFloorGrid;            // +0x28
    D2DrlgGridStrc pDirtPathGrid;         // +0x3C
    
    D2DrlgVertexStrc* pVertex;            // +0x50
    int32_t dwFlags;                      // +0x54
    
    // Autres champs ...
};
```

**Utilisation**:
- Données pour une salle extérieure (par exemple, une zone générée)

### D2CoordStrc - Coordonnées Simples

Coordonnées X,Y simples.

```cpp
struct D2CoordStrc
{
    int nX;                               // +0x00
    int nY;                               // +0x04
};
```

### D2AutomapCellStrc - Cellule Minimap

```cpp
struct D2AutomapCellStrc
{
    D2AutomapCellStrc* pCell;             // +0x00: Pointeur
    uint16_t nCellNo;                     // +0x04: Numéro de cellule
    uint16_t xPixel;                      // +0x06: Position X sur minimap (pixels)
    uint16_t yPixel;                      // +0x08: Position Y sur minimap (pixels)
    uint16_t wWeight;                     // +0x0A: Poids (AVL tree)
    D2AutomapCellStrc* pPrev;             // +0x0C: Pointeur précédent (AVL)
    D2AutomapCellStrc* pNext;             // +0x10: Pointeur suivant (AVL)
};
```

**Utilisation**:
- Représente une cellule sur la minimap
- Stockée dans un AVL tree pour efficient searching
- Position en pixels de minimap (`xPixel/yPixel = client_coord / 10`)

---

## Précisions des Coordonnées

| Precision          | Ratio à parent | Unités de base | Taille type   | Usage                                                  |
|--------------------|----------------|----------------|---------------|--------------------------------------------------------|
| **Room**           | 1              | 1 room (40x40) | 1280x640 px   | Grouppage d'unités/tuiles; limites de salle          |
| **Tile (Tuile)**   | 8              | 5 subtiles     | 160x80 px     | Édition de map; grille de tuiles; preset objects     |
| **Subtile**        | 5              | 1 subtile      | 32x16 px      | Collision; logique; unités; tuiles individuelles      |
| **Fractional**     | 2^16 = 65536   | 1/65536 subtile| Variable      | Pathfinding; mouvement fluide; précision calc         |
| **Minimap Pixel**  | 1/10           | 1 pixel        | 128x64 minimap| Affichage minimap uniquement                         |

---

## Navigation entre les Systèmes

### 1. Obtenir les Coordonnées d'une Unité

```cpp
// Méthode 1: Via UNITS_* (recommandé)
D2UnitStrc* pUnit = /* ... */;
int clientX = UNITS_GetClientCoordX(pUnit);  // Coordonnée écran pixel X
int clientY = UNITS_GetClientCoordY(pUnit);  // Coordonnée écran pixel Y

// Méthode 2: Via le chemin (plus détaillé)
D2DynamicPathStrc* pPath = pUnit->pDynamicPath;
if (pPath) {
    // Coordonnées fractionnelles (fixed-point 16.16)
    uint16_t gameX = pPath->tGameCoords.wPosX;
    uint16_t gameY = pPath->tGameCoords.wPosY;
    
    // Coordonnées client (écran) en pixels
    int clientX = pPath->dwClientCoordX;
    int clientY = pPath->dwClientCoordY;
    
    // Salle actuelle
    D2ActiveRoomStrc* pRoom = pPath->pRoom;
}

// Méthode 3: Via les coordonnées D2CoordStrc
D2CoordStrc coord;
UNITS_GetCoords(pUnit, &coord);  // Coordonnées game en subtiles (entier)
```

### 2. Obtenir les Coordonnées d'une Tuile

```cpp
// Via D2DrlgTileDataStrc
D2DrlgTileDataStrc* pTile = /* ... */;
int tileX_subtiles = pTile->nPosX;     // En subtiles (game coords)
int tileY_subtiles = pTile->nPosY;     // En subtiles (game coords)
int tileW_subtiles = pTile->nWidth;
int tileH_subtiles = pTile->nHeight;

// Conversion en tuiles (floor tiles)
int tileX_tiles = tileX_subtiles / 5;  // 1 tuile = 5 subtiles
int tileY_tiles = tileY_subtiles / 5;
```

### 3. Obtenir les Coordonnées d'une Salle

```cpp
// Via D2ActiveRoomStrc (runtime)
D2ActiveRoomStrc* pRoom = /* ... */;
D2DrlgCoordsStrc* pCoords = &pRoom->tCoords;

int subtileX = pCoords->nSubtileX;        // Position X en subtiles
int subtileY = pCoords->nSubtileY;        // Position Y en subtiles
int subtileW = pCoords->nSubtileWidth;    // Largeur en subtiles
int subtileH = pCoords->nSubtileHeight;   // Hauteur en subtiles

int tileX = pCoords->nTileXPos;           // Position X en tuiles
int tileY = pCoords->nTileYPos;           // Position Y en tuiles
int tileW = pCoords->nTileWidth;          // Largeur en tuiles
int tileH = pCoords->nTileHeight;         // Hauteur en tuiles

// Vérifier si une position est dans la salle
bool isInside = (px >= subtileX && px < subtileX + subtileW &&
                 py >= subtileY && py < subtileY + subtileH);
```

### 4. Transformer Game → Client

```cpp
// D2MOO ne fournit pas les fonctions de conversion (elles sont dans les DLLs)
// Mais voici les formules:

// Pour subtiles/game coords:
// clientX = (gameX - gameY) / 2
// clientY = (gameX + gameY) / 4

// Exemple:
int gameX = 100, gameY = 50;
int clientX = (gameX - gameY) / 2;  // = 25
int clientY = (gameX + gameY) / 4;  // = 37
```

### 5. Transformer Client → Minimap

```cpp
// Minimap = Client / 10
int clientX = 100, clientY = 50;
int minimapX = clientX / 10;        // = 10
int minimapY = clientY / 10;        // = 5

// Utilisation dans D2Client_AutomapAddObjectCell:
int x = UNITS_GetClientCoordX(pUnit) / 10 + 1;   // +1 pour centrage
int y = UNITS_GetClientCoordY(pUnit) / 10 - 3;   // -3 pour centrage
```

---

## Cas d'Usage Pratiques

### Scénario 1: Obtenir la Position Minimap d'un Monstre

```cpp
void PrintMonsterOnMinimap(D2UnitStrc* pMonster) {
    // Étape 1: Obtenir les coordonnées écran du monstre
    int screenX = UNITS_GetClientCoordX(pMonster);
    int screenY = UNITS_GetClientCoordY(pMonster);
    
    // Étape 2: Convertir en coordonnées minimap (diviser par 10, ajouter offset)
    int minimapX = screenX / 10 + 1;   // +1 pour centrage visuel
    int minimapY = screenY / 10 - 3;   // -3 pour centrage visuel
    
    printf("Monster at minimap: (%d, %d)\n", minimapX, minimapY);
    
    // Étape 3 (optionnel): Insérer dans l'AVL tree de minimap
    // D2Client_AutomapAddObjectCell(pMonster, cellNo, &pLayer->pObjects);
}
```

### Scénario 2: Checker si une Unité est dans une Salle

```cpp
bool IsUnitInRoom(D2UnitStrc* pUnit, D2ActiveRoomStrc* pRoom) {
    // Obtenir la position game de l'unité
    D2CoordStrc unitCoord;
    UNITS_GetCoords(pUnit, &unitCoord);
    
    // Obtenir les limites de la salle
    D2DrlgCoordsStrc* pRoomCoords = &pRoom->tCoords;
    
    // Vérifier si l'unité est dans les limites
    return (unitCoord.nX >= pRoomCoords->nSubtileX &&
            unitCoord.nX < pRoomCoords->nSubtileX + pRoomCoords->nSubtileWidth &&
            unitCoord.nY >= pRoomCoords->nSubtileY &&
            unitCoord.nY < pRoomCoords->nSubtileY + pRoomCoords->nSubtileHeight);
}
```

### Scénario 3: Obtenir Toutes les Tuiles Visibles sur la Minimap d'une Salle

```cpp
void RevealRoomOnMinimap(D2ActiveRoomStrc* pRoom, D2DrlgRoomStrc* pDrlgRoom) {
    // Obtenir les tuiles de la salle
    int tilesCount;
    D2DrlgTileDataStrc* pTiles = DUNGEON_GetFloorTilesFromRoom(pRoom, &tilesCount);
    
    for (int i = 0; i < tilesCount; i++) {
        D2DrlgTileDataStrc* pTile = &pTiles[i];
        
        // Coordonnées de la tuile en game coords (subtiles)
        int tileX = pTile->nPosX;
        int tileY = pTile->nPosY;
        
        // Convertir en client (en utilisant la formule ou fonction)
        // clientX = (tileX - tileY) / 2
        // clientY = (tileX + tileY) / 4
        
        int clientX = (tileX - tileY) / 2;
        int clientY = (tileX + tileY) / 4;
        
        // Convertir en minimap
        int minimapX = clientX / 10;
        int minimapY = clientY / 10;
        
        // Créer une cellule minimap
        // D2Client_AutomapAddTileCell(pTile, pDrlgRoom, &pLayer->pFloors);
    }
}
```

### Scénario 4: Distance entre Deux Unités

```cpp
int GetDistanceBetweenUnits(D2UnitStrc* pUnit1, D2UnitStrc* pUnit2) {
    // Utiliser les fonctions D2MOO
    int distX = UNITS_GetAbsoluteXDistance(pUnit1, pUnit2);
    int distY = UNITS_GetAbsoluteYDistance(pUnit1, pUnit2);
    
    // Distance euclidienne
    return sqrt(distX * distX + distY * distY);
}
```

---

## Minimap (Automap)

### Structure de la Minimap

La minimap utilise un **AVL tree** pour stocker les cellules affichées:

```cpp
struct D2AutomapLayerStrc {
    uint32_t nLayerNo;            // Numéro du layer
    uint32_t fSaved;              // Sauvegardé ou non
    D2AutomapCellStrc* pFloors;   // Root de l'AVL tree des sols
    D2AutomapCellStrc* pWalls;    // Root de l'AVL tree des murs
    D2AutomapCellStrc* pObjects;  // Root de l'AVL tree des objets
    D2AutomapCellStrc* pExtras;   // Root de l'AVL tree des extras
};
```

### Comparaison d'Ordre (AVL Tree)

Les cellules sont triées par:
1. Y pixel (ascendant)
2. X pixel (ascendant)
3. Numéro de cellule (ascendant)

```cpp
int cmp = newNode->yPixel - current->yPixel;  // 1. Par Y
if (cmp == 0) {
    cmp = newNode->xPixel - current->xPixel;  // 2. Par X
    if (cmp == 0) {
        cmp = newNode->nCellNo - current->nCellNo;  // 3. Par numéro de cellule
    }
}
```

### Ajout d'une Tuile à la Minimap

```cpp
// Fonction: D2Client_AutomapAddTileCell
// Entrée: Tuile DRLG, Salle, Pointeur vers root de l'AVL
// Sortie: 1 si hauteur a augmenté, 0 sinon

D2DrlgTileDataStrc* pTile = /* ... */;
D2DrlgRoomStrc* pRoom = /* ... */;

// Étapes:
// 1. Obtenir le style/type/séquence de la tuile
// 2. Chercher le numéro de cellule automap correspondant (LUT)
// 3. Convertir les coordonnées:
//    - tile → pixels client (via dimetric projection)
//    - pixels client → pixels minimap (diviser par 10)
// 4. Insérer dans l'AVL tree (pFloors, pWalls, etc)

// Résultat: Une cellule dans le tree de minimap
```

### Ajout d'un Objet à la Minimap

```cpp
// Fonction: D2Client_AutomapAddObjectCell
// Entrée: Unité, Numéro de cellule automap, Pointeur vers root AVL
// Sortie: 1 si hauteur augmentée, 0 sinon

int D2Client_AutomapAddObjectCell(
    D2UnitStrc* pUnit,              // Unité (monstre, objet, etc)
    int automapCellNumber,          // Numéro de cellule automap
    D2AutomapCellStrc** pCellTree)  // Root de l'AVL tree
{
    // Étape 1: Allouer une nouvelle cellule
    D2AutomapCellStrc* pNewCell = D2Client_AutomapDataPool_Alloc();
    
    // Étape 2: Obtenir les coordonnées client de l'unité
    int clientX = UNITS_GetClientCoordX(pUnit);
    int clientY = UNITS_GetClientCoordY(pUnit);
    
    // Étape 3: Convertir en minimap et ajouter offset de centrage
    int x = clientX / 10 + 1;   // +1 pour centrage visuel X
    int y = clientY / 10 - 3;   // -3 pour centrage visuel Y (amplifié par projection isométrique)
    
    // Étape 4: Remplir la cellule
    pNewCell->nCellNo = automapCellNumber;
    pNewCell->xPixel = x;
    pNewCell->yPixel = y;
    
    // Étape 5: Insérer dans l'AVL tree
    return D2Client_AutomapAVL_Insert(pNewCell, pCellTree);
}
```

### Révélation d'une Salle sur la Minimap

```cpp
// Fonction: D2Client_RevealAutomapRoom
// Révèle une salle complète sur la minimap

int D2Client_RevealAutomapRoom(
    D2ActiveRoomStrc* pRoom,        // Salle à révéler
    DWORD clip_flag,                // Flags de clipping
    D2AutomapLayerStrc* pLayer)     // Layer minimap cible
{
    // Étape 1: Obtenir la salle D2DrlgRoomStrc
    D2DrlgRoomStrc* pDrlgRoom = pRoom->pDrlgRoom;
    
    // Étape 2: Ajouter tous les sols
    // Itérer sur chaque tuile de sol
    // D2Client_AutomapAddTileCell(tile, pDrlgRoom, &pLayer->pFloors)
    
    // Étape 3: Ajouter tous les murs
    // Itérer sur chaque tuile de mur
    // D2Client_AutomapAddTileCell(tile, pDrlgRoom, &pLayer->pWalls)
    
    // Étape 4: Ajouter tous les objets/monstres
    for (D2UnitStrc* pUnit = pRoom->pUnitFirst; pUnit; pUnit = pUnit->pRoomNext) {
        // Déterminer le type d'objet et le numéro de cellule automap
        int cellNo = GetAutomapCellNumber(pUnit);
        
        // Ajouter à la minimap
        D2Client_AutomapAddObjectCell(pUnit, cellNo, &pLayer->pObjects);
    }
}
```

---

## Diagramme Récapitulatif

```
┌─────────────────────────────────────────────────────────────────┐
│ COORDONNÉES DIABLO 2 - Vue d'Ensemble                           │
└─────────────────────────────────────────────────────────────────┘

D2UnitStrc (Monstre/Objet/Joueur)
    │
    └─> pDynamicPath (ou pStaticPath)
        │
        ├─> tGameCoords (D2FP32_16)
        │   └─> Fixed-point 16.16 (1/65536 subtile precision)
        │       └─> wPosX, wPosY (partie entière)
        │
        └─> dwClientCoordX, dwClientCoordY
            └─> Pixels écran (après dimetric projection)
                └─> / 10 = Pixels minimap
                    └─> +offset = Position minimap finale
                    
D2ActiveRoomStrc (Salle Runtime)
    │
    └─> tCoords (D2DrlgCoordsStrc)
        ├─> nSubtileX/Y, nSubtileWidth/Height (subtiles)
        └─> nTileXPos/Y, nTileWidth/Height (tuiles)

D2DrlgRoomStrc (Définition Salle)
    │
    ├─> nTileXPos/Y, nTileWidth/Height (tuiles)
    └─> pRoom (pointe vers D2ActiveRoomStrc correspondant)

D2DrlgTileDataStrc (Tuile)
    │
    └─> nPosX/Y (subtiles, game coords)
        └─> / 10 * 5 = tuiles
        
D2CoordStrc (Coordonnées simples)
    │
    └─> nX, nY (généralement subtiles ou screen pixels)
```

---

## Résumé Rapide

| Besoin | Structure | Champ | Notes |
|--------|-----------|-------|-------|
| Position unité écran | D2DynamicPathStrc | dwClientCoordX/Y | En pixels |
| Position unité game | D2DynamicPathStrc | tGameCoords.wPosX/Y | En subtiles (entier) |
| Position unité minimap | Calculée | clientCoord / 10 ± offset | Position minimap finale |
| Position salle | D2ActiveRoomStrc | tCoords.nSubtile* | En subtiles |
| Position tuile | D2DrlgTileDataStrc | nPosX/Y | En subtiles |
| Position preset object | D2PresetUnitStrc | nXpos/Ypos | En subtiles |
| Limites salle | D2ActiveRoomStrc | tCoords.nSubtile* | Récupérer width/height |

