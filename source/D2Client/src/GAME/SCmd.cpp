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
			pUnitPacketList = D2Client_AllocatePacketListForUnit_6FAB54C0(pUnit);
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
	D2Client_AllocatePacketListForUnit_6FAB54C0 = reinterpret_cast<D2Client_AllocatePacketListForUnit_6FAB54C0_t>(nDllBase + 0x154C0);
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

		if (pPacketDesc->flags)
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