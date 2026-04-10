#include <chrono>
#include <thread>
#include <type_traits>

#include <D2Debugger.h>
#include <GAME/Game.h>
#include <UNIT/SUnitDmg.h>


decltype(&GAME_UpdateProgress) GAME_UpdateProgress_Original = nullptr;
decltype(&SUNITDMG_ExecuteEvents) SUNITDMG_ExecuteEvents_Original = nullptr;

bool CheckEnvVarTrue(const char* envVarName)
{
	char* envBuffer = nullptr;
	size_t bufferSize = 0;

	if (0 == _dupenv_s(&envBuffer, &bufferSize, envVarName))
	{
		const bool isSet = envBuffer && *envBuffer == '1';
		free(envBuffer);
		return isSet;
	}
	return false;
}

bool IsDebuggerEnabled()
{
	static bool bEnabledFromCommandLine = strstr(GetCommandLineA(), "-debug") || CheckEnvVarTrue("D2_DEBUGGER");
	if (bEnabledFromCommandLine)
	{
		return true;
	}
	// TODO: support toggling window based on chat command ?
	return false;
}

void __fastcall GAME_UpdateProgress_WithDebugger(D2GameStrc* pGame)
{
	if (IsDebuggerEnabled())
	{
		static bool bDebuggerAvailable = D2DebuggerInit() == 0;
		if (bDebuggerAvailable)
		{
			static bool bFreezeGame = false;
			do {
				D2DebuggerNewFrame();
				if (!bFreezeGame)
				{
					GAME_UpdateProgress_Original(pGame);
				}
				bFreezeGame = D2DebugGame(pGame);
				D2DebuggerEndFrame(bFreezeGame/*vsync ON if frozen*/);
			} while (bFreezeGame);
			return;
		}
	}
	GAME_UpdateProgress_Original(pGame);
}

static void D2Debugger_ApplyGodMode(D2UnitStrc* pDefender, D2DamageStrc* pDamage)
{
	if (!pDefender || !pDamage || pDefender->dwUnitType != UNIT_PLAYER || !D2DebuggerIsGodModeEnabled())
	{
		return;
	}

	pDamage->dwPhysDamage = 0;
	pDamage->dwFireDamage = 0;
	pDamage->dwBurnDamage = 0;
	pDamage->dwBurnLen = 0;
	pDamage->dwLtngDamage = 0;
	pDamage->dwMagDamage = 0;
	pDamage->dwColdDamage = 0;
	pDamage->dwPoisDamage = 0;
	pDamage->dwPoisLen = 0;
	pDamage->dwColdLen = 0;
	pDamage->dwFrzLen = 0;
	pDamage->dwManaLeech = 0;
	pDamage->dwStamLeech = 0;
	pDamage->dwStunLen = 0;
	pDamage->dwDmgTotal = 0;
	pDamage->wResultFlags &= static_cast<uint16_t>(~DAMAGERESULTFLAG_WILLDIE);
}

void __fastcall SUNITDMG_ExecuteEvents_WithDebugger(D2GameStrc* pGame, D2UnitStrc* pAttacker, D2UnitStrc* pDefender, int32_t bMissile, D2DamageStrc* pDamage)
{
	D2Debugger_ApplyGodMode(pDefender, pDamage);
	SUNITDMG_ExecuteEvents_Original(pGame, pAttacker, pDefender, bMissile, pDamage);
}

static_assert(std::is_same_v<decltype(&GAME_UpdateProgress), decltype(&GAME_UpdateProgress_WithDebugger)>, "GAME_UpdateProgress_WithDebugger has a different type than previously known.");
static_assert(std::is_same_v<decltype(&SUNITDMG_ExecuteEvents), decltype(&SUNITDMG_ExecuteEvents_WithDebugger)>, "SUNITDMG_ExecuteEvents_WithDebugger has a different type than previously known.");
