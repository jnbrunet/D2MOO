# Outdoor Passages - Implementation Guide

This guide shows how to use the discovered connection data to implement, modify, or debug outdoor level passages.

---

## PART 1: UNDERSTANDING THE VERTEX SYSTEM

### How Vertices Are Stored in Memory

```c
D2DrlgOutdoorInfoStrc::pVertices[24]   // Static array of up to 24 vertices
                                       // Layout:
                                       // [0-5]:   Initial perimeter vertices (from DRLGVER_CreateVertices)
                                       // [6-11]:  Adjusted exit points
                                       // [12-17]: Exit points in adjacent level coordinates
                                       // [18-23]: Special/bridge vertices
```

For a typical Act 1 Wilderness level with 6 exits:

```
Memory Layout:
==============

pVertices[0]  -> (X=100, Y=100, Dir=WEST,  Flags=1)     ┐
pVertices[1]  -> (X=100, Y=50,  Dir=NORTH, Flags=1)     │
pVertices[2]  -> (X=150, Y=50,  Dir=NORTH, Flags=1)     ├─ Initial vertices
pVertices[3]  -> (X=150, Y=100, Dir=EAST,  Flags=1)     │  (from level border)
pVertices[4]  -> (X=150, Y=150, Dir=SOUTH, Flags=1)     │
pVertices[5]  -> (X=100, Y=150, Dir=SOUTH, Flags=1)     ┘

pVertices[6]  -> (X=111, Y=100, Dir=WEST,  Flags=0)     ┐
pVertices[7]  -> (X=100, Y=61,  Dir=NORTH, Flags=0)     │
pVertices[8]  -> (X=150, Y=61,  Dir=NORTH, Flags=0)     ├─ Adjusted vertices
pVertices[9]  -> (X=139, Y=100, Dir=EAST,  Flags=0)     │  (for exit calculation)
pVertices[10] -> (X=150, Y=139, Dir=SOUTH, Flags=0)     │
pVertices[11] -> (X=100, Y=139, Dir=SOUTH, Flags=0)     ┘

pVertices[12] -> (X=200, Y=200, Dir=?,     Flags=0)     ┐
pVertices[13] -> (X=180, Y=180, Dir=?,     Flags=0)     │
pVertices[14] -> (X=220, Y=180, Dir=?,     Flags=0)     ├─ Adjacent level vertices
pVertices[15] -> (X=240, Y=200, Dir=?,     Flags=0)     │  (coordinates in neighbor's space)
pVertices[16] -> (X=220, Y=220, Dir=?,     Flags=0)     │
pVertices[17] -> (X=180, Y=220, Dir=?,     Flags=0)     ┘

pVertices[18] -> Special (bridges, river crossings, etc)
...
pVertices[23] -> Special
```

### Linked List Structure

```c
D2DrlgVertexStrc* pPathStarts[6]
                 │
                 ├─> pPathStarts[0] ─────┐
                 │                       │ First vertex of passage 0
                 │                       v
                 │             .nPosX = 111
                 │             .nPosY = 100
                 │             .pNext -> Node1
                 │                        │
                 │                        v
                 │              .nPosX = 112
                 │              .nPosY = 100
                 │              .pNext -> Node2
                 │                         │
                 │                         v
                 │               .nPosX = 113
                 │               .nPosY = 100
                 │               .pNext -> NULL (end of path)
                 │
                 ├─> pPathStarts[1] ─────┐
                 │                       │ First vertex of passage 1
                 │                       v [etc...]
                 │
                 ├─> pPathStarts[2]
                 │    [3 vertices for passage 2]
                 │
                 └─> etc...

nVertices = 6 (number of active passages)
```

---

## PART 2: CALCULATING EXIT POSITIONS

### Step 1: From Initial Vertex to Adjusted Vertex

**What:** Each initial vertex (pVertices[0-5]) represents a point on the level's perimeter.

