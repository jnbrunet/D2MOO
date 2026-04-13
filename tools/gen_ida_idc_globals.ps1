# gen_ida_idc_globals.ps1
# Generates IDA IDC scripts that rename and type global variables.
# Parses D2MOO C++ headers for annotated extern declarations and D2VAR macros.
#
# Supported input patterns:
#   // DLL + 0xOFFSET -> VA           (followed by extern TYPE name;)
#   // DLL + 0xOFFSET -> VA           (followed by extern TYPE& name;  -- C++ ref, same offset)
#   D2VAR(DLL, NAME, TYPE, OFFSET)    (macro: variable at base + OFFSET)
#   D2PTR(DLL, NAME, TYPE, OFFSET)    (macro: pointer at base + OFFSET)
#
# For each global, the IDC script will:
#   1. set_name(base + offset, "name", 0x880)      -- always
#   2. SetType(base + offset, "TYPE name")          -- best-effort (may fail for complex structs)
#
# PREREQUISITE for SetType: import D2MOO types header into IDA first.
#   File > Load file > Parse C header file -> tools/idc/d2moo_types_for_ida.h
#
# Usage:
#   .\gen_ida_idc_globals.ps1                          # all configured DLLs
#   .\gen_ida_idc_globals.ps1 -Dlls D2Client           # specific DLL
#   .\gen_ida_idc_globals.ps1 -OutputDir C:\tmp\idc

