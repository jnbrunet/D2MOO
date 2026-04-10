#pragma once

// D2Client + 0x12970 -> D2Client.0x6FAB2970
D2UnitStrc *__fastcall D2Client_sSCmd_UnitWarpXY(UnitWarpXY *arg);

// D2Client + 0x150B0 -> 6FAB50B0
void D2Client_ParseGamePacket(uint8_t *pPacketBuffer, uint32_t nPacketSize);
