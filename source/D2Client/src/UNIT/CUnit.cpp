#include <UNIT/CUnit.h>

#include <Fog.h>
#include <Path/Path.h>
#include <Units/Units.h>

#ifdef D2_VERSION_110F
// Direct alias to original D2Client global (1.10f): D2Client + 0x11C200 -> 0x6FBBC200
D2UnitStrc*& g_pCurrentUnit = *reinterpret_cast<D2UnitStrc**>(0x6FBBC200);
// Direct pointer to original D2Client unit hash table (1.10f): D2Client + 0x11AA00 -> 0x6FBBAA00
// Array of 0x300 D2UnitStrc* entries (128 per unit type × 6 types), indexed by (id & 0x7F) + (type << 7).
D2UnitStrc** g_GlobalUnitTables = reinterpret_cast<D2UnitStrc**>(0x6FBBAA00);
#else
static D2UnitStrc* g_pCurrentUnitStorage = nullptr;
D2UnitStrc*& g_pCurrentUnit = g_pCurrentUnitStorage;
static D2UnitStrc* g_GlobalUnitTablesStorage[0x300] = {};
D2UnitStrc** g_GlobalUnitTables = g_GlobalUnitTablesStorage;
#endif

// D2Client.0x6FB283D0 (RVA: 0x883D0)
D2UnitStrc* UNIT_GetCurrentUnit()
{
    return g_pCurrentUnit;
}

// D2Client.0x6FB29370 (RVA: 0x89370)
D2ActiveRoomStrc* UNIT_GetCurrentUnitRoom()
{
    D2UnitStrc* pUnit = g_pCurrentUnit;
    if (!pUnit)
    {
        return nullptr;
    }
    D2DynamicPathStrc* pPath = pUnit->pDynamicPath;
    if (!pPath)
    {
        return nullptr;
    }
    return pPath->pRoom;
}

// D2Client.0x6FB269F0 (RVA: 0x869F0)
D2UnitStrc* __fastcall UNIT_GetUnitFromIndex(int dwUnitId, D2C_UnitTypes unitType)
{
    // Index into the hash table: low 7 bits of id + (type * 128)
    const int nIndex = (dwUnitId & 0x7F) + ((int)unitType << 7);
    D2UnitStrc* pUnit = g_GlobalUnitTables[nIndex];

    while (pUnit != nullptr)
    {
        if (pUnit->dwUnitId == static_cast<D2UnitGUID>(dwUnitId))
        {
            if (static_cast<D2C_UnitTypes>(pUnit->dwUnitType) == unitType)
            {
                return pUnit;
            }
            // ID matched but wrong type: log then assert
            FOG_Trace("current unit is %d, table is supposed to be %d\n",
                      pUnit->dwUnitType, (uint32_t)unitType);
            D2_ASSERTM(static_cast<D2C_UnitTypes>(pUnit->dwUnitType) == unitType, "ptUnit->eType == eTOU");
        }
        pUnit = pUnit->pListNext;
    }

    return nullptr;
}