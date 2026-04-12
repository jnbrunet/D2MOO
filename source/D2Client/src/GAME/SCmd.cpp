#include <GAME/SCmd.h>

#include <cstdint>
#include <cstring>
#include <cstdio>

#include <D2BitManip.h>
#include <D2Dll.h>
#include <D2PacketDef.h>
#include <Fog.h>
#include <Server.h>
#include <Units/Units.h>
#include <UNIT/CUnit.h>

namespace
{
	constexpr uint32_t NUM_SCMDS = 0xAE;
	constexpr uint32_t INVALID_PACKET_SIZE = UINT32_MAX;
	constexpr uintptr_t GAME_PACKET_HANDLERS_RVA = 0xD6270;

	int32_t ReadPacketUnitGUID(const uint8_t* pPacketBuffer, size_t nOffset)
	{
		int32_t nUnitGUID = -1;
		std::memcpy(&nUnitGUID, pPacketBuffer + nOffset, sizeof(nUnitGUID));
		return nUnitGUID;
	}

	D2UnitStrc* GetPacketUnit(uint8_t nCmd, const uint8_t* pPacketBuffer)
	{
		if (nCmd >= 0x67 && nCmd <= 0x6D)
		{
			return UNIT_GetUnitFromIndex(ReadPacketUnitGUID(pPacketBuffer, 1), static_cast<D2C_UnitTypes>(1));
		}

		return UNIT_GetUnitFromIndex(ReadPacketUnitGUID(pPacketBuffer, 2), static_cast<D2C_UnitTypes>(pPacketBuffer[1]));
	}

	void QueuePacketForUnit(D2UnitStrc* pUnit, const uint8_t* pPacketBuffer, uint32_t nPacketSize, uint32_t nPacketBufferSize)
	{
		D2ClientUnitPacketListStrc* pUnitPacketList = D2Client_GetUnitPacketList_6FB29450(pUnit);
		if (!pUnitPacketList || pUnitPacketList->nCount + 1 == pUnitPacketList->nCapacity)
		{
			pUnitPacketList = SCMD_AllocatePacketListForUnit(pUnit);
		}

		D2_ASSERTM(nPacketSize <= nPacketBufferSize, "nSize <= sgnMaxUnitPacketDataSize");

		std::memcpy(pUnitPacketList->pPacketData + pUnitPacketList->nCount * nPacketBufferSize, pPacketBuffer, nPacketSize);
		++pUnitPacketList->nCount;
	}

	void ConsumePackedPacket0x18(uint8_t* pPacketBuffer)
	{
		D2BitBufferStrc bitBuffer = {};
		BITMANIP_Initialize(&bitBuffer, pPacketBuffer, sizeof(D2GSPacketSrv18));

		BITMANIP_Read(&bitBuffer, 8);
		BITMANIP_Read(&bitBuffer, 15);
		BITMANIP_Read(&bitBuffer, 15);
		BITMANIP_Read(&bitBuffer, 15);
		BITMANIP_Read(&bitBuffer, 7);
		BITMANIP_Read(&bitBuffer, 7);
		BITMANIP_Read(&bitBuffer, 16);
		BITMANIP_Read(&bitBuffer, 16);
		BITMANIP_Read(&bitBuffer, 8);
		BITMANIP_Read(&bitBuffer, 8);
	}

	void ConsumePackedPacket0x95(uint8_t* pPacketBuffer)
	{
		D2BitBufferStrc bitBuffer = {};
		BITMANIP_Initialize(&bitBuffer, pPacketBuffer, sizeof(D2GSPacketSrv95));

		BITMANIP_Read(&bitBuffer, 8);
		BITMANIP_Read(&bitBuffer, 15);
		BITMANIP_Read(&bitBuffer, 15);
		BITMANIP_Read(&bitBuffer, 15);
		BITMANIP_Read(&bitBuffer, 16);
		BITMANIP_Read(&bitBuffer, 16);
		BITMANIP_Read(&bitBuffer, 8);
		BITMANIP_Read(&bitBuffer, 8);
	}

	void ConsumePackedPacket0x96(uint8_t* pPacketBuffer)
	{
		D2BitBufferStrc bitBuffer = {};
		BITMANIP_Initialize(&bitBuffer, pPacketBuffer, sizeof(D2GSPacketSrv96));

		BITMANIP_Read(&bitBuffer, 8);
		BITMANIP_Read(&bitBuffer, 15);
		BITMANIP_Read(&bitBuffer, 16);
		BITMANIP_Read(&bitBuffer, 16);
		BITMANIP_Read(&bitBuffer, 8);
		BITMANIP_Read(&bitBuffer, 8);
	}
}