```c
// Given initial vertex at perimeter:
D2DrlgVertexStrc* pInitial = &pOutdoors->pVertices[0];
D2DrlgVertexStrc* pAdjusted = &pOutdoors->pVertices[6];

// DRLGOUTDOORS_CalculatePathCoordinates does:
int relX = pInitial->nPosX - pLevel->nPosX;  // Relative to level origin
int relY = pInitial->nPosY - pLevel->nPosY;

// Apply direction-based offset
switch (pInitial->nDirection) {
    case ALTDIR_WEST:  // 0 - Left edge
        pAdjusted->nPosX = 8 * (relX / 8) + 11;    // Snap to grid, add 11
        break;
    case ALTDIR_NORTH: // 1 - Top edge
        pAdjusted->nPosY = 8 * (relY / 8) + 11;
        break;
    case ALTDIR_EAST:  // 2 - Right edge
        pAdjusted->nPosX = 8 * (relX / 8) - 5;     // Snap to grid, subtract 5
        break;
    case ALTDIR_SOUTH: // 3 - Bottom edge
        pAdjusted->nPosY = 8 * (relY / 8) - 5;
        break;
}

pAdjusted->nPosX += pLevel->nPosX;  // Convert back to world coords
pAdjusted->nPosY += pLevel->nPosY;
```

**Grid Alignment:**
- `8 * (coord / 8)` rounds down to nearest multiple of 8
- Adding +11 places exit roughly in the middle of a grid cell
- Subtracting -5 places exit slightly before center

### Step 2: From Adjusted Vertex to Endpoint in Neighbor

**What:** Calculate where the passage connects in the neighboring level's coordinates.

```c
// Given:
// pVertices[6 + i]   = exit point on this level
// pVertices[12 + i]  = entry point from neighbor's coordinate system

// The coordinates in pVertices[12+i] come from the linking function
// which calculated the neighbor's position relative to this level

// For example, if linking function did:
int neighborX = thisLevel.posX + 100;
int neighborY = thisLevel.posY - 200;

// Then pVertices[12+i] would be positioned relative to this offset
// The actual coordinates depend on neighbor's perimeter vertices
```

### Step 3: Pathfinding Between Points

**What:** Create the actual walkable path from exit point to neighbor.

```c
// Input: Two grid coordinates
int startX = (pVertices[6 + i].nPosX - pLevel->nPosX) / 8;
int startY = (pVertices[6 + i].nPosY - pLevel->nPosY) / 8;

int endX = (pVertices[12 + i].nPosX - pLevel->nPosX) / 8;
int endY = (pVertices[12 + i].nPosY - pLevel->nPosY) / 8;

// sub_6FD80750 now finds the shortest path using A* or Dijkstra variants
// Result stored as linked list:
// pPathStarts[i] -> Vertex1 -> Vertex2 -> ... -> VertexN

// Simple case (distance < 2):
if (manhattanDistance < 2) {
    // Just connect the two points directly
    pPathStarts[i] = allocVertex(startX, startY);
    pPathStarts[i]->pNext = allocVertex(endX, endY);
} else {
    // Complex pathfinding needed
    // Fill intermediate waypoints
    pPathStarts[i] = allocVertex(startX, startY);
    pCurrent = pPathStarts[i];
    
    // Add waypoints
    pCurrent->pNext = allocVertex(startX+1, startY);
    pCurrent = pCurrent->pNext;
    
    pCurrent->pNext = allocVertex(startX+1, startY+1);
    pCurrent = pCurrent->pNext;
    
    // ... continue until destination
    pCurrent->pNext = allocVertex(endX, endY);
}
```

---

## PART 3: ADDING A NEW LEVEL CONNECTION

### Example: Add a new level to Act 1 Cold Plains

**Step 1: Modify gAct1WildernessDrlgLink**

```c
// Original:
static D2DrlgLinkStrc gAct1WildernessDrlgLink[15] = {
    { sub_6FD81330, LEVEL_STONYFIELD,      -1, -1 },
    { sub_6FD81380, LEVEL_COLDPLAINS,       0, -1 },
    { sub_6FD81950, LEVEL_BLOODMOOR,        1, -1 },
    { sub_6FD81720, LEVEL_ROGUEENCAMPMENT,  2, -1 },
    { sub_6FD81380, LEVEL_BURIALGROUNDS,    1, -1 },
    { NULL, 0, -1, -1 },
};

// Modified - Insert new level linked to Cold Plains:
static D2DrlgLinkStrc gAct1WildernessDrlgLink[15] = {
    { sub_6FD81330, LEVEL_STONYFIELD,      -1, -1 },              // [0]
    { sub_6FD81380, LEVEL_COLDPLAINS,       0, -1 },              // [1]
    { sub_6FD81950, LEVEL_BLOODMOOR,        1, -1 },              // [2]
    { sub_6FD81720, LEVEL_ROGUEENCAMPMENT,  2, -1 },              // [3]
    { sub_6FD81380, LEVEL_BURIALGROUNDS,    1, -1 },              // [4]
    { sub_6FD81380, LEVEL_NEWAREA,          1, -1 },              // [5] NEW
    { NULL, 0, -1, -1 },                                          // Sentinel
};
```

