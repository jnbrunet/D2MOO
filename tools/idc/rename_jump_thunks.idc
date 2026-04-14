#include <idc.idc>

// =========================================================================
// rename_jump_thunks.idc  -  D2MOO helper
//
// Scans every function in the database and identifies "jump thunks" --
// functions whose body is a single JMP instruction.  Each thunk is then
// renamed:
//       j_<target>           (first occurrence)
//       j_<target>_0         (second occurrence)
//       j_<target>_1         (third occurrence)  …
//
// Typical use-case (D2Client.dll / D2Game.dll thunks into D2Common.dll):
//       j_d2common_10369    ->  j_UNITS_GetAnimOrSeqMode
//       j_d2common_10369_0  ->  j_UNITS_GetAnimOrSeqMode_0
//
// Run AFTER all DLL renaming scripts (D2Common.idc, D2Client.idc, …)
// so that the target function names are already resolved.
//
// Tested with IDA 7.x on a Diablo II 1.10f process dump.
// IDA 6.x compat: replace  set_name(a,n,0x880)     with  MakeNameEx(a,n,0x800)
//                 replace  get_name_ea(BADADDR,n)   with  LocByName(n)
//                 replace  next_head(ea,end)        with  NextHead(ea,end)
// =========================================================================

// -------------------------------------------------------------------------
// Returns 1 if the name is an IDA auto-generated name for a data item or
// code label that is NOT a meaningful function name.
// We refuse to use these as thunk target names.
// -------------------------------------------------------------------------
static IsAutoDataName(name)
{
    if (strstr(name, "dword_") == 0) return 1;
    if (strstr(name, "word_")  == 0) return 1;
    if (strstr(name, "byte_")  == 0) return 1;
    if (strstr(name, "qword_") == 0) return 1;
    if (strstr(name, "unk_")   == 0) return 1;
    if (strstr(name, "off_")   == 0) return 1;
    if (strstr(name, "loc_")   == 0) return 1;
    return 0;
}

// -------------------------------------------------------------------------
// Strip common import-table name prefixes (__imp__, _imp__, etc.)
// so that  __imp__UNITS_GetAnimOrSeqMode  becomes  UNITS_GetAnimOrSeqMode.
// -------------------------------------------------------------------------
static StripImpPrefix(name)
{
    if (strstr(name, "__imp__") == 0) return substr(name, 7, -1);
    if (strstr(name, "__imp_")  == 0) return substr(name, 6, -1);
    if (strstr(name, "_imp__")  == 0) return substr(name, 6, -1);
    if (strstr(name, "_imp_")   == 0) return substr(name, 5, -1);
    return name;
}

// -------------------------------------------------------------------------
// Given the EA of a JMP instruction, return the name of its callee.
//
// For  jmp [mem]  (IAT thunk in a process dump) the IAT pointer is
// dereferenced with Dword() – this works because the loader has already
// resolved the import table in the dump image.
//
// Returns "" when the name cannot be determined.
// -------------------------------------------------------------------------
static ResolveJmpTargetName(ea)
{
    auto optype, opval, n, ptr, slot;
    optype = GetOpType(ea, 0);
    opval  = GetOperandValue(ea, 0);

    // Direct near/far jump:  jmp 0x12345678
    if (optype == 7 || optype == 6) {
        n = get_name(opval);
        if (n != 0 && n != "") return n;
        return "";
    }

    // Indirect through memory:  jmp ds:[0x12345678]  (typical IAT thunk)
    if (optype == 2) {
        // In a process dump the IAT has been filled by the loader, so
        // Dword(opval) is the runtime address of the actual target function.
        ptr = Dword(opval);
        if (ptr != 0 && ptr != 0xFFFFFFFF) {
            n = get_name(ptr);
            if (n != 0 && n != "") return n;
        }
        // Fall-back: use the name IDA gave to the IAT slot itself and strip
        // any __imp__ / _imp__ decoration.
        // Reject auto-generated data names (dword_, unk_, …) as unusable.
        slot = get_name(opval);
        if (slot != 0 && slot != "" && !IsAutoDataName(slot))
            return StripImpPrefix(slot);
    }

    return "";
}