// D2Client + 0x10B9B0 -> 0x6FBAB9B0
// Max per-packet buffer size, lazily initialized from the handlers table.
#ifdef D2_VERSION_110F
static int32_t& g_nSCmdPacketBufferSize = *reinterpret_cast<int32_t*>(0x6FBAB9B0);
#else
static int32_t g_nSCmdPacketBufferSizeStorage = 0;
static int32_t& g_nSCmdPacketBufferSize = g_nSCmdPacketBufferSizeStorage;
#endif

void D2Client_SCmd_LoadOffsets()
{
	const uintptr_t nDllBase = uintptr_t(delayedD2ClientDllBaseGet());
	if (!nDllBase)
	{
		return;
	}

	D2Client_GamePacketHandlers = reinterpret_cast<D2Client_GamePacketHandlers_vt*>(nDllBase + GAME_PACKET_HANDLERS_RVA);
	D2Client_LastGamePacketCmd = reinterpret_cast<D2Client_LastGamePacketCmd_vt*>(nDllBase + 0x10B9B4);
	D2Client_ProcessedGamePacketBytes = reinterpret_cast<D2Client_ProcessedGamePacketBytes_vt*>(nDllBase + 0x121AF0);
	D2Client_CompletedGamePacketBatches = reinterpret_cast<D2Client_CompletedGamePacketBatches_vt*>(nDllBase + 0x121AF8);
	D2Client_LargeGamePacketCount = reinterpret_cast<D2Client_LargeGamePacketCount_vt*>(nDllBase + 0x121B00);

	D2Client_GetUnitPacketList_6FB29450 = reinterpret_cast<D2Client_GetUnitPacketList_6FB29450_t>(nDllBase + 0x89450);
	D2Client_SetUnitPacketList_6FB29480 = reinterpret_cast<D2Client_SetUnitPacketList_6FB29480_t>(nDllBase + 0x89480);
}

// TODO: reconstruct - D2Client + 0x11CB0
void __fastcall SCMD_HandleSystemPacket(uint8_t* /*pPacketBuffer*/, int /*nPacketSize*/) {}

