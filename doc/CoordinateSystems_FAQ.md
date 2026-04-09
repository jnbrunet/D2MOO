# Système de Coordonnées Diablo 2: FAQ et Cas d'Usage

Réponses directes aux 4 questions principales sur le système de coordonnées Diablo 2.

---

## Question 1: Que Représentent les Structures Principales?

### D2DrlgRoomStrc (Définition de Salle - Blueprint)

**Que représente**: La **définition statique** d'une salle, créée au chargement du niveau.

**Rôle principal**:
- Stocke les coordonnées et dimensions de la salle en tuiles (tile coords)
- Contient tous les grilles de tuiles (walls, floors, etc)
- Référence les unités prédéfinies (objets, monstres placés)
- Lien vers la version active via `pRoom` (D2ActiveRoomStrc)

**Accès aux positions**:
```cpp
D2DrlgRoomStrc* pDrlgRoom = /* ... */;

// Via l'union directe:
int tileX = pDrlgRoom->nTileXPos;      // Position X en tuiles
int tileY = pDrlgRoom->nTileYPos;      // Position Y en tuiles
int tileW = pDrlgRoom->nTileWidth;     // Largeur en tuiles
int tileH = pDrlgRoom->nTileHeight;    // Hauteur en tuiles

// Via la structure compacte D2DrlgCoordStrc:
D2DrlgCoordStrc* pCoord = &pDrlgRoom->pDrlgCoord;
// Même données, format différent
```

---

### D2ActiveRoomStrc (Salle Active - Runtime Instance)

**Que représente**: L'**instance active** d'une salle pendant que le jeu tourne.

**Différences avec D2DrlgRoomStrc**:
- Créée dynamiquement via `DUNGEON_AllocRoom()` appelée depuis `DRLG_CreateRoomForRoomEx()` lors de l'initialisation du niveau
- Contient la liste des unités actuellement dans la salle
- Contient les grilles de collision calculées
- Lien vers la version statique via `pDrlgRoom`
- **Important**: Les coordonnées ne sont PAS simples copies! Voir "Accès aux positions" ci-dessous.

**Rôle principal**:
- Stocke l'état runtime de la salle (mobs, objets)
- Gère les unités actuelles
- Utilisé pour la logique de jeu et les interactions

**Accès aux positions**:

⚠️ **Important**: Les coordonnées ne sont pas des copies simples!

Le processus est:
1. Les champs **nTile*** viennent directement de `D2DrlgRoomStrc`
2. Les champs **nSubtile*** sont **calculés** (tuiles × 5) via `DUNGEON_GameTileToSubtileCoords()`

```cpp
D2ActiveRoomStrc* pRoom = /* ... */;

// Les coordonnées de tuiles viennent directement de D2DrlgRoomStrc:
int tileX = pRoom->tCoords.nTileXPos;           // Position X en TUILES (copie directe)
int tileY = pRoom->tCoords.nTileYPos;           // Position Y en TUILES (copie directe)
int tileW = pRoom->tCoords.nTileWidth;          // Largeur en tuiles
int tileH = pRoom->tCoords.nTileHeight;         // Hauteur en tuiles

// Les coordonnées de subtiles sont CALCULÉES (tuiles * 5):
int subtileX = pRoom->tCoords.nSubtileX;        // = tileX * 5
int subtileY = pRoom->tCoords.nSubtileY;        // = tileY * 5
int subtileW = pRoom->tCoords.nSubtileWidth;    // = tileW * 5
int subtileH = pRoom->tCoords.nSubtileHeight;   // = tileH * 5

// Vérification:
ASSERT(subtileX == tileX * 5);

// Note: D2ActiveRoomStrc.tCoords = D2DrlgCoordsStrc (0x20 bytes)
// Contient à la fois les coordonnées en subtiles ET en tuiles!
// Les spéciales calculs sont faits dans DRLG_CreateRoomForRoomEx() 
// avant d'appeler DUNGEON_AllocRoom().
```

