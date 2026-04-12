// d2moo_types_for_ida.h
// Generated from D2MOO project for IDA Pro.
//
// HOW TO LOAD IN IDA:
//   File > Load file > Parse C header file
//   In the dialog:
//     - Set Compiler: Visual C++
//     - Set Target: x86 (32-bit)
//   Click OK  ->  IDA adds all types to the local type library.
//
// After loading, re-run D2Common_types.idc / D2Game_types.idc
// to apply function signatures (SetType failures should drop to 0).
//
// This file provides:
//   - Basic integer typedefs (uint8_t, uint32_t, int32_t, BOOL, ...)
//   - D2MOO basic typedefs  (D2UnitGUID, D2GameGUID, HD2ARCHIVE)
//   - Function pointer typedefs (StatListRemoveCallback, StatListValueChangeFunc)
//   - Forward declarations for ALL D2MOO struct types used in the IDC files
//
// NOTE: Only plain C syntax is used here. No C++ features (no class,
// no ::, no using, no templates, no namespaces). IDA's C parser requires this.

#ifndef D2MOO_TYPES_FOR_IDA_H
#define D2MOO_TYPES_FOR_IDA_H

// ============================================================
// Basic integer types  (safe to re-define; guarded by macros)
// ============================================================

#ifndef _DEFINED_UINT8_T
typedef unsigned char uint8_t;
#define _DEFINED_UINT8_T
#endif

#ifndef _DEFINED_UINT16_T
typedef unsigned short uint16_t;
#define _DEFINED_UINT16_T
#endif

#ifndef _DEFINED_UINT32_T
typedef unsigned int uint32_t;
#define _DEFINED_UINT32_T
#endif

#ifndef _DEFINED_UINT64_T
typedef unsigned long long uint64_t;
#define _DEFINED_UINT64_T
#endif

#ifndef _DEFINED_INT8_T
typedef signed char int8_t;
#define _DEFINED_INT8_T
#endif

#ifndef _DEFINED_INT16_T
typedef short int16_t;
#define _DEFINED_INT16_T
#endif

#ifndef _DEFINED_INT32_T
typedef int int32_t;
#define _DEFINED_INT32_T
#endif

#ifndef _DEFINED_INT64_T
typedef long long int64_t;
#define _DEFINED_INT64_T
#endif

#ifndef _DEFINED_BOOL
typedef int BOOL;
#define _DEFINED_BOOL
#endif

// ============================================================
// D2MOO basic types
// ============================================================

/* D2UnitGUID: unique identifier for a unit (uint32_t alias) */
typedef unsigned int D2UnitGUID;

/* D2GameGUID: unique identifier for a game (uint32_t alias) */
typedef unsigned int D2GameGUID;

/* HD2ARCHIVE: opaque handle to an MPQ archive */
struct HD2ARCHIVE__;
typedef struct HD2ARCHIVE__* HD2ARCHIVE;

/* HGAMEDATA: opaque handle to game data */
typedef void* HGAMEDATA;

/* D2C_Difficulties enum (treated as int) */
typedef int D2C_Difficulties;
typedef int D2C_TransactionTypes;
typedef int D2C_UnitTypes;
typedef int D2C_UnitEventTypes;
typedef int D2C_EventTypes;
typedef int D2C_AiSpecialState;
typedef int D2C_SRV2CLT5A_TYPES;

// ============================================================
// Function pointer typedefs (used as parameter/return types)
// ============================================================

/* Forward declarations needed for the callback typedefs below */
struct D2UnitStrc;
struct D2StatListStrc;
struct D2GameStrc;

/* StatListRemoveCallback: called when a stat list/buff expires */
typedef void (__fastcall *StatListRemoveCallback)(
    struct D2UnitStrc* pUnit,
    int nState,
    struct D2StatListStrc* pStatList);

