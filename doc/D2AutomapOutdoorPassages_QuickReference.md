# Outdoor Passages - Quick Reference Card

## Component Sizes & Memory Layout

```
D2DrlgOutdoorInfoStrc: ~0x268 bytes total

pVertex              @ +0x64  8 bytes  - Pointer to linked list
pPathStarts[6]       @ +0x68  48 bytes - Array of 6 pointers (8 bytes each)
pVertices[24]        @ +0x80  0x240 bytes = 6 * 0x18 * 4 entries = 24 vertices
nVertices            @ +0x260 4 bytes  - Count of active passages (0-6 typically)
pRoomData            @ +0x264 8 bytes  - Pointer to room geometry
```

## Data Structures Quick Look

### D2DrlgVertexStrc (0x18 bytes / 24 bytes)
```
+0x00: int32_t nPosX                    // World X coordinate
+0x04: int32_t nPosY                    // World Y coordinate  
+0x08: uint8_t nDirection               // 0=W, 1=N, 2=E, 3=S
+0x09: uint8_t[3] padding
+0x0C: int32_t dwFlags                  // Bit 0 = level connection, Bit 1 = special
+0x10: D2DrlgVertexStrc* pNext          // Circular linked list pointer
       ────────────────────
       0x18 bytes total
```

### D2DrlgOutdoorInfoStrc (relevant fields)
```
+0x54-0x63: D2DrlgCoordStrc pCoord      // Level rectangle (nPosX, nPosY, nWidth, nHeight)
+0x64: D2DrlgVertexStrc* pVertex        // Perimeter vertices start
+0x68: D2DrlgVertexStrc* pPathStarts[6] // Exit point paths
+0x80: D2DrlgVertexStrc pVertices[24]   // Working array
+0x260: int32_t nVertices               // 1-6 active passages
```

### D2DrlgLinkStrc (0x10 bytes)
```
+0x00: void* pfLinker          // Function to calculate position
+0x04: int32_t nLevel          // LEVEL_ID
+0x08: int32_t nLevelLink      // Parent index in array
+0x0C: int32_t nLevelLinkEx    // Extra parameter
```

---

## Memory Layout Visualization: 24 Vertices Array

```
           ACT 1 WILDERNESS EXAMPLE (6 exits)
           ════════════════════════════════════

Vertex     Used For                    Contains
─────────  ──────────────────────────  ────────────────────
[0-5]      Perimeter points            Circle of 6 boundary points
           (from DRLGVER_CreateVertices)

[6-11]     Exit calculation            Adjusted points for passage start
           pVertices[6+i] used for     Distance between exit and neighbor
           pathfinding start

[12-17]    Neighbor endpoint           Where path should lead in neighbor's
           pVertices[12+i] used for    coordinate space
           pathfinding destination

[18-23]    Special areas               Bridges, rivers, special crossings
           (populated by sub_6FD7F5B0) Only used if OUTDOOR_RIVER flag

       Most Common Usage Pattern:
       ┌──────────────────────────────────────┐
       │ For each passage (i = 0 to 5):      │
       │  pVertices[i] → starting point       │
       │  pVertices[6+i] → adjusted point     │
       │  pVertices[12+i] → destination       │
       │  Create path: pPathStarts[i]         │
       └──────────────────────────────────────┘
```

---

## Passage Creation Flow (Simplified)

```
Level Generation
  │
  ├─1. Calculate neighbor positions
  │   └─> sub_6FD81380(...) // 4-direction placement
  │       └─> Position Cold Plains relative to Stony Field
  │
  ├─2. Create perimeter vertices
  │   └─> DRLGVER_CreateVertices()
  │       ├─ Scan level boundary
  │       ├─ Find connection points (bit 0x01 in flags)
  │       └─ Store in pVertex circular linked list (typically 6 items)
  │
  ├─3. For each exit (i=0..5):
  │   │
  │   ├─3a. Calculate adjusted point
  │   │   └─> pVertices[6+i] = pVertices[i] + grid_align_offset
  │   │       Offsets: +11 (W/N), -5 (E/S)
  │   │
  │   ├─3b. Calculate destination in neighbor
  │   │   └─> pVertices[12+i] = destination_from_linking_function()
  │   │
  │   └─3c. Pathfinding
  │       └─> sub_6FD80750()
  │           ├─ If distance < 2: direct connection (2 vertices)
  │           └─ Else: A*/Dijkstra with up to 900 waypoints
  │               Result: pPathStarts[i] → linked list
  │
  └─4. Mark grid cells as passage areas
      └─> DRLGGRID_SetVertexGridFlags()
          └─ bLvlLink = 1 for all cells in pPathStarts[]
```

