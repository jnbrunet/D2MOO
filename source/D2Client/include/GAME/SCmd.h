#pragma once

#include <Windows.h>

#include <cstdint>

#include <D2Dll.h>


struct D2UnitStrc;
struct D2PacketBufferStrc;
struct D2PacketListStrc;
struct UnitWarpXY;
enum D2C_UnitTypes : int;

struct D2ClientUnitPacketListStrc
{
	uint32_t nCapacity;
	uint32_t nCount;
	uint32_t unk0x08;
	uint8_t* pPacketData;
};


using D2ClientGamePacketHandler = void(__fastcall*)(uint8_t* pPacketBuffer);
using D2ClientGamePacketUnitHandler = void(__fastcall*)(D2UnitStrc* pUnit, D2PacketBufferStrc* pPacketBuffer);

struct GamePacketDesc
{
	D2ClientGamePacketHandler handler;
	uint32_t expectedSize;
	D2ClientGamePacketUnitHandler pfProcessUnit;
};

HMODULE delayedD2ClientDllBaseGet();

D2VAR(D2Client, GamePacketHandlers, GamePacketDesc, 0xD6270);
D2VAR(D2Client, LastGamePacketCmd, uint8_t, 0x10B9B4);
D2VAR(D2Client, ProcessedGamePacketBytes, uint32_t, 0x121AF0);
D2VAR(D2Client, CompletedGamePacketBatches, uint32_t, 0x121AF8);
D2VAR(D2Client, LargeGamePacketCount, uint32_t, 0x121B00);

D2FUNC(D2Client, GetUnitPacketList_6FB29450, D2ClientUnitPacketListStrc*, __fastcall, (D2UnitStrc * pUnit), 0x89450);
D2FUNC(D2Client, SetUnitPacketList_6FB29480, void, __fastcall, (D2UnitStrc* pUnit, D2PacketListStrc* pPacketList), 0x89480);

// D2Client + 0x155C0 -> 0x6FAB55C0
void __fastcall SCMD_FreeUnitPacketList(D2UnitStrc* pUnit);

// D2Client + 0x154C0 -> 0x6FAB54C0
D2ClientUnitPacketListStrc* __fastcall SCMD_AllocatePacketListForUnit(D2UnitStrc* pUnit);

// D2Client + 0x15610 -> 0x6FAB5610
void __fastcall SCMD_ProcessUnitPackets(D2UnitStrc* pUnit);

void D2Client_SetOriginalModuleBase(void* hOriginalModule);
void D2Client_LoadOffsets();
void D2Client_SCmd_LoadOffsets();

// D2Client + 0x12970 -> D2Client.0x6FAB2970
D2UnitStrc *__fastcall SCMD_UnitWarpXY(UnitWarpXY *arg);

// D2Client + 0x150B0 -> 6FAB50B0
void __fastcall SCMD_ParseGamePacket(uint8_t *pPacketBuffer, uint32_t nPacketSize);
