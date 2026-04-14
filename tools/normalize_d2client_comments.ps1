# normalize_d2client_comments.ps1
# Normalizes all D2Client function/global address comments to the canonical format:
#   // D2Client.dll + 0xRVA (0xVA)
#
# Handles all current variants:
#   // D2Client + 0xRVA -> 0xVA [extra]          (old style, with or without 0x prefix on VA)
#   // D2Client + 0xRVA -> D2Client.0xVA [extra]  (old style, with D2Client. prefix)
#   // D2Client.0xVA (RVA: 0xRVA)                 (new style, space optional)
#   //D2Client.0xVA                               (new style, no space)
#
# Lines NOT changed:
#   //D2Client.0x6FAA0000                          (ImageBase marker -- preserved as-is)
#   D2FUNC / D2VAR / D2PTR macros                  (untouched)
#
# Usage:
#   .\normalize_d2client_comments.ps1              # normalize in place
#   .\normalize_d2client_comments.ps1 -DryRun      # preview only (no file writes)

param(
    [string] $SourceDir = "$PSScriptRoot\..\source\D2Client\include",
    [switch] $DryRun    = $false
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

[Int64] $ImageBase = 0x6FAA0000L

$totalChanged = 0
$totalLines   = 0

function NormalizeCommentLine([string]$line) {
    $trimmed = $line.TrimStart()

    # -- Pattern 1: // D2Client + 0xRVA -> ... --------------------------------
    # Captures RVA from left side; VA from right side (strips D2Client. and 0x prefixes).
    # Discards any trailing annotation (pfProcessUnit, etc.)
    if ($trimmed -match '^// D2Client \+ 0x([0-9A-Fa-f]+)\s*->\s*(?:D2Client\.)?(?:0x)?([0-9A-Fa-f]{7,9})') {
        [Int64]$rva = [Convert]::ToInt64($Matches[1], 16)
        [Int64]$va  = [Convert]::ToInt64($Matches[2], 16)
        # Sanity: VA should be >= ImageBase
        if ($va -lt $ImageBase) {
            # VA doesn't have 0x prefix and is actually just the lower nibbles? recompute
            $va = $ImageBase + $rva
        }
        $indent = $line.Substring(0, $line.Length - $line.TrimStart().Length)
        return "${indent}// D2Client.dll + 0x{0:X} (0x{1:X8})" -f $rva, $va
    }

    # -- Pattern 2: // D2Client.0xVA or //D2Client.0xVA  (space optional) -----
    if ($trimmed -match '^// ?D2Client\.0x([0-9A-Fa-f]{8})') {
        [Int64]$va = [Convert]::ToInt64($Matches[1], 16)
        # Skip the ImageBase marker
        if ($va -eq $ImageBase) { return $null }
        [Int64]$rva = $va - $ImageBase
        if ($rva -lt 0 -or $rva -ge 0x10000000L) { return $null }
        $indent = $line.Substring(0, $line.Length - $line.TrimStart().Length)
        return "${indent}// D2Client.dll + 0x{0:X} (0x{1:X8})" -f $rva, $va
    }

    return $null  # no match -- leave line unchanged
}

$headers = Get-ChildItem $SourceDir -Recurse -Filter "*.h" -ErrorAction SilentlyContinue
if ($headers.Count -eq 0) {
    Write-Warning "No .h files found in $SourceDir"
    exit 1
}

foreach ($hdr in $headers) {
    $lines      = Get-Content $hdr.FullName -Encoding UTF8
    $newLines   = [System.Collections.Generic.List[string]]::new($lines.Count)
    $fileChanged = $false

    foreach ($line in $lines) {
        $normalized = NormalizeCommentLine $line
        if ($null -ne $normalized -and $normalized -ne $line) {
            Write-Verbose "  [$($hdr.Name)] `n    OLD: $line`n    NEW: $normalized"
            $newLines.Add($normalized)
            $fileChanged = $true
            $totalLines++
        } else {
            $newLines.Add($line)
        }
    }

    if ($fileChanged) {
        $totalChanged++
        if ($DryRun) {
            Write-Host "[DRY-RUN] Would update: $($hdr.FullName)"
        } else {
            # Write back with UTF-8 without BOM
            [System.IO.File]::WriteAllLines($hdr.FullName, $newLines, [System.Text.UTF8Encoding]::new($false))
            Write-Host "[OK] $($hdr.Name)  ($($hdr.FullName))"
        }
    }
}

Write-Host ""
if ($DryRun) {
    Write-Host "DRY-RUN complete: $totalLines lines would be changed in $totalChanged files."
} else {
    Write-Host "Done: $totalLines comment lines normalized in $totalChanged files."
}

