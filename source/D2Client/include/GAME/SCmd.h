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

// D2Client.dll + 0x155C0 (0x6FAB55C0)
void __fastcall SCMD_FreeUnitPacketList(D2UnitStrc* pUnit);

// D2Client.dll + 0x154C0 (0x6FAB54C0)
D2ClientUnitPacketListStrc* __fastcall SCMD_AllocatePacketListForUnit(D2UnitStrc* pUnit);

// D2Client.dll + 0x15610 (0x6FAB5610)
void __fastcall SCMD_ProcessUnitPackets(D2UnitStrc* pUnit);

void D2Client_SetOriginalModuleBase(void* hOriginalModule);
void D2Client_LoadOffsets();
void D2Client_SCmd_LoadOffsets();

// D2Client.dll + 0x11CB0 (0x6FAB1CB0)
void __fastcall SCMD_HandleSystemPacket(uint8_t* pPacketBuffer, int nPacketSize);

// D2Client.dll + 0x12190 (0x6FAB2190)
void __fastcall SCMD_GameLoad(uint8_t* pPacketBuffer);

// D2Client.dll + 0x122A0 (0x6FAB22A0)
void __fastcall SCMD_SetClientIsInSight(uint8_t* pPacketBuffer);

// D2Client.dll + 0x12310 (0x6FAB2310)
void __fastcall SCMD_UnsetClientIsInSight(uint8_t* pPacketBuffer);

// D2Client.dll + 0x12970 (0x6FAB2970)
D2UnitStrc *__fastcall SCMD_UnitWarpXY(UnitWarpXY *arg);

// D2Client.dll + 0x12A80 (0x6FAB2A80)
void __fastcall SCMD_ReassignUnit(D2UnitStrc* pUnit, D2PacketBufferStrc* pPacketBuffer);

// D2Client.dll + 0x12B00 (0x6FAB2B00)
void __fastcall SCMD_MultipleSetAttributes(uint8_t* pPacketBuffer);

// D2Client.dll + 0x12F10 (0x6FAB2F10)
void __fastcall SCMD_SetAttribute(uint8_t* pPacketBuffer);

// D2Client.dll + 0x13570 (0x6FAB3570)
void __fastcall SCMD_SKILLS_AssignSkill(uint8_t* pPacketBuffer);

// D2Client.dll + 0x13610 (0x6FAB3610)
void __fastcall SCMD_UpdateUnitEx(uint8_t* pPacketBuffer);

// D2Client.dll + 0x13670 (0x6FAB3670)
void __fastcall SCMD_SKILLS_SetQuantity(uint8_t* pPacketBuffer);

// D2Client.dll + 0x13E90 (0x6FAB3E90)
void __fastcall SCMD_UNITS_SetPlayerPortalFlags(uint8_t* pPacketBuffer);

// D2Client.dll + 0x13ED0 (0x6FAB3ED0)
void __fastcall SCMD_UNITS_SetObjectPortalFlags(uint8_t* pPacketBuffer);

// D2Client.dll + 0x143E0 (0x6FAB43E0)
void __fastcall SCMD_WeaponSwap(uint8_t* pPacketBuffer);

// D2Client.dll + 0x14420 (0x6FAB4420)
void __fastcall SCMD_UnitAction(uint8_t* pPacketBuffer);

// D2Client.dll + 0x14590 (0x6FAB4590)
void __fastcall SCMD_AssignNPC(uint8_t* pPacketBuffer);

// D2Client.dll + 0x14700 (0x6FAB4700)
void __fastcall SCMD_UnitStateOnEx(D2UnitStrc* pUnit, D2PacketBufferStrc* pPacketBuffer);

// D2Client.dll + 0x14750 (0x6FAB4750)
void __fastcall SCMD_UnitStateOnValEx(D2UnitStrc* pUnit, D2PacketBufferStrc* pPacketBuffer);

// D2Client.dll + 0x14900 (0x6FAB4900)
void __fastcall SCMD_UnitStateOffEx(D2UnitStrc* pUnit, D2PacketBufferStrc* pPacketBuffer);

// D2Client.dll + 0x14950 (0x6FAB4950)
void __fastcall SCMD_UnitStateAllEx(D2UnitStrc* pUnit, D2PacketBufferStrc* pPacketBuffer);

// D2Client.dll + 0x14B40 (0x6FAB4B40)
void __fastcall SCMD_HealthUpdate(D2UnitStrc* pUnit, D2PacketBufferStrc* pPacketBuffer);

// D2Client.dll + 0x14BD0 (0x6FAB4BD0)
void __fastcall SCMD_NewMonsterEx(D2UnitStrc* pUnit, D2PacketBufferStrc* pPacketBuffer);

// D2Client.dll + 0x15090 (0x6FAB5090)
void __fastcall SCMD_SetViewPos(uint8_t* pPacketBuffer);

// D2Client.dll + 0x150B0 (0x6FAB50B0)
void __fastcall SCMD_ParseGamePacket(uint8_t *pPacketBuffer, uint32_t nPacketSize);
