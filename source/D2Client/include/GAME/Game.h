#pragma once

#include <Windows.h>

// D2Client.dll + 0xB370 (0x6FAAB370)
int GAME_Main();

// D2Client.dll + 0xA3B0 (0x6FAAA3B0)
DWORD __stdcall GAME_OpenServerThread(LPVOID a1);

// D2Client.dll + 0x9AF0 (0x6FAA9AF0)
int __stdcall GAME_InGameTick(); // Arguments not sure

// D2Client.dll + 0x9450 (0x6FAA9450)
int GAME_AllocateGlobals();

// D2Client.dll + 0x28E0 (0x6FAA28E0)
void GAME_DisplayPerformanceStatistics();
