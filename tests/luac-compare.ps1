param(
    [ValidateSet('Debug', 'Release', 'Test', 'Asan')][string]$Configuration = 'Release',
    [ValidateSet('Win32', 'x64')][string]$Platform = 'Win32'
)
$ErrorActionPreference = 'Stop'
$testsDir = $PSScriptRoot
$root = Split-Path $testsDir -Parent
. (Join-Path $testsDir 'common.ps1')

$plainCase = 'Unit::LuacCompare::luac x86 and x64 write the same plain bytecode'
$lvlCase = 'Unit::LuacCompare::luac x86 and x64 write the same lvls'
$msbuild = Find-MSBuild
$env:SWBF2_NOSTAGE = 'true'
foreach ($p in 'Win32', 'x64') {
    & $msbuild (Join-Path $root 'LuaCompiler.vcxproj') "/p:Configuration=$Configuration" "/p:Platform=$p" '/p:SWBF2_NOSTAGE=true' /m /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) { Write-Host "[FAIL] luac-compare: LuaCompiler $p $Configuration build exited $LASTEXITCODE"; exit 1 }
    $exe = Join-Path $root "bin\$p\$Configuration\LuaCompiler.exe"
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { Write-Host "[FAIL] luac-compare: $exe missing after its build"; exit 1 }
}
$testsExe = Join-Path $root "bin\Tests\$Platform\$Configuration\OpenSWBF2Tests.exe"
if (-not (Test-Path -LiteralPath $testsExe -PathType Leaf)) { Write-Host "[FAIL] luac-compare: $testsExe not built"; exit 1 }

$ErrorActionPreference = 'Continue'
$out = @(& powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $testsDir 'run.ps1') -Platform $Platform -Configuration $Configuration -NoBuild -Smoke None -Filter 'Unit::LuacCompare::luac x86 and x64 write the same *' 2>&1 | ForEach-Object { $t = "$_"; Write-Host $t; $t })
$code = $LASTEXITCODE
$ErrorActionPreference = 'Stop'
$verdict = {
    param([string]$name)
    $m = @($out | Where-Object { $_ -match ('^\[(PASS|FAIL|SKIP|STALL|CRASH)\] ' + [regex]::Escape($name) + ' ') })
    if ($m.Count -ne 1) { return "ran $($m.Count) times" }
    return $m[0].Substring(1, $m[0].IndexOf(']') - 1)
}
$problems = @()
if ($code -ne 0) { $problems += "run.ps1 exited $code" }
$plain = & $verdict $plainCase
if ($plain -ne 'PASS') { $problems += "$plainCase`: $plain" }
$lvl = & $verdict $lvlCase
if ($lvl -ne 'PASS' -and -not ($lvl -eq 'SKIP' -and -not (Find-GameDataRoot))) { $problems += "$lvlCase`: $lvl" }
if ($problems.Count) {
    foreach ($p in $problems) { Write-Host "[FAIL] luac-compare: $p" }
    exit 1
}
Write-Host "[PASS] luac-compare: $Configuration plain $plain, lvls $lvl"
exit 0