/* StatListValueChangeFunc: called when a unit stat value changes */
typedef void (__fastcall *StatListValueChangeFunc)(
    struct D2GameStrc* pGame,
    struct D2UnitStrc* pUnit1,
    struct D2UnitStrc* pUnit2,
    int nStatId,
    int nValue,
    int nUnused);

// ============================================================
// Forward declarations for all D2MOO struct types
// (referenced in D2Common_types.idc and D2Game_types.idc)
// ============================================================

struct D2Act1Quest5Strc;
struct D2Act4Quest2Strc;
struct D2Act5Quest3Strc;
struct D2Act5Quest5Strc;
struct D2ActiveRoomStrc;
struct D2AiCmdStrc;
struct D2AiControlStrc;
struct D2AiParamStrc;
struct D2AiTableStrc;
struct D2AiTickParamStrc;
struct D2AnimDataRecordStrc;
struct D2AnimDataTableStrc;
struct D2AnimSeqRecordStrc;
struct D2AnimSeqTxt;
struct D2ArenaTxt;
struct D2ArmTypeTxt;
struct D2AuraCallbackStrc;
struct D2BeltsTxt;
struct D2BinFieldStrc;
struct D2BitBufferStrc;
struct D2BooksTxt;
struct D2BoundingBoxStrc;
struct D2CharacterPreviewInfoStrc;
struct D2CharItemStrc;
struct D2CharStatsTxt;
struct D2CharTemplateTxt;
struct D2ClientInfoStrc;
struct D2ClientPlayerDataStrc;
struct D2ClientStrc;
struct D2ClientUnitUpdateSortStrc;
struct D2CompositTxt;
struct D2CoordStrc;
struct D2CorpseStrc;
struct D2CubeItemStrc;
struct D2CubeMainTxt;
struct D2CurseStrc;
struct D2DamageInfoStrc;
struct D2DamageStatTableStrc;
struct D2DamageStrc;
struct D2DifficultyLevelsTxt;
struct D2DrlgActStrc;
struct D2DrlgCoordsStrc;
struct D2DrlgCoordStrc;
struct D2DrlgDeleteStrc;
struct D2DrlgEnvironmentStrc;
struct D2DrlgFileStrc;
struct D2DrlgGridStrc;
struct D2DrlgLevelLinkDataStrc;
struct D2DrlgLevelStrc;
struct D2DrlgLinkStrc;
struct D2DrlgLogicalRoomInfoStrc;
struct D2DrlgMapStrc;
struct D2DrlgOrthStrc;
struct D2DrlgRoomStrc;
struct D2DrlgRoomTilesStrc;
struct D2DrlgStrc;
struct D2DrlgSubstGroupStrc;
struct D2DrlgTileDataStrc;
struct D2DrlgTileGridStrc;
struct D2DrlgVertexStrc;
struct D2DrlgWarpStrc;
struct D2DynamicPathStrc;
struct D2EffectStrc;
struct D2EventTimerQueueStrc;
struct D2EventTimerStrc;
struct D2FieldStrc;
struct D2GameDataTableStrc;
struct D2GameInfoStrc;
struct D2GameStatisticsStrc;
/* D2GameStrc: forward-declared above for callback typedefs */
struct D2GemsTxt;
struct D2GfxLightStrc;
struct D2HirelingInitStrc;
struct D2HirelingTxt;
struct D2HoverTextStrc;
struct D2InactiveItemNodeStrc;
struct D2InactiveMonsterNodeStrc;
struct D2InactiveUnitListStrc;
struct D2InvCompGridStrc;
struct D2InventoryGridInfoStrc;
struct D2InventoryGridStrc;
struct D2InventoryNodeStrc;
struct D2InventoryStrc;
struct D2InvRectStrc;
struct D2ItemDropStrc;
struct D2ItemExtraDataStrc;
struct D2ItemModeArgStrc;
struct D2ItemRatioTxt;
struct D2ItemSaveStrc;
struct D2ItemStatCostTxt;
struct D2ItemsTxt;
struct D2ItemTypesTxt;
struct D2JungleStrc;
struct D2LevelsTxt;
struct D2LinkStrc;
struct D2LowQualityItemsTxt;
struct D2LvlMazeTxt;
struct D2LvlPrestTxt;
struct D2LvlSubTxt;
struct D2LvlTypesTxt;
struct D2LvlWarpTxt;
struct D2MagicAffixTxt;
struct D2MapAIPathPositionStrc;
struct D2MapAIStrc;
struct D2MazeLevelIdStrc;
struct D2MercDataStrc;
struct D2MercSaveDataStrc;
struct D2MessageListStrc;
struct D2MinionListStrc;
struct D2MissileDamageDataStrc;
struct D2MissileStrc;
struct D2MissileStreamStrc;
struct D2ModeChangeStrc;
struct D2MonItemPercentTxt;
struct D2MonModeCallbackTableStrc;
struct D2MonModeTxt;
struct D2MonPresetTxt;
struct D2MonRegDataStrc;
struct D2MonSkillInfoStrc;
struct D2MonSoundsTxt;
struct D2MonStats2Txt;
struct D2MonStatsInitStrc;
struct D2MonStatsTxt;
struct D2MonsterDataStrc;
struct D2MonsterInteractStrc;
struct D2MonsterRegionStrc;
struct D2MonUModTxt;
struct D2NpcRecordStrc;
struct D2NpcTradeStrc;
struct D2NpcTxt;
struct D2NpcVendorChainStrc;
struct D2ObjectControlStrc;
struct D2ObjectRegionStrc;
struct D2ObjectsTxt;
struct D2ObjGroupTxt;
struct D2ObjInitFnStrc;
struct D2ObjModeTypeTxt;
struct D2ObjOperateFnStrc;
struct D2PacketDataStrc;
struct D2PathFoWallContextStrc;
struct D2PathFoWallNodeStrc;
struct D2PathInfoStrc;
struct D2PathPointStrc;
struct D2PetDataStrc;
struct D2PetInfoStrc;
struct D2PetListStrc;
struct D2PlayerCountBonusStrc;
struct D2PlayerDataStrc;
struct D2PlayerPetStrc;
struct D2PlrIntroStrc;
struct D2PlrModeTypeTxt;
struct D2PresetUnitStrc;
struct D2PropertyStrc;
struct D2QualityItemsTxt;
struct D2QuestArgStrc;
struct D2QuestChainStrc;
struct D2QuestDataStrc;
struct D2QuestGUIDStrc;
struct D2RareAffixTxt;
struct D2RoomCollisionGridStrc;
struct D2RoomCoordListStrc;
struct D2RunesTxt;
struct D2SavedItemStrc;
struct D2SaveHeaderStrc;
struct D2SeedStrc;
struct D2SetItemsTxt;
struct D2ShrineDataStrc;
struct D2ShrinesTxt;
struct D2SkillListStrc;
struct D2SkillStrc;
struct D2SkillsTxt;
struct D2SLayerStatIdStrc;
struct D2StatListExStrc;
/* D2StatListStrc: forward-declared above for callback typedefs */
struct D2StatsArrayStrc;
struct D2StatStrc;
struct D2SummonArgStrc;
struct D2SuperUniquesTxt;
struct D2TaskStrc;
struct D2TCExInfoStrc;
struct D2TCExShortStrc;
struct D2TextHeaderStrc;
struct D2TileLibraryEntryStrc;
struct D2TimerArgStrc;
struct D2UniqueItemsTxt;
struct D2UnitDescriptionListStrc;
struct D2UnitEventStrc;
struct D2UnitFindArgStrc;
struct D2UnitFindDataStrc;
/* D2UnitStrc: forward-declared above for callback typedefs */
struct D2UnkDrlgLogicStrc;
struct D2UnkItemModeStrc;
struct D2UnkMonCreateStrc;
struct D2UnkMonsterDataStrc;
struct D2UnkOutdoorStrc;
struct D2WaypointDataStrc;

