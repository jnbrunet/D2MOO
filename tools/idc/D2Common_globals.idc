#include <idc.idc>

// =============================================================
// D2Common_globals.idc  --  D2MOO (reconstructed from headers + patch file)
// Renames and types global variables in D2Common.
// 8 globals extracted from:
//   - source/D2Common/include/D2DataTbls.h  (address annotations)
//   - D2.Detours.patches/1.10f/D2Common.patch.cpp  (extraPatchActions)
//
// Address layout (VA = D2Common ImageBase 0x6FD40000 + RVA):
//   0x96A20  sgptDataTables              (also exported @10042)
//   0x96A24  DATATBLS_LoadFromBin
//   0x9AF34  gpAutomapSeed
//   0xA95F8  gpCharTemplateTxtTable
//   0xA9600  gpArenaTxtTable
//   0xA9604  gpBeltsTxtTable
//   0xAA700  gpLevelFilesList_6FDEA700   (type unknown)
//   0xAA704  gpLvlSubTypeFilesCriticalSection
//
// For each global:
//   set_name(base + offset, name)   -- always applied
//   SetType(base + offset, type)     -- best-effort (needs d2moo_types_for_ida.h)
//
// *** PREREQUISITE for SetType: ***
//   File > Load file > Parse C header file
//   -> select tools/idc/d2moo_types_for_ida.h
// =============================================================

static main()
{
    // Find D2Common segment base dynamically
    auto base = BADADDR;
    auto seg;
    for (seg = FirstSeg(); seg != BADADDR; seg = NextSeg(seg)) {
        if (strstr(get_segm_name(seg), "D2Common") != -1) {
            base = SegStart(seg);
            break;
        }
    }
    if (base == BADADDR) {
        Message("[D2Common_globals] ERROR: segment 'D2Common' not found.\n");
        return;
    }
    Message("[D2Common_globals] base=0x%X  (8 globals)\n", base);

    auto addr;
    auto renamed = 0;
    auto typed   = 0;
    auto typeFail = 0;

    // sgptDataTables  (extern "C" D2COMMON_DLL_DECL D2DataTablesStrc * sgptDataTables)
    // D2Common.0x6FDD6A20 (#10042)  -- also exported by ordinal, ParseExports.idc covers it too
    addr = base + 0x96A20;
    set_name(addr, "sgptDataTables", 0x880); renamed++;
    if (SetType(addr, "D2DataTablesStrc * sgptDataTables")) typed++; else typeFail++;

    // DATATBLS_LoadFromBin  (extern BOOL DATATBLS_LoadFromBin)
    // D2Common.0x6FDD6A24
    addr = base + 0x96A24;
    set_name(addr, "DATATBLS_LoadFromBin", 0x880); renamed++;
    if (SetType(addr, "BOOL DATATBLS_LoadFromBin")) typed++; else typeFail++;

    // gpAutomapSeed  (extern D2SeedStrc* gpAutomapSeed)
    // D2Common.0x6FDDAF34  -- from extraPatchActions in D2Common.patch.cpp
    addr = base + 0x9AF34;
    set_name(addr, "gpAutomapSeed", 0x880); renamed++;
    if (SetType(addr, "D2SeedStrc * gpAutomapSeed")) typed++; else typeFail++;

    // gpCharTemplateTxtTable  (extern D2CharTemplateTxt* gpCharTemplateTxtTable)
    // D2Common.0x6FDE95F8  -- from D2DataTbls.h
    addr = base + 0xA95F8;
    set_name(addr, "gpCharTemplateTxtTable", 0x880); renamed++;
    if (SetType(addr, "D2CharTemplateTxt * gpCharTemplateTxtTable")) typed++; else typeFail++;

    // gpArenaTxtTable  (extern D2ArenaTxt* gpArenaTxtTable)
    // D2Common.0x6FDE9600  -- from D2DataTbls.h
    addr = base + 0xA9600;
    set_name(addr, "gpArenaTxtTable", 0x880); renamed++;
    if (SetType(addr, "D2ArenaTxt * gpArenaTxtTable")) typed++; else typeFail++;

    // gpBeltsTxtTable  (extern D2BeltsTxt* gpBeltsTxtTable)
    // D2Common.0x6FDE9604  -- from D2DataTbls.h
    addr = base + 0xA9604;
    set_name(addr, "gpBeltsTxtTable", 0x880); renamed++;
    if (SetType(addr, "D2BeltsTxt * gpBeltsTxtTable")) typed++; else typeFail++;

    // gpLevelFilesList_6FDEA700  (type unknown -- adjacent to gpLvlSubTypeFilesCriticalSection)
    // D2Common.0x6FDEA700  -- from extraPatchActions in D2Common.patch.cpp (commented-out entry)
    addr = base + 0xAA700;
    set_name(addr, "gpLevelFilesList_6FDEA700", 0x880); renamed++;
    if (SetType(addr, "void * gpLevelFilesList_6FDEA700")) typed++; else typeFail++;

    // gpLvlSubTypeFilesCriticalSection  (extern LPCRITICAL_SECTION gpLvlSubTypeFilesCriticalSection)
    // D2Common.0x6FDEA704  -- from D2DataTbls.h AND LevelsTbls.cpp (double confirmed)
    addr = base + 0xAA704;
    set_name(addr, "gpLvlSubTypeFilesCriticalSection", 0x880); renamed++;
    if (SetType(addr, "LPCRITICAL_SECTION gpLvlSubTypeFilesCriticalSection")) typed++; else typeFail++;

    Message("[D2Common_globals] Globals: %d renamed, %d typed, %d type-fail\n", renamed, typed, typeFail);
    Message("[D2Common_globals] type-fail is normal for struct types not yet in IDA.\n");
    Message("[D2Common_globals] NOTE: sgptDataTables also exported @10042 (ParseExports.idc covers it).\n");
    Message("[D2Common_globals] NOTE: gpLevelFilesList_6FDEA700 type unknown, typed as void*.\n");
}