// TODO: reconstruct - D2Client + 0x12190
void __fastcall SCMD_GameLoad(uint8_t* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x122A0
void __fastcall SCMD_SetClientIsInSight(uint8_t* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x12310
void __fastcall SCMD_UnsetClientIsInSight(uint8_t* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x12970
D2UnitStrc* __fastcall SCMD_UnitWarpXY(UnitWarpXY* /*arg*/) { return nullptr; }

// TODO: reconstruct - D2Client + 0x12A80 (pfProcessUnit)
void __fastcall SCMD_ReassignUnit(D2UnitStrc* /*pUnit*/, D2PacketBufferStrc* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x12B00
void __fastcall SCMD_MultipleSetAttributes(uint8_t* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x12F10
void __fastcall SCMD_SetAttribute(uint8_t* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x13570
void __fastcall SCMD_SKILLS_AssignSkill(uint8_t* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x13610
void __fastcall SCMD_UpdateUnitEx(uint8_t* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x13670
void __fastcall SCMD_SKILLS_SetQuantity(uint8_t* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x13E90
void __fastcall SCMD_UNITS_SetPlayerPortalFlags(uint8_t* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x13ED0
void __fastcall SCMD_UNITS_SetObjectPortalFlags(uint8_t* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x143E0
void __fastcall SCMD_WeaponSwap(uint8_t* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x14420
void __fastcall SCMD_UnitAction(uint8_t* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x14590
void __fastcall SCMD_AssignNPC(uint8_t* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x14700 (pfProcessUnit, sSCmd_UnitStateOnEx)
void __fastcall SCMD_UnitStateOnEx(D2UnitStrc* /*pUnit*/, D2PacketBufferStrc* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x14750 (pfProcessUnit, sSCmd_UnitStateOnValEx)
void __fastcall SCMD_UnitStateOnValEx(D2UnitStrc* /*pUnit*/, D2PacketBufferStrc* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x14900 (pfProcessUnit, sSCmd_UnitStateOffEx)
void __fastcall SCMD_UnitStateOffEx(D2UnitStrc* /*pUnit*/, D2PacketBufferStrc* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x14950 (pfProcessUnit, sSCmd_UnitStateAllEx)
void __fastcall SCMD_UnitStateAllEx(D2UnitStrc* /*pUnit*/, D2PacketBufferStrc* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x14B40 (pfProcessUnit, sSCmd_HealthUpdate)
void __fastcall SCMD_HealthUpdate(D2UnitStrc* /*pUnit*/, D2PacketBufferStrc* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x14BD0 (pfProcessUnit, sSCmd_NewMonsterEx)
void __fastcall SCMD_NewMonsterEx(D2UnitStrc* /*pUnit*/, D2PacketBufferStrc* /*pPacketBuffer*/) {}

// TODO: reconstruct - D2Client + 0x15090
void __fastcall SCMD_SetViewPos(uint8_t* /*pPacketBuffer*/) {}

// D2Client.0x6FAB54C0 (RVA: 0x154C0)
D2ClientUnitPacketListStrc* __fastcall SCMD_AllocatePacketListForUnit(D2UnitStrc* pUnit)
{
    // Step 1: lazily compute max packet buffer size from the handlers table on first call.
    int32_t nMaxSize = g_nSCmdPacketBufferSize;
    if (!g_nSCmdPacketBufferSize)
    {
        for (uint32_t i = 0; i < NUM_SCMDS; ++i)
        {
            if (D2Client_GamePacketHandlers[i].pfProcessUnit &&
                (int32_t)D2Client_GamePacketHandlers[i].expectedSize > nMaxSize)
            {
                nMaxSize = (int32_t)D2Client_GamePacketHandlers[i].expectedSize;
            }
        }
        g_nSCmdPacketBufferSize = nMaxSize + 5;
    }

    // Step 2: get or allocate the packet list header for this unit.
    D2_ASSERT(pUnit);
    auto* pPacketList = reinterpret_cast<D2ClientUnitPacketListStrc*>(pUnit->pPacketList);
    if (!pPacketList)
    {
        pPacketList = D2_CALLOC_STRC(D2ClientUnitPacketListStrc);
        D2_ASSERT(pUnit);
        pUnit->pPacketList = reinterpret_cast<D2PacketListStrc*>(pPacketList);
    }

    // Step 3: grow the data buffer by 5 slots (capacity += 5).
    const int32_t nNewCapacity = pPacketList->nCapacity + 5;
    auto* pNewBuffer = static_cast<uint8_t*>(D2_ALLOC(nNewCapacity * g_nSCmdPacketBufferSize));
    std::memset(pNewBuffer, 0, nNewCapacity * g_nSCmdPacketBufferSize);

    // Step 4: copy existing packets from old buffer, then free it.
    if (pPacketList->pPacketData)
    {
        std::memcpy(pNewBuffer, pPacketList->pPacketData,
                    g_nSCmdPacketBufferSize * pPacketList->nCapacity);
        D2_FREE(pPacketList->pPacketData);
    }

    pPacketList->nCapacity = nNewCapacity;
    pPacketList->pPacketData = pNewBuffer;
    return pPacketList;
}

//D2Client.0x6FAB50B0
void __fastcall SCMD_ParseGamePacket(uint8_t* pPacketBuffer, uint32_t nPacketSize)
{
	const uintptr_t nDllBase = uintptr_t(delayedD2ClientDllBaseGet());
	D2_ASSERTM(nDllBase != 0, "nDllBase != 0");

	int32_t nRemainingBytes = static_cast<int32_t>(nPacketSize);
	if (nRemainingBytes == -1)
	{
		return;
	}

	(void)::GetTickCount();

	if (nRemainingBytes > 0x1F4)
	{
		++(*D2Client_LargeGamePacketCount);
	}

	while (nRemainingBytes > 0)
	{
		const uint8_t nCmd = pPacketBuffer[0];
		int32_t nSize = 0;
		if (!SERVER_GetServerPacketSize(reinterpret_cast<D2PacketBufferStrc*>(pPacketBuffer), nRemainingBytes, &nSize))
		{
			return;
		}

		const uint32_t nPacketSize16 = static_cast<uint16_t>(nSize);

		D2_ASSERTM(nCmd < NUM_SCMDS, "bCmd < NUM_SCMDS");

		GamePacketDesc* pPacketDesc = &D2Client_GamePacketHandlers[nCmd];
		if (pPacketDesc->expectedSize != INVALID_PACKET_SIZE && nPacketSize16 != pPacketDesc->expectedSize)
		{
			D2_ASSERTM(false, "(ptMsgStruct->nCmdSize == -1) || (packetSize == ptMsgStruct->nCmdSize)");
		}

		if (pPacketDesc->pfProcessUnit)
		{
			if (D2UnitStrc* pUnit = GetPacketUnit(nCmd, pPacketBuffer))
			{
				QueuePacketForUnit(pUnit, pPacketBuffer, nPacketSize16, nPacketSize);
			}
		}

		bool bShouldDispatch = pPacketDesc->handler != nullptr;
		switch (nCmd)
		{
		case 0x0D:
			bShouldDispatch = bShouldDispatch
				&& reinterpret_cast<D2GSPacketSrv0D*>(pPacketBuffer)->nUnitType == 1
				&& UNIT_GetUnitFromIndex(reinterpret_cast<D2GSPacketSrv0D*>(pPacketBuffer)->dwUnitGUID, static_cast<D2C_UnitTypes>(1)) != nullptr;
			break;

		case 0x18:
			ConsumePackedPacket0x18(pPacketBuffer);
			break;

		case 0x95:
			ConsumePackedPacket0x95(pPacketBuffer);
			break;

		case 0x96:
			ConsumePackedPacket0x96(pPacketBuffer);
			break;

		default:
			break;
		}

		if (bShouldDispatch)
		{
			pPacketDesc->handler(pPacketBuffer);
		}

		*D2Client_LastGamePacketCmd = nCmd;
		nRemainingBytes -= static_cast<int32_t>(nPacketSize16);
		pPacketBuffer += nPacketSize16;
		*D2Client_ProcessedGamePacketBytes += nPacketSize16;
	}

	++(*D2Client_CompletedGamePacketBatches);
}

// D2Client.0x6FAB55C0
void __fastcall SCMD_FreeUnitPacketList(D2UnitStrc* pUnit)
{
	D2ClientUnitPacketListStrc* pPacketList = D2Client_GetUnitPacketList_6FB29450(pUnit);
	if (!pPacketList)
	{
		return;
	}

	D2_FREE(pPacketList->pPacketData);
	D2_FREE(pPacketList);
	D2Client_SetUnitPacketList_6FB29480(pUnit, nullptr);
}

// D2Client.0x6FAB5610
void __fastcall SCMD_ProcessUnitPackets(D2UnitStrc* pUnit)
{
	D2ClientUnitPacketListStrc* pPacketList = D2Client_GetUnitPacketList_6FB29450(pUnit);
	if (!pPacketList)
	{
		return;
	}

	D2_ASSERTM(pPacketList->nCount <= pPacketList->nCapacity, "ptPacket");

	int32_t i = 0;
	if (static_cast<int32_t>(pPacketList->nCount) > 0)
	{
		while (true)
		{
			auto* pPacketBuffer = reinterpret_cast<D2PacketBufferStrc*>(
				pPacketList->pPacketData + i * g_nSCmdPacketBufferSize);

			int32_t nSize = 0;
			if (!SERVER_GetServerPacketSize(pPacketBuffer, g_nSCmdPacketBufferSize, &nSize))
			{
				return;
			}

			const uint8_t nCmd = pPacketBuffer->data[0];
			D2_ASSERTM(nCmd < NUM_SCMDS, "bCmd < NUM_SCMDS");

			const GamePacketDesc* pPacketDesc = &D2Client_GamePacketHandlers[nCmd];
			if (pPacketDesc->expectedSize != INVALID_PACKET_SIZE &&
				static_cast<uint16_t>(nSize) != pPacketDesc->expectedSize)
			{
				D2_ASSERTM(false, "(ptMsgStruct->nCmdSize == -1) || (packetSize == ptMsgStruct->nCmdSize)");
			}

			D2_ASSERTM(pPacketDesc->pfProcessUnit != nullptr, "ptMsgStruct->pfProcessUnit");
			pPacketDesc->pfProcessUnit(pUnit, pPacketBuffer);

			// Re-read nCount: pfProcessUnit may have modified the queue
			if (++i >= static_cast<int32_t>(pPacketList->nCount))
			{
				break;
			}
		}
	}

	if (pPacketList->nCount)
	{
		std::memset(pPacketList->pPacketData, 0, g_nSCmdPacketBufferSize * pPacketList->nCount);
	}
	pPacketList->nCount = 0;
}

