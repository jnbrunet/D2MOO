#include <chrono>
#include <thread>
#include <type_traits>

#include <D2Debugger.h>
#include <GAME/Game.h>
#include <UNIT/SUnitDmg.h>


extern decltype(&GAME_UpdateProgress) GAME_UpdateProgress_Original;
void __fastcall GAME_UpdateProgress_WithDebugger(D2GameStrc* pGame);

extern decltype(&SUNITDMG_ExecuteEvents) SUNITDMG_ExecuteEvents_Original;
void __fastcall SUNITDMG_ExecuteEvents_WithDebugger(D2GameStrc* pGame, D2UnitStrc* pAttacker, D2UnitStrc* pDefender, int32_t bMissile, D2DamageStrc* pDamage);
