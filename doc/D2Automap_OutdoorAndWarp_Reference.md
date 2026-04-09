# D2 Automap — Outdoor Passages & Warp Tiles Reference

Consolidation des 4 fichiers de documentation précédents, corrigée et étendue.
Couvre les deux systèmes de connexion entre levels outdoor : les **passages piétons** (border exits)
et les **warp tiles** (portails/escaliers).

---

## TABLE DES MATIÈRES

1. [Vue d'ensemble — deux systèmes distincts](#1-vue-densemble)
2. [Structures de données](#2-structures-de-données)
3. [Système 1 — Passages outdoor (border exits)](#3-système-1--passages-outdoor)
4. [Système 2 — Warp tiles (portails/escaliers)](#4-système-2--warp-tiles)
5. [Systèmes de coordonnées](#5-systèmes-de-coordonnées)
6. [Référence implémentation automap](#6-référence-implémentation-automap)

---

## 1. Vue d'ensemble

| Caractéristique | Passages outdoor | Warp tiles |
|---|---|---|
| Type | Frontière nivelée entre deux zones | Portail/escalier vers une autre zone |
| Exemples | Cold Plains → Blood Moor | Forgotten Tower (Black Marsh), Arcane Sanctuary |
| Donnée disponible | Dès `DRLG_InitLevel` (blueprint time) | Dès `DRLG_InitLevel` (blueprint time) pour destinations/flags |
| Position exacte | `pOutdoors->pVertices[i].nPosX/Y` en tiles | Position précise **uniquement** après activation de la room |
| Position approchée | N/A | Centre de la DrlgRoom hôte `(nTileXPos + nTileWidth/2) * 5` en subtiles |
| Données lazy | `pRoomData`, certains vertex path lists | `pRoomTiles`, `pPresetUnits`, `pRoom` |
| Structure clé | `D2DrlgOutdoorInfoStrc::pVertices[24]` | `D2DrlgRoomStrc::dwFlags` bits `HAS_WARP_*` |
| Max par level | 6 (`pPathStarts[6]`, `nVertices <= 6`) | 8 (un par bit `DRLGROOMFLAG_HAS_WARP_0..7`) |

---

## 2. Structures de données

### 2.1 `D2DrlgVertexStrc` (0x14 bytes sur 32-bit)

Défini dans [D2DrlgDrlgVer.h](source/D2Common/include/Drlg/D2DrlgDrlgVer.h).

```cpp
struct D2DrlgVertexStrc      // #pragma pack(1)
{
    int32_t  nPosX;          // +0x00  Coordonnée X mondiale (en tiles)
    int32_t  nPosY;          // +0x04  Coordonnée Y mondiale (en tiles)
    uint8_t  nDirection;     // +0x08  D2AltDirections (ALTDIR_WEST=0..ALTDIR_SOUTH=3)
    uint8_t  pad0x09[3];     // +0x09
    int32_t  dwFlags;        // +0x0C  bit 0x01 = connexion level, bit 0x02 = spécial
    D2DrlgVertexStrc* pNext; // +0x10  Liste chaînée simple (NULL = fin de chemin)
};                           // Total : 0x14 bytes (32-bit), 0x18 bytes (64-bit)
```

> **Correction doc précédente :** la taille indiquée "0x18 bytes" était pour 64-bit.
> Le code LOD est 32-bit → taille réelle = **0x14 bytes**.

### 2.2 `D2DrlgOutdoorInfoStrc` (partie pertinente)

Défini dans [D2DrlgOutdoors.h](source/D2Common/include/Drlg/D2DrlgOutdoors.h).

```cpp
struct D2DrlgOutdoorInfoStrc  // #pragma pack(1)
{
    uint32_t dwFlags;              // +0x00  D2C_OutDoorInfoFlags
    D2DrlgGridStrc pGrid[4];       // +0x04  [0]=LevelPresetId [2]=D2DrlgOutdoorPackedGrid2InfoStrc
    union {
        struct {
            int32_t nWidth;        // +0x54
            int32_t nHeight;       // +0x58
            int32_t nGridWidth;    // +0x5C
            int32_t nGridHeight;   // +0x60
        };
        D2DrlgCoordStrc pCoord;
    };
    D2DrlgVertexStrc* pVertex;     // +0x64  Liste circulaire des vertex de périmètre
    D2DrlgVertexStrc* pPathStarts[6]; // +0x68  6 listes de chemin (une par passage actif)
    D2DrlgVertexStrc pVertices[24];   // +0x80  Tableau statique de travail (voir §3.3)
    int32_t nVertices;             // +0x260  Nombre de passages actifs (0-6)
    D2DrlgOrthStrc* pRoomData;     // +0x264
};
```

**Layout mémoire vérifié (32-bit) :**
- `pVertex` à 0x64 = pointeur 4 bytes
- `pPathStarts[6]` à 0x68 = 6 × 4 = 24 (0x18) bytes → fin à 0x80 ✓
- `pVertices[24]` à 0x80 = 24 × 0x14 = 0x1E0 bytes → fin à 0x260 ✓
- `nVertices` à 0x260 ✓, `pRoomData` à 0x264 ✓

### 2.3 `D2DrlgLinkStrc` (0x10 bytes)

```cpp
struct D2DrlgLinkStrc
{
    void*    pfLinker;      // +0x00  Fonction de positionnement
    int32_t  nLevel;        // +0x04  Level ID cible (0 = sentinel)
    int32_t  nLevelLink;    // +0x08  Index du level parent dans le tableau (-1 = racine)
    int32_t  nLevelLinkEx;  // +0x0C  Paramètre supplémentaire (souvent -1)
};
```

### 2.4 `D2DrlgLevelLinkDataStrc`

```cpp
struct D2DrlgLevelLinkDataStrc
{
    D2SeedStrc        pSeed;           // +0x00
    D2DrlgCoordStrc   pLevelCoord[15]; // +0x08  Position/taille de chaque level calculée
    D2DrlgLinkStrc*   pLink;           // +0xF8
    int32_t           nRand[4][15];    // +0xFC  Valeurs RNG pour variation de placement
    int32_t           nIteration;      // +0x1EC
    int32_t           nCurrentLevel;   // +0x1F0
};
```

### 2.5 `D2DrlgWarpStrc` (0x48 bytes)

Défini dans [D2DrlgDrlg.h](source/D2Common/include/Drlg/D2DrlgDrlg.h#L529).

```cpp
struct D2DrlgWarpStrc
{
    int32_t          nLevel;    // +0x00  Level ID propriétaire de cette entrée
    int32_t          nVis[8];   // +0x04  Level ID adjacent réel pour chaque slot (0 = slot vide)
    int32_t          nWarp[8];  // +0x24  Index LvlWarpTxt pour chaque slot (-1 = passage piéton, ≥0 = portail/escalier)
    D2DrlgWarpStrc*  pNext;     // +0x44
};
```

`pDrlg->pWarp` est une liste chaînée d'overrides par level. Quand un level n'a pas d'entrée
dans cette liste, les données sont lues depuis `D2LevelDefBin` (même convention) :

```cpp
// D2LevelDefBin (LevelsTbls.h)
int32_t  dwVis[8];   // +0x48  idem nVis : level IDs adjacents réels
uint32_t dwWarp[8];  // +0x68  idem nWarp : index LvlWarpTxt (cast en int pour comparer à -1)
```

> **⚠️ PIÈGE CRITIQUE — Confusion des noms :**
>
> | Champ | Contenu réel | Valeur nulle | Usage correct |
> |---|---|---|---|
> | `nWarp[i]` / `dwWarp[i]` | **Index LvlWarpTxt** (pas un level ID !) | `-1` = pas de portail | Passer à `DATATBLS_GetLvlWarpTxtRecordFromLevelIdAndDirection(nWarp[i], szDir)` |
> | `nVis[i]` / `dwVis[i]`  | **Vrai level ID** de la zone voisine | `0` = slot vide | Stocker comme `pWarpDestLevelId[n]` |
> | `D2LvlWarpTxt::dwLevelId` | **Index LvlWarpTxt** (malgré le nom) | — | Ne **pas** utiliser comme level ID destination |
>
> La fonction `DRLGWARP_GetWarpDestinationFromArray` retourne `nWarp[i]`, **pas** un level ID.
> Pour la destination automap, utiliser toujours `nVis[i]`.

### 2.6 Flags de warp sur `D2DrlgRoomStrc`

```cpp
enum D2DrlgRoomFlags {
    DRLGROOMFLAG_HAS_WARP_0 = 0x00000010,   // bits 4-11
    DRLGROOMFLAG_HAS_WARP_1 = 0x00000020,
    DRLGROOMFLAG_HAS_WARP_2 = 0x00000040,
    DRLGROOMFLAG_HAS_WARP_3 = 0x00000080,
    DRLGROOMFLAG_HAS_WARP_4 = 0x00000100,
    DRLGROOMFLAG_HAS_WARP_5 = 0x00000200,
    DRLGROOMFLAG_HAS_WARP_6 = 0x00000400,
    DRLGROOMFLAG_HAS_WARP_7 = 0x00000800,
    DRLGROOMFLAG_HAS_WARP_MASK = 0x00000FF0,
    DRLGROOMFLAG_HAS_WARP_FIRST_BIT = 4,    // bit shift pour obtenir l'index
};
```

### 2.7 `D2DrlgOrthStrc` (0x18 bytes, 32-bit)

Défini dans [D2DrlgDrlg.h](source/D2Common/include/Drlg/D2DrlgDrlg.h#L463).

```cpp
struct D2DrlgOrthStrc   // #pragma pack(1)
{
    union {
        D2DrlgRoomStrc*  pDrlgRoom;  // +0x00  Connexions room-à-room (ex. Barracks→OuterCloister)
        D2DrlgLevelStrc* pLevel;     // +0x00  Connexions level-à-level (passages outdoor)
    };
    uint8_t          nDirection;     // +0x04  D2AltDirections (WEST=0, NORTH=1, EAST=2, SOUTH=3)
    uint8_t          unk0x05[3];     // +0x05
    BOOL             bPreset;        // +0x08  TRUE si la zone voisine est DRLGTYPE_PRESET
    BOOL             bInit;          // +0x0C  TRUE si cette connexion est supprimée/désactivée
    D2DrlgCoordStrc* pBox;           // +0x10  &pLevel->pLevelCoords (bounding box du voisin)
    D2DrlgOrthStrc*  pNext;          // +0x14  Prochain élément (liste chaînée)
};
```

`pOutdoors->pRoomData` est une liste chaînée de `D2DrlgOrthStrc`. Chaque entrée représente
un level adjacent dont `nVis[j] != 0` AND `nWarp[j] == -1`.

**Champs clés :**

- **`pLevel->nLevelId`** : level ID de la zone voisine (toujours valide, même si `bPreset=TRUE`).

- **`nDirection`** : côté de la frontière depuis lequel la zone voisine est accessible.
  Correspond au `pVertices[i].nDirection` du vertex généré pour ce passage.

- **`bPreset`** : mis à `TRUE` par `DRLGROOM_AddOrth` quand `pWarpLevel->nDrlgType == DRLGTYPE_PRESET`.
  Cela inclut les caves/donjons **et** les zones preset piétonnes telles que **Rogue Encampment**
  et **Monastery Gate** (tous deux `DRLGTYPE_PRESET` mais connectés par simple passage de bordure).
  `bPreset` ne distingue donc **pas** un donjon d'un passage piéton — le vrai filtre est
  `nWarp[j] == -1`, appliqué en amont lors de la construction de `pRoomData` par `sub_6FD82750`.
  Les caves (Forgotten Tower…) ont un warp RoomTile, donc `nWarp[j] != -1` → elles ne figurent
  **jamais** dans `pRoomData`.

- **`bInit`** : mis à `TRUE` pour désactiver une connexion. Toujours ignorer ces entrées.

**Résumé — quand filtrer :**

```
 bPreset=FALSE, bInit=FALSE  → passage piéton de bordure         ✅ à émettre
 bPreset=TRUE,  bInit=FALSE  → zone DRLGTYPE_PRESET adjacente    ✅ à émettre (nWarp==-1 déjà filtré)
 bPreset=*,     bInit=TRUE   → connexion désactivée              ❌ à ignorer
```

### 2.8 `D2DrlgVertexStrc` et `pOutdoors->pVertices[]`

Représentation optionnelle des points de passage — distincte de `pRoomData` et complémentaire.

```cpp
struct D2DrlgVertexStrc  // 6 bytes, #pragma pack(1)
{
    int16_t  nPosX;       // +0x00  Coordonnée X absolue en tiles
    int16_t  nPosY;       // +0x02  Coordonnée Y absolue en tiles
    uint8_t  nDirection;  // +0x04  D2AltDirections (WEST=0, NORTH=1, EAST=2, SOUTH=3)
                          //        ou 4 = invalide / vertex de cave-entrance
    uint8_t  unk0x05;
};
```

Le tableau `pOutdoors->pVertices[24]` est organisé en 4 groupes de 6 :

| Plage       | Rôle |
|---|---|
| `[0..5]`    | Border vertices : point sur la frontière détectant une connexion |
| `[6..11]`   | Exit points ajustés par `DRLGOUTDOORS_CalculatePathCoordinates` |
| `[12..17]`  | Destination endpoints dans le level adjacent (fournis par le voisin) |
| `[18..23]`  | Bridge/river vertices (sub_6FD7F5B0, flag OUTDOOR_BRIDGE/OUTDOOR_RIVER) |

**`pRoomData` vs `pVertices[]` — source de vérité vs positionnement :**

| Structure | Rôle | Présent dans |
|---|---|---|
| `pRoomData` (`D2DrlgOrthStrc`) | Source de vérité — **tous** les levels adjacents | Tous les actes |
| `pVertices[]` (`D2DrlgVertexStrc`) | Positionnement précis du passage en tiles | **Act 1 uniquement** |

`pOutdoors->pVertices[]` est peuplé **uniquement** par `DRLGOUTDOORS_SpawnAct1DirtPaths`, appelée
dans `DRLGOUTWILD_InitAct1OutdoorLevel`. Les initialisateurs des autres actes n'appellent
**jamais** cette fonction.

**Conséquences par acte :**

| Acte | `pVertices[]` peuplé | Méthode de position dans `AutomapGetOutdoorPassagesGameCoords` |
|---|---|---|
| 1 | ✅ oui (`SpawnAct1DirtPaths`) | Vertex précis — extremum vers la bordure |
| 2 | ❌ non | Fallback bounding-box (`pOrth->pBox` ↔ `pLevel->pLevelCoords`) |
| 3 | ❌ non (`InitAct3OutdoorLevel` → BuildJungle + BuildKurast) | Fallback bounding-box |
| 4 | ❌ non | Fallback bounding-box |
| 5 | ❌ non | Fallback bounding-box |

---

## 3. Système 1 — Passages outdoor

### 3.1 Flux de génération

```
DRLGOUTPLACE_CreateLevelConnections(pDrlg, nAct)
  │
  ├─ Choisit le tableau D2DrlgLinkStrc selon l'acte
  │   (gAct1WildernessDrlgLink, gAct2OutdoorDrlgLink, etc.)
  │
  └─ sub_6FD823C0(pDrlg, pDrlgLink, ...)
       │
       ├─ Pour chaque entrée : pfLinker(pLevelLinkData)
       │   → calcule position du level dans l'espace mondial
       │   → valide non-chevauchement
       │
       └─ DRLGOUTDOORS_InitOutdoorLevelAfterSeed(pLevel)
            │
            ├─ DRLGVER_CreateVertices()         → pOutdoors->pVertex (liste circulaire)
            ├─ (calcul des endpoints voisins)   → pVertices[12..17]
            └─ DRLGGRID_SetVertexGridFlags()    → pGrid[2].bLvlLink = 1

sub_6FD82750(pDrlg, nStartId, nEndId)           [passe séparée — construit pRoomData]
  │
  └─ Pour chaque level DRLGTYPE_OUTDOOR dans la range :
       pVisArray  = DRLGROOM_GetVisArrayFromLevelId()
       pWarpArray = DRLGWARP_GetWarpIdArrayFromLevelId()
       Pour j = 0..7 :
         Si nVis[j] != 0 && nWarp[j] == -1 :
           DRLGROOM_AddOrth(&pOutdoors->pRoomData, pVisLevel, nDirection,
                            bIsPreset = (pVisLevel->nDrlgType == DRLGTYPE_PRESET))

DRLGOUTDOORS_SpawnAct1DirtPaths(pLevel)         [Act1 — construit pVertices[]+pPathStarts[]]
  │
  ├─ nVertices = 0
  ├─ Phase 1 : vertex spéciaux depuis pRoomData
  │   ROGUEENCAMPMENT → coords hardcodées selon nDirection
  │   MONASTERYGATE   → coord fixe (nPosX+27, nPosY+13), direction ALTDIR_NORTH
  ├─ Phase 2 : scan grille nGridWidth × nGridHeight (codes DS1 outdoor, voir §3.9)
  │   → nDirection = 4 par défaut ; vertex ajouté seulement si nDirection ≠ 4
  ├─ DRLGOUTDOORS_CalculatePathCoordinates() × nVertices → pVertices[6+i]
  ├─ sub_6FD7F5B0()                            → pVertices[18..23] (ponts/rivières)
  └─ sub_6FD80750() × nVertices               → pPathStarts[i]
```

### 3.2 Tableaux de connexions par acte

Source : [DrlgOutPlace.cpp](source/D2Common/src/Drlg/DrlgOutPlace.cpp#L29).

#### Acte 1 — Wilderness (`gAct1WildernessDrlgLink`)
```
Index | pfLinker        | Level                  | Parent
  0   | sub_6FD81330    | LEVEL_STONYFIELD       | -1 (racine fixe)
  1   | sub_6FD81380    | LEVEL_COLDPLAINS       |  0
  2   | sub_6FD81950    | LEVEL_BLOODMOOR        |  1
  3   | sub_6FD81720    | LEVEL_ROGUEENCAMPMENT  |  2
  4   | sub_6FD81380    | LEVEL_BURIALGROUNDS    |  1
  5   | NULL            | (sentinel)             | -1
```
```
STONYFIELD ←── COLDPLAINS ←── BURIALGROUNDS
                    └──────── BLOODMOOR ←── ROGUEENCAMPMENT
```

#### Acte 1 — Monastery (`gAct1MonasteryDrlgLink`)
```
Index | pfLinker        | Level                  | Parent
  0   | sub_6FD81330    | LEVEL_MOOMOOFARM       | -1 (racine fixe)
  1   | sub_6FD81330    | LEVEL_MONASTERYGATE    | -1 (racine fixe)
  2   | sub_6FD81AD0    | LEVEL_TAMOEHIGHLAND    |  1
  3   | sub_6FD81380    | LEVEL_BLACKMARSH       |  2
  4   | sub_6FD81380    | LEVEL_DARKWOOD         |  3
  5   | NULL            | (sentinel)             | -1
```
Deux racines indépendantes : MooMoo Farm et Monastery Gate.

#### Acte 2 — Desert (`gAct2OutdoorDrlgLink`)
```
Index | pfLinker        | Level                  | Parent
  0   | sub_6FD81330    | LEVEL_LUTGHOLEIN       | -1
  1   | sub_6FD81B30    | LEVEL_ROCKYWASTE       |  0  (2 positions)
  2   | sub_6FD81530    | LEVEL_DRYHILLS         |  1  (8 directions)
  3   | sub_6FD81530    | LEVEL_FAROASIS         |  2
  4   | sub_6FD81530    | LEVEL_LOSTCITY         |  3
  5   | sub_6FD81BF0    | LEVEL_VALLEYOFSNAKES   |  4
  6   | NULL            | (sentinel)             | -1
```

#### Acte 2 — Canyon (`gAct2CanyonDrlgLink`)
```
  0   | sub_6FD81330    | LEVEL_CANYONOFTHEMAGI  | -1 (isolé)
```

#### Acte 4 — Outer Steppes (`gAct4OutdoorDrlgLink`)
```
Index | pfLinker        | Level                       | Parent
  0   | sub_6FD81330    | LEVEL_THEPANDEMONIUMFORTRESS | -1
  1   | sub_6FD81CA0    | LEVEL_OUTERSTEPPES          |  0
  2   | sub_6FD81380    | LEVEL_PLAINSOFDESPAIR        |  1
  3   | sub_6FD81380    | LEVEL_CITYOFTHEDAMNED        |  2
```

#### Acte 4 — Chaos Sanctum (`gAct4ChaosSanctumDrlgLink`)
```
  0   | sub_6FD81330    | LEVEL_CHAOSSANCTUM     | -1 (isolé)
```

#### Acte 5 — Outdoor (`gAct5OutdoorDrlgLink`)
```
Index | pfLinker                           | Level                      | Parent
  0   | sub_6FD81330                       | LEVEL_HARROGATH            | -1
  1   | sub_6FD81330                       | LEVEL_BLOODYFOOTHILLS      |  0
  2   | DRLGOUTROOM_LinkLevelsByLevelCoords | LEVEL_ID_ACT5_BARRICADE_1 |  1
  3   | DRLGOUTROOM_LinkLevelsByOffsetCoords| LEVEL_ARREATPLATEAU       |  2
```

#### Acte 5 — Tundra (`gAct5TundraDrlgLink`)
```
  0   | DRLGOUTROOM_LinkLevelsByLevelDef   | LEVEL_TUNDRAWASTELANDS     | -1
```

### 3.3 Fonctions de positionnement (pfLinker)

| Fonction | Directions | Masque RNG | Utilisée pour |
|---|---|---|---|
| `sub_6FD81330` | Fixe (LevelDef offset) | — | Racines, positions fixes |
| `sub_6FD81380` | 4 | `nRand & 3` | A1 Cold Plains, Burial Grounds, A4 |
| `sub_6FD81530` | 8 | `nRand & 7` | A2 Dry Hills, Far Oasis, Lost City |
| `sub_6FD81720` | Dual | — | A1 Rogue Encampment |
| `sub_6FD81950` | 8 (2 modes × 4) | `(nRand & 3) + nRand2 & 1` | A1 Blood Moor |
| `sub_6FD81AD0` | Offset fixe | — | A1 Tamoe Highland |
| `sub_6FD81B30` | 2 | `nRand & 1` | A2 Rocky Waste |
| `sub_6FD81BF0` | 8 | `nRand & 7` | A2 Valley of Snakes |
| `sub_6FD81CA0` | 2 modes | — | A4 Outer Steppes (global flag) |
| `DRLGOUTROOM_LinkLevelsByLevelCoords` | Relatif au parent | — | A5 Barricade |
| `DRLGOUTROOM_LinkLevelsByOffsetCoords` | Table statique | — | A5 Arreat Plateau |
| `DRLGOUTROOM_LinkLevelsByLevelDef` | Via DATATBLS | — | A5 Tundra |

### 3.4 Le tableau de 24 vertices (`pVertices[24]`)

```
pVertices[0..5]    Initial perimeter vertices
                   → générés par DRLGVER_CreateVertices()
                   → points sur la frontière du level détectant une connexion
                   → nDirection indique le côté (ALTDIR_WEST=0..ALTDIR_SOUTH=3)

pVertices[6..11]   Adjusted exit points
                   → calculés par DRLGOUTDOORS_CalculatePathCoordinates()
                   → alignés sur la grille 8×8, décalés de +11 (W/N) ou -5 (E/S)
                   → point de départ réel du chemin sur ce level

pVertices[12..17]  Destination endpoints (coords du level voisin)
                   → fournis par la fonction de linking du level voisin
                   → point d'arrivée du chemin dans l'espace mondial

pVertices[18..23]  Special/bridge vertices
                   → peuplés par sub_6FD7F5B0 si flag OUTDOOR_BRIDGE/OUTDOOR_RIVER
                   → direction nDirection = ALTDIR_EAST (2) pour les ponts
```

### 3.5 Calcul des points ajustés (DRLGOUTDOORS_CalculatePathCoordinates)

```cpp
// pVertices[6+i] = exit point ajusté à partir de pVertices[i]
int relX = pVertices[i].nPosX - pLevel->nPosX;
int relY = pVertices[i].nPosY - pLevel->nPosY;

switch (pVertices[i].nDirection) {
    case ALTDIR_WEST:  // 0  → frontière gauche
        pVertices[6+i].nPosX = 8 * (relX / 8) + 11; break;
    case ALTDIR_NORTH: // 1  → frontière haute
        pVertices[6+i].nPosY = 8 * (relY / 8) + 11; break;
    case ALTDIR_EAST:  // 2  → frontière droite
        pVertices[6+i].nPosX = 8 * (relX / 8) - 5;  break;
    case ALTDIR_SOUTH: // 3  → frontière basse
        pVertices[6+i].nPosY = 8 * (relY / 8) - 5;  break;
}
pVertices[6+i].nPosX += pLevel->nPosX;
pVertices[6+i].nPosY += pLevel->nPosY;
```

### 3.6 Pathfinding entre points (sub_6FD80750)

```
Entrée : pVertices[6+i] (départ)  et  pVertices[12+i] (arrivée)
         → convertis en coords grille : (posX - pLevel->nPosX) / 8

Distance Manhattan < 2 → connexion directe (2 noeuds)
Sinon              → A*/Dijkstra, max 900 waypoints, max 35 itérations d'expansion

Résultat : pPathStarts[i] → liste chaînée de D2DrlgVertexStrc
           chaque noeud contient (nPosX, nPosY) en coords monde
```

### 3.7 Flags de grille (`pGrid[2]`)

```cpp
union D2DrlgOutdoorPackedGrid2InfoStrc {
    uint32_t nPackedValue;
    struct {
        uint32_t nUnkb00 : 1;        // 0x00000001
        uint32_t bHasDirection : 1;  // 0x00000002 — direction non nulle
        uint32_t nUnkb02 : 5;        // 0x0000007C
        uint32_t nUnkb07 : 1;        // 0x00000080 — spawn preset area
        uint32_t nUnkb08 : 1;        // 0x00000100 — cellule vide ?
        uint32_t bHasPickedFile : 1; // 0x00000200 — preset spawné
        uint32_t bLvlLink : 1;       // 0x00000400 — PASSAGE (level link)
        uint32_t nUnkb11 : 3;        // 0x00001800 + 0x00001000
        uint32_t nUnkb13 : 3;        // 0x0000E000
        uint32_t nPickedFile : 4;    // 0x000F0000 — ID preset
        uint32_t nUnkb20 : 12;       // 0xFFF00000
    };
};
// Toutes les cellules de pPathStarts[i] ont bLvlLink = 1
```

### 3.8 Lire les passages pour l'automap

À n'importe quel moment après `DRLG_InitLevel` :

```cpp
// Données disponibles (blueprint time)
D2DrlgOutdoorInfoStrc* pOutdoors = pLevel->pOutdoors;  // non-null si DRLGTYPE_OUTDOOR
int nPassages = pOutdoors->nVertices;                   // 0..6 (inclut les cave-entrance vertices)

int nOut = 0;
for (int i = 0; i < nPassages; ++i) {
    const D2DrlgVertexStrc* pVert = &pOutdoors->pVertices[i];

    // Résoudre le level voisin via pRoomData, en filtrant bPreset et bInit.
    // sans ce filtre, les cave-entrance vertices (grille 51/52) seraient émis
    // comme des passages piétons avec la mauvaise direction (0 ou 1).
    int nDestLevelId = -1;
    for (D2DrlgOrthStrc* pOrth = pOutdoors->pRoomData; pOrth; pOrth = pOrth->pNext) {
        if (pOrth->bInit || pOrth->bPreset)          // ignorer supprimés ET presets (caves)
            continue;
        if (pOrth->nDirection == pVert->nDirection) {
            nDestLevelId = pOrth->pLevel->nLevelId;
            break;
        }
    }
    if (nDestLevelId < 0)                            // aucun orth piéton ne correspond → cave
        continue;

    int tileX = pVert->nPosX;
    int tileY = pVert->nPosY;
    // → subtiles = tile * 5
    // → minimap  = 8*(tileX-tileY)+1, 4*(tileX+tileY)-3
    ++nOut;
}
```

> **ATTENTION — cette approche est insuffisante (voir §3.11 pour la version correcte).**

### 3.9 Codes de grille DS1 outdoor et vertices de cave-entrance

`DRLGOUTDOORS_SpawnAct1DirtPaths` (Phase 2) scan toutes les cellules de `pGrid[0]`
(taille `nGridWidth × nGridHeight`, une cellule = 8 tiles outdoor).
Chaque cellule a un code entier (`nGrid0Entry`) et un `nPickedFile` (fichier de preset tiré).
Un vertex est ajouté si et seulement si `nDirection ≠ 4` après le switch :

| Code grille | Condition | `nDirection` attribué | Type |
|---|---|---|---|
| `4` | `nPickedFile == 3` | 3 (SOUTH) | Passage de bordure |
| `5` | `nPickedFile == 3` | 0 (WEST) | Passage de bordure |
| `6` | `nPickedFile == 3` | 1 (NORTH) | Passage de bordure |
| `7` | `nPickedFile == 3` | 2 (EAST) | Passage de bordure |
| `24` | — | 1 (NORTH) | Passage de bordure |
| `25` | — | 0 (WEST) | Passage de bordure |
| `28` | `nPickedFile==1 && i==(nGridWidth-2)` | 2 (EAST) | Passage de bordure |
| **`51`** | — | `nPickedFile != 0` (0 ou 1) | **Cave/donjon entrance** |
| **`52`** | — | `nPickedFile != 0` (0 ou 1) | **Cave/donjon entrance** |

**Codes 51 et 52 — cellules de transition vers une cave :**

Ces cellules désignent l'emplacement d'un escalier/entrée de cave (ex. Cave Level 1 en
Cold Plains). Elles ajoutent un vertex avec `nDirection = (nPickedFile != 0)`, c'est-à-dire
0 ou 1 — les mêmes directions que des passages piétons ordinaires.

C'est la source du problème : sans filtrage, on émettrait ces vertices comme des passages
alors qu'ils correspondent à une entrée de donjon (`bPreset=TRUE` dans `pRoomData`).

Le vertex de cave est facilement détectable : **aucun `D2DrlgOrthStrc` avec `bPreset=FALSE`
n'a la même `nDirection`** que ce vertex. Le filtre dans §3.8 exploite exactement ce fait.

**Cependant**, ce test est insuffisant dès que la direction du vertex cave (0 ou 1) coïncide
avec la direction d'un voisin outdoor réel — les deux vertices trouvent le même orth et
sont tous les deux émis. Voir §3.11 pour la correction complète.

### 3.11 Approche correcte — itérer par orth, pas par vertex

Le problème de §3.8 : si un vertex cave-entrance a la même direction que Blood Moor (ex. WEST=0),
les deux vertices (cave + border exit Blood Moor) trouvent le même orth Blood Moor → deux sorties
émises pour le même voisin, dont une fausse.

**Solution** : itérer `nVis[]/nWarp[]` directement (source toujours complète) plutôt que
`pRoomData` (qui peut être incomplet — voir ci-dessous). Pour chaque voisin outdoor,
calculer la direction via `DRLG_GetDirectionFromCoordinates`, puis choisir parmi tous
les vertices de même direction celui qui est le plus **extrême vers la bordure**.

**Pourquoi `pRoomData` est incomplet pour Frigid Highlands (Act 5) :**

```
sub_6FD82750(LEVEL_ID_ACT5_BARRICADE_1, LEVEL_ARREATPLATEAU)  ← construit pRoomData
                                                                    pour Frigid Highlands
sub_6FD826D0(LEVEL_BLOODYFOOTHILLS, LEVEL_ID_ACT5_BARRICADE_1) ← ajoute Bloody Foothills
                                                                    dans nVis[] de Frigid
                                                                    Highlands TROP TARD
```

Résultat : Frigid Highlands a Bloody Foothills dans `nVis[]` mais PAS dans `pRoomData`.
En itérant `nVis[]/nWarp[]` directement, la connexion est toujours trouvée.

Si aucun vertex n'est disponible (Act 2/3/4/5 — `SpawnAct1DirtPaths` jamais appelé,
`nVertices==0`), on calcule le midpoint de la frontière commune entre les deux bounding
boxes.

```cpp
// Obtenir les tableaux nVis[]/nWarp[] pour ce level.
// Ces tableaux sont toujours complets à query time (contrairement à pRoomData).
int* pVisArray  = DRLGROOM_GetVisArrayFromLevelId(pLevel->pDrlg, pLevel->nLevelId);
int* pWarpArray = DRLGWARP_GetWarpIdArrayFromLevelId(pLevel->pDrlg, pLevel->nLevelId);
if (!pVisArray || !pWarpArray) return 0;

int nOut = 0;
for (int j = 0; j < 8; ++j) {
    if (!pVisArray[j] || pWarpArray[j] != -1)
        continue;                               // slot vide ou portail — pas un passage piéton

    D2DrlgLevelStrc* pAdj = DRLG_GetLevel(pLevel->pDrlg, pVisArray[j]);
    if (!pAdj) continue;

    const int nDirection = DRLG_GetDirectionFromCoordinates(&pLevel->pLevelCoords, &pAdj->pLevelCoords);

    const D2DrlgVertexStrc* pBestVert = nullptr;
    for (int i = 0; i < pOutdoors->nVertices; ++i) {
        const D2DrlgVertexStrc* pVert = &pOutdoors->pVertices[i];
        if (pVert->nDirection != nDirection) continue;
        if (!pBestVert) { pBestVert = pVert; continue; }
        switch (nDirection) {                   // extremum vers la bordure
        case ALTDIR_WEST:  if (pVert->nPosX < pBestVert->nPosX) pBestVert = pVert; break;
        case ALTDIR_NORTH: if (pVert->nPosY < pBestVert->nPosY) pBestVert = pVert; break;
        case ALTDIR_EAST:  if (pVert->nPosX > pBestVert->nPosX) pBestVert = pVert; break;
        case ALTDIR_SOUTH: if (pVert->nPosY > pBestVert->nPosY) pBestVert = pVert; break;
        }
    }
    if (!pBestVert) {
        // Fallback Act 2/3/4/5 : midpoint de la frontière commune.
        const D2DrlgCoordStrc* pAdjCoords = &pAdj->pLevelCoords;
        ...
        pPassageDestLevelId[nOut] = pAdj->nLevelId;
        ++nOut;
        continue;
    }

    // pBestVert est le border exit réel
    pPassageSubX[nOut]        = pBestVert->nPosX * 5;
    pPassageSubY[nOut]        = pBestVert->nPosY * 5;
    pPassageDestLevelId[nOut] = pAdj->nLevelId;
    ++nOut;
}
```

**Garanties** :
- Exactement un résultat par voisin outdoor (y compris `bPreset=TRUE` comme Monastery Gate)
- Ne nécessite pas de connaître le code grille d'origine du vertex (51/52 vs 24/25/etc.)
- Robuste quelle que soit la direction assignée au vertex cave
- Fonctionne pour tous les actes y compris Act 3/4/5 (via fallback bounding-box)
- Capture les connexions ajoutées après `sub_6FD82750` (Frigid Highlands → Bloody Foothills)

### 3.12 Résumé du pipeline complet pour les passages outdoor

```
DRLG_InitLevel
  ↓
sub_6FD82750 : construit pRoomData
   nVis[j]!=0 && nWarp[j]==-1 → DRLGROOM_AddOrth
   bPreset = (dest->nDrlgType == DRLGTYPE_PRESET)
  ↓
DRLGOUTDOORS_SpawnAct1DirtPaths : construit pVertices[0..nVertices-1]  [Act 1 uniquement]
   Phase 1 (pRoomData) : ROGUEENCAMPMENT, MONASTERYGATE (bPreset=TRUE → inclus quand même)
   Phase 2 (grille)   : codes 4/5/6/7/24/25/28 → passages  bPreset=FALSE
                        codes 51/52             → caves      bPreset=TRUE
  ↓
Automap query time (approche nVis/nWarp directe, voir §3.11) :
   pVisArray  = DRLGROOM_GetVisArrayFromLevelId(pLevel->pDrlg, pLevel->nLevelId)
   pWarpArray = DRLGWARP_GetWarpIdArrayFromLevelId(pLevel->pDrlg, pLevel->nLevelId)
   pour chaque slot j où pVis[j]!=0 && pWarp[j]==-1 :
     pAdj = DRLG_GetLevel(pLevel->pDrlg, pVis[j])
     nDir = DRLG_GetDirectionFromCoordinates(&pLevel->pLevelCoords, &pAdj->pLevelCoords)
     chercher pBestVert dans pVertices[] : même nDirection, le plus extrême vers la bordure
     → pBestVert trouvé (Act 1)              : émettre (nPosX*5, nPosY*5) comme passage
     → pBestVert absent (Act 2/3/4/5, nVertices==0) :
         calculer midpoint de la frontière commune (bounding-box fallback)
         émettre le midpoint*5 comme passage
```

> **Pourquoi itérer `nVis/nWarp` et non `pRoomData` :** Pour Frigid Highlands (Act 5),
> `sub_6FD82750(BARRICADE_1, ARREATPLATEAU)` construit `pRoomData` AVANT que
> `sub_6FD826D0(BLOODYFOOTHILLS, BARRICADE_1)` n'ajoute Bloody Foothills dans `nVis[]`.
> `pRoomData` pour Frigid Highlands ne contient donc que Arreat Plateau.
> En lisant `nVis[]` à query time (toujours complet), les deux passages sont émis.

---

## 4. Système 2 — Warp tiles

### 4.1 Ce qui est disponible au moment du blueprint (sans activation)

| Donnée | Disponible | Structure | Remarque |
|---|---|---|---|
| `pMaze->pMap->pFile` | ✅ OUI (si `dwScan=1` ou `dwPops>0`) | `D2DrlgMapStrc` | Chargé par `DRLGPRESET_LoadDrlgFile` dans `DRLGPRESET_BuildPresetArea` |
| `pDrlg->pWarp->nWarp[]` | ✅ OUI | `D2DrlgWarpStrc` | Rempli pendant `DRLG_InitLevel` |
| `DATATBLS_GetLevelDefRecord()->dwWarp[]` | ✅ OUI | `D2LevelDefBin` | Table statique (fallback) |
| `pLevel->nRoom_Center_Warp_X/Y[]` | ✅ OUI | `D2DrlgLevelStrc` | Calculé par `DRLG_ComputeLevelWarpInfo` |
| `pDrlgRoom->nTileXPos/Y/Width/Height` | ✅ OUI | `D2DrlgRoomStrc` | Dimensions en tiles |
| `pDrlgRoom->dwFlags` bits `HAS_WARP_*` | ⚠️ PARTIEL | `D2DrlgRoomStrc` | Voir §4.8 — **ne peut pas être utilisé comme filtre de rooms portail** |
| `pDrlgRoom->pRoomTiles` | ❌ LAZY | — | Peuplé dans `sub_6FD77BB0` → `DrlgActivate.cpp` |
| `pDrlgRoom->pPresetUnits` (UNIT_TILE) | ❌ LAZY | — | Via `DRLGROOMTILE_AddWarp` → `DRLGACTIVATE_RoomEx_EnsureHasRoom` |
| `pDrlgRoom->pRoom` | ❌ LAZY | — | Créé dans `DRLG_CreateRoomForRoomEx` → activation |

### 4.2 Résolution des deux tableaux parallèles nVis / nWarp

Pour chaque level, il y a **deux** tableaux de 8 slots, toujours utilisés **en parallèle** :

- `nVis[i]` / `dwVis[i]` → level ID réel du voisin (0 = slot vide)
- `nWarp[i]` / `dwWarp[i]` → index LvlWarpTxt (-1 = passage piéton, ≥0 = portail/escalier)

Il faut résoudre les deux en même temps :

```cpp
// Résolution prioritaire depuis la liste chaînée runtime
int* pWarpArray = nullptr;  // nWarp[] = index LvlWarpTxt
int* pVisArray  = nullptr;  // nVis[]  = level IDs réels
for (D2DrlgWarpStrc* w = pLevel->pDrlg->pWarp; w; w = w->pNext) {
    if (w->nLevel == pLevel->nLevelId) {
        pWarpArray = w->nWarp;
        pVisArray  = w->nVis;
        break;
    }
}

// Fallback : table statique LevelDef (mêmes conventions de nommage)
if (!pWarpArray) {
    D2LevelDefBin* pDef = DATATBLS_GetLevelDefRecord(pLevel->nLevelId);
    pWarpArray = (int*)pDef->dwWarp;
    pVisArray  = (int*)pDef->dwVis;
}

// Usage pour un slot i :
const int nWarpTableIdx = pWarpArray[i];  // index LvlWarpTxt (-1 = pas de portail)
const int nDestLevelId  = pVisArray[i];   // vrai level ID destination (0 = vide)

if (nWarpTableIdx < 0 || nDestLevelId <= 0)
    continue;  // slot vide ou passage piéton

// Obtenir l'offset de placement depuis LvlWarpTxt
D2LvlWarpTxt* pWarpTxt = DATATBLS_GetLvlWarpTxtRecordFromLevelIdAndDirection(nWarpTableIdx, szDir);
// pWarpTxt->dwOffsetX/Y → décalage en subtiles depuis la tile du portail
// nDestLevelId            → level de destination (PAS pWarpTxt->dwLevelId !)
```

### 4.3 Itérer les warps d'un level

```cpp
// Disponible dès blueprint time — pas d'activation nécessaire
for (D2DrlgRoomStrc* pDrlgRoom = pLevel->pFirstRoomEx;
     pDrlgRoom; pDrlgRoom = pDrlgRoom->pDrlgRoomNext)
{
    if (!(pDrlgRoom->dwFlags & DRLGROOMFLAG_HAS_WARP_MASK))
        continue;

    // Position approchée = centre de la DrlgRoom hôte, en subtiles
    int subX = (pDrlgRoom->nTileXPos + pDrlgRoom->nTileWidth  / 2) * 5;
    int subY = (pDrlgRoom->nTileYPos + pDrlgRoom->nTileHeight / 2) * 5;

    // Un seul DrlgRoom peut porter plusieurs bits HAS_WARP_*
    int warpIndex = 0;
    for (int mask = DRLGROOMFLAG_HAS_WARP_0;
         mask & DRLGROOMFLAG_HAS_WARP_MASK;
         mask <<= 1, ++warpIndex)
    {
        if (!(pDrlgRoom->dwFlags & mask)) continue;

        // nWarp[warpIndex] = index LvlWarpTxt (pas un level ID !)
        // nVis[warpIndex]  = vrai level ID destination
        const int nWarpTableIdx = pWarpArray[warpIndex];
        const int nDestLevelId  = pVisArray[warpIndex];
        if (nWarpTableIdx < 0 || nDestLevelId <= 0) continue;

        // → (subX, subY, nDestLevelId) est un warp valide
    }
}
```

### 4.4 `DRLG_ComputeLevelWarpInfo` et `nRoom_Center_Warp_X/Y`

`D2DrlgLevelStrc` contient :

```cpp
int32_t nRoom_Center_Warp_X[9];  // +0x1C8  position X centre de la room warp (en tiles), par index
int32_t nRoom_Center_Warp_Y[9];  // +0x1EC  position Y
int32_t nRoomCoords;             // +0x210  nombre d'entrées valides
```

`DRLG_ComputeLevelWarpInfo` (appelé en fin de `DRLG_InitLevel`) parcourt les DrlgRooms avec
`HAS_WARP_*` exactement comme le code ci-dessus et stocke le centre en tiles (pas en subtiles).
C'est la **source de vérité** pour la position approximative des warps.

### 4.5 Position exacte — requiert l'activation

Pour obtenir la position exacte de l'arche/portail (pas seulement le centre de la room) :

```cpp
// Après DRLGACTIVATE_InitializeRoomEx(pDrlgRoom) :
for (D2PresetUnitStrc* p = pDrlgRoom->pPresetUnits; p; p = p->pNext) {
    if (p->nUnitType == UNIT_TILE) {
        // Position absolue en subtiles
        int subX = pDrlgRoom->nTileXPos * 5 + p->nXpos;
        int subY = pDrlgRoom->nTileYPos * 5 + p->nYpos;
    }
}
```

### 4.6 `D2C_PackedTileInformation` — décodage des tiles DS1

Chaque entrée dans `pFile->pWallLayer[i]` est un `uint32_t` qui encode plusieurs informations :

```cpp
union D2C_PackedTileInformation   // source : D2DrlgRoomTile.h
{
    uint32_t nPackedValue;
    struct {
        uint32_t bIsWall       : 1;  // BIT( 0)  mur
        uint32_t bIsFloor      : 1;  // BIT( 1)  sol
        uint32_t bLOS          : 1;  // BIT( 2)  généré par code (bords)
        uint32_t bEnclosed     : 1;  // BIT( 3)  délimite une enclosure/arbres
        uint32_t bExit         : 1;  // BIT( 4)  arche ou ouverture dans un mur
        /* ... */
        uint32_t nTileSequence : 8;  // BIT(8-15)  variante de frame (AKA tile subindex)
        /* ... */
        uint32_t nTileStyle    : 6;  // BIT(20-25) style/index de warp (AKA tile index)
        /* ... */
        uint32_t bHidden       : 1;  // BIT(31)    tile invisible en jeu (trigger logique)
    };
};
```

**`nTileStyle`** (bits 20–25) — l'**index de slot warp** dans le DS1 (0 à 7).
C'est cet index qui sert à lire `pWarpArray[nTileStyle]` et `pVisArray[nTileStyle]`
pour obtenir respectivement le row LvlWarpTxt et le level ID destination.
Les styles ≥ 8 sont des POPs (un autre système, indépendant des warps).

**`nTileSequence`** (bits 8–15) — la **variante de frame** pour un style donné.
Pour un warp (même `nTileStyle`), le DS1 peut contenir plusieurs tiles :

| nTileSequence | bHidden | Signification |
|---|---|---|
| 0 | 0 | Tile visuel **principal** (les escaliers/arche visibles) |
| 1, 2, 3, … | 0 | Tiles visuels secondaires (bordures/décorations adjacentes) |
| 4 | 0 | Variante d'orientation alternative du tile principal |
| ≥ 8 (souvent 21+) | 1 | Tile **invisible** de trigger logique (le plus courant dans les DS1 standard) |

Exemple concret — Jail Level 1 niveau 29, room JailEW.ds1, style 0 (→ Barracks) :
```
(3539,1018)  type=RIGHT_EXIT  bHidden=0  nTileSeq= 0  → tile visuel principal ★
(3540,1018)  type=RIGHT_EXIT  bHidden=0  nTileSeq= 1  → tile visuel secondaire
(3566,1018)  type=LEFT_EXIT   bHidden=1  nTileSeq=21  → trigger invisible
```

**`bHidden`** (bit 31) — tile **invisible** en jeu.
La plupart des DS1 modernes utilisent un tile caché (`bHidden=1`) comme trigger logique,
car le moteur l'utilise pour créer `pRoomTiles` et positionner le preset unit UNIT_TILE.

### 4.7 Condition d'acceptation exacte d'un tile warp (blueprinttime)

La référence est `DRLGPRESET_BuildPresetArea` (`DrlgPreset.cpp`) qui détermine
quels tiles sont des warps valides lors de l'initialisation du DRLG :

```cpp
// Condition originale dans DRLGPRESET_BuildPresetArea :
// nGrid2FlagsByte1 = nTileSequence  (BYTE1 du packed int = bits 8-15)
// nGrid2UpperBit   = bit 31         (= bHidden)
if (dwScan && nStyle <= 7 && (nTileSequence == 0 || nTileSequence == 4 || bHidden))
```

Traduit en termes de `D2C_PackedTileInformation` :

```cpp
// Un tile EXIT est un warp valide si :
bool bIsWarp = (nTileInfo.nTileStyle < 8)
            && (nTileInfo.bHidden
                || nTileInfo.nTileSequence == 0
                || nTileInfo.nTileSequence == 4);
```

> **⚠️ PIÈGE — vérifier seulement `bHidden` est insuffisant :**
> Le tile visuel principal (`nTileSeq=0, bHidden=0`) est aussi un warp valide.
> C'est précisément ce cas qui fait manquer la staircase Jail→Barracks :
> son tile principal a `bHidden=0, nTileSeq=0` et serait ignoré par un filtre `!bHidden` pur.

### 4.8 `DRLGROOMFLAG_HAS_WARP_*` — signification réelle et limitations

```cpp
// DRLGPRESET_BuildPresetArea :
for (int i = 0; i < 8; ++i) {
    if (pVisArray[i] && DRLGWARP_GetWarpDestinationFromArray(pLevel, i) == -1) {
        nFlags |= (DRLGROOMFLAG_HAS_WARP_0 << i);
    }
}
```

Ce code pose le flag HAS_WARP_i quand `vis[i] != 0 AND warp[i] == -1`.
`warp[i] == -1` signifie **passage piéton** (pas de portail/escalier).
Donc ces flags indiquent les **connexions de passage piéton visibles** depuis cette room,
**pas** les portails/warps tiles. Ils sont utilisés par `sub_6FD77BB0` pour lier
les rooms adjacentes au moment de l'activation.

Deux raisons pour lesquelles ce flag **ne peut pas servir de filtre** dans le scan warp :

1. **Pour la Jail→Barracks (`warp[0]=13, vis[0]=28`)** : `warp[0] != -1` → flag HAS_WARP_0
   jamais mis, même si la room contient physiquement les escaliers.

2. **Pour les levels où `dwScan=0`** : `DRLGPRESET_BuildPresetArea` ne charge pas le DS1
   → n'entre pas dans la boucle de scan → flag jamais propagé aux rooms.

**Conclusion** : pour trouver les rooms avec warp tiles, il faut scanner directement
le DS1 (`pFile->pTileTypeLayer` + `pFile->pWallLayer`) sans se fier à `HAS_WARP_MASK`.

### 4.9 Stratégie de scan DS1 (approche blueprint)

```
Pour chaque DrlgRoom de type DRLGTYPE_PRESET :
│
├─ pMap->pFile est non-null ?
│   OUI (dwScan=1 ou dwPops>0) → utiliser directement
│   NON (dwScan=0, pas de POPs) → DRLGPRESET_LoadDrlgFile / FreeDrlgFile
│
└─ Pour chaque wall layer i de pFile :
     Pour chaque tile (nX, nY) de la room :
       idx = (nRelY + nY) * (pMap->nWidth + 1) + (nRelX + nX)
       nTileType = pFile->pTileTypeLayer[i][idx]
       Si nTileType != WALL_LEFT_EXIT et != WALL_RIGHT_EXIT → skip

       nTileInfo = D2C_PackedTileInformation{ pFile->pWallLayer[i][idx] }
       Si nTileInfo.nTileStyle >= 8 → skip (POP, pas un warp)
       Si !nTileInfo.bHidden AND nTileInfo.nTileSequence != 0
                              AND nTileInfo.nTileSequence != 4 → skip
       Si déjà émis ce style pour cette room → skip (nFoundMask)

       nWarpTableIdx = pWarpArray[nTileStyle]   si < 0 → skip
       nDestLevelId  = pVisArray[nTileStyle]    si <= 0 → skip
       szDir = nTileType == WALL_RIGHT_EXIT ? 'r' : 'l'
       pWarpTxt = DATATBLS_GetLvlWarpTxtRecordFromLevelIdAndDirection(nWarpTableIdx, szDir)
       Si null → skip

       subX = (nTileXPos + nX) * 5 + pWarpTxt->dwOffsetX
       subY = (nTileYPos + nY) * 5 + pWarpTxt->dwOffsetY
       → émettre (subX, subY, nDestLevelId)
```

**`nRelX` / `nRelY`** — décalage de la room dans l'espace plat du fichier DS1 :
```cpp
const int nRelX = pDrlgRoom->nTileXPos - pMap->pDrlgCoord.nPosX;
const int nRelY = pDrlgRoom->nTileYPos - pMap->pDrlgCoord.nPosY;
```

**`nFoundMask`** (uint32_t, 1 bit par style 0-7) — évite les doublons quand plusieurs
wall layers ou tiles du même style `nTileStyle` sont présents dans la même room.

---

## 5. Systèmes de coordonnées

```
TILES (coords monde, unité du DRLG)
   nPosX / nPosY dans D2DrlgLevelStrc, D2DrlgRoomStrc, D2DrlgVertexStrc

SUBTILES = tiles × 5
   unité de D2PathStrc (pDynamicPath / pStaticPath)
   D2PresetUnitStrc::nXpos/nYpos = relatif à la room, en subtiles

CLIENT = subtiles → isométrique
   clientX =  16 * (subX - subY)
   clientY =   8 * (subX + subY)
   (DUNGEON_GameSubtileToClientCoords)

MINIMAP = client → écran minimap
   mapX = clientX / 10 + 1
   mapY = clientY / 10 - 3

GRILLE outdoor = coords monde / 8
   gridX = (worldX - pLevel->nPosX) / 8   (pour pGrid[])
   worldX = gridX * 8 + pLevel->nPosX
```

### Formule directe tile → minimap
```
mapX = 8 * (tileX - tileY) + 1
mapY = 4 * (tileX + tileY) - 3
```

### Formule directe subtile → minimap
```
mapX = (clientX) / 10 + 1  =  UNITS_GetClientCoordX(unit) / 10 + 1
mapY = (clientY) / 10 - 3  =  UNITS_GetClientCoordY(unit) / 10 - 3
```

---

## 6. Référence implémentation automap

Fichier : [automap_outdoor_line.cpp](automap_outdoor_line.cpp)

### Vue d'ensemble des constantes

```cpp
#define OUTDOOR_PASSAGES_MAX 6   // pPathStarts[6] — passages piétons max par level
#define WARP_TILES_MAX       8   // DRLGROOMFLAG_HAS_WARP_0..7 — warps max par level
// Les deux systèmes sont INDÉPENDANTS, leurs comptes ne s'additionnent pas.
```

### `AutomapGetOutdoorPassagesMinimapCoords`

```
Entrée  : pPlayer, pRoom
Sortie  : pPlayerMapX/Y, pPassageMapX[OUTDOOR_PASSAGES_MAX], pPassageMapY[...]
Retour  : nombre de passages (0 si level non-outdoor ou données absentes)

Source  : pLevel->pOutdoors->pVertices[i].nPosX/Y  (tiles, blueprint time)
Formule : mapX = 8*(tileX-tileY)+1,  mapY = 4*(tileX+tileY)-3
```

### `AutomapGetOutdoorPassagesGameCoords`

```
Entrée  : pPlayer, pRoom
Sortie  : pPlayerSubX/Y, pPassageSubX[OUTDOOR_PASSAGES_MAX], pPassageSubY[...]
Retour  : nombre de passages

Source  : même que ci-dessus × 5 pour les passages
Joueur  : dispatch UNIT_OBJECT/ITEM/TILE → pStaticPath, autres → pDynamicPath
```

### `AutomapGetWarpTileGameCoords`

```
Entrée  : pRoom, pWarpSubX[WARP_TILES_MAX], pWarpSubY[...], pWarpDestLevelId[...]
Retour  : nombre de warps

Approche : scan direct du fichier DS1 brut (pMaze->pMap->pFile),
           disponible dès DRLGPRESET_LoadDrlgFile (blueprint time, sans activation).
           Reproduit la condition de DRLGPRESET_BuildPresetArea (§4.7).
           NE PAS utiliser DRLGROOMFLAG_HAS_WARP_MASK comme filtre (§4.8).

Source  :
  1. Résolution des deux tableaux parallèles (voir §4.2) :
      - pWarpArray = nWarp[] (index LvlWarpTxt) depuis pDrlg->pWarp ou LevelDef
      - pVisArray  = nVis[]  (level IDs réels)  depuis pDrlg->pWarp ou LevelDef
  2. Itération de TOUTES les DrlgRooms DRLGTYPE_PRESET (sans filtre HAS_WARP_MASK) :
      - Résoudre pFile : utiliser pMap->pFile s'il est non-null,
        sinon LoadDrlgFile/FreeDrlgFile (cas dwScan=0 sans POPs).
  3. Scan des wall layers de pFile :
      - nTileType == TILETYPE_WALL_LEFT_EXIT  → direction 'l'
      - nTileType == TILETYPE_WALL_RIGHT_EXIT → direction 'r'
      - Condition d'acceptation (§4.7) :
          nTileStyle < 8  AND  (bHidden OR nTileSeq==0 OR nTileSeq==4)
  4. Pour chaque tile portail acceptée :
      - nWarpTableIdx = pWarpArray[nTileStyle]  (doit être ≥ 0)
      - nDestLevelId  = pVisArray[nTileStyle]   (vrai level ID, pas pWarpTxt->dwLevelId)
      - Offset exact : pWarpTxt->dwOffsetX/Y depuis DATATBLS_GetLvlWarpTxtRecord...
      - Position abs : (nTileXPos + nX) * 5 + dwOffsetX
      - nFoundMask : émettre au plus 1 résultat par style/room
```

### Includes nécessaires

```cpp
#include "source/D2Common/include/DataTbls/LevelsTbls.h"   // D2LevelDefBin, DATATBLS_GetLevelDefRecord
#include "source/D2Common/include/Drlg/D2DrlgDrlg.h"       // D2DrlgWarpStrc, D2DrlgRoomStrc, flags
#include "source/D2Common/include/Drlg/D2DrlgOutdoors.h"   // D2DrlgOutdoorInfoStrc, D2DrlgVertexStrc
#include "source/D2Common/include/Units/Units.h"            // UNITS_GetClientCoordX/Y
#include "source/D2Common/include/Path/Path.h"              // PATH_GetXPosition/YPosition
```

---

## Annexe — Résumé des bugs corrigés dans la doc précédente

| Erreur | Correction |
|---|---|
| `D2DrlgVertexStrc` = 0x18 bytes | 0x14 bytes (code 32-bit) ; 0x18 seulement en 64-bit |
| `pPathStarts[i]` "indices 0-5 = directions W/N/E/S/unused/unused" | Indices non fixes : l'ordre dépend de l'ordre dans `pVertex`, pas d'un mapping direction→index |
| Black Marsh a 3 passages (dont Forgotten Tower) | La Forgotten Tower est un **warp tile** (portail), pas un passage outdoor |
| `OUTDOOR_PASSAGES_MAX + WARP_TILES_MAX` "somme des connexions" | Systèmes totalement indépendants |
| Position warp = `pPresetUnits->nXpos/nYpos` disponible au blueprint | LAZY — seulement après `DRLGACTIVATE_RoomEx_EnsureHasRoom` |
| `pRoomTiles` disponible au blueprint | LAZY — `sub_6FD77BB0` appelé uniquement depuis `DrlgActivate.cpp` |
| `nWarp[i]` = "level ID destination" | **FAUX** — `nWarp[i]` est un **index LvlWarpTxt**. Le vrai level ID est dans `nVis[i]` |
| `D2LvlWarpTxt::dwLevelId` = level ID | **FAUX** — c'est aussi un index LvlWarpTxt (malgré le nom) |
| Cave-entrance vertices (`grille 51/52`) lus comme passages piétons | Ces vertices peuvent avoir la même direction (0 ou 1) qu'un voisin outdoor réel — filtrer `bPreset` seul ne suffit pas. Utiliser l'approche orth-first + extremum (§3.11) |
| Approche vertex-first avec filtre `bPreset` | **Insuffisante** : si cave direction == Blood Moor direction, les deux vertices trouvent le même orth → deux sorties émises. Inverser la boucle : itérer pRoomData, prendre le vertex le plus extrême par direction |
| `pRoomData` contient uniquement les passages piétons | `pRoomData` contient **toutes** les zones avec `nVis[j]!=0 && nWarp[j]==-1`, y compris les caves (bPreset=TRUE) |
| `DRLGROOMFLAG_HAS_WARP_*` identifie les rooms portail | **FAUX** — ces flags sont mis quand `vis[i]!=0 AND warp[i]==-1`, c'est-à-dire pour les **passages piétons**, pas les portails. Pour Jail→Barracks `warp[0]=13 != -1` → flag jamais posé. Ne jamais utiliser comme filtre de rooms warp (§4.8) |
| Filtre `bHidden==1` seul suffit pour détecter les tiles warp | **INSUFFISANT** — `DRLGPRESET_BuildPresetArea` accepte aussi `nTileSeq==0` et `nTileSeq==4` sans `bHidden`. Exemple : staircase Jail→Barracks a `bHidden=0, nTileSeq=0` — ignorée par un filtre `!bHidden` pur. Condition correcte : `bHidden OR nTileSeq==0 OR nTileSeq==4` (§4.7) |