/* Additional types that may appear in D2Game signatures */
struct D2CombatStrc;
struct D2InventoryItemListStrc;
struct D2PacketListStrc;
struct D2StaticPathStrc;
struct D2AnimDataStrc;
struct D2UnitPacketListStrc;
struct D2GfxDataStrc;
struct D2DataTablesStrc;
struct D2LevelFileListStrc;
struct D2EnvironmentCycleStrc;

/* ============================================================
 * Full struct definitions needed for SetType() on global data.
 * (Forward declarations above satisfy pointer usage in function
 *  signatures; full definitions are required for array types.)
 * ============================================================ */

/* D2AnimSeqTxt: one animation sequence frame (6 bytes) */
struct D2AnimSeqTxt {
    uint16_t wSequence;    /* 0x00 */
    uint8_t  nMode;        /* 0x02 */
    uint8_t  nFrame;       /* 0x03 */
    uint8_t  nDir;         /* 0x04 */
    uint8_t  nEvent;       /* 0x05 */
};

/* D2AnimSeqRecordStrc: sequence pointer + frame counts (12 bytes) */
struct D2AnimSeqRecordStrc {
    struct D2AnimSeqTxt *pAnimSeqTxtRecord; /* 0x00 */
    int32_t nSeqFramesCount;                /* 0x04 */
    int32_t nFramesCount;                   /* 0x08 */
};

