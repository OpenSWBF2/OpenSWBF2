param(
    [int]$Runs = 5,
    [string]$Configuration = 'Release',
    [ValidateSet('Win32', 'x64')][string]$Platform = 'Win32',
    [switch]$NoBuild,
    [string]$OutDir,
    [ValidateRange(1,64)][int]$Jobs = 8
)
$ErrorActionPreference = 'Stop'
$testsDir = $PSScriptRoot
$root = Split-Path $testsDir -Parent
. (Join-Path $testsDir 'common.ps1')

if ($Runs -lt 5) { Write-Host "INFO: -Runs raised from $Runs to 5 (minimum)"; $Runs = 5 }

if (-not $NoBuild) {
    $msbuild = Find-MSBuild
    foreach ($proj in (Join-Path $root 'BattlefrontII.vcxproj'), (Join-Path $testsDir 'OpenSWBF2Tests.vcxproj')) {
        & $msbuild $proj "/p:Configuration=$Configuration" "/p:Platform=$Platform" /m /v:minimal /nologo
        if ($LASTEXITCODE -ne 0) { Write-Host 'Build failed.'; exit 2 }
    }
}

$exeDir = Join-Path $root "bin\$Platform\$Configuration"
$reason = Get-SmokePrecheck (Find-GameDataRoot) $exeDir $false $Platform
if ($reason) { Write-Host "[SKIP] laa-compare - $reason"; exit 0 }
$testsExe = Join-Path $root "bin\Tests\$Platform\$Configuration\OpenSWBF2Tests.exe"
if (-not (Test-Path -LiteralPath $testsExe -PathType Leaf)) { Write-Host "[FAIL] laa-compare - tests exe not found: $testsExe"; exit 2 }

if (-not $OutDir) { $OutDir = New-StampedDir (Join-Path $root 'artifacts\laa-compare') }
New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
Write-Host "INFO: laa-compare runs=$Runs per side jobs=$Jobs out=$OutDir"
Push-Location -LiteralPath $testsDir
try {
    & $testsExe --runner "--out=$OutDir" "--platform=$Platform" "--config=$Configuration" "--exclude=$(Join-Path $testsDir 'excluded.tsv')" '--tags=*[Laa][OnRequest]*' "--game-dir=$exeDir" "--jobs=$Jobs" "--param=$Runs" | ForEach-Object { Write-Host $_ }
    $code = $LASTEXITCODE
}
finally { Pop-Location }
if ($code -ne 0) { Write-Host "[FAIL] laa-compare - the test runner exited $code"; exit 1 }
Write-Host "[PASS] laa-compare ($Runs runs per side)"
exit 0
