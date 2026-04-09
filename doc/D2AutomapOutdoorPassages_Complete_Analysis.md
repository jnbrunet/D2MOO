# Passages/Portails entre Levels Outdoor - Analyse Complète

## Vue d'ensemble

Les passages entre levels outdoor (connues sous le nom de "portail" ou "exit point") sont définis par un système de données hiérarchiques multi-niveaux:

1. **Données de connexions statiques** - défier quels levels se connectent entre eux
2. **Structures de positionnement** - calculer où placer les connexions
3. **Système de Vertex** - créer les points d'entrée/sortie et les chemins physiques
4. **Grid de placement** - marquer les cellules du terrain comme points de passage

---

## 1. STRUCTURES DE DONNÉES FONDAMENTALES

### D2DrlgVertexStrc - Point de Passage Individuel (0x18 bytes)
```c
struct D2DrlgVertexStrc {
    int32_t nPosX;                 // +0x00 - Position X globale du passage
    int32_t nPosY;                 // +0x04 - Position Y globale du passage
    uint8_t nDirection;            // +0x08 - Direction (ALTDIR_NORTH=0, ALTDIR_WEST=1, etc)
    uint8_t pad0x09[3];            // +0x09 - Padding
    int32_t dwFlags;               // +0x0C - Flags (bit 0=connexion level, bit 1=special handling)
    D2DrlgVertexStrc* pNext;       // +0x10 - Linked list (doubly-linked circular)
};
```
- Représente UN point de passage unique
- Peut être lié à d'autres vertices pour former un chemin
- Les flags indiquent si c'est une connexion level (bit 0x01) ou un point spécial (bit 0x02)

### D2DrlgOutdoorInfoStrc - Collection de Passages par Level (partie pertinente)
```c
struct D2DrlgOutdoorInfoStrc {
    // ... autres champs ...
    D2DrlgVertexStrc* pVertex;              // +0x64 - Pointeur vers le vertex root (circular linked list)
    D2DrlgVertexStrc* pPathStarts[6];       // +0x68 - 6 pointeurs de départ: 6 passages max par level
    D2DrlgVertexStrc pVertices[24];         // +0x80 - Tableau statique de 24 vertices (0xC0 bytes)
    int32_t nVertices;                      // +0x260 - Nombre de passages actifs
    D2DrlgOrthStrc* pRoomData;              // +0x264 - Pointeur vers les données géographiques du level
};
```

**Organisation des 24 Vertices Statiques:**
- `pVertices[0-5]` - Initial vertices (générés par DRLGVER_CreateVertices)
- `pVertices[6-11]` - Adjusted vertices pour la sortie (calculés par DRLGOUTDOORS_CalculatePathCoordinates)
- `pVertices[12-17]` - Endpoint vertices pour l'autre level
- `pVertices[18-23]` - Special/bridge vertices (pour les rivers/special areas)

### D2DrlgLinkStrc - Définition de Connexion Level
```c
struct D2DrlgLinkStrc {
    void* pfLinker;                 // +0x00 - Pointeur vers fonction de linking (ex: sub_6FD81380)
    int32_t nLevel;                 // +0x04 - Level ID (ex: LEVEL_COLDPLAINS)
    int32_t nLevelLink;             // +0x08 - Index du level parent dans le tableau
    int32_t nLevelLinkEx;           // +0x0C - Index supplémentaire ou flag
};
```

### D2DrlgLevelLinkDataStrc - Données de Linking pour Iteration
```c
struct D2DrlgLevelLinkDataStrc {
    D2SeedStrc pSeed;               // +0x00 - Seed pour RNG
    D2DrlgCoordStrc pLevelCoord[15];// +0x08 - Rectangles de chaque level (64 bytes chacun = 15 levels)
    D2DrlgLinkStrc* pLink;          // +0xF8 - Pointeur vers le tableau de links
    int32_t nRand[4][15];           // +0xFC - RNG values pour placement variation
    int32_t nIteration;             // +0x1EC - Index itération actuelle
    int32_t nCurrentLevel;          // +0x1F0 - Level ID courant
};
```

---

## 2. DONNÉES DE CONNEXIONS STATIQUES