---

## Quick Function Reference

### Core Creation Functions
| Function | Job | Key Input | Key Output |
|----------|-----|-----------|-----------|
| DRLGOUTPLACE_CreateLevelConnections | Top-level coordinator | nAct | All levels positioned |
| sub_6FD823C0 | Process link array | pDrlgLink[] | All levels positioned |
| DRLGOUTDOORS_InitOutdoorLevelAfterSeed | Build passages | Level seed | pPathStarts[] populated |
| DRLGVER_CreateVertices | Perimeter detection | Box coords | pVertex linked list |
| DRLGOUTDOORS_CalculatePathCoordinates | Exit adjustment | pVertices[i] | pVertices[6+i] calculated |
| sub_6FD80750 | Path calculation | Start/End points | pPathStarts[i] linked list |

### Linker Functions (Position Calculation)
| Function | Directions | Used In | Pattern |
|----------|-----------|---------|---------|
| sub_6FD81330 | Fixed | Most roots | nRand=0, uses Level Definition |
| sub_6FD81380 | 4 | Act 1, 2, 4 | nRand & 0x3 |
| sub_6FD81530 | 8 | Act 2 | nRand & 0x7 |
| sub_6FD81950 | 8 | Act 1 Bloodmoor | nRand(0-3) + nRand2(0-1) |
| sub_6FD81B30 | 2 | Act 2 Rocky Waste | nRand & 0x1 |
| sub_6FD81BF0 | 8 | Act 2 Valley | nRand & 0x7 |
| sub_6FD81CA0 | 2 modes | Act 4 Outer Steppes | Sets global flag |
| DRLGOUTROOM_...LinkLevelsByLevelCoords | Relative | Act 5 | Offset from parent |
| DRLGOUTROOM_...LinkLevelsByOffsetCoords | Predefined | Act 5 | Static offset table |
| DRLGOUTROOM_...LinkLevelsByLevelDef | From def | Act 5 | Uses DATATBLS |

---

## Debugging Decision Tree

```
Is pPathStarts[i] NULL?
├─ YES → sub_6FD80750() was not called or failed
│   ├─ Check if pVertices[6+i] and pVertices[12+i] initialized
│   ├─ Check nVertices count (must be > i)
│   └─ Check level coordinates are valid
│
└─ NO → Continue

Is pPathStarts[i] linked list valid?
├─ YES → Waypoints exist
│   ├─ Check each vertex coordinates are in valid range
│   ├─ Count waypoint length (should be 2+ with complex paths)
│   └─ Check final vertex touches neighbor level boundary
│
└─ NO → Likely memory corruption
    └─ Re-examine pPathStarts array allocation

Does path connect to correct neighbor?
├─ YES → Check for grid conflicts
│   ├─ Run test loop to find overlapping passages
│   └─ Check for "bLvlLink" flag overlap in pGrid[2]
│
└─ NO → Neighbor position calculation wrong
    ├─ Check linker function (sub_6FD81380, etc)
    └─ Verify nLevelLink index in D2DrlgLinkStrc
```

---

## Common Values Reference

### Direction Constants
```c
ALTDIR_WEST  = 0  // Left edge    -> Offset +11 on X
ALTDIR_NORTH = 1  // Top edge     -> Offset +11 on Y
ALTDIR_EAST  = 2  // Right edge   -> Offset -5 on X
ALTDIR_SOUTH = 3  // Bottom edge  -> Offset -5 on Y
ALTDIR_NONE  = 4  // No direction (special cases)
```

### Grid Coordinate System
```c
World Coord = Grid Coord * 8
Grid Coord = World Coord / 8

Sub-grid division = 8x8 units per grid square
Example: World (640, 480) = Grid (80, 60)
```

### Level Link Array Sizes
```c
gAct1WildernessDrlgLink[15]      // 14 levels + 1 sentinel
gAct1MonasteryDrlgLink[15]       // 4-5 levels + sentinel
gAct2OutdoorDrlgLink[15]         // 6-7 levels + sentinel
gAct2CanyonDrlgLink[15]          // 1 level + sentinel
gAct4OutdoorDrlgLink[15]         // 4 levels + sentinel
gAct4ChaosSanctumDrlgLink[15]    // 1 level + sentinel
gAct5OutdoorDrlgLink[15]         // 4 levels + sentinel
gAct5TundraDrlgLink[15]          // 1level + sentinel
```