**Parcourir les unités**:
```cpp
D2ActiveRoomStrc* pRoom = /* ... */;

// Itérer sur toutes les unités dans la salle
for (D2UnitStrc* pUnit = pRoom->pUnitFirst; pUnit; pUnit = pUnit->pRoomNext) {
    // Chaque pUnit est dans cette salle
}
```

---

### D2RoomTileStrc (Tuile de Salle Link)

**Que représente**: Un **lien vers une tuile** (pour les warps/transitions entre salles).

**Rôle principale**:
- Stocke les références aux salles voisines (via portes/warps)
- `pDrlgRoom`: Pointeur vers la salle destination
- `bEnabled`: Si le warp est actif
- Utilisé pour la génération du dungeon et la navigation

**Pas utilisé pour les coordonnées des objets normales.**

---

### D2DrlgRoomTilesStrc (Collection de Tuiles Graphiques)

**Que représente**: La **collection** de toutes les tuiles graphiques d'une salle.

**Structure**:
```cpp
struct D2DrlgRoomTilesStrc
{
    D2DrlgTileDataStrc* pWallTiles;    // Tuiles de murs
    int32_t nWalls;                    // Nombre de murs
    D2DrlgTileDataStrc* pFloorTiles;   // Tuiles de sols
    int32_t nFloors;                   // Nombre de sols
    D2DrlgTileDataStrc* pRoofTiles;    // Tuiles de toit
    int32_t nRoofs;                    // Nombre de toits
};
```

**Accès**:
```cpp
D2DrlgRoomTilesStrc* pRoomTiles = pRoom->pRoomTiles;

// Itérer sur les tuiles de sol:
for (int i = 0; i < pRoomTiles->nFloors; i++) {
    D2DrlgTileDataStrc* pTile = &pRoomTiles->pFloorTiles[i];
    int posX = pTile->nPosX;  // En subtiles
    int posY = pTile->nPosY;
}
```

---

### D2PresetUnitStrc (Objet Prédéfini)

**Que représente**: Un **objet ou monstre placé statiquement** dans la salle (lors de la génération du niveau).

**Rôle principal**:
- Objets/monstres qui apparaissent toujours au même endroit
- Données prédéfinies du preset de salle
- Convertis en unités runtime lors de la création de la salle

**Structure**:
```cpp
struct D2PresetUnitStrc
{
    int32_t nUnitType;      // Type (UNIT_PLAYER, UNIT_MONSTER, UNIT_OBJECT)
    int32_t nXpos;          // Position X en SUBTILES (game coords)
    int32_t nYpos;          // Position Y en SUBTILES (game coords)
    int32_t nMode;          // Mode initial
    // Autres champs...
};
```

**Accès**:
```cpp
D2DrlgRoomStrc* pDrlgRoom = /* ... */;

// Itérer sur les unités prédéfinies:
for (D2PresetUnitStrc* pPreset = pDrlgRoom->pPresetUnits; pPreset; pPreset = pPreset->pNext) {
    int presetX = pPreset->nXpos;     // En subtiles
    int presetY = pPreset->nYpos;
    
    // Ces coordonnées sont utilisées pour spawner l'unité runtime
}
```

---

### D2DrlgPresetRoomStrc (Salle Preset - Layout Fixe)

**Que représente**: Les **données d'une salle avec layout prédéfini** (par exemple: salle d'un ennemi unique, ou caverne avec layout fixe).

**Rôle principal**:
- Contient les grilles de tuiles (walls, floor, tiles, cells)
- Optionnellement un labyrinthe (maze) avec son propre grid
- Stocke les positions des tomb stone tiles

**Pas d'accès direct aux coordonnées des objets** - celles-ci sont dans les grilles.

**Accès**:
```cpp
D2DrlgPresetRoomStrc* pPreset = pDrlgRoom->pMaze;

// Les grilles sont pWallGrid, pFloorGrid, etc.
// Chaque grille contient les tuiles individuelles avec leurs positions.
```

---

### D2DrlgOutdoorRoomStrc (Salle Extérieure - Layout Généré)

**Que représente**: Les **données d'une salle générée aléatoirement** (par exemple: zones extérieures, forêts).