// -------------------------------------------------------------------------
// If funcEA is a pure JMP thunk, return the callee name; otherwise "".
//
// A function qualifies as a thunk when:
//   1. its first decoded instruction is a JMP, AND
//   2. every subsequent decoded instruction inside the function body is a
//      NOP  (alignment padding that IDA sometimes folds into the function).
// -------------------------------------------------------------------------
static GetThunkTargetName(funcEA)
{
    auto start = get_func_attr(funcEA, FUNCATTR_START);
    auto end   = get_func_attr(funcEA, FUNCATTR_END);
    if (start == BADADDR || end == BADADDR) return "";

    // Function start is always an instruction head; no need for FirstHead().
    auto insn = start;
    if (GetMnem(insn) != "jmp") return "";

    // Verify that only NOPs follow (alignment padding).
    // next_head(ea, maxea) returns the next head strictly after ea (IDA 7.x).
    auto next = next_head(insn, end);
    while (next != BADADDR && next < end) {
        if (GetMnem(next) != "nop") return "";
        next = next_head(next, end);
    }

    return ResolveJmpTargetName(insn);
}

// -------------------------------------------------------------------------
// Build a name that is not yet in use (or is already owned by thisAddr).
//   first try  j_<base>
//   then       j_<base>_0 , j_<base>_1 , …
// Returns "" after 256 failed attempts.
// -------------------------------------------------------------------------
static BuildUniqueName(base, thisAddr)
{
    auto candidate = "j_" + base;
    auto existing  = get_name_ea(BADADDR, candidate);
    if (existing == BADADDR || existing == thisAddr) return candidate;

    auto i;
    for (i = 0; i < 256; i++) {
        candidate = "j_" + base + "_" + i;
        existing  = get_name_ea(BADADDR, candidate);
        if (existing == BADADDR || existing == thisAddr) return candidate;
    }
    return "";
}

// -------------------------------------------------------------------------
// Main pass – iterate all functions and rename every thunk found.
// -------------------------------------------------------------------------
static RenameJumpThunks()
{
    auto renamed, already, skipped;
    auto funcEA, targetName, newName, curName;
    renamed = 0;
    already = 0;
    skipped = 0;

    Message("[RenameJumpThunks] Scanning all functions...\n");

    for (funcEA = NextFunction(0); funcEA != BADADDR; funcEA = NextFunction(funcEA)) {

        targetName = GetThunkTargetName(funcEA);
        if (targetName == "" || targetName == 0) {
            skipped++;
            continue;
        }

        // Reject auto-generated IDA data/label names (dword_, byte_, loc_, …)
        // – these are not meaningful function names.
        if (IsAutoDataName(targetName)) {
            skipped++;
            continue;
        }

        // If the resolved target is itself a thunk (j_foo), strip its "j_"
        // prefix so we do not produce double-prefixed names like j_j_foo.
        if (strstr(targetName, "j_") == 0)
            targetName = substr(targetName, 2, -1);

        newName = BuildUniqueName(targetName, funcEA);
        if (newName == "") {
            Message("[RenameJumpThunks] WARNING: no unique name for 0x%X (target=%s)\n",
                    funcEA, targetName);
            skipped++;
            continue;
        }

        curName = get_func_name(funcEA);
        if (curName == newName) {
            already++;
            continue;
        }

        if (set_name(funcEA, newName, 0x880)) {
            Message("[RenameJumpThunks]  0x%X  %-40s  ->  %s\n", funcEA, curName, newName);
            renamed++;
        } else {
            Message("[RenameJumpThunks]  FAILED  0x%X  %s  ->  %s\n",
                    funcEA, curName, newName);
            skipped++;
        }
    }

    Message("[RenameJumpThunks] Done.  renamed=%d  already_ok=%d  skipped/non-thunk=%d\n",
            renamed, already, skipped);
}

static main()
{
    RenameJumpThunks();
}