### Max Values
```c
MAX_PASSAGES_PER_LEVEL = 6       // pPathStarts[6]
MAX_VERTICES_STORAGE = 24        // pVertices[24]
MAX_WAYPOINTS_IN_PATH = 900      // sub_6FD80750 array size
MAX_PATHFINDING_ITERATIONS = 35  // (v58 + 35) limit
```

---

## Magic Numbers in Code

```c
// Grid alignment offsets (in DRLGOUTDOORS_CalculatePathCoordinates)
+11  → Snap to grid boundary + 11 units (slightly past start of cell)
-5   → Snap to grid boundary - 5 units (slightly before start of cell)

// Pathfinding parameters (in sub_6FD80750)
v58 = tInitOutPlace.field_4 + tInitOutPlace.field_4 / 2
v64 = v58 + 35                    // Maximum search expansion

// RNG masks (in linker functions)
& 1  → 2 choices (1 bit)
& 3  → 4 choices (2 bits)   
& 7  → 8 choices (3 bits)

// Direction offset arrays (globals in DrlgOutPlace.cpp)
byte_6FDCFB80[] = { 1, 0, -1, 0 }  // X offsets for pathfinding
byte_6FDCFB84[] = { 0, 1,  0, -1 } // Y offsets for pathfinding
```

---

## File Locations Summary

| File | Key Content | Lines |
|------|-------------|-------|
| DrlgOutPlace.cpp | Link arrays | 29-103 |
| DrlgOutPlace.cpp | sub_6FD80750 (pathfinding) | 230-495 |
| DrlgOutPlace.cpp | Linker functions | 1331-2138 |
| DrlgOutdoors.cpp | DRLGVER_CreateVertices call | 705 |
| DrlgOutdoors.cpp | DRLGOUTDOORS_CalculatePathCoordinates | 1120-1147 |
| DrlgOutdoors.cpp | sub_6FD80750 call loop | 1119 |
| DrlgDrlgVer.cpp | DRLGVER_CreateVertices impl | 15-150+ |
| D2DrlgOutdoors.h | D2DrlgOutdoorInfoStrc def | 47-71 |
| D2DrlgDrlgVer.h | D2DrlgVertexStrc def | 7-24 |
| D2DrlgDrlg.h | D2DrlgLinkStrc def | 433-439 |

---

## Typical Act 1 Cold Plains Passage

```
Raw Data:
  gAct1WildernessDrlgLink[1] = { sub_6FD81380, LEVEL_COLDPLAINS, 0, -1 }

Execution (index i=1):
  
  1. Position calculation:
     nRand = SEED_RollRandomNumber(&pSeed) & 3  // 0-3 direction
     Calculate new level position based on parent level + offset
     
  2. Vertex creation:
     pVertex = [perimeter point 0]
               [perimeter point 1] ← connects to Stony Field
               [perimeter point 2]
               ...
               [back to 0]
     
  3. Path calculation (for connection to Stony Field):
     pVertices[6+0] = adjusted exit point    (111, 100)
     pVertices[12+0] = destination point     (180, 150)
     pPathStarts[0] = {(111,100) → (112,100) → ... → (180,150)}
     
  4. Grid marking:
     For each cell in pPathStarts[0]:
        DRLGGRID_SetVertexGridFlags(..., bLvlLink=1)

Result:
  Player walks (111,100) → (112,100) → ... → (180,150)
  Crosses into Stony Field territory
  Game detects level change and transitions
```

---

## One-Liner Explanations

- **pVertex**: Circular linked list of perimeter points (where level boundary touches connections)
- **pVertices[24]**: Static workspace holding 4 sets of 6 vertices for calculation
- **pPathStarts[6]**: Six independent linked lists, each a walking path to a neighbor
- **pVertices[6+i]**: Adjusted exit point (from actual level border to passage entry)
- **pVertices[12+i]**: Should-be endpoint in neighbor's coordinate system
- **pPathStarts[i]**: The actual walkable waypoints from here to neighbor
- **nVertices**: How many of the 6 possible passages are actually active (1-6)
- **sub_6FD80750**: The magic AI pathfinding that creates the connect path

---

## Testing Checklist

- [ ] pPathStarts[i] != NULL for all i < nVertices
- [ ] Each path has > 1 waypoint
- [ ] Final waypoint is on neighbor level boundary
- [ ] No two paths occupy same grid cell
- [ ] Grid cell flags set correctly (bLvlLink=1)
- [ ] All coordinates in valid range (-10000 to 10000)
- [ ] pVertices[6+i] is between perimeter and normal endpoint
- [ ] Circular linked list pVertex has nVertices+1 nodes
- [ ] No infinite loops in any linked list
- [ ] RNG distribution across different runs varies passages

