# Build the DraCopy-based RLECOPY (dc64) with cc65, then wrap into a .d64.
# Requires cc65 (cl65) and VICE (c1541) on PATH, or in C:\Tools\{cc65,vice}.
# Usage:  powershell -ExecutionPolicy Bypass -File build-dc64.ps1
$ErrorActionPreference = 'Stop'
if (-not $env:CC65_HOME) { $env:CC65_HOME = 'C:\Tools\cc65' }
$env:Path += ';C:\Tools\cc65\bin;C:\Tools\vice\bin;C:\Tools\exomizer'

Set-Location $PSScriptRoot

# F3/F4 viewers (cat.c) are intentionally dropped; rle.c is the RLE codec.
# -DNDEBUG drops assert() code (smaller release build).
$src = 'src/dc.c','src/screen.c','src/dir.c','src/base.c','src/ops.c','src/rle.c'
& cl65 -I include -O -Or -Os -r -DNDEBUG -t c64 -m dc64.map @src -o dc64.prg
if ($LASTEXITCODE -ne 0) { throw "cl65 failed ($LASTEXITCODE)" }
Write-Host "built dc64.prg ($((Get-Item dc64.prg).Length) bytes)"

# Compress to a self-extracting .prg if exomizer is available (this is how the
# stock DraCopy release ships - cuts the file roughly in half).  Falls back to
# the uncompressed dc64.prg if exomizer isn't installed.
$run = 'dc64.prg'
if (Get-Command exomizer -ErrorAction SilentlyContinue) {
    & exomizer sfx systrim -q -o dc64.exo.prg dc64.prg
    if ($LASTEXITCODE -eq 0 -and (Test-Path dc64.exo.prg)) {
        $run = 'dc64.exo.prg'
        Write-Host "compressed -> dc64.exo.prg ($((Get-Item dc64.exo.prg).Length) bytes)"
    } else {
        Write-Host "warning: exomizer failed - using uncompressed dc64.prg"
    }
} else {
    Write-Host "note: exomizer not found - shipping uncompressed dc64.prg"
}

if (Get-Command c1541 -ErrorAction SilentlyContinue) {
    & c1541 -format "rlecopy,rc" d64 dc64.d64 -write $run dc64 | Out-Null
    Write-Host "built dc64.d64 (contains $run as DC64)"
} else {
    Write-Host "note: c1541 not found - skipping .d64 (SD2IEC runs the .prg directly)"
}