**Important:** 
- `nLevelLink = 1` means parent is LEVEL_COLDPLAINS (index 1)
- `sub_6FD81380` means 4 directional placement options
- Must update array bounds if you exceed 15 entries

**Step 2: Ensure validator allows connection**

```c
// sub_6FD82050 validates wilderness connections
// Need to add rule for new level:

// Check if LEVEL_NEWAREA can connect to parent:
if (gAct1WildernessDrlgLink[nIteration].nLevel == LEVEL_NEWAREA) {
    // Add validation logic
    if (someCondition) return FALSE;  // Reject placement
}
```

**Step 3: Update level linking call**

No change needed - linking function `sub_6FD823C0` already handles all entries.

---

## PART 4: DEBUGGING PASSAGE ISSUES

### Issue: Passage not appearing

**Diagnostic Steps:**

```c
1. Check if pPathStarts[i] is allocated:
   if (pLevel->pOutdoors->pPathStarts[i] == NULL) {
       printf("ERROR: pPathStarts[%d] not allocated\n", i);
       // sub_6FD80750 failed or wasn't called
   }

2. Check if vertex list populated:
   int vertexCount = pLevel->pOutdoors->nVertices;
   if (vertexCount < i) {
       printf("ERROR: Not enough vertices (%d) for passage %d\n", 
              vertexCount, i);
       // DRLGVER_CreateVertices didn't find enough connections
   }

3. Check starting coordinates:
   D2DrlgVertexStrc* pStart = pLevel->pOutdoors->pPathStarts[i];
   if (pStart) {
       printf("Passage[%d] starts at (%d, %d)\n", i, pStart->nPosX, pStart->nPosY);
   }

4. Walk the linked list:
   int pathLength = 0;
   for (D2DrlgVertexStrc* p = pStart; p != NULL; p = p->pNext) {
       printf("  Step %d: (%d, %d)\n", pathLength++, p->nPosX, p->nPosY);
       if (pathLength > 1000) break;  // Prevent infinite loops
   }
```

### Issue: Passage leads to wrong level

**Diagnostic Steps:**

```c
// Check level coordinates
D2DrlgLevelStrc* currentLevel = pOutdoors->pVertex owner;
D2DrlgLevelStrc* neighborLevel = NULL;

// Determine which level should be at the exit:
// From pPathStarts[i] final vertex, figure out which neighbor it connects to

int finalX = pPathStarts[i]->nPosX;
int finalY = pPathStarts[i]->nPosY;

// Check all neighbors to find which contains this point:
for (D2DrlgLevelStrc* pLevel = pDrlg->pLevelFirst; pLevel; pLevel = pLevel->pNextLevel) {
    if (finalX >= pLevel->nPosX && finalX < pLevel->nPosX + pLevel->nWidth &&
        finalY >= pLevel->nPosY && finalY < pLevel->nPosY + pLevel->nHeight) {
        printf("Passage [%d] should connect to %d (at %d,%d to %d,%d)\n",
               i, pLevel->nLevelId, pLevel->nPosX, pLevel->nPosY,
               pLevel->nPosX + pLevel->nWidth, pLevel->nPosY + pLevel->nHeight);
        neighborLevel = pLevel;
        break;
    }
}

if (neighborLevel == NULL) {
    printf("ERROR: Endpoint (%d,%d) doesn't overlap any level!\n", finalX, finalY);
}
```

### Issue: Two passages cross or overlap

**Finding overlaps:**

```c
// Method 1: Check pGrid[2] for conflicts
for (int passage1 = 0; passage1 < 6; ++passage1) {
    for (D2DrlgVertexStrc* p1 = pPathStarts[passage1]; p1; p1 = p1->pNext) {
        for (int passage2 = passage1+1; passage2 < 6; ++passage2) {
            for (D2DrlgVertexStrc* p2 = pPathStarts[passage2]; p2; p2 = p2->pNext) {
                if (p1->nPosX == p2->nPosX && p1->nPosY == p2->nPosY) {
                    printf("Paths CROSS at (%d,%d)!\n", p1->nPosX, p1->nPosY);
                }
            }
        }
    }
}

// Method 2: Check grid flags
D2DrlgOutdoorPackedGrid2InfoStrc info = 
    DRLGOUTDOORS_GetPackedGrid2Info(pOutdoors, gridX, gridY);

if (info.nUnkb07 && info.bHasPickedFile) {
    // Cell marked as both spawn_preset and passage - conflict!
}
```

