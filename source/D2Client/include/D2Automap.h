#include "D2CommonDefinitions/include/D2Structs.OtherDLLs.h"

#pragma pack(1)
struct D2AutomapCellStrc
{
    D2AutomapCellStrc *pCell;				    //0x00 nœud résultant après AVL_Insert (= nouvelle racine du sous-arbre)
    uint16_t nCellNo;							//0x04
    uint16_t xPixel;							//0x06
    uint16_t yPixel;							//0x08
    uint16_t wWeight;							//0x0A facteur AVL {-1, 0, +1}
    D2AutomapCellStrc* pPrev;				//0x0C fils gauche  (= pLessNode)
    D2AutomapCellStrc* pNext;				//0x10 fils droit   (= pMoreNode)
};

 struct AutomapDataBlock // sizeof=0x2804
{
    D2AutomapCellStrc entries[512]; // 0x00
    AutomapDataBlock *pNextBlock;   // 0x2800
};

struct AutomapFileHeader
{
    int   version;         // +0x00  magic/version : doit valoir 12
    int   index;         // +0x04  index courant dans le buffer circulaire [0..3]
    DWORD seed_slots[4];      // +0x08  table des 4 slots : checksum/seed de session
};

struct D2AutomapLayerStrc
{
    uint32_t nLayerNo;							//0x00
    uint32_t fSaved;							//0x04
    D2AutomapCellStrc* pFloors;				//0x08 // racine de l'arbre AVL des sols
    D2AutomapCellStrc* pWalls;				//0x0C // racine de l'arbre AVL des murs
    D2AutomapCellStrc* pObjects;			//0x10 // racine de l'arbre AVL des objets
    D2AutomapCellStrc* pExtras;				//0x14 // racine de l'arbre AVL des éléments de décoration (arbres, etc.)
    D2AutomapLayerStrc* pNext;				//0x18
};

struct D2AutomapViewportStrc
{
    D2CellFileStrc* pCellFile;      // +0x00
    int nLeft;                       // +0x04
    int nRight;                      // +0x08
    int nTop;                        // +0x0C
    int nBottom;                     // +0x10
    int nOffsetX;                    // +0x14
    int nOffsetY;                    // +0x18
};
#pragma pack()
// D2Client + 0x10F990 -> 0x6FBAF990
extern DWORD * g_AutomapCellGroupByNo; // Tableau de DWORD utilisé pour grouper les cellules par numéro de cellule dans l'AVL (valeur -1 pour les numéros de cellule non valides, sinon valeur >= 0 pour grouper les cellules par groupe de numéros de cellule)

// D2Client + 111998 -> 0x6FBB1998
extern AutomapDataBlock *g_pAutomapDataPool;

// D2Client + 0x11199C -> 0x6FBB199C
extern int g_nAutomapDataCount;

// D2Client + 0x1119A0 -> 0x6FBB19A0
extern D2AutomapLayerStrc * g_pAutomapLayers;

// D2Client + 0x1119A4 -> 0x6FBB19A4
extern D2AutomapLayerStrc * g_pCurrentAutomapLayer;

// D2Client + 0x1119E0 -> 0x6FBB19E0
extern DWORD g_bAutomapRevealAll;

// D2Client + 0x11A6D0 -> 6FBBA6D0
extern BOOL p_D2CLIENT_bAutomapEnabled;

// D2Client + 2BA40 -> 0x6FACBA40
D2AutomapCellStrc *D2Client_AutomapDataPool_Alloc();

// D2Client + 0x2CD50 -> 0x6FACCD50
int __fastcall D2Client_AutomapAVL_Insert(D2AutomapCellStrc *automapData, D2AutomapCellStrc **objectsAutomapCells);

// D2Client + 0x2D120 -> 0x6FACD120
void __fastcall D2Client_AutomapAVL_RotateRight(D2AutomapCellStrc **automapNode);

// D2Client + 0x2D3C0 -> 0x6FACD3C0
// Convertit une tuile DRLG en cellule automap et l'insère dans l'arbre AVL
// - Récupère le style/type/séquence de la tuile
// - Recherche le numéro de cellule automap correspondant (LUT)
// - Convertit les coordonnées tile → pixel client → pixel automap (/10)
// - Insère dans l'arbre AVL pFloors ou pWalls
int __fastcall D2Client_AutomapAddTileCell(D2DrlgTileDataStrc *tileData, D2DrlgRoomStrc *room2, D2AutomapCellStrc **pCellTree);

// D2Client + 0x2D560 -> 0x6FACD560
// Convertit une unité/objet en cellule automap et l'insère dans l'arbre AVL
// - Récupère les coordonnées client de l'unité
// - Applique un offset fixe (+1 X, -3 Y) pour le centrage visuel
// - Insère dans l'arbre AVL pObjects
int __fastcall D2Client_AutomapAddObjectCell(D2UnitStrc *unit, int automapCellNumber, D2AutomapCellStrc **pCellTree);


// D2Client + 0x2D0C0 -> 0x6FACD0C0
void __fastcall D2Client_AutomapAVL_RotateLeft(D2AutomapCellTree **tree);

// D2Client + 0x2D180 -> 0x6FACD180
//  D2Client_AutomapRevealRoom(room, clip_flag, layer)
//      │
//      ├──[sol]────► D2Client_AutomapAddTileCell(tile, room2, &layer->pFloors)
//      │                  └─► D2Client_AutomapDataPool_Alloc()
//      │                  └─► coords: GameTile → Client → /10 → pixel automap
//      │                  └─► D2Client_AutomapAVL_Insert(node, &layer->pFloors)
//      │
//      ├──[murs]───► D2Client_AutomapAddTileCell(tile, room2, &layer->pWalls)
//      │
//      └──[objets/monstres]
//             ├─► UNIT_OBJECT  → D2Client_AutomapAddObjectCell(unit, cellNo, &layer->pObjects)
//             └─► UNIT_MONSTER → D2Client_AutomapAddObjectCell(unit, cellNo, &layer->pObjects)
//                                     └─► coords: ClientCoord → /10 + offset(+1,-3)
int __fastcall D2Client_RevealAutomapRoom(D2ActiveRoomStrc *room1, DWORD clip_flag, D2AutomapLayerStrc *layer);

// D2Client + 0x2DCB0 -> 6FACDCB0
void D2Client_DrawAutomap();

// D2Client + 0x2BAF0 -> 0x6FACBAF0
int D2Client_InitAutomapLayer();

// D2Client + 0x2C080 -> 0x6FACC080
HANDLE __thiscall D2Client_AutomapLayer_Save(void **buf);

// D2Client + 0x2C2B0 -> 0x6FACC2B0
HANDLE D2Client_AutomapFile_Open();

// D2Client + 0x2C5B0 -> 0x6FACC5B0
void __fastcall D2Client_Automap_Serialize_Cell(D2AutomapCellStrc *pCell);

// D2Client + 0x2C580 -> 0x6FACC580
int __fastcall D2Client_Automap_PreSerialize_Cell(D2AutomapCellStrc *pCell, _DWORD *a2);

// D2Client + 0x2BCD0 -> 0x6FACBCD0
void D2Client_Automap_Serialize_Layer();

// D2Client + 0x2C610 -> 0x6FACC610
D2AutomapLayerStrc *D2Client_AutomapLayer_Load();

// D2Client + 0x2E510 -> 6FACE510
void D2Client_DrawUnitsOnMinimap();