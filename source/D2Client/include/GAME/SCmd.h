#pragma once

#include <Windows.h>

#include <cstdint>

#include <D2Dll.h>


struct D2UnitStrc;
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

struct GamePacketDesc
{
	D2ClientGamePacketHandler handler;
	uint32_t expectedSize;
	uint32_t flags;
};

HMODULE delayedD2ClientDllBaseGet();

D2VAR(D2Client, GamePacketHandlers, GamePacketDesc, 0xD6270);
D2VAR(D2Client, LastGamePacketCmd, uint8_t, 0x10B9B4);
D2VAR(D2Client, ProcessedGamePacketBytes, uint32_t, 0x121AF0);
D2VAR(D2Client, CompletedGamePacketBatches, uint32_t, 0x121AF8);
D2VAR(D2Client, LargeGamePacketCount, uint32_t, 0x121B00);

D2FUNC(D2Client, GetUnitPacketList_6FB29450, D2ClientUnitPacketListStrc*, __fastcall, (D2UnitStrc * pUnit), 0x89450);
D2FUNC(D2Client, AllocatePacketListForUnit_6FAB54C0, D2ClientUnitPacketListStrc*, __fastcall, (D2UnitStrc * pUnit), 0x154C0);
D2FUNC(D2Client, GetUnitFromIndex_6FB269F0, D2UnitStrc*, __fastcall, (int32_t nUnitGUID, D2C_UnitTypes nUnitType), 0x869F0);

void D2Client_SetOriginalModuleBase(void* hOriginalModule);
void D2Client_LoadOffsets();
void D2Client_SCmd_LoadOffsets();

// D2Client + 0x12970 -> D2Client.0x6FAB2970
D2UnitStrc *__fastcall D2Client_sSCmd_UnitWarpXY(UnitWarpXY *arg);

// D2Client + 0x150B0 -> 6FAB50B0
void __fastcall D2Client_ParseGamePacket(uint8_t *pPacketBuffer, uint32_t nPacketSize);