param(
    [string]   $SourceDir = "$PSScriptRoot\..\source",
    [string]   $OutputDir = "$PSScriptRoot\idc",
    [string[]] $Dlls      = @("D2Client", "D2Common", "D2Game")
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

# ---------------------------------------------------------------------------
# DLL configuration
# CommentRx: captures the offset (RVA) from a "// DLL + 0xOFFSET ->" comment.
# ---------------------------------------------------------------------------
$DllConfig = @{
    "D2Client" = @{
        ImageBase  = 0x6FAA0000L
        SegPattern = "D2Client"
        HeaderDir  = "D2Client\include"
        # // D2Client + 0xOFFSET -> ... (offset IS the RVA)
        CommentRx  = '^// D2Client \+ 0x([0-9A-Fa-f]+)\s*->'
    }
    "D2Common" = @{
        ImageBase  = 0x6FD40000L
        SegPattern = "D2Common"
        HeaderDir  = "D2Common\include"
        # //D2Common.0xVA (absolute VA -> subtract ImageBase)
        CommentRx  = '^//D2Common\.0x([0-9A-Fa-f]{8})'
        CommentIsAbsoluteVA = $true
    }
    "D2Game" = @{
        ImageBase  = 0x6FC30000L
        SegPattern = "D2Game"
        HeaderDir  = "D2Game\include"
        CommentRx  = '^//D2Game\.0x([0-9A-Fa-f]{8})'
        CommentIsAbsoluteVA = $true
    }
}

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

# Returns $true if a declaration line looks like a global variable (extern).
function IsGlobalDecl([string]$line) {
    return $line -match '^\s*extern\b'
}

# Parses "extern TYPE[&] name[ARRAY];" -> @{ Name; TypeForIDA }
# TypeForIDA is a C type string suitable for IDA's SetType (e.g. "int", "D2UnitStrc *").
function ParseExternGlobal([string]$line) {
    # Strip leading 'extern' and trailing ';'
    $d = $line -replace '^\s*extern\s+', ''
    $d = $d.TrimEnd(';').Trim()

    # Capture and strip array subscript (keep for type reconstruction)
    $arraySuffix = ""
    if ($d -match '\s*(\[\s*[0-9]*\s*\])\s*$') {
        $arraySuffix = $Matches[1] -replace '\s', ''
        $d = $d -replace [regex]::Escape($Matches[1]) + '\s*$', ''
    }

    # Strip C++ reference '&' -- the actual memory is the value, not a pointer-to-pointer
    $d = $d -replace '&', ''

    # Normalize whitespace and pointer spacing
    $d = ($d -replace '\s+', ' ').Trim()
    $d = $d -replace '\s*\*\s*', ' * '
    $d = ($d -replace '\s+', ' ').Trim()

    # The variable name is the last identifier
    if ($d -notmatch '^(.*?)\s+(\w+)\s*$') { return $null }
    $typePart = $Matches[1].TrimEnd()
    $namePart = $Matches[2]

    # Rebuild clean type string
    $typeForIDA = $typePart
    if ($arraySuffix) { $typeForIDA += $arraySuffix }

    return @{ Name = $namePart; TypeForIDA = $typeForIDA }
}

# Extracts RVA from a comment line. Returns -1 on no match.
function TryExtractRvaFromComment([string]$commentLine, [hashtable]$cfg) {
    if ($commentLine -match $cfg.CommentRx) {
        $hexVal = [Convert]::ToInt64($Matches[1], 16)
        if ($cfg.ContainsKey('CommentIsAbsoluteVA') -and $cfg.CommentIsAbsoluteVA) {
            return $hexVal - $cfg.ImageBase
        }
        return $hexVal  # already an offset/RVA
    }
    return -1
}

# ---------------------------------------------------------------------------
# Main generator
# ---------------------------------------------------------------------------

function GenerateGlobalsIDC {
    param(
        [string]    $DllName,
        [hashtable] $Config,
        [string]    $HeaderRootDir,
        [string]    $OutPath
    )

    $segPat = $Config.SegPattern

    $headers = Get-ChildItem $HeaderRootDir -Recurse -Filter "*.h" -ErrorAction SilentlyContinue
    if ($headers.Count -eq 0) {
        Write-Warning "$DllName : no headers found in $HeaderRootDir"
        return
    }

    $entries = [System.Collections.Generic.List[PSCustomObject]]::new()

    foreach ($hdr in $headers) {
        $lines = Get-Content $hdr.FullName -Encoding UTF8
        for ($i = 0; $i -lt $lines.Count; $i++) {
            $line = $lines[$i].Trim()

            # === Pattern A: address comment followed by extern declaration ===
            $rva = TryExtractRvaFromComment $line $Config
            if ($rva -ge 0 -and $rva -lt 0x10000000) {
                # Find the next non-blank, non-comment line
                for ($j = $i + 1; $j -lt [Math]::Min($i + 3, $lines.Count); $j++) {
                    $next = $lines[$j].Trim()
                    if ($next -eq "" -or $next.StartsWith("//")) { continue }
                    if (IsGlobalDecl $next) {
                        $parsed = ParseExternGlobal $next
                        if ($parsed) {
                            $entries.Add([PSCustomObject]@{
                                RVA        = $rva
                                Name       = $parsed.Name
                                TypeForIDA = $parsed.TypeForIDA
                                Source     = "extern"
                            })
                        }
                    }
                    break
                }
            }

            # === Pattern B: D2VAR(DllName, NAME, TYPE, OFFSET) ===
            # Expands to: static DllName_NAME_vt * DllName_NAME = base + OFFSET
            if ($line -match ('^D2VAR\(\s*' + [regex]::Escape($DllName) +
                '\s*,\s*(\w+)\s*,\s*(.+?)\s*,\s*0x([0-9A-Fa-f]+)\s*\)')) {
                $macroName = $DllName + "_" + $Matches[1]
                $macroType = $Matches[2].Trim()
                $macroRva  = [Convert]::ToInt64($Matches[3], 16)
                if ($macroRva -lt 0x10000000) {
                    $entries.Add([PSCustomObject]@{
                        RVA        = $macroRva
                        Name       = $macroName
                        TypeForIDA = $macroType
                        Source     = "D2VAR"
                    })
                }
            }

            # === Pattern C: D2PTR(DllName, NAME, TYPE, OFFSET) ===
            # Expands to: static TYPE* DllName_NAME = base + OFFSET
            if ($line -match ('^D2PTR\(\s*' + [regex]::Escape($DllName) +
                '\s*,\s*(\w+)\s*,\s*(.+?)\s*,\s*0x([0-9A-Fa-f]+)\s*\)')) {
                $macroName = $DllName + "_" + $Matches[1]
                $macroType = $Matches[2].Trim() + " *"
                $macroRva  = [Convert]::ToInt64($Matches[3], 16)
                if ($macroRva -lt 0x10000000) {
                    $entries.Add([PSCustomObject]@{
                        RVA        = $macroRva
                        Name       = $macroName
                        TypeForIDA = $macroType
                        Source     = "D2PTR"
                    })
                }
            }
        }
    }

    if ($entries.Count -eq 0) {
        Write-Warning "$DllName : no global variables found"
        return
    }

    # Deduplicate by RVA (keep first occurrence)
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
    $out = [System.Collections.Generic.List[string]]::new($entries.Count * 3 + 60)

    $out.Add('#include <idc.idc>')
    $out.Add('')
    $out.Add("// =============================================================")
    $out.Add("// ${DllName}_globals.idc  --  generated by gen_ida_idc_globals.ps1 (D2MOO)")
    $out.Add("// Renames and types global variables.")
    $out.Add("// $($entries.Count) globals extracted from $DllName headers.")
    $out.Add("//")
    $out.Add("// For each global:")
    $out.Add("//   set_name(base + offset, name)   -- always applied")
    $out.Add("//   SetType(base + offset, type)     -- best-effort (needs d2moo_types_for_ida.h)")
    $out.Add("//")
    $out.Add("// *** PREREQUISITE for SetType: ***")
    $out.Add("//   File > Load file > Parse C header file")
    $out.Add("//   -> select tools/idc/d2moo_types_for_ida.h")
    $out.Add("// =============================================================")
    $out.Add('')
    $out.Add("static Apply_${DllName}_globals()")
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
    $out.Add("    Message(`"[$DllName] base=0x%X  ($($entries.Count) globals)\n`", base);")
    $out.Add('')
    $out.Add('    auto addr;')
    $out.Add('    auto renamed = 0;')
    $out.Add('    auto typed   = 0;')
    $out.Add('    auto typeFail = 0;')
    $out.Add('')

    foreach ($e in $entries) {
        $rvaHex  = "0x{0:X}" -f $e.RVA
        $nameEsc = $e.Name -replace '"', '\"'
        $typeEsc = ($e.TypeForIDA -replace '"', '\"').Trim()
        # type string for IDA: "TYPE varname"
        $idaType = "$typeEsc $nameEsc"

        $out.Add("    // $($e.Name)  ($($e.Source))")
        $out.Add("    addr = base + $rvaHex;")
        $out.Add("    set_name(addr, `"$nameEsc`", 0x880); renamed++;")
        $out.Add("    if (SetType(addr, `"$idaType`")) typed++; else typeFail++;")
        $out.Add('')
    }

    $out.Add("    Message(`"[$DllName] Globals: %d renamed, %d typed, %d type-fail\n`", renamed, typed, typeFail);")
    $out.Add("    Message(`"[$DllName] type-fail is normal for struct types not yet in IDA.\n`");")
    $out.Add('}')
    $out.Add('')
    $out.Add('#ifndef D2MOO_ALL_IDC')
    $out.Add("static main() { Apply_${DllName}_globals(); }")
    $out.Add('#endif')

    [System.IO.File]::WriteAllLines($OutPath, $out, [System.Text.ASCIIEncoding]::new())
    Write-Host "[OK] $DllName -- $($entries.Count) globals  ->  $OutPath"
}

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

Write-Host ""
Write-Host "=== gen_ida_idc_globals.ps1  (D2MOO -> IDA globals IDC) ==="
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
    $outPath = Join-Path $OutputDir "${dll}_globals.idc"
    GenerateGlobalsIDC -DllName $dll -Config $cfg -HeaderRootDir $hdrDir -OutPath $outPath
}

Write-Host ""
Write-Host "Done. IDC files in: $OutputDir"
Write-Host ""
Write-Host "IDA usage:"
Write-Host "  1. (Optional) File > Load file > Parse C header file -> d2moo_types_for_ida.h"
Write-Host "  2. File > Script file... (Alt+F7) -> select D2Client_globals.idc"
Write-Host "  Check Output window for rename/type counts."