**Différence avec D2DrlgPresetRoomStrc**:
- Générée procedurally au lieu d'être prédéfinie
- Contient des grilles générées (tile type, walls, floors, dirt paths)
- Peut avoir des vertex (points d'intéressements)

**Similaire à PresetRoom** en termes d'accès - pas d'accès direct aux coordonnées dans la structure elle-même.

---

## Question 2: Quels Sont les Différentes Positions et Que Représentent-Elles?

### Les 3 Champs de Position Principaux

#### A. `#sym:pDrlgCoord` (D2DrlgCoordStrc) - Boîte de Salle

**Où trouve-t-on?**: `D2DrlgRoomStrc.pDrlgCoord` (union)

**Que représente**: Le **rectangle délimitant une salle** en coordonnées de tuiles.

**Structure**:
```cpp
struct D2DrlgCoordStrc
{
    int32_t nPosX;       // Position X en tuiles
    int32_t nPosY;       // Position Y en tuiles
    int32_t nWidth;      // Largeur en tuiles
    int32_t nHeight;     // Hauteur en tuiles
};
```

**Précision**: **Tuiles** (1 tuile = 5 subtiles = 160x80 pixels)

**Usage**:
- Définir les limites globales d'une salle
- Zone de rendu approximative

**Exemple**:
```cpp
D2DrlgRoomStrc* pDrlgRoom = /* ... */;
D2DrlgCoordStrc* pCoord = &pDrlgRoom->pDrlgCoord;

// Salle occupe [pCoord->nPosX * 5 .. (pCoord->nPosX + pCoord->nWidth) * 5[ en subtiles
```

---

#### B. `#sym:nTileXPos, nTileYPos` - Position Tuile Directe

**Où trouve-t-on**:
- `D2DrlgRoomStrc.nTileXPos`, `.nTileYPos` (et width/height)
- Alternative à `pDrlgCoord` (union)

**Que représente**: Les **mêmes coordonnées**, mais dans une union simple plutôt que structure.

**Précision**: **Tuiles**

**Usage**: Identique à `pDrlgCoord`, juste une autre façon d'accéder aux mêmes données.

```cpp
D2DrlgRoomStrc* pDrlgRoom = /* ... */;

// Ces deux sont équivalents:
int x1 = pDrlgRoom->nTileXPos;           // Via l'union simple
int x2 = pDrlgRoom->pDrlgCoord.nPosX;   // Via la structure
// x1 == x2
```

---

#### C. `#sym:tCoords` (D2DrlgCoordsStrc) - Coordonnées Complètes

**Où trouve-t-on**: `D2ActiveRoomStrc.tCoords`

**Que représente**: Les **coordonnées complètes** d'une salle en **PLUSIEURS PRÉCISIONS**.

**Structure**:
```cpp
struct D2DrlgCoordsStrc
{
    int32_t nSubtileX;      // Position X en SUBTILES (game coords)
    int32_t nSubtileY;      // Position Y en SUBTILES (game coords)
    int32_t nSubtileWidth;  // Largeur en SUBTILES
    int32_t nSubtileHeight; // Hauteur en SUBTILES
    
    int32_t nTileXPos;      // Position X en TUILES
    int32_t nTileYPos;      // Position Y en TUILES
    int32_t nTileWidth;     // Largeur en TUILES
    int32_t nTileHeight;    // Hauteur en TUILES
};
```

**Précision**:
- Première moitié: **Subtiles** (1/5 de tuile)
- Deuxième moitié: **Tuiles** (5 subtiles)

**Relation**:
```cpp
nTileXPos = nSubtileX / 5
nTileYPos = nSubtileY / 5
nTileWidth = nSubtileWidth / 5
nTileHeight = nSubtileHeight / 5
```

**Usage**:
- Délimiters précis de salle (en subtiles) pour collision et logique
- Délimiters groupés de salle (en tuiles) pour optimisation

**Exemple**:
```cpp
D2ActiveRoomStrc* pRoom = /* ... */;

// Vérifier si une unité est dans la salle:
D2CoordStrc unitCoord;
UNITS_GetCoords(pUnit, &unitCoord);

bool inRoom = (
    unitCoord.nX >= pRoom->tCoords.nSubtileX &&
    unitCoord.nX < pRoom->tCoords.nSubtileX + pRoom->tCoords.nSubtileWidth &&
    unitCoord.nY >= pRoom->tCoords.nSubtileY &&
    unitCoord.nY < pRoom->tCoords.nSubtileY + pRoom->tCoords.nSubtileHeight
);
```

---

### Résumé: Comparaison des 3 Positions

| Champ | Type | Précision | Utilisation | Où |
|-------|------|-----------|-------------|-----|
| `pDrlgCoord` | D2DrlgCoordStrc | Tuiles | Limites simples | D2DrlgRoomStrc |
| `nTileXPos/Y` | int32_t (direct) | Tuiles | Alternative à pDrlgCoord | D2DrlgRoomStrc |
| `tCoords` | D2DrlgCoordsStrc | Subtiles + Tuiles | Coordonnées précises | D2ActiveRoomStrc |

---

### Pour les Objets Individuels

#### Positions sur des Tuiles

**Via D2DrlgTileDataStrc**:
```cpp
struct D2DrlgTileDataStrc
{
    int32_t nPosX;       // Position X en SUBTILES (game coords)
    int32_t nPosY;       // Position Y en SUBTILES (game coords)
    int32_t nWidth;      // Largeur en subtiles
    int32_t nHeight;     // Hauteur en subtiles
};
```

**Précision**: **Subtiles** (game coordinates)

---

#### Positions sur des Unités (Units)

**Via D2DynamicPathStrc (chemin de l'unité)**:
```cpp
struct D2DynamicPathStrc
{
    D2FP32_16 tGameCoords;    // +0x00: Fractional fixed-point 16.16
    int32_t dwClientCoordX;   // +0x08: Pixels écran X
    int32_t dwClientCoordY;   // +0x0C: Pixels écran Y
};
```

**3 précisions possibles**:
1. **`tGameCoords`**: Fixed-point 16.16 (fractal, 1/65536 subtile)
   - `wPosX/wPosY`: Partie entière (en subtiles)
   - `wOffsetX/wOffsetY`: Partie fractionnaire
   - Utilisé pour mouvement fluide et pathfinding

2. **`dwClientCoordX/Y`**: Pixels écran (après dimetric projection)
   - Utilisé pour rendu après projection isométrique
   - Directement utilisable pour conversion minimap

3. **Convertible en game coords**: Via `UNITS_GetCoords()`
   - Donne position en subtiles (entier)

---

#### Positions sur des Objets Prédéfinis

**Via D2PresetUnitStrc**:
```cpp
struct D2PresetUnitStrc
{
    int32_t nXpos;  // Position X en SUBTILES (game coords)
    int32_t nYpos;  // Position Y en SUBTILES (game coords)
};
```

**Précision**: **Subtiles** (game coordinates)

---

## Question 3: Position Minimap d'une Unité (D2UnitStrc)

### Processus Complet

**Objectif**: Afficher une unité sur la minimap.

**Étapes**:

```cpp
D2UnitStrc* pUnit = /* monstre, joueur, objet, etc */;

// ÉTAPE 1: Obtenir les coordonnées écran (pixels)
int screenCoordX = UNITS_GetClientCoordX(pUnit);  // Pixels écran X
int screenCoordY = UNITS_GetClientCoordY(pUnit);  // Pixels écran Y

// Vérification rapide:
printf("Unit at screen: (%d, %d) pixels\n", screenCoordX, screenCoordY);

// ÉTAPE 2: Convertir en coordonnées minimap (diviser par 10)
int minimapX = screenCoordX / 10;
int minimapY = screenCoordY / 10;

// Résultat: Position sur la minimap (avant offset)

// ÉTAPE 3 (optionnel): Ajouter offset de centrage visuel
int minimapX_final = minimapX + 1;   // +1 pour centrage X
int minimapY_final = minimapY - 3;   // -3 pour centrage Y (amplified by isometric)

// Résultat: Position finale sur la minimap
printf("Unit on minimap: (%d, %d)\n", minimapX_final, minimapY_final);

// ÉTAPE 4 (optionnel): Insérer dans l'AVL tree de minimap
// (voir fonction D2Client_AutomapAddObjectCell)
```

### Détail: Formulae de Conversion

```cpp
// Méthode 1: Via coordonnées game (si vous avez les game coords)
int gameX, gameY;  // En subtiles
UNITS_GetCoords(pUnit, { .nX = gameX, .nY = gameY });

// Convertir game → client (dimetric projection):
int clientX = (gameX - gameY) / 2;
int clientY = (gameX + gameY) / 4;

// Convertir client → minimap:
int minimapX = clientX / 10;
int minimapY = clientY / 10;

// Méthode 2: Direct via UNITS_GetClientCoordX/Y (recommandé)
int clientX = UNITS_GetClientCoordX(pUnit);  // Déjà après projection
int clientY = UNITS_GetClientCoordY(pUnit);

int minimapX = clientX / 10;
int minimapY = clientY / 10;
```

### Code Complet pour Minimap

```cpp
void AddUnitToMinimap(D2UnitStrc* pUnit, D2AutomapLayerStrc* pLayer) {
    // Déterminer le type de cellule automap
    int cellNo = 0;
    if (pUnit->dwUnitType == UNIT_MONSTER) {
        cellNo = AUTOMAPCELL_MONSTER;  // ou dériver du type
    } else if (pUnit->dwUnitType == UNIT_OBJECT) {
        cellNo = AUTOMAPCELL_OBJECT;   // ou du sous-type
    }
    
    // Utiliser la fonction de D2Client
    D2Client_AutomapAddObjectCell(pUnit, cellNo, &pLayer->pObjects);
    
    // Résultat interne:
    // - Obtient screenX via UNITS_GetClientCoordX
    // - Convertit en minimapX = screenX / 10 + 1
    // - Crée D2AutomapCellStrc avec xPixel = minimapX
    // - Insère dans l'AVL tree
}
```

---

## Question 4: Position Minimap d'une Tuile ou Object Prédéfini

### Pour une Tuile (D2DrlgTileDataStrc)

**Objectif**: Afficher une tuile sur la minimap.

**Processus**:

```cpp
D2DrlgTileDataStrc* pTile = /* ... */;
D2DrlgRoomStrc* pRoom = /* ... */;

// ÉTAPE 1: Position de la tuile en SUBTILES (game coords)
int tileX_subtiles = pTile->nPosX;
int tileY_subtiles = pTile->nPosY;

// ÉTAPE 2: Convertir game subtiles → client pixels (dimetric projection)
// Formule: clientX = (gameX - gameY) / 2; clientY = (gameX + gameY) / 4
int clientX = (tileX_subtiles - tileY_subtiles) / 2;
int clientY = (tileX_subtiles + tileY_subtiles) / 4;

// ÉTAPE 3: Convertir client pixels → minimap pixels (diviser par 10)
int minimapX = clientX / 10;
int minimapY = clientY / 10;

// ÉTAPE 4: Ajouter offset de centrage (si souhaité)
int minimapX_final = minimapX;  // Pas d'offset pour les tuiles généralement
int minimapY_final = minimapY;

// Résultat: Position sur la minimap
printf("Tile at minimap: (%d, %d)\n", minimapX_final, minimapY_final);
```

### Pour un Objet Prédéfini (D2PresetUnitStrc)

**Objectif**: Afficher un objet prédéfini sur la minimap.

**Processus**:

```cpp
D2PresetUnitStrc* pPreset = /* ... */;

// ÉTAPE 1: Position prédéfinie en SUBTILES (game coords)
int presetX_subtiles = pPreset->nXpos;
int presetY_subtiles = pPreset->nYpos;

// ÉTAPE 2: Convertir game subtiles → client pixels (dimetric projection)
int clientX = (presetX_subtiles - presetY_subtiles) / 2;
int clientY = (presetX_subtiles + presetY_subtiles) / 4;

// ÉTAPE 3: Convertir client pixels → minimap pixels
int minimapX = clientX / 10;
int minimapY = clientY / 10;

// ÉTAPE 4: Ajouter offset de centrage (selon le type d'objet)
// Les objets peuvent avoir un offset similaire aux unités
int minimapX_final = minimapX + 1;  // +1 offset possible
int minimapY_final = minimapY - 3;  // -3 offset possible

printf("Preset object at minimap: (%d, %d)\n", minimapX_final, minimapY_final);
```

### Code Complet pour Révéler une Salle sur Minimap

```cpp
void RevealRoomOnMinimap(D2DrlgRoomStrc* pDrlgRoom, D2ActiveRoomStrc* pRoom, D2AutomapLayerStrc* pLayer) {
    // Ajouter tous les sols
    {
        int tilesCount = pRoom->pRoomTiles->nFloors;
        D2DrlgTileDataStrc* pTiles = pRoom->pRoomTiles->pFloorTiles;
        
        for (int i = 0; i < tilesCount; i++) {
            // D2Client_AutomapAddTileCell(pTiles[i], pDrlgRoom, &pLayer->pFloors);
            // Internement:
            // - Obtient nPosX/Y en subtiles
            // - Convertit via dimetric projection
            // - Divise par 10 pour minimap
            // - Insère dans AVL tree de pFloors
        }
    }
    
    // Ajouter tous les murs
    {
        int tilesCount = pRoom->pRoomTiles->nWalls;
        D2DrlgTileDataStrc* pTiles = pRoom->pRoomTiles->pWallTiles;
        
        for (int i = 0; i < tilesCount; i++) {
            // D2Client_AutomapAddTileCell(pTiles[i], pDrlgRoom, &pLayer->pWalls);
        }
    }
    
    // Ajouter tous les objets/monstres runtime
    {
        for (D2UnitStrc* pUnit = pRoom->pUnitFirst; pUnit; pUnit = pUnit->pRoomNext) {
            int cellNo = DetermineAutomapCellNumber(pUnit);
            // D2Client_AutomapAddObjectCell(pUnit, cellNo, &pLayer->pObjects);
        }
    }
}
```

### Offset de Centrage Minimap

**Question**: Pourquoi `+1` et `-3`?

**Réponse**: Pour le centrage visuel des objets sur la minimap.

- `+1` en X: Léger décalage vers la droite
- `-3` en Y: Décalage vers le haut (amplifié par projection isométrique 2:1)

Ces offsets s'appliquent seulement aux **objets dynamiques** (unités), pas aux tuiles fixes.

```cpp
// Pour les objets dynamiques:
int minimapX = screenX / 10 + 1;   // Avec offset
int minimapY = screenY / 10 - 3;

// Pour les tuiles (pas d'offset généralement):
int minimapX = clientX / 10;        // Sans offset
int minimapY = clientY / 10;
```

---

## Résumé Rapide: Cheat Sheet

### Obtenir Position Minimap

```cpp
// Unité/Monstre/Joueur:
int mX = UNITS_GetClientCoordX(pUnit) / 10 + 1;
int mY = UNITS_GetClientCoordY(pUnit) / 10 - 3;

// Tuile:
int mX = (pTile->nPosX - pTile->nPosY) / 2 / 10;
int mY = (pTile->nPosX + pTile->nPosY) / 4 / 10;

// Objet prédéfini:
int mX = (pPreset->nXpos - pPreset->nYpos) / 2 / 10 + 1;
int mY = (pPreset->nXpos + pPreset->nYpos) / 4 / 10 - 3;
```

### Structures de Coordonnées par Usage

**Pour les salles**: `D2DrlgCoordsStrc` (subtiles ET tuiles) dans `D2ActiveRoomStrc.tCoords`

**Pour les unités**: `D2DynamicPathStrc` (fractional + client) dans `D2UnitStrc.pDynamicPath`

**Pour les tuiles**: `D2DrlgTileDataStrc` (subtiles) directement

**Pour les objets prédéfinis**: `D2PresetUnitStrc` (subtiles) directement

### Conversions Clés

- **1 tuile = 5 subtiles = 160x80 pixels client = 16x8 pixels minimap**
- **Game (subtiles) → Client (pixels): `dimetric projection` formule**
- **Client (pixels) → Minimap: `/10 ± offset`**