/* D2PlayerWeaponSequencesStrc: table of per-weapon-class sequences (168 bytes) */
struct D2PlayerWeaponSequencesStrc {
    struct D2AnimSeqRecordStrc weaponRecords[14]; /* 0x00 */
};

/* D2CompositStrc: weapon-class code <-> id mapping (8 bytes) */
struct D2CompositStrc {
    int32_t nWeaponClassCode; /* 0x00 */
    int32_t nWeaponClassId;   /* 0x04 */
};

/* D2DrlgLinkStrc: outdoor level connection descriptor (16 bytes) */
struct D2DrlgLinkStrc {
    void    *pfLinker;     /* 0x00 */
    int32_t  nLevel;       /* 0x04 */
    int32_t  nLevelLink;   /* 0x08 */
    int32_t  nLevelLinkEx; /* 0x0C */
};

/* D2InventoryGridInfoStrc: inventory grid layout parameters (24 bytes) */
struct D2InventoryGridInfoStrc {
    uint8_t  nGridX;         /* 0x00 */
    uint8_t  nGridY;         /* 0x01 */
    uint16_t pad0x02;        /* 0x02 */
    int32_t  nGridLeft;      /* 0x04 */
    int32_t  nGridRight;     /* 0x08 */
    int32_t  nGridTop;       /* 0x0C */
    int32_t  nGridBottom;    /* 0x10 */
    uint8_t  nGridBoxWidth;  /* 0x14 */
    uint8_t  nGridBoxHeight; /* 0x15 */
    uint16_t pad0x16;        /* 0x16 */
};

/* D2EnvironmentCycleStrc: one environmental period descriptor (12 bytes) */
struct D2EnvironmentCycleStrc {
    int32_t nTicksBegin;  /* 0x00 */
    int32_t nPeriodOfDay; /* 0x04 */
    uint8_t nRed;         /* 0x08 */
    uint8_t nGreen;       /* 0x09 */
    uint8_t nBlue;        /* 0x0A */
    uint8_t nIntensity;   /* 0x0B */
};

/* D2InventoryComponentItemTypeStrc: component fourcc -> item type mapping (8 bytes)
 * (Private struct defined in D2Inventory.cpp; replicated here for IDA.) */
struct D2InventoryComponentItemTypeStrc {
    int32_t dwCode;    /* 0x00 */
    int32_t nItemType; /* 0x04 */
};

#endif /* D2MOO_TYPES_FOR_IDA_H */

