# Build the DraCopy-based RLECOPY (dc64) with cc65, then wrap into a .d64.
# Builds two variants:
#   dc64      - the regular C64 program
#   dc64-reu  - same, with Commodore REU support (needs the c64-reu.emd driver
#               on the same device; it is loaded at runtime)
# Requires cc65 (cl65) and VICE (c1541) on PATH, or in C:\Tools\{cc65,vice}.
# Usage:  powershell -ExecutionPolicy Bypass -File build-dc64.ps1
$ErrorActionPreference = 'Stop'
if (-not $env:CC65_HOME) { $env:CC65_HOME = 'C:\Tools\cc65' }
$env:Path += ';C:\Tools\cc65\bin;C:\Tools\vice\bin;C:\Tools\exomizer'

Set-Location $PSScriptRoot

# F3/F4 viewers (cat.c) are intentionally dropped; rle.c is the RLE codec.
# -DNDEBUG drops assert() code (smaller release build).
$src = 'src/dc.c','src/screen.c','src/dir.c','src/base.c','src/ops.c','src/rle.c'

function Build-Variant {
    param([string]$name, [string[]]$extraDefs)

    $clArgs = @('-I','include','-O','-Or','-Os','-r','-DNDEBUG') + $extraDefs +
              @('-t','c64','-m',"$name.map") + $src + @('-o',"$name.prg")
    & cl65 @clArgs
    if ($LASTEXITCODE -ne 0) { throw "cl65 failed for $name ($LASTEXITCODE)" }
    Write-Host "built $name.prg ($((Get-Item "$name.prg").Length) bytes)"

    # Compress to a self-extracting .prg (this is how stock DraCopy ships - cuts
    # the file roughly in half).  Falls back to the uncompressed .prg.
    $run = "$name.prg"
    if (Get-Command exomizer -ErrorAction SilentlyContinue) {
        & exomizer sfx systrim -q -o "$name.exo.prg" "$name.prg"
        if ($LASTEXITCODE -eq 0 -and (Test-Path "$name.exo.prg")) {
            $run = "$name.exo.prg"
            Write-Host "  compressed -> $name.exo.prg ($((Get-Item "$name.exo.prg").Length) bytes)"
        } else {
            Write-Host "  warning: exomizer failed - using uncompressed $name.prg"
        }
    } else {
        Write-Host "  note: exomizer not found - shipping uncompressed $name.prg"
    }
    return $run
}

# ---- regular build ------------------------------------------------------------
$run = Build-Variant 'dc64' @()
if (Get-Command c1541 -ErrorAction SilentlyContinue) {
    & c1541 -format "rlecopy,rc" d64 dc64.d64 -write $run dc64 | Out-Null
    Write-Host "built dc64.d64 (contains $run as DC64)"
}

# ---- REU build ----------------------------------------------------------------
# -DREU_C64REU expands (in defines.h) to REU="c64-reu.emd"; the program calls
# em_load_driver("c64-reu.emd") at startup, so the driver file must sit next to
# the program on the device.  (Quote-free flag -> no PowerShell quoting issues.)
$reuRun = Build-Variant 'dc64-reu' @('-DREU_C64REU')

$emd = Join-Path $env:CC65_HOME 'target\c64\drv\emd\c64-reu.emd'
if (Test-Path $emd) {
    Copy-Item $emd 'c64-reu.emd' -Force
    Write-Host "copied REU driver c64-reu.emd ($((Get-Item 'c64-reu.emd').Length) bytes)"
    if (Get-Command c1541 -ErrorAction SilentlyContinue) {
        & c1541 -format "rlecopy reu,rc" d64 dc64-reu.d64 `
            -write $reuRun dc64 -write c64-reu.emd "c64-reu.emd" | Out-Null
        Write-Host "built dc64-reu.d64 (contains $reuRun as DC64 + c64-reu.emd)"
    }
} else {
    Write-Host "warning: REU driver not found at $emd - REU .d64 not built"
}
