# gen_ida_idc_types.ps1
# Generates IDA IDC scripts that apply function type signatures (SetType).
# Parses D2MOO C++ headers for annotated function declarations and emits:
#   addr = base + RVA;
#   SetType(addr, "RETURN CONV FUNCNAME(ARGS)");
#
# Supported comment formats per DLL:
#   D2Common/D2Game : //D2Common.0xVA  or  //D2Game.0xVA
#   D2Client        : // D2Client + 0xOFFSET -> ...   (offset = RVA)
#                     // D2Client.0xVA  (absolute VA)
#                     D2FUNC(D2Client, NAME, RET, CONV, (ARGS), OFFSET)
#
# PREREQUISITE: Import D2MOO type headers into IDA first, otherwise SetType
# will fail for functions that use custom struct types (D2DrlgStrc*, etc.).
# See tools/idc/README_types.md for the import procedure.
#
# Usage:
#   .\gen_ida_idc_types.ps1                          # D2Common + D2Game + D2Client
#   .\gen_ida_idc_types.ps1 -Dlls D2Common           # specific DLL
#   .\gen_ida_idc_types.ps1 -OutputDir C:\tmp\idc

param(
    [string]   $SourceDir = "$PSScriptRoot\..\source",
    [string]   $OutputDir = "$PSScriptRoot\idc",
    [string[]] $Dlls      = @("D2Common", "D2Game", "D2Client", "D2CMP", "D2Gfx", "D2Lang", "D2Net", "D2Win", "D2Sound", "Fog", "Storm")
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

# ---------------------------------------------------------------------------
# DLL-specific configuration
# CommentPatterns: array of @{ Rx = regex; IsOffset = bool }
#   IsOffset=$true  -> captured group is an RVA (offset from ImageBase)
#   IsOffset=$false -> captured group is an absolute VA (subtract ImageBase)
# ---------------------------------------------------------------------------
$DllConfig = @{
    "D2Common" = @{
        ImageBase         = 0x6FD40000L
        SegPattern        = "D2Common"
        HeaderDir         = "D2Common\include"
        StripRx           = 'D2COMMON_DLL_DECL\s+'
        GenerateRenameIDC = $false   # covered by gen_ida_idc.ps1 via .def file
        CommentPatterns = @(
            @{ Rx = '^// ?D2Common\.0x([0-9A-Fa-f]{8})'; IsOffset = $false }
        )
    }
    "D2Game" = @{
        ImageBase         = 0x6FC30000L
        SegPattern        = "D2Game"
        HeaderDir         = "D2Game\include"
        StripRx           = 'D2GAME_DLL_DECL\s+'
        GenerateRenameIDC = $false   # covered by gen_ida_idc.ps1 via .def file
        CommentPatterns = @(
            @{ Rx = '^// ?D2Game\.0x([0-9A-Fa-f]{8})'; IsOffset = $false }
        )
    }
    "D2Client" = @{
        ImageBase         = 0x6FAA0000L
        SegPattern        = "D2Client"
        HeaderDir         = "D2Client\include"
        StripRx           = ''
        GenerateRenameIDC = $true    # no .def exports -- generate set_name IDC from headers
        CommentPatterns   = @(
            # Canonical format: // D2Client.dll + 0xRVA (0xVA)
            @{ Rx = '^// D2Client\.dll \+ 0x([0-9A-Fa-f]+) \('; IsOffset = $true }
        )
    }
    "D2CMP" = @{
        ImageBase         = 0x6FDF0000L
        SegPattern        = "D2CMP"
        HeaderDir         = "D2CMP\include"
        StripRx           = ''
        GenerateRenameIDC = $false   # covered by gen_ida_idc.ps1 via .def file
        CommentPatterns   = @()      # no //DllName.0xVA comments; uses D2FUNC_DLL macros
    }
    "D2Gfx" = @{
        ImageBase         = 0x6FA70000L
        SegPattern        = "D2Gfx"
        HeaderDir         = "D2Gfx\include"
        StripRx           = 'D2GFX_DLL_DECL\s+'
        GenerateRenameIDC = $false
        CommentPatterns   = @(
            @{ Rx = '^// ?D2Gfx\.0x([0-9A-Fa-f]{8})'; IsOffset = $false }
        )
    }
    "D2Lang" = @{
        ImageBase         = 0x6FC10000L
        SegPattern        = "D2Lang"
        HeaderDir         = "D2Lang\include"
        StripRx           = 'D2LANG_DLL_DECL\s+'
        GenerateRenameIDC = $false
        CommentPatterns   = @(
            @{ Rx = '^// ?D2Lang\.0x([0-9A-Fa-f]{8})'; IsOffset = $false }
        )
    }
    "D2Net" = @{
        ImageBase         = 0x6FC00000L
        SegPattern        = "D2Net"
        HeaderDir         = "D2Net\include"
        StripRx           = 'D2NET_DLL_DECL\s+'
        GenerateRenameIDC = $false
        CommentPatterns   = @(
            @{ Rx = '^// ?D2Net\.0x([0-9A-Fa-f]{8})'; IsOffset = $false }
        )
    }
    "D2Win" = @{
        ImageBase         = 0x6F8A0000L
        SegPattern        = "D2Win"
        HeaderDir         = "D2Win\include"
        StripRx           = 'D2WIN_DLL_DECL\s+'
        GenerateRenameIDC = $false
        CommentPatterns   = @(
            @{ Rx = '^// ?D2Win\.0x([0-9A-Fa-f]{8})'; IsOffset = $false }
        )
    }
    "D2Sound" = @{
        ImageBase         = 0x6F980000L
        SegPattern        = "D2Sound"
        HeaderDir         = "D2Sound\include"
        StripRx           = ''
        GenerateRenameIDC = $false
        CommentPatterns   = @()      # uses D2FUNC_DLL macros (D2SOUND prefix)
    }
    "Fog" = @{
        ImageBase         = 0x6FF50000L
        SegPattern        = "Fog"
        HeaderDir         = "Fog\include"
        StripRx           = 'FOG_DLL_DECL\s+'
        GenerateRenameIDC = $false
        CommentPatterns   = @(
            @{ Rx = '^// 1\.10f: 0x([0-9A-Fa-f]{8})'; IsOffset = $false }
        )
    }
    "Storm" = @{
        ImageBase         = 0x15000000L  # unverified -- verify in your dump (see README)
        SegPattern        = "Storm"
        HeaderDir         = "Storm\include"
        StripRx           = ''
        GenerateRenameIDC = $false
        CommentPatterns   = @()      # uses D2FUNC_DLL_NP macros (no DLL name prefix)
    }
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

function IsFunctionDecl([string]$line) {
    # Exclude extern globals, D2FUNC/D2VAR/D2PTR macros
    if ($line -match '^\s*extern\b')           { return $false }
    if ($line -match '^D2FUNC\(|^D2VAR\(|^D2PTR\(') { return $false }
    # Must have parentheses
    if ($line -notmatch '\(')                   { return $false }
    if ($line -notmatch '\)')                   { return $false }
    # Explicit calling convention -> definitely a function
    if ($line -match '__stdcall|__fastcall|__cdecl') { return $true }
    # No calling convention but looks like a declaration (ends with ); )
    if ($line -match '\)\s*;?\s*$') {
        # Exclude control flow, struct/union/etc.
        if ($line -match '^\s*(if|for|while|switch|struct|union|class|enum)\b') { return $false }
        return $true
    }
    return $false
}

function CleanSignature([string]$line, [string]$stripRx) {
    $sig = if ($stripRx) { $line -replace $stripRx, '' } else { $line }
    # Strip inline trailing comments (e.g. "// Arguments not sure")
    $sig = $sig -replace '\s*//.*$', ''
    $sig = $sig.TrimEnd(';').Trim()
    $sig = $sig -replace '\s+', ' '
    return $sig
}

function FixSignatureForIDA([string]$sig) {
    # C++ scope resolution that IDA's C parser cannot handle
    $sig = $sig -replace 'D2SLayerStatIdStrc::PackedType', 'int32_t'
    # Bare 'unsigned' return type -> IDA needs explicit base type
    $sig = $sig -replace '^unsigned (__stdcall|__fastcall|__cdecl)\b', 'unsigned int $1'
    return $sig
}

# Extracts the function name (identifier immediately before the first '(').
function ExtractFunctionName([string]$sig) {
    # Strip everything from the first '(' onward, then take the last identifier
    $beforeParen = $sig -replace '\(.*$', ''
    if ($beforeParen -match '(\w+)\s*$') {
        return $Matches[1]
    }
    return $null
}

# Returns the RVA for a comment line, or -1 if no pattern matched.
function TryExtractRva([string]$commentLine, [object[]]$patterns, [Int64]$imageBase) {
    foreach ($pat in $patterns) {
        if ($commentLine -match $pat.Rx) {
            $hexVal = [Convert]::ToInt64($Matches[1], 16)
            return $(if ($pat.IsOffset) { $hexVal } else { $hexVal - $imageBase })
        }
    }
    return -1
}

# ---------------------------------------------------------------------------
# Main generator
# ---------------------------------------------------------------------------

function GenerateTypesIDC {
    param(
        [string]    $DllName,
        [hashtable] $Config,
        [string]    $HeaderRootDir,
        [string]    $OutPath
    )

    $imageBase = $Config.ImageBase
    $segPat    = $Config.SegPattern
    $stripRx   = $Config.StripRx
    $patterns  = $Config.CommentPatterns

    $headers = @(Get-ChildItem $HeaderRootDir -Recurse -Filter "*.h" -ErrorAction SilentlyContinue)
    if ($headers.Count -eq 0) {
        Write-Warning "$DllName : no headers found in $HeaderRootDir"
        return
    }

    $entries = [System.Collections.Generic.List[PSCustomObject]]::new()

    foreach ($hdr in $headers) {
        $lines = @(Get-Content $hdr.FullName -Encoding UTF8)
        for ($i = 0; $i -lt $lines.Count; $i++) {
            $commentLine = $lines[$i].Trim()

            # --- Pattern A: address comment + next declaration line ---
            $rva = TryExtractRva $commentLine $patterns $imageBase
            if ($rva -ge 0 -and $rva -lt 0x10000000) {
                $declLine = ""
                for ($j = $i + 1; $j -lt [Math]::Min($i + 3, $lines.Count); $j++) {
                    $candidate = $lines[$j].Trim()
                    if ($candidate -ne "" -and -not $candidate.StartsWith("//")) {
                        $declLine = $candidate
                        break
                    }
                }
                if (IsFunctionDecl $declLine) {
                    $sig = CleanSignature $declLine $stripRx
                    if ($sig -ne "") {
                        $sig = FixSignatureForIDA $sig
                        $entries.Add([PSCustomObject]@{ RVA = $rva; Sig = ($sig -replace '"', '\"') })
                    }
                }
            }

            # --- Pattern B: D2FUNC / D2FUNC_DLL (DllName, NAME, RET, CONV, (ARGS), OFFSET) ---
            # Handles both dynamic-load pointers (D2FUNC) and import-library exports (D2FUNC_DLL).
            # Note: PowerShell -match is case-insensitive, so "D2Lang" matches "D2LANG" etc.
            if ($commentLine -match ('^D2FUNC(?:_DLL)?\(\s*' + [regex]::Escape($DllName) +
                '\s*,\s*(\w+)\s*,\s*(.+?)\s*,\s*(__\w+)\s*,\s*(\([^)]*\))\s*,\s*0x([0-9A-Fa-f]+)\s*\)')) {
                $fnName  = $DllName + "_" + $Matches[1]
                $retType = $Matches[2].Trim()
                $conv    = $Matches[3].Trim()
                $args    = $Matches[4].Trim()
                $rvaFn   = [Convert]::ToInt64($Matches[5], 16)
                if ($rvaFn -gt 0 -and $rvaFn -lt 0x10000000) {
                    $sig = FixSignatureForIDA "$retType $conv $fnName$args"
                    $entries.Add([PSCustomObject]@{ RVA = $rvaFn; Sig = ($sig -replace '"', '\"') })
                }
            }

            # --- Pattern C: D2FUNC_DLL_NP (DllName, NAME, RET, CONV, (ARGS), OFFSET) ---
            # NP = No Prefix: function name is NAME without the DllName_ prefix (e.g. Storm).
            if ($commentLine -match ('^D2FUNC_DLL_NP\(\s*' + [regex]::Escape($DllName) +
                '\s*,\s*(\w+)\s*,\s*(.+?)\s*,\s*(__\w+)\s*,\s*(\([^)]*\))\s*,\s*0x([0-9A-Fa-f]+)\s*\)')) {
                $fnName  = $Matches[1]   # No DLL prefix for NP variant
                $retType = $Matches[2].Trim()
                $conv    = $Matches[3].Trim()
                $args    = $Matches[4].Trim()
                $rvaFn   = [Convert]::ToInt64($Matches[5], 16)
                if ($rvaFn -gt 0 -and $rvaFn -lt 0x10000000) {
                    $sig = FixSignatureForIDA "$retType $conv $fnName$args"
                    $entries.Add([PSCustomObject]@{ RVA = $rvaFn; Sig = ($sig -replace '"', '\"') })
                }
            }
        }
    }

    if ($entries.Count -eq 0) {
        Write-Warning "$DllName : no function signatures extracted"
        return
    }

    # Deduplicate by RVA (keep first)
    $seen   = @{}
    $unique = [System.Collections.Generic.List[PSCustomObject]]::new()
    foreach ($e in $entries) {
        if (-not $seen.ContainsKey($e.RVA)) {
            $seen[$e.RVA] = $true
            $unique.Add($e)
        }
    }
    $entries = $unique

    # --- Emit IDC ---
    $out = [System.Collections.Generic.List[string]]::new($entries.Count * 2 + 60)

    $out.Add('#include <idc.idc>')
    $out.Add('')
    $out.Add("// =============================================================")
    $out.Add("// ${DllName}_types.idc  --  generated by gen_ida_idc_types.ps1 (D2MOO)")
    $out.Add("// Applies function type signatures via SetType().")
    $out.Add("// $($entries.Count) signatures extracted from $DllName headers.")
    $out.Add("//")
    $out.Add("// *** PREREQUISITE: Load D2MOO types into IDA first ***")
    $out.Add("//   File > Load file > Parse C header file")
    $out.Add("//   -> select tools/idc/d2moo_types_for_ida.h  (see README_types.md)")
    $out.Add("// Without this, SetType() will fail on any function using custom structs.")
    $out.Add("// =============================================================")
    $out.Add('')
    $out.Add("static Apply_${DllName}_types()")
    $out.Add('{')
    $out.Add("    // Find $DllName segment base dynamically")
    $out.Add('    auto base = BADADDR;')
    $out.Add('    auto seg;')
    $out.Add('    for (seg = FirstSeg(); seg != BADADDR; seg = NextSeg(seg)) {')
    $out.Add("        if (strstr(get_segm_name(seg), `"$segPat`") != -1) {")
    $out.Add('            base = SegStart(seg);')
    $out.Add('            break;')
    $out.Add('        }')
    $out.Add('    }')
    $out.Add('    if (base == BADADDR) {')
    $out.Add("        Message(`"[$DllName] ERROR: segment '$segPat' not found.\n`");")
    $out.Add('        return;')
    $out.Add('    }')
    $out.Add("    Message(`"[$DllName] base=0x%X\n`", base);")
    $out.Add('')
    $out.Add('    auto addr;')
    $out.Add('    auto ok = 0;')
    $out.Add('    auto fail = 0;')
    $out.Add('')

    foreach ($e in $entries) {
        $rvaHex = "0x{0:X}" -f $e.RVA
        $out.Add("    addr = base + $rvaHex; MakeFunction(addr, BADADDR); if (SetType(addr, `"$($e.Sig)`")) ok++; else { Message(`"[FAIL] 0x%X  $($e.Sig)\n`", addr); fail++; }")
    }

    $out.Add('')
    $out.Add("    Message(`"[$DllName] Types applied: %d ok, %d failed\n`", ok, fail);")
    $out.Add("    Message(`"[$DllName] If fail > 0, run: File > Load file > Parse C header file\n`");")
    $out.Add("    Message(`"[$DllName]   -> select tools/idc/d2moo_types_for_ida.h\n`");")
    $out.Add('}')
    $out.Add('')
    $out.Add('#ifndef D2MOO_ALL_IDC')
    $out.Add("static main() { Apply_${DllName}_types(); }")
    $out.Add('#endif')

    [System.IO.File]::WriteAllLines($OutPath, $out, [System.Text.ASCIIEncoding]::new())
    Write-Host "[OK] $DllName -- $($entries.Count) signatures  ->  $OutPath"
}

# ---------------------------------------------------------------------------
# Generator: rename-only IDC  (set_name via base + RVA)
# Used for DLLs with no .def export table (e.g. D2Client internal functions).
# ---------------------------------------------------------------------------

function GenerateRenameIDC {
    param(
        [string]    $DllName,
        [hashtable] $Config,
        [string]    $HeaderRootDir,
        [string]    $OutPath
    )

    $imageBase = $Config.ImageBase
    $segPat    = $Config.SegPattern
    $stripRx   = $Config.StripRx
    $patterns  = $Config.CommentPatterns

    $headers = @(Get-ChildItem $HeaderRootDir -Recurse -Filter "*.h" -ErrorAction SilentlyContinue)
    if ($headers.Count -eq 0) {
        Write-Warning "$DllName : no headers found in $HeaderRootDir"
        return
    }

    $entries = [System.Collections.Generic.List[PSCustomObject]]::new()

    foreach ($hdr in $headers) {
        $lines = @(Get-Content $hdr.FullName -Encoding UTF8)
        for ($i = 0; $i -lt $lines.Count; $i++) {
            $commentLine = $lines[$i].Trim()

            # --- Pattern A: address comment + next declaration line ---
            $rva = TryExtractRva $commentLine $patterns $imageBase
            if ($rva -ge 0 -and $rva -lt 0x10000000) {
                $declLine = ""
                for ($j = $i + 1; $j -lt [Math]::Min($i + 3, $lines.Count); $j++) {
                    $candidate = $lines[$j].Trim()
                    if ($candidate -ne "" -and -not $candidate.StartsWith("//")) {
                        $declLine = $candidate
                        break
                    }
                }
                if (IsFunctionDecl $declLine) {
                    $sig  = CleanSignature $declLine $stripRx
                    $name = ExtractFunctionName $sig
                    if ($name -and $name -ne "") {
                        $entries.Add([PSCustomObject]@{ RVA = $rva; Name = $name })
                    }
                }
            }

            # --- Pattern B: D2FUNC / D2FUNC_DLL macro ---
            if ($commentLine -match ('^D2FUNC(?:_DLL)?\(\s*' + [regex]::Escape($DllName) +
                '\s*,\s*(\w+)\s*,\s*(.+?)\s*,\s*(__\w+)\s*,\s*(\([^)]*\))\s*,\s*0x([0-9A-Fa-f]+)\s*\)')) {
                $fnName = $DllName + "_" + $Matches[1]
                $rvaFn  = [Convert]::ToInt64($Matches[5], 16)
                if ($rvaFn -gt 0 -and $rvaFn -lt 0x10000000) {
                    $entries.Add([PSCustomObject]@{ RVA = $rvaFn; Name = $fnName })
                }
            }

            # --- Pattern C: D2FUNC_DLL_NP macro (No Prefix) ---
            if ($commentLine -match ('^D2FUNC_DLL_NP\(\s*' + [regex]::Escape($DllName) +
                '\s*,\s*(\w+)\s*,\s*(.+?)\s*,\s*(__\w+)\s*,\s*(\([^)]*\))\s*,\s*0x([0-9A-Fa-f]+)\s*\)')) {
                $fnName = $Matches[1]   # No DLL prefix for NP variant
                $rvaFn  = [Convert]::ToInt64($Matches[5], 16)
                if ($rvaFn -gt 0 -and $rvaFn -lt 0x10000000) {
                    $entries.Add([PSCustomObject]@{ RVA = $rvaFn; Name = $fnName })
                }
            }
        }
    }

    if ($entries.Count -eq 0) {
        Write-Warning "$DllName : no function names extracted for rename IDC"
        return
    }

    # Deduplicate by RVA (keep first)
    $seen   = @{}
    $unique = [System.Collections.Generic.List[PSCustomObject]]::new()
    foreach ($e in $entries) {
        if (-not $seen.ContainsKey($e.RVA)) {
            $seen[$e.RVA] = $true
            $unique.Add($e)
        }
    }
    $entries = $unique

    # --- Emit IDC ---
    $out = [System.Collections.Generic.List[string]]::new($entries.Count + 60)

    $out.Add('#include <idc.idc>')
    $out.Add('')
    $out.Add("// =============================================================")
    $out.Add("// ${DllName}.idc  --  generated by gen_ida_idc_types.ps1 (D2MOO)")
    $out.Add("// Renames functions via set_name() using base + RVA.")
    $out.Add("// $($entries.Count) names extracted from $DllName headers.")
    $out.Add("//")
    $out.Add("// TARGET: IDA process dump")
    $out.Add("// Finds the DLL base by scanning IDA segments for name containing")
    $out.Add("// `"$segPat`" -- no hardcoded address needed.")
    $out.Add("//")
    $out.Add("// IDA 6.x: replace  set_name(a,n,0x880)  with  MakeNameEx(a,n,0x800)")
    $out.Add("// =============================================================")
    $out.Add('')
    $out.Add("static Apply_${DllName}()")
    $out.Add('{')
    $out.Add("    // --- Find module base via segment name ---")
    $out.Add('    auto base = BADADDR;')
    $out.Add('    auto seg;')
    $out.Add('    for (seg = FirstSeg(); seg != BADADDR; seg = NextSeg(seg)) {')
    $out.Add("        if (strstr(get_segm_name(seg), `"$segPat`") != -1) {")
    $out.Add('            base = SegStart(seg);')
    $out.Add('            break;')
    $out.Add('        }')
    $out.Add('    }')
    $out.Add('    if (base == BADADDR) {')
    $out.Add("        Message(`"[$DllName] ERROR: no segment with name containing '$segPat'.\n`");")
    $out.Add('        return;')
    $out.Add('    }')
    $out.Add("    Message(`"[$DllName] base=0x%X\n`", base);")
    $out.Add('')
    $out.Add('    auto renamed = 0;')
    $out.Add('    auto skipped = 0;')
    $out.Add('')

    foreach ($e in $entries) {
        $rvaHex = "0x{0:X}" -f $e.RVA
        $out.Add("    if (set_name(base + $rvaHex, `"$($e.Name)`", 0x880)) renamed++; else skipped++;")
    }

    $out.Add('')
    $out.Add("    Message(`"[$DllName] Renamed: %d ok, %d skipped/failed\n`", renamed, skipped);")
    $out.Add('}')
    $out.Add('')
    $out.Add('#ifndef D2MOO_ALL_IDC')
    $out.Add("static main() { Apply_${DllName}(); }")
    $out.Add('#endif')

    [System.IO.File]::WriteAllLines($OutPath, $out, [System.Text.ASCIIEncoding]::new())
    Write-Host "[OK] $DllName rename -- $($entries.Count) names  ->  $OutPath"
}

Write-Host ""
Write-Host "=== gen_ida_idc_types.ps1  (D2MOO -> IDA SetType IDC) ==="
Write-Host ""

foreach ($dll in $Dlls) {
    if (-not $DllConfig.ContainsKey($dll)) {
        Write-Warning "$dll : not configured in DllConfig table -- skipped."
        continue
    }
    $cfg     = $DllConfig[$dll]
    $hdrDir  = Join-Path $SourceDir $cfg.HeaderDir
    if (-not (Test-Path $hdrDir)) {
        Write-Warning "$dll : header dir not found at $hdrDir -- skipped."
        continue
    }
    $outPath = Join-Path $OutputDir "${dll}_types.idc"
    GenerateTypesIDC -DllName $dll -Config $cfg -HeaderRootDir $hdrDir -OutPath $outPath

    if ($cfg.GenerateRenameIDC) {
        $renameOutPath = Join-Path $OutputDir "${dll}.idc"
        GenerateRenameIDC -DllName $dll -Config $cfg -HeaderRootDir $hdrDir -OutPath $renameOutPath
    }
}

Write-Host ""
Write-Host "Done."
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. In IDA: File > Load file > Parse C header file"
Write-Host "     -> select tools/idc/d2moo_types_for_ida.h  (see gen_ida_types_header.ps1)"
Write-Host "  2. File > Script file... (Alt+F7) -> select D2Common_types.idc / D2Game_types.idc / D2Client_types.idc"
