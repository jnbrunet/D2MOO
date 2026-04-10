#include <UNIT/CUnit.h>

#include <Path/Path.h>
#include <Units/Units.h>

// D2Client + 0x11C200 -> 0x6FBBC200 (patched via D2Client.patch.cpp)
D2UnitStrc* g_pCurrentUnit = nullptr;

// D2Client.0x6FB283D0 (RVA: 0x883D0)
D2UnitStrc* D2Client_GetCurrentUnit()
{
    return g_pCurrentUnit;
}

// D2Client.0x6FB29370 (RVA: 0x89370)
D2ActiveRoomStrc* D2Client_GetCurrentUnitRoom()
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