---

## PART 5: COORDINATE SYSTEMS

### Understanding Level vs Grid vs World Coordinates

```c
// Three coordinate systems:

1. WORLD COORDINATES (largest scope)
   - Used by D2DrlgLevelStrc: nPosX, nPosY (level origin in world space)
   - Used by individual vertices: nPosX, nPosY (world space)
   - Example: (5000, 6000)

2. LEVEL-RELATIVE COORDINATES (medium scope)
   - Relative to the level's origin (nPosX -= pLevel->nPosX)
   - Used during pathfinding within a level
   - Example: (100, 150) where level is at (5000, 6000)

3. GRID COORDINATES (smallest scope)
   - World coordinates divided by 8
   - Used for grid-based calculations
   - Example: (625, 750) = (5000/8, 6000/8)
```

### Conversion Examples

```c
// World -> Level-relative
int relX = worldX - pLevel->nPosX;
int relY = worldY - pLevel->nPosY;

// Level-relative -> Grid
int gridX = relX / 8;
int gridY = relY / 8;

// Grid -> Level-relative
int relX = gridX * 8;
int relY = gridY * 8;

// Grid -> World
int worldX = (gridX * 8) + pLevel->nPosX;
int worldY = (gridY * 8) + pLevel->nPosY;

// Important: Notice the /8 for grid conversion
// Diablo 2 uses 8x8 subcells per grid square
```

---

## PART 6: GRID FLAGS FOR PASSAGES

### What's marked in pGrid[2]?

```c
D2DrlgOutdoorPackedGrid2InfoStrc {
    uint32_t nUnkb00 : 1;        // 0x00000001 - Border marker?
    uint32_t bHasDirection : 1;  // 0x00000002 - Has non-zero direction
    uint32_t nUnkb02 : 5;        // 0x0000007C - Unknown
    uint32_t nUnkb07 : 1;        // 0x00000080 - Spawn preset area
    uint32_t nUnkb08 : 1;        // 0x00000100 - Blank/empty grid?
    uint32_t bHasPickedFile : 1; // 0x00000200 - Preset spawned
    uint32_t bLvlLink : 1;       // 0x00000400 - LEVEL LINK (passage!)
    uint32_t nUnkb11 : 1;        // 0x00000800
    uint32_t nUnkb12 : 1;        // 0x00001000
    uint32_t nUnkb13 : 3;        // 0x0000E000
    uint32_t nPickedFile : 4;    // 0x000F0000 - Preset ID
    uint32_t nUnkb20 : 12;       // 0xFFF00000
};
```

**Setting passage flags:**

```c
D2DrlgOutdoorPackedGrid2InfoStrc tPackedInfo{ 0 };
tPackedInfo.nUnkb07 = true;  // Mark as spawn preset area
tPackedInfo.bHasDirection = (vertex->nDirection != 0);  // Has direction?

// Actually set in grid for all cells in path
for (D2DrlgVertexStrc* p = pPathStarts[i]; p; p = p->pNext) {
    DRLGGRID_SetVertexGridFlags(&pOutdoors->pGrid[2], p, 
                               tPackedInfo.nPackedValue);
}
```

---

## PART 7: TESTING A NEW PASSAGE

### Complete Test Checklist

