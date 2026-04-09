# Connections Map - All Acts

## ACT I - WILDERNESS

### Connection Array: gAct1WildernessDrlgLink
**Source:** [DrlgOutPlace.cpp:29-39](source/D2Common/src/Drlg/DrlgOutPlace.cpp#L29)

```
Index | Linker Function   | Level ID              | Parent Link | Flags
------|-------------------|----------------------|-------------|-------
  0   | sub_6FD81330      | LEVEL_STONYFIELD     | -1          | Root
  1   | sub_6FD81380      | LEVEL_COLDPLAINS     | 0           | 4 directions
  2   | sub_6FD81950      | LEVEL_BLOODMOOR      | 1           | Dual-mode
  3   | sub_6FD81720      | LEVEL_ROGUEENCAMPMENT| 2           | Dual-mode
  4   | sub_6FD81380      | LEVEL_BURIALGROUNDS  | 1           | 4 directions
  5   | NULL              | SENTINEL             | -1          |
```

**Connection Graph:**
```
                    STONYFIELD [0]
                         ▲
                         │ (root position)
                         │
                    COLDPLAINS [1]
                     ▲        ▲
                  /  │         \
            B.G.[4]  │         BLOODMOOR [2]
                     │             ▲
                     │             │ (direct neighbor)
                     │             │
                     │     ROGUEENCAMPMENT [3]
                     │             ▲
                     └─────────────┘
```

**Linking Strategy:**
- **sub_6FD81330:** Fixed position from Level Definition (STONYFIELD)
- **sub_6FD81380:** 4-direction placement with validation against DRLGOUTPLACE_CheckPlacementValidity
- **sub_6FD81950:** 2-mode (rotation) + 4-direction = 8 possible positions
- **sub_6FD81720:** Similar to 1950 with different offset calculations

### Linker Functions Detail

#### sub_6FD81380 (Cold Plains, Rogue, Burial Grounds)
```
nRand & 3 → 4 directions
    0 = SOUTH (below parent)    -> offset ~ (+0, +Height)
    1 = WEST  (left of parent)  -> offset ~ (-Width, +0)
    2 = NE corner               -> offset ~ (Width-NewW, -Height)
    3 = EAST  (right of parent) -> offset ~ (+Width, +Height-NewH)
```

#### sub_6FD81950 (Blood Moor)
```
nRand & 3 → base direction + rotation
nRand3 & 1 → 2 modes (2 variations per direction)
Total = 8 possible positions
Uses sub_6FD81430() or sub_6FD81850() for coordinate calculation
```

---

## ACT I - MONASTERY

### Connection Array: gAct1MonasteryDrlgLink
**Source:** [DrlgOutPlace.cpp:40-50](source/D2Common/src/Drlg/DrlgOutPlace.cpp#L40)

```
Index | Linker Function   | Level ID              | Parent Link | Flags
------|-------------------|----------------------|-------------|-------
  0   | sub_6FD81330      | LEVEL_MOOMOOFARM     | -1          | Root (fixed)
  1   | sub_6FD81330      | LEVEL_MONASTERYGATE  | -1          | Root (fixed)
  2   | sub_6FD81AD0      | LEVEL_TAMOEHIGHLAND  | 1           | Fixed offset
  3   | sub_6FD81380      | LEVEL_BLACKMARSH     | 2           | 4 directions
  4   | sub_6FD81380      | LEVEL_DARKWOOD       | 3           | 4 directions
  5   | NULL              | SENTINEL             | -1          |
```

**Connection Graph:**
```
MOOMOOFARM [0] (fixed position)   MONASTERYGATE [1] (fixed position)
     ▲                                 ▲
     │ (separate roots)                │
     │                            TAMOEHIGHLAND [2]
     │                                 ▲
     │                                 │ (fixed S offset)
     │                                 │
     │                            BLACKMARSH [3]
     │                                 ▲
     │                                 │ (4-dir)
     │                                 │
     │                            DARKWOOD [4]
     │                                 ▲
     │                                 │ (4-dir)
     └─────────────────────────────────┘
            (separate wilderness areas)
```

**Special Characteristics:**
- Two independent root levels (farm and gate)
- Limited connectivity (strict chain)
- Used for monastery/church area (Act 1 side quest)

---

## ACT II - DESERT OUTDOOR

### Connection Array: gAct2OutdoorDrlgLink
**Source:** [DrlgOutPlace.cpp:51-61](source/D2Common/src/Drlg/DrlgOutPlace.cpp#L51)

```
Index | Linker Function   | Level ID              | Parent Link | Flags
------|-------------------|----------------------|-------------|-------
  0   | sub_6FD81330      | LEVEL_LUTGHOLEIN     | -1          | Root (fixed)
  1   | sub_6FD81B30      | LEVEL_ROCKYWASTE     | 0           | 2 positions
  2   | sub_6FD81530      | LEVEL_DRYHILLS       | 1           | 8 directions
  3   | sub_6FD81530      | LEVEL_FAROASIS       | 2           | 8 directions
  4   | sub_6FD81530      | LEVEL_LOSTCITY       | 3           | 8 directions
  5   | sub_6FD81BF0      | LEVEL_VALLEYOFSNAKES | 4           | 8 directions
  6   | NULL              | SENTINEL             | -1          |
```

**Connection Graph:**
```
                          LUTGHOLEIN [0] (root)
                               ▲
                               │ (city start)
                               │
                        ROCKYWASTE [1]
                        (2 positions)
                              ▲
                              │
            ┌─────────────────┼──────────────────┐
            │                 │                  │
        DRY HILLS [2]   FAROASIS [3]      LOSTCITY [4]
        (8-dir)        (8-dir)            (8-dir)
        
        All connected to next level via:
        VALLEYOFSNAKES [5] (8-dir)
```

**Linking Strategy:**
- **sub_6FD81B30:** 2 modes (horizontal vs vertical orientation)
- **sub_6FD81530:** 8-direction with sub_6FD815E0 offset table
- Heavy use of RNG to avoid repetitive layouts

### sub_6FD81530 (8-Direction Function)
```
nRand & 7 → 8 octants around parent level

0 = SOUTH
1 = SOUTH (alt)
2 = WEST
3 = WEST (alt)
4 = NORTH
5 = NORTH (alt)
6 = EAST
7 = EAST (alt)

Offset calculation via sub_6FD815E0:
    pCoord[new] = pCoord[parent] + offset_table[nRand]
    
    With optional centering adjustment:
    ± (nNewWidth/2 + 8) or ± (nNewHeight/2 + 8)
```

---

## ACT II - CANYON OF THE MAGI

### Connection Array: gAct2CanyonDrlgLink
**Source:** [DrlgOutPlace.cpp:63-68](source/D2Common/src/Drlg/DrlgOutPlace.cpp#L63)

```
Index | Linker Function   | Level ID              | Parent Link | Flags
------|-------------------|----------------------|-------------|-------
  0   | sub_6FD81330      | LEVEL_CANYONOFTHEMAGI| -1          | Root (fixed)
  1   | NULL              | SENTINEL             | -1          |
```

**Connection Graph:**
```
CANYONOFTHEMAGI [0] (isolated, single-level area)
```

**Special Characteristics:**
- Standalone level (no exits to outdoor wilderness)
- Only connected via city entrance (LUTGHOLEIN)
- Single fixed position

---

## ACT IV - OUTER STEPPES

### Connection Array: gAct4OutdoorDrlgLink
**Source:** [DrlgOutPlace.cpp:70-78](source/D2Common/src/Drlg/DrlgOutPlace.cpp#L70)

```
Index | Linker Function   | Level ID              | Parent Link | Flags
------|-------------------|----------------------|-------------|-------
  0   | sub_6FD81330      | LEVEL_THEPANDEMONIUM  | -1          | Root (fixed)
  1   | sub_6FD81CA0      | LEVEL_OUTERSTEPPES   | 0           | Dual-mode
  2   | sub_6FD81380      | LEVEL_PLAINSOFDESPAIR| 1           | 4 directions
  3   | sub_6FD81380      | LEVEL_CITYOFTHEDAMNED| 2           | 4 directions
  4   | NULL              | SENTINEL             | -1          |
```

**Connection Graph:**
```
        THEPANDEMONIUMFORTRESS [0] (root)
                 ▲
                 │
            OUTERSTEPPES [1]
           (dual-mode placement)
           dword_6FDEA6FC flag set
                 ▲
                 │
        PLAINSSOFDESPAIR [2]
            (4-dir)
                 ▲
                 │
        CITYOFTHEDAMNED [3]
            (4-dir)
```

**Special Characteristics:**
- **sub_6FD81CA0:** Sets global `dword_6FDEA6FC` flag (0x400000 or 0x800000)
- Acts as difficulty/theme indicator
- Outer Steppes can spawn with 2 orientation modes

---

## ACT IV - CHAOS SANCTUM

### Connection Array: gAct4ChaosSanctumDrlgLink
**Source:** [DrlgOutPlace.cpp:80-85](source/D2Common/src/Drlg/DrlgOutPlace.cpp#L80)

```
Index | Linker Function   | Level ID              | Parent Link | Flags
------|-------------------|----------------------|-------------|-------
  0   | sub_6FD81330      | LEVEL_CHAOSSANCTUM   | -1          | Root (fixed)
  1   | NULL              | SENTINEL             | -1          |
```

**Connection Graph:**
```
CHAOSSANCTUM [0] (isolated final dungeon)
```

**Special Characteristics:**
- Single fixed level (no additional exits)
- Final boss area
- Only accessible via Outer Steppes doorway

---

## ACT V - BARRICADE/ARREAT

### Connection Array: gAct5OutdoorDrlgLink
**Source:** [DrlgOutPlace.cpp:87-96](source/D2Common/src/Drlg/DrlgOutPlace.cpp#L87)

```
Index | Linker Function                      | Level ID           | Flags
------|--------------------------------------|-------------------|-------
  0   | sub_6FD81330                         | LEVEL_HARROGATH   | Root (fixed)
  1   | sub_6FD81330                         | LEVEL_BLOODYFOOT  | Fixed position
  2   | DRLGOUTROOM_LinkLevelsByLevelCoords | LEVEL_BARRICADE_1 | Position relative to parent
  3   | DRLGOUTROOM_LinkLevelsByOffsetCoords| LEVEL_ARREATPLAT  | Predefined offsets
  4   | NULL                                 | SENTINEL          |
```

**Connection Graph:**
```
            HARROGATH [0] (root)
                 ▲
                 │
            BLOODYFOOTHILLS [1]
           (fixed position)
                 ▲
                 │
           BARRICADE_1 [2]
        (level-relative coords)
                 ▲
                 │
           ARREATPLATEAU [3]
        (predefined offsets)
```

**Linking Strategy:**
- **DRLGOUTROOM_LinkLevelsByLevelCoords:** Position calculated as offset from parent's geometry
- **DRLGOUTROOM_LinkLevelsByOffsetCoords:** Uses static offset table (pOffsetCoords[4])

### sub_6FD81330 Usage
```
pLevelCoord[current] = DATATBLS_GetLevelDefRecord(nCurrentLevel)->dwOffset{X,Y}
```

---

## ACT V - TUNDRA

### Connection Array: gAct5TundraDrlgLink
**Source:** [DrlgOutPlace.cpp:97-101](source/D2Common/src/Drlg/DrlgOutPlace.cpp#L97)

```
Index | Linker Function                | Level ID           | Flags
------|--------------------------------|-------------------|-------
  0   | DRLGOUTROOM_LinkLevelsByLevelDef| LEVEL_TUNDRAWASTE | Level definition
  1   | NULL                            | SENTINEL          |
```

**Connection Graph:**
```
TUNDRAWASTELANDS [0] (isolated or fixed position from level definition)
```

**Linking Strategy:**
- **DRLGOUTROOM_LinkLevelsByLevelDef:** Uses position from D2LevelDefBin

---

## LINKING FUNCTION CLASSIFICATION

### Fixed Position Functions
| Function | Behavior | Use Cases |
|----------|----------|-----------|
| `sub_6FD81330` | Use position from Level Definition | Root cities, starting areas |
| `sub_6FD81AD0` | Fixed offset (direction 0 always) | Tamoehighland, branch areas |

### Variable Position Functions
| Function | Directions | Use Cases |
|----------|-----------|-----------|
| `sub_6FD81380` | 4 (N/S/E/W) | Common outdoor transitions |
| `sub_6FD81530` | 8 (octants) | Desert areas with more variety |
| `sub_6FD81B30` | 2 (H/V modes) | Rocky Waste (orientation variant) |
| `sub_6FD81950` | 8 (4-dir + 2-mode) | Blood Moor (dual-mode variant) |
| `sub_6FD81BF0` | 8 (octants+) | Valley of Snakes |
| `sub_6FD81CA0` | 2 modes | Outer Steppes (difficulty variant) |

### Room-Based Functions (Act 5)
| Function | Calculation | Use Cases |
|----------|-------------|-----------|
| `DRLGOUTROOM_LinkLevelsByLevelCoords` | Relative to parent rect | Barricade levels |
| `DRLGOUTROOM_LinkLevelsByOffsetCoords` | Static offset table | Arreat Plateau |
| `DRLGOUTROOM_LinkLevelsByLevelDef` | Level Definition | Areas with fixed geography |

---

## VALIDATION FUNCTIONS

Lors du placement de chaque level, une fonction de validation est appelée:

### Act 1 Linking Validation
**Function:** `sub_6FD82050` (for Wilderness)

```c
// Check 1: Non-overlapping with all previous levels
for (int i = 0; i < nIteration; ++i) {
    if (i != parentIndex) {
        DRLG_CheckNotOverlappingUsingManhattanDistance(pCoord[i], pCoord[nIteration])
    }
}

// Check 2: Special rules for ROGUEENCAMPMENT + BURIALGROUNDS
// Can't have same random direction as another level connected to same parent
```

**Function:** `sub_6FD82130` (for Monastery)

```c
// Similar overlapping check
// Additional: Monastery levels get extra validation against root city
```

### Act 2 Linking Validation
**Function:** `DRLGOUTPLACE_LinkAct2Outdoors`

```c
// Validate non-overlapping against all previous outdoor levels
```

### Act 4 Linking Validation
**Function:** `DRLGOUTPLACE_LinkAct4Outdoors`

```c
// Standard overlapping check
// Special: Outer Steppes flag affects Plains collision
```

---

## DECORATION FUNCTIONS

Après placement, chaque Act peut marquer des flags spéciaux via une fonction de décoration:

### Act 1 Decoration: sub_6FD82360
```c
static const D2UnkOutdoorStrc3 stru_6FDD06C0[15] = {
    { 0, LEVEL_BLOODMOOR,   LEVEL_COLDPLAINS,   1, 0, 0x04 },     // Blood Moor mask
    { 0, LEVEL_BLOODMOOR,   LEVEL_COLDPLAINS,   2, 3, 0x04 },
    { 0, LEVEL_COLDPLAINS,  LEVEL_BURIALGROUNDS, 2, 1, 0x08 },    // Burial Grounds mask
    { 0, LEVEL_COLDPLAINS,  LEVEL_BURIALGROUNDS, 3, 0, 0x08 },
    { 0, LEVEL_COLDPLAINS,  LEVEL_BURIALGROUNDS, 1, 1, 0x10 },    // Cold Plains mask
    // ...
};

// Based on RNG selections, set outdoor flags:
//   0x04  = OUTDOOR_RIVER ?
//   0x08  = OUTDOOR_CLIFFS
//   0x10  = OUTDOOR_BRIDGE ?
//   0x200 = OUTDOOR_NORTHEAST
//   0x400 = OUTDOOR_NORTHWEST
```

**Acts 2, 4, 5:** No decoration function (pass NULL)

---

## CONNECTION VALIDATION ALGORITHM

```
For each level in connection array:
    1. Get linker function from pDrlgLink[i].pfLinker
    
    2. Iterate until valid placement found:
        for attempt in 0..N:
            linkerFunc(pLevelLinkData)
            {
                if not _ChirumQuest_CheckPlacementValidity(pCoord[i]) continue
                else break
            }
    
    3. Optional: Call decoration function to set flags
    
    4. Next level
```

---

## IMPORTANT CONSTANTS

### Sub_6FD80750 Pathfinding Parameters
```c
D2UnkOutPlaceStrc12 tOutPlaceArray[900];    // Max 900 waypoints in path
int v58 = base_heuristic + v58 / 2;        // Search radius
int v64 = v58 + 35;                        // Max search iterations
```

### Vertex Adjustment Offsets
```c
// Direction offsets for calculating exit points
char byte_6FDCFB80[] = { 1, 0, -1, 0 };   // X offsets
char byte_6FDCFB84[] = { 0, 1, 0, -1 };   // Y offsets

// Used for pathfinding step calculations
```

---

## SPAWN PRESET PLACEMENT

After passages are defined, preset borders are placed:

**Function:** `DRLGOUTPLACE_PlaceAct1245OutdoorBorders`

```
For each vertex v in pOutdoors->pVertex:
    1. Calculate border preset ID based on:
       - pVertex.nDirection (direction facing)
       - nLevelType (Act theme)
       - pVertex.dwFlags (connection markers)
    
    2. Spawn preset at vertex location
    
    3. Mark grid cells as "has picked file"
```

Border presets vary by act:
- Act 1: Cliff borders + standard borders
- Act 2: Desert-specific borders
- Act 4: Mesa borders
- Act 5: Barricade cliff borders

