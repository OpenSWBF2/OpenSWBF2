param(
    [ValidateSet('Test')][string]$Configuration = 'Test',
    [switch]$HogAddrSpace,
    [string]$OutDir
)
$ErrorActionPreference = 'Stop'
$testsDir = $PSScriptRoot
$root = Split-Path $testsDir -Parent
. (Join-Path $testsDir 'smoke.ps1')

$gameData = Find-GameDataRoot
if (-not $gameData) { Write-Host '[SKIP] savecross - no retail GameData found (set SWBF2_GAMEDATA or install Steam App 6060)'; exit 0 }
if (-not $OutDir) { $OutDir = New-StampedDir (Join-Path $root 'artifacts\savecross') }
if (-not [IO.Path]::IsPathRooted($OutDir)) { $OutDir = Join-Path $root $OutDir }
$OutDir = [IO.Path]::GetFullPath($OutDir)
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
Write-Host "INFO: savecross Win32 $Configuration and x64 $Configuration$(if ($HogAddrSpace) { ' (x64 -hogaddrspace)' }) out=$OutDir"

try { $lines = @(Invoke-SaveCross $root $Configuration ([bool]$HogAddrSpace) $OutDir $gameData ${function:Invoke-SmokeCase}) }
catch { $lines = @("[FAIL] savecross:run - $($_.Exception.Message)") }
foreach ($l in $lines) { Write-Host $l }
[IO.File]::WriteAllText((Join-Path $OutDir 'result.txt'), (($lines -join "`r`n") + "`r`n"))

$bad = @($lines | Where-Object { $_ -like '`[FAIL`]*' }).Count
if ($bad -or $lines.Count -eq 0) { Write-Host "[FAIL] savecross ($bad failed)"; exit 1 }
Write-Host "[PASS] savecross ($($lines.Count) checks)"
exit 0
