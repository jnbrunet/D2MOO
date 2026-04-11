#pragma once

#include <Windows.h>

// D2Client + 0xB370 -> 6FAAB370
int GAME_Main();

// D2Client + 0xA3B0 -> 6FAAA3B0
DWORD __stdcall GAME_OpenServerThread(LPVOID a1);

// D2Client + 0x9AF0 -> 6FAA9AF0
int __stdcall GAME_InGameTick(); // Arguments not sure

// D2Client + 0x9450 -> 6FAA9450
int GAME_AllocateGlobals();

// D2Client + 0x28E0 -> 6FAA28E0
void GAME_DisplayPerformanceStatistics();
