#include <GAME/SCmd.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <D2BitManip.h>
#include <D2Dll.h>
#include <D2PacketDef.h>
#include <Fog.h>
#include <Server.h>
#include <Units/Units.h>

namespace
{
	constexpr uint32_t NUM_SCMDS = 0xAE;
	constexpr uint32_t INVALID_PACKET_SIZE = UINT32_MAX;
	constexpr uintptr_t GAME_PACKET_HANDLERS_RVA = 0xD6270;
	constexpr const char* D2CLIENT_SCMD_CPP = "C:\\projects\\D2\\head\\Diablo2\\Source\\D2Client\\SCmd.cpp";

	[[noreturn]] void D2Client_SCmdAbort(const char* szExpression, int nLine)
	{
		FOG_DisplayAssert(szExpression, D2CLIENT_SCMD_CPP, nLine);
		std::exit(-1);
	}

	void LogPacketSizeMismatch(uint8_t nCmd, int32_t nSizeRaw, uint32_t nSize16, uint32_t nExpected, const uint8_t* pPacketBuffer)
	{
		if (FILE* pFile = std::fopen("SCmd_size_mismatch.log", "a"))
		{
			std::fprintf(
				pFile,
				"cmd=0x%02X sizeRaw=%d size16=%u expected=%u bytes=%02X %02X %02X %02X\\n",
				nCmd,
				nSizeRaw,
				nSize16,
				nExpected,
				pPacketBuffer ? pPacketBuffer[0] : 0,
				pPacketBuffer ? pPacketBuffer[1] : 0,
				pPacketBuffer ? pPacketBuffer[2] : 0,
				pPacketBuffer ? pPacketBuffer[3] : 0);
			std::fclose(pFile);
		}
	}

	void LogSCmdBaseInfo(uintptr_t nDllBase)
	{
		static bool bLogged = false;
		if (bLogged)
		{
			return;
		}
		bLogged = true;

		char szPath[MAX_PATH] = {};
		GetModuleFileNameA(reinterpret_cast<HMODULE>(nDllBase), szPath, MAX_PATH);

		const uintptr_t nHandlersAddr = reinterpret_cast<uintptr_t>(D2Client_GamePacketHandlers);
		const uint32_t nCmd1Expected = D2Client_GamePacketHandlers ? D2Client_GamePacketHandlers[1].expectedSize : 0;

		if (FILE* pFile = std::fopen("SCmd_size_mismatch.log", "a"))
		{
			std::fprintf(
				pFile,
				"base=0x%p handlers=0x%p expectedCmd1=%u module=%s\\n",
				reinterpret_cast<void*>(nDllBase),
				reinterpret_cast<void*>(nHandlersAddr),
				nCmd1Expected,
				szPath);
			std::fclose(pFile);
		}
	}

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
			return D2Client_GetUnitFromIndex_6FB269F0(ReadPacketUnitGUID(pPacketBuffer, 1), static_cast<D2C_UnitTypes>(1));
		}

		return D2Client_GetUnitFromIndex_6FB269F0(ReadPacketUnitGUID(pPacketBuffer, 2), static_cast<D2C_UnitTypes>(pPacketBuffer[1]));
	}

	void QueuePacketForUnit(D2UnitStrc* pUnit, const uint8_t* pPacketBuffer, uint32_t nPacketSize, uint32_t nPacketBufferSize)
	{
		D2ClientUnitPacketListStrc* pUnitPacketList = D2Client_GetUnitPacketList_6FB29450(pUnit);
		if (!pUnitPacketList || pUnitPacketList->nCount + 1 == pUnitPacketList->nCapacity)
		{
			pUnitPacketList = D2Client_AllocatePacketListForUnit_6FAB54C0(pUnit);
		}

		if (nPacketSize > nPacketBufferSize)
		{
			D2Client_SCmdAbort("nSize <= sgnMaxUnitPacketDataSize", 1271);
		}

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
	D2Client_GetUnitFromIndex_6FB269F0 = reinterpret_cast<D2Client_GetUnitFromIndex_6FB269F0_t>(nDllBase + 0x869F0);
}

//D2Client.0x6FAB50B0
void __fastcall SCMD_ParseGamePacket(uint8_t* pPacketBuffer, uint32_t nPacketSize)
{
	const uintptr_t nDllBase = uintptr_t(delayedD2ClientDllBaseGet());
	if (!nDllBase)
	{
		D2Client_SCmdAbort("nDllBase != 0", 1202);
	}

	if (D2Client_GamePacketHandlers != reinterpret_cast<D2Client_GamePacketHandlers_vt*>(nDllBase + GAME_PACKET_HANDLERS_RVA))
	{
		D2Client_SCmd_LoadOffsets();
		if (D2Client_GamePacketHandlers != reinterpret_cast<D2Client_GamePacketHandlers_vt*>(nDllBase + GAME_PACKET_HANDLERS_RVA))
		{
			D2Client_SCmdAbort("g_D2Client_GamePacketHandlers == base + GAME_PACKET_HANDLERS_RVA", 1207);
		}
	}

	LogSCmdBaseInfo(nDllBase);

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

		if (nCmd >= NUM_SCMDS)
		{
			D2Client_SCmdAbort("bCmd < NUM_SCMDS", 1442);
		}

		GamePacketDesc* pPacketDesc = &D2Client_GamePacketHandlers[nCmd];
		if (pPacketDesc->expectedSize != INVALID_PACKET_SIZE && nPacketSize16 != pPacketDesc->expectedSize)
		{
			LogPacketSizeMismatch(nCmd, nSize, nPacketSize16, pPacketDesc->expectedSize, pPacketBuffer);
			D2Client_SCmdAbort("(ptMsgStruct->nCmdSize == -1) || (packetSize == ptMsgStruct->nCmdSize)", 1449);
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
				&& D2Client_GetUnitFromIndex_6FB269F0(reinterpret_cast<D2GSPacketSrv0D*>(pPacketBuffer)->dwUnitGUID, static_cast<D2C_UnitTypes>(1)) != nullptr;
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