#pragma once

#include <Windows.h>

// D2Client + 0xB370 -> 6FAAB370
int D2Client_Main();

// D2Client + 0xA3B0 -> 6FAAA3B0
DWORD __stdcall D2Client_OpenServerThread(LPVOID a1);

// D2Client + 0x9AF0 -> 6FAA9AF0
int __stdcall D2Client_InGame_Tick(); // Arguments not sure

// D2Client + 0x9450 -> 6FAA9450
int D2Client_AllocateGlobals();

// D2Client + 0x28E0 -> 6FAA28E0
void D2Client_DisplayPerformanceStatistics();
