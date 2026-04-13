// =============================================================
// D2MOO_all.idc  --  master script: runs all D2MOO IDC scripts
//
// Execution order:
//   1. Rename scripts  (set_name -- PE export table or base+RVA)
//   2. Extra renames   (non-exported internals from patch file)
//   3. Type signatures (SetType -- needs d2moo_types_for_ida.h loaded first)
//   4. Globals         (set_name + SetType for global variables)
//
// PREREQUISITE for type/globals scripts:
//   File > Load file > Parse C header file
//   -> select tools/idc/d2moo_types_for_ida.h
//   (without this, SetType calls will silently fail on custom types)
//
// All sub-scripts can still be run individually -- they each have their own
// main() which is suppressed here via #define D2MOO_ALL_IDC.
// =============================================================

// Must be defined before any #include so sub-scripts' main() are suppressed.
#define D2MOO_ALL_IDC

// --- 1. Rename scripts ---
#include "D2Common.idc"
#include "D2Game.idc"
#include "D2Client.idc"
#include "Fog.idc"
#include "D2Win.idc"
#include "D2Gfx.idc"
#include "D2Lang.idc"
#include "D2Net.idc"
#include "D2Sound.idc"
#include "Storm.idc"
#include "D2CMP.idc"

// --- 2. Extra renames ---
#include "D2Common_extra.idc"

// --- 3. Type signatures ---
#include "D2Common_types.idc"
#include "D2Game_types.idc"
#include "D2Client_types.idc"

// --- 4. Globals ---
#include "D2Common_globals.idc"
#include "D2Client_globals.idc"

static main()
{
    Message("\n[D2MOO] ============================================\n");
    Message("[D2MOO]  D2MOO_all.idc  --  applying all names/types\n");
    Message("[D2MOO] ============================================\n\n");

    // --- 1. Rename scripts ---
    Message("[D2MOO] --- 1/4  Rename scripts ---\n");
    Apply_D2Common();
    Apply_D2Game();
    Apply_D2Client();
    Apply_Fog();
    Apply_D2Win();
    Apply_D2gfx();
    Apply_D2Lang();
    Apply_D2Net();
    Apply_D2sound();
    Apply_Storm();
    Apply_D2CMP();

    // --- 2. Extra renames ---
    Message("\n[D2MOO] --- 2/4  Extra renames ---\n");
    Apply_D2Common_extra();

    // --- 3. Type signatures ---
    Message("\n[D2MOO] --- 3/4  Type signatures (needs d2moo_types_for_ida.h) ---\n");
    Apply_D2Common_types();
    Apply_D2Game_types();
    Apply_D2Client_types();

    // --- 4. Globals ---
    Message("\n[D2MOO] --- 4/4  Globals ---\n");
    Apply_D2Common_globals();
    Apply_D2Client_globals();

    Message("\n[D2MOO] ============================================\n");
    Message("[D2MOO]  All scripts finished.\n");
    Message("[D2MOO] ============================================\n\n");
}

