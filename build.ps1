# build.ps1
#
# Keeps exactly two builds around:
#   engine.exe      <- the freshly compiled version (newest)
#   engine_old.exe  <- whatever engine.exe was right before this build
#
# Every time you run this, the current engine.exe becomes engine_old.exe
# (overwriting whatever was there before), and a new engine.exe gets built.
# This gives you a rolling "new vs previous" pair to load into CuteChess.
#
# Usage:
#   .\build.ps1

$exeName = "engine.exe"
$oldName = "engine_old.exe"

if (Test-Path $exeName) {
    # Overwrite whatever old backup exists with the current engine.exe
    Move-Item -Path $exeName -Destination $oldName -Force
    Write-Host "Previous build saved as $oldName" -ForegroundColor Cyan
} else {
    Write-Host "No existing engine.exe found - nothing to back up yet." -ForegroundColor Yellow
}

Write-Host "Compiling..." -ForegroundColor Green
g++ -std=c++20 -O2 -I fathom/src -o engine.exe `
    Engine.cpp Search.cpp Eval.cpp fathom/src/tbprobe.c

if ($LASTEXITCODE -eq 0) {
    Write-Host "Build succeeded: engine.exe (new)  vs  engine_old.exe (previous)" -ForegroundColor Green
} else {
    Write-Host "Build failed - see errors above. engine_old.exe is untouched." -ForegroundColor Red
}
