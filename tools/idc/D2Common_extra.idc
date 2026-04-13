#include <idc.idc>
// =============================================================
// D2Common_extra.idc  --  D2MOO non-exported internal functions
// Source: D2.Detours.patches/1.10f/D2Common.patch.cpp extraPatchActions[]
//
// These are NOT in the PE export table -- patched by RVA directly.
// Addresses are computed as:  base + RVA
// where RVA = original_VA - 0x6FD40000 (D2Common ImageBase)
//
// The base is found dynamically by scanning IDA segments for "D2Common".
// Same approach as ParseExports.idc.
// =============================================================
static Apply_D2Common_extra()
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
        Message("[D2Common_extra] ERROR: segment 'D2Common' not found.\n");
        return;
    }
    Message("[D2Common_extra] base=0x%X\n", base);
    auto addr;
    auto renamed = 0;
    // RVAs from D2.Detours.patches/1.10f/D2Common.patch.cpp extraPatchActions[]
    // RVA = original_VA - 0x6FD40000
    addr = base + 0x46050; set_name(addr, "DRLGPRESET_LoadDrlgFile",        0x880); renamed++;
    addr = base + 0x46190; set_name(addr, "DRLGPRESET_FreeDrlgFile",        0x880); renamed++;
    addr = base + 0x47F20; set_name(addr, "DRLGPRESET_FreeDrlgMap",         0x880); renamed++;
    addr = base + 0x22020; set_name(addr, "DATATBLS_LoadLvlSubTxt",         0x880); renamed++;
    addr = base + 0x22600; set_name(addr, "DATATBLS_UnloadLvlSubTxt",       0x880); renamed++;
    addr = base + 0x07840; set_name(addr, "DATATBLS_LoadArenaTxt",          0x880); renamed++;
    addr = base + 0x079B0; set_name(addr, "DATATBLS_UnloadArenaTxt",        0x880); renamed++;
    addr = base + 0x079D0; set_name(addr, "DATATBLS_LoadCharTemplateTxt",   0x880); renamed++;
    addr = base + 0x08770; set_name(addr, "DATATBLS_UnloadCharTemplateTxt", 0x880); renamed++;
    addr = base + 0x093A0; set_name(addr, "DATATBLS_UnloadBeltsTxt",        0x880); renamed++;
    Message("[D2Common_extra] Done: %d non-exported functions renamed.\n", renamed);
}

#ifndef D2MOO_ALL_IDC
static main() { Apply_D2Common_extra(); }
#endif