Définis dans [DrlgOutPlace.cpp](source/D2Common/src/Drlg/DrlgOutPlace.cpp#L29-L103):

### Act 1 Wilderness Connections
```c
static D2DrlgLinkStrc gAct1WildernessDrlgLink[15] = {
    { sub_6FD81330, LEVEL_STONYFIELD,      -1, -1 },    // [0] Root - no parent
    { sub_6FD81380, LEVEL_COLDPLAINS,       0, -1 },    // [1] Links to STONYFIELD
    { sub_6FD81950, LEVEL_BLOODMOOR,        1, -1 },    // [2] Links to COLDPLAINS
    { sub_6FD81720, LEVEL_ROGUEENCAMPMENT,  2, -1 },    // [3] Links to BLOODMOOR
    { sub_6FD81380, LEVEL_BURIALGROUNDS,    1, -1 },    // [4] Also links to COLDPLAINS
    { NULL, 0, -1, -1 }, // Sentinel
    // ...
};
```

**Fonctions de Linking disponibles:**
- `sub_6FD81330` - Position fixe (utilise Level Definition offset)
- `sub_6FD81380` - 4 positions possibles (directions: N, S, E, W)
- `sub_6FD81530` - 8 positions possibles (toutes directions)
- `sub_6FD81950` - Configuration spéciale avec rotation
- `DRLGOUTROOM_LinkLevelsByLevelCoords` - Position relative au level parent
- `DRLGOUTROOM_LinkLevelsByOffsetCoords` - Position avec offset prédéfini
- `DRLGOUTROOM_LinkLevelsByLevelDef` - Position depuis Level Definition

### Act 2 Outdoor Connections
```c
static D2DrlgLinkStrc gAct2OutdoorDrlgLink[15] = {
    { sub_6FD81330, LEVEL_LUTGHOLEIN,   -1, -1 },
    { sub_6FD81B30, LEVEL_ROCKYWASTE,    0, -1 },    // 2 positions
    { sub_6FD81530, LEVEL_DRYHILLS,      1, -1 },    // 8 positions
    { sub_6FD81530, LEVEL_FAROASIS,      2, -1 },
    { sub_6FD81530, LEVEL_LOSTCITY,      3, -1 },
    { sub_6FD81BF0, LEVEL_VALLEYOFSNAKES,4, -1 },    // 8 positions
    { NULL, 0, -1, -1 },
};
```

### Act 5 Outdoor Connections (avec linking functions modernes)
```c
static D2DrlgLinkStrc gAct5OutdoorDrlgLink[15] = {
    { sub_6FD81330, LEVEL_HARROGATH,         -1, -1 },
    { sub_6FD81330, LEVEL_BLOODYFOOTHILLS,   0, -1 },
    { DRLGOUTROOM_LinkLevelsByLevelCoords, LEVEL_ID_ACT5_BARRICADE_1,   1, -1 },
    { DRLGOUTROOM_LinkLevelsByOffsetCoords, LEVEL_ARREATPLATEAU,       2, -1 },
    { NULL, 0, -1, -1 },
};
```

---

## 3. FLUX DE CHARGEMENT ET PLACEMENT

### Phase 1: Positionnement des Levels (sub_6FD823C0)
```c
// Location: DrlgOutPlace.cpp:1697
void __fastcall sub_6FD823C0(
    D2DrlgStrc* pDrlg,                      // Instance DRLG
    D2DrlgLinkStrc* pDrlgLink,              // Array de connexions (ex: gAct1WildernessDrlgLink)
    int(__fastcall* linkerFunc)(...),       // Fonction validation placement
    void(__fastcall* decorFunc)(...)        // Fonction pour décorer
)
```

**Processus:**
1. Pour chaque connexion dans le tableau:
   - Obtenir la fonction de linking (pfLinker)
   - Appeler pfLinker pour calculer les coordonnées du nouvel level
   - Valider que le level ne chevauche pas les autres (linkerFunc)
   - Marquer les drapeaux de décoration (decorFunc)

### Phase 2: Itération de Linking (dans sub_6FD823C0)
```
For each link in pDrlgLink:
    linkerFunc(pLevelLinkData) {
        // Obtenir le RNG value (direction/position randomisée)
        nRand = SEED_RollRandomNumber(&pSeed) & mask
        
        // Calculer la position basée sur le parent level
        pCoord[current] = CalculatePosition(pCoord[parent], nRand)
        
        // Vérifier non-chevauchement
        return ValidateNonOverlap(pCoord[current], all_previous)
    }
```

### Phase 3: Création des Vertices Physiques
**Étape 1: Initialisation de base (DRLGVER_CreateVertices)**

```c
// Location: DrlgOutdoors.cpp:705
DRLGVER_CreateVertices(
    pLevel->pDrlg->pMempool,
    &pOutdoorInfo->pVertex,
    &pLevel->pLevelCoords,          // Rectangle du level
    0,                              // nDirection
    pOutdoorInfo->pRoomData         // Données géographiques
);
```

**Qu'est-ce que DRLGVER_CreateVertices fait:**
- Crée un linked list circulaire de vertices
- Traverse le périmètre du level
- Détecte les points de sortie (où pRoomData indique une connexion)
- Divise les vertices par 8 (conversion subgrid → grid)

**Résultat du linked list pVertex (exemple 6 directions):**
```
pVertex -> V0 (NW corner) -> V1 (N edge) -> V2 (NE corner) -> 
V3 (E edge) -> V4 (SE corner) -> V5 (S edge) -> [back to V0]
```

**Étape 2: Calcul des Points de Sortie (DRLGOUTDOORS_CalculatePathCoordinates)**

```c
// Location: DrlgOutdoors.cpp:1120-1132

// Pour chaque vertex, calculer un point d'ajustement pour l'exit
DRLGOUTDOORS_CalculatePathCoordinates(pLevel, &pVertices[i], &pVertices[6 + i]);

// Dans la fonction:
void CalculatePathCoordinates(D2DrlgLevelStrc* pLevel, 
                               D2DrlgVertexStrc* pVertex1,    // Point initial
                               D2DrlgVertexStrc* pVertex2)    // Point ajusté
{
    pVertex2->nPosX = pVertex1->nPosX - pLevel->nPosX;
    pVertex2->nPosY = pVertex1->nPosY - pLevel->nPosY;

    // Ajuster par rapport à la direction
    switch (pVertex1->nDirection) {
        case ALTDIR_WEST:  // 0
            pVertex2->nPosX = 8 * (pVertex2->nPosX / 8) + 11;  // +11 nuaux
            break;
        case ALTDIR_NORTH: // 1
            pVertex2->nPosY = 8 * (pVertex2->nPosY / 8) + 11;
            break;
        case ALTDIR_EAST:  // 2
            pVertex2->nPosX = 8 * (pVertex2->nPosX / 8) - 5;   // -5 unités
            break;
        case ALTDIR_SOUTH: // 3
            pVertex2->nPosY = 8 * (pVertex2->nPosY / 8) - 5;
            break;
    }

    pVertex2->nPosX += pLevel->nPosX;
    pVertex2->nPosY += pLevel->nPosY;
}
```

**Étape 3: Calcul du Chemin Entre les Points (sub_6FD80750)**

```c
// Location: DrlgOutPlace.cpp:230
// Cette fonction calcule le CHEMIN entre deux vertices
// Utilise un algorithme pathfinding (A*-like)

BOOL __fastcall sub_6FD80750(D2DrlgLevelStrc* pLevel, int nVertexId)
{
    // Points initial et final
    int nX1 = (pLevel->pOutdoors->pVertices[6 + nVertexId].nPosX - pLevel->nPosX) / 8;
    int nY1 = (pLevel->pOutdoors->pVertices[6 + nVertexId].nPosY - pLevel->nPosY) / 8;
    
    int nX2 = (pLevel->pOutdoors->pVertices[12 + nVertexId].nPosX - pLevel->nPosX) / 8;
    int nY2 = (pLevel->pOutdoors->pVertices[12 + nVertexId].nPosY - pLevel->nPosY) / 8;
    
    // Si la distance < 2, simplement créer un vertex direct
    if (|nX1 - nX2| + |nY1 - nY2| < 2) {
        pPathStarts[nVertexId] = DRLGVER_AllocVertex(...);
        pPathStarts[nVertexId]->nPosX = nX1;
        pPathStarts[nVertexId]->nPosY = nY1;
        
        pPathStarts[nVertexId]->pNext = DRLGVER_AllocVertex(...);
        pPathStarts[nVertexId]->pNext->nPosX = nX2;
        pPathStarts[nVertexId]->pNext->nPosY = nY2;
    } else {
        // Utiliser l'algorithme complexe de pathfinding
        // Crée une liste chaînée de points intermédiaires
        // Utilise un tableau D2UnkOutPlaceStrc12 de 900 positions
        // Pathfinding avec A* heuristique
        
        // Résultat: pPathStarts[nVertexId] -> linked list de tous les points entre source et dest
    }
    
    return TRUE;
}
```

**Format du Chemin (D2UnkOutPlaceStrc12 -> Linked List):**
```
pPathStarts[0] -> Vertex1 (nPosX, nPosY) 
               -> Vertex2 (nPosX, nPosY)
               -> ... (points intermédiaires)
               -> VertexN (nPosX, nPosY)  [last point = destination]
```

---

## 4. LES 6 PASSAGES (pPathStarts[6])

Les 6 éléments de `pPathStarts[6]` représentent les 6 passages côtés du level outdoor:

```
Index [0-5] corresponds to:
[0] = Direction 0 (ALTDIR_WEST = West Chacun connects to a specific neighboring level
[1] = Direction 1 (ALTDIR_NORTH)
[2] = Direction 2 (ALTDIR_EAST)
[3] = Direction 3 (ALTDIR_SOUTH)
[4] = Direction 4 (Unused/Reserved)
[5] = Direction 5 (Unused/Reserved)
```

**Pour chaque direction:**
- `pPathStarts[i]` pointe sur le premier vertex du passage
- Suivre `pNext` pour parcourir le chemin complet jusqu'au level voisin
- Chaque vertex contient (nPosX, nPosY) du point de passage

**Exemple Act 1 Wilderness:**
```
STONYFIELD (root at [0])
  |
  +---> pPathStarts[0] -> Vertex path to COLDPLAINS
  
COLDPLAINS [1]
  |
  +---> pPathStarts[1] -> Vertex path to same STONYFIELD
  +---> pPathStarts[2] -> Vertex path to BLOODMOOR
  
COLDPLAINS also has:
  +---> pPathStarts[?] -> Vertex path to BURIALGROUNDS
```

---

## 5. ALGORITHME DE POSITIONNEMENT (Exemples de Linking Functions)

### Fonction: sub_6FD81380 (4 directions)
```c
// Act 1: Used for LEVEL_COLDPLAINS

BOOL __fastcall sub_6FD81380(D2DrlgLevelLinkDataStrc* pLevelLinkData)
{
    // Générer RNG 0-3 (4 directions possibles)
    int nRand = SEED_RollRandomNumber(&pSeed) & 3;
    
    pLevelCoord[current].pPosX = BaseLevel->pPosX + offsets[nRand][0];
    pLevelCoord[current].pPosY = BaseLevel->pPosY + offsets[nRand][1];
    
    return TRUE;
}

// Les 4 offsets correspondent à:
// 0 = SUD (bottom)
// 1 = OUEST (left)
// 2 = NORD-EST (top-right)
// 3 = EST (right)
```

### Fonction: sub_6FD81530 (8 directions)
```c
// Act 2: Used for LEVEL_DRYHILLS, LEVEL_LOSTCITY

BOOL __fastcall sub_6FD81530(D2DrlgLevelLinkDataStrc* pLevelLinkData)
{
    // Générer RNG 0-7 (8 directions possibles)
    int nRand = SEED_RollRandomNumber(&pSeed) & 7;
    
    // Utilise sub_6FD815E0 qui a 8 cases offset
    sub_6FD815E0(BaseLevel, NewLevel, nRand, 1);
    
    return TRUE;
}
```

### Fonction: sub_6FD81B30 (2 directions)
```c
// Act 2: Used for LEVEL_ROCKYWASTE

BOOL __fastcall sub_6FD81B30(D2DrlgLevelLinkDataStrc* pLevelLinkData)
{
    // Seulement 2 positions possibles (horizontal vs vertical)
    int nRand = (SEED_RollRandomNumber(&pSeed) & 1) + 1;
    
    // Peut être 1 (horizontal) ou 2 (vertical)
    sub_6FD81430(BaseLevel, NewLevel, nRand, 0);
    
    return TRUE;
}
```

### Fonction: DRLGOUTROOM_LinkLevelsByOffsetCoords (Offsets prédéfinis)
```c
// Act 5: Used with fixed offsets

BOOL __fastcall DRLGOUTROOM_LinkLevelsByOffsetCoords(
    D2DrlgLevelLinkDataStrc* pLevelLinkData)
{
    static const D2CoordStrc pOffsetCoords[4] = {
        { 0, -160 },  // Nord
        { -96, -64 }, // Nord-Ouest
        { -64, -96 }, // Nord-Nord-Est
        { -160, 0 }   // Ouest
    };
    
    // Select offset based on RNG or previous decisions
    nIndex = pRand[0] + 2 * pRand[1];
    
    pNewLevel->nPosX = pBaseLevel->nPosX + pOffsetCoords[nIndex].nX;
    pNewLevel->nPosY = pBaseLevel->nPosY + pOffsetCoords[nIndex].nY;
    
    return TRUE;
}
```

---

## 6. MARQUAGE DU GRID (bLvlLink Flag)

Une fois le chemin créé, chaque cellule du chemin est marquée:

```c
// Location: DrlgOutdoors.cpp:1123

D2DrlgOutdoorPackedGrid2InfoStrc tPackedInfo{ 0 };
tPackedInfo.nUnkb07 = true;  // Mark as spawn preset area
DRLGGRID_SetVertexGridFlags(&pOutdoors->pGrid[2], pOutdoors->pPathStarts[i], 
                           tPackedInfo.nPackedValue);

// Flags utilized:
// bLvlLink (bit 0x0400) - Indique que cette cellule est un passage level
// nPickedFile (bits 0x000F0000) - Fichier preset spécifique pour le passage
```

---

## 7. FORMULE DE CALCUL DES POSITIONS

### Distance Manhattan
```
distance = |X1 - X2| + |Y1 - Y2|
```

### Points de Passage Finaux
```
pPathStarts[i]->nPosX = (pVertices[6 + i].nPosX - pLevel->nPosX) / 8
pPathStarts[i]->nPosY = (pVertices[6 + i].nPosY - pLevel->nPosY) / 8

// Conversion from world coordinates to grid coordinates
// Chaque cellule = 8x8 unités du monde
```

### Ajustement par Direction
```
// Direction de sortie détermine l'ajustement du point final
WEST (0):   Point X aligned to grid boundary + 11
NORTH (1):  Point Y aligned to grid boundary + 11
EAST (2):   Point X aligned to grid boundary - 5
SOUTH (3):  Point Y aligned to grid boundary - 5
```

---

## 8. POINT D'ENTRÉE COMPLET: DRLGOUTPLACE_CreateLevelConnections

```c
// Location: DrlgOutPlace.cpp:1381

void __fastcall DRLGOUTPLACE_CreateLevelConnections(D2DrlgStrc* pDrlg, uint8_t nAct)
{
    switch (nAct) {
    case ACT_I:
        // Créer connections Wilderness
        sub_6FD823C0(pDrlg, gAct1WildernessDrlgLink, sub_6FD82050, sub_6FD82360);
        // Créer connections Monastery
        sub_6FD823C0(pDrlg, gAct1MonasteryDrlgLink, sub_6FD82130, sub_6FD82360);
        break;
        
    case ACT_II:
        // Créer connections Desert Outdoor
        sub_6FD823C0(pDrlg, gAct2OutdoorDrlgLink, DRLGOUTPLACE_LinkAct2Outdoors, 0);
        // Créer connections Canyon
        sub_6FD823C0(pDrlg, gAct2CanyonDrlgLink, DRLGOUTPLACE_LinkAct2Canyon, 0);
        break;
        
    // Même pattern pour ACTS III, IV, V
    }
}
```

---

## 9. RÉSUMÉ DES FICHIERS IMPLIQUÉS

| Fichier | Rôle |
|---------|------|
| [DrlgOutPlace.cpp](source/D2Common/src/Drlg/DrlgOutPlace.cpp) | Connection data + pathfinding algorithm (sub_6FD80750) |
| [DrlgOutdoors.cpp](source/D2Common/src/Drlg/DrlgOutdoors.cpp) | Vertex creation + path coordinate calculation |
| [DrlgOutRoom.cpp](source/D2Common/src/Drlg/DrlgOutRoom.cpp) | Room-based linking functions |
| [DrlgDrlgVer.cpp](source/D2Common/src/Drlg/DrlgDrlgVer.cpp) | Vertex allocation + boundary detection |
| [D2DrlgOutdoors.h](source/D2Common/include/Drlg/D2DrlgOutdoors.h) | Struct definitions (OutdoorInfo, Vertex) |
| [D2DrlgDrlgVer.h](source/D2Common/include/Drlg/D2DrlgDrlgVer.h) | Vertex structure definition |
| [D2DrlgDrlg.h](source/D2Common/include/Drlg/D2DrlgDrlg.h) | Link structure definition |

---

## 10. FLUX COMPLET ÉTAPE PAR ÉTAPE

```
1. DRLGOUTPLACE_CreateLevelConnections(pDrlg, nAct)
   |
   +-> sub_6FD823C0(pDrlg, gActXLink, linkerFunc, decorFunc)
       |
       +-> Pour chaque link:
           |
           +-> linkerFunc(pLevelLinkData)
               |- Générer position RNG
               |- Calculer pCoord[i] basé sur parent
               |- Valider non-chevauchement
               |
           +-> decorFunc(pLevel, nIteration)  [optional]
               |- Appliquer flags de décoration
               |
2. DRLGOUTDOORS_InitOutdoorLevelAfterSeed(pLevel)
   |
   +-> DRLGVER_CreateVertices()
       |- Parcourir le périmètre du level
       |- Détecter les connexions
       |- Créer linked list circulaire
       |
   +-> Pour chaque vertex (i = 0 to nVertices):
       |
       +-> DRLGOUTDOORS_CalculatePathCoordinates(pVertices[i], pVertices[6+i])
           |- Calculer le point d'ajustement
           |
       +-> sub_6FD80750(pLevel, i)
           |- Pathfinding entre pVertices[6+i] et pVertices[12+i]
           |- Créer linked list de vertices pour le chemin
           |- Stocker dans pPathStarts[i]
           |
       +-> DRLGGRID_SetVertexGridFlags()
           |- Marquer les cellules du gel comme passage level
```

---

## 11. FORMAT DES PASSAGES ENTRE DEUX LEVELS

**De Level A vers Level B:**

```
Level A (ex: COLDPLAINS)
├── pPathStarts[1] (direction NORTH)
    ├── Vertex1 (grid cell position)
    ├── Vertex2 (next grid cell)
    ├── ...
    └── VertexN (entrance to neighbor)
    
Level B (ex: STONYFIELD)
└── pPathStarts[3] (direction SOUTH)  [opposite direction]
    ├── Vertex1 (exit point from B)
    ├── Vertex2
    ├── ...
    └── VertexN (entrance from A)
```

La player qui traverse le passage:
1. Entre par `pPathStarts[i]->nPosX/Y`
2. Parcourt la linked list pour chaque nPosX/Y
3. Sort par le dernier vertex du chemin
4. Spawne dans le level voisin

---

## 12. CAS SPÉCIAL: RIVER/BRIDGE

Pour les niveaux avec rivières (avoir le flag OUTDOOR_RIVER):

```c
// Location: DrlgOutdoors.cpp:1175-1197

sub_6FD7F5B0(pLevel)
{
    // Récupérer les coordonnées du pont
    DRLGOUTWILD_GetBridgeCoords(pLevel, &nX, &nY);
    
    // Créer vertices spéciaux pour le pont
    pVertices[18 + i].nPosY = nPosY;    // River crossing
    pVertices[18 + i].nPosX = nPosX;
    pVertices[18 + i].nDirection = 2;   // ALTDIR_EAST
}
```

Ceci crée des passages alternatifs (pVertices[18-23]) qui peuvent être utilisés comme points de traversée spéciaux.