```c
void TestNewPassage(D2DrlgLevelStrc* pLevel, int passageIndex) {
    D2DrlgOutdoorInfoStrc* pOut = pLevel->pOutdoors;
    
    // 1. Allocation check
    ASSERT(pOut->pPathStarts[passageIndex] != NULL,
           "Passage %d not allocated", passageIndex);
    
    // 2. Vertex array check
    ASSERT(pOut->nVertices > passageIndex,
           "Vertex count %d insufficient for passage %d",
           pOut->nVertices, passageIndex);
    
    // 3. Starting point
    D2DrlgVertexStrc* pStart = pOut->pPathStarts[passageIndex];
    printf("Passage[%d] starts at (%d, %d)\n", 
           passageIndex, pStart->nPosX, pStart->nPosY);
    
    // 4. Path continuity
    int pathLen = 0;
    D2DrlgVertexStrc* pCur = pStart;
    while (pCur && pathLen < 1000) {
        ASSERT(pCur->nPosX >= -10000 && pCur->nPosX <= 10000,
               "Vertex X coordinate out of range: %d", pCur->nPosX);
        ASSERT(pCur->nPosY >= -10000 && pCur->nPosY <= 10000,
               "Vertex Y coordinate out of range: %d", pCur->nPosY);
        pCur = pCur->pNext;
        pathLen++;
    }
    printf("Path length: %d waypoints\n", pathLen);
    
    // 5. Grid marking
    int gridMarked = 0;
    D2DrlgOutdoorPackedGrid2InfoStrc info;
    for (int i = 0; i < pOut->nGridWidth; ++i) {
        for (int j = 0; j < pOut->nGridHeight; ++j) {
            info = DRLGOUTDOORS_GetPackedGrid2Info(pOut, i, j);
            if (info.nUnkb07) gridMarked++;
        }
    }
    printf("Grid cells marked as spawn area: %d\n", gridMarked);
    
    // 6. Neighbor check
    D2DrlgLevelStrc* pNeighbor = NULL;
    D2DrlgVertexStrc* pEnd = pCur;  // Last vertex before NULL
    for (D2DrlgLevelStrc* p = pLevel->pDrlg->pLevelFirst; p; p = p->pNextLevel) {
        if (p->nLevelId != pLevel->nLevelId) {
            if (pEnd->nPosX >= p->nPosX && pEnd->nPosX < p->nPosX + p->nWidth &&
                pEnd->nPosY >= p->nPosY && pEnd->nPosY < p->nPosY + p->nHeight) {
                pNeighbor = p;
                printf("Connects to Level %d\n", p->nLevelId);
                break;
            }
        }
    }
    ASSERT(pNeighbor != NULL, "No valid neighbor found for passage %d", passageIndex);
}
```

---

## PART 8: QUICK REFERENCE

### When Each Component is Created

```
DRLGOUTPLACE_CreateLevelConnections()
  ├─> sub_6FD823C0()
  │   ├─> Calculate level positions using pfLinker functions
  │   └─> Store in pLevelLinkData->pLevelCoord[index]
  │
  ├─> DRLGOUTDOORS_InitOutdoorLevelAfterSeed()
  │   ├─> DRLGVER_CreateVertices()
  │   │   └─> Create pVertex linked list (perimeter points)
  │   │
  │   ├─> DRLGOUTDOORS_CalculatePathCoordinates()
  │   │   └─> Calculate pVertices[6..11] (adjusted exit points)
  │   │
  │   ├─> sub_6FD80750() for each passage
  │   │   └─> Create pPathStarts[i] linked lists (walkable paths)
  │   │
  │   └─> DRLGGRID_SetVertexGridFlags()
  │       └─> Mark grid cells as passage areas
  │
  └─> DRLGOUTPLACE_PlaceAct1245OutdoorBorders()
      └─> Spawn preset borders along passages
```

### Key Structures at a Glance

```c
// Connections defined
gAct1WildernessDrlgLink[15]          // Who connects to whom + how

// Level positioning calculated
D2DrlgLevelLinkDataStrc::pLevelCoord[15]  // Where each level is placed

// Vertices created
pOutdoors->pVertex                   // Circular linked list of perimeter points
pOutdoors->pVertices[24]             // Static array workspace for calculation
pOutdoors->pPathStarts[6]            // 6 linked lists = 6 passages

// Grid marked
pOutdoors->pGrid[2]                  // Grid with LevelLink and PresetFile flags
```

### Common Pitch Traps

```c
// WRONG: Accessing pVertices without bounds checking
D2DrlgVertexStrc* v = &pOut->pVertices[nVertices];  // Could be 24+!

// RIGHT: Check bounds against nVertices
if (nVertices <= i) return ERROR;
D2DrlgVertexStrc* v = &pOut->pVertices[i];

// WRONG: Assuming pPathStarts[i] exists
if (pOut->pPathStarts[5]) {  // What if nVertices is only 3?
    // Use passage 5
}

// RIGHT: Check both index and allocation
if (i < pOut->nVertices && pOut->pPathStarts[i]) {
    // Use passage i
}

// WRONG: World coords directly as grid index
int gridVal = DRLGGRID_GetGridEntry(..., pVertex->nPosX, pVertex->nPosY);

// RIGHT: Convert to grid coords first
int gridX = (pVertex->nPosX - pLevel->nPosX) / 8;
int gridY = (pVertex->nPosY - pLevel->nPosY) / 8;
int gridVal = DRLGGRID_GetGridEntry(..., gridX, gridY);
```

