$ErrorActionPreference = 'Stop'
$run = Join-Path $PSScriptRoot 'run.ps1'
$tmp = Join-Path $env:TEMP ("openswbf2-selftest-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $tmp | Out-Null

$list = 'echo %* | findstr /C:"list-test-cases" >nul && (echo ==========& echo fake_case& echo ==========& exit /b 0)'
$oneFail = 'echo [doctest] test cases: 1 ^| 0 passed ^| 1 failed'

$scenarios = @(
    @('listing exit non-zero', 'unit', 'exit /b 3', $null, 2, 'Listing failed', $null),
    @('listing zero cases', 'unit', 'exit /b 0', $null, 2, 'listed 0 cases', $null),
    @('listing timeout', 'unit', "ping -n 3 127.0.0.1 >nul`r`nexit /b 0", '1000', 2, 'timed out', $null),
    @('known_bug did not run once', 'known_bug', "$list`r`nexit /b 0", $null, 1, '\[FAIL\] fake_case', '\[KNOWN\]'),
    @('known_bug genuine failure', 'known_bug', "$list`r`n$oneFail`r`nexit /b 1", $null, 0, '\[KNOWN\] fake_case', $null),
    @('known_bug exit 1 without 1-case output', 'known_bug', "$list`r`nexit /b 1", $null, 1, '\[FAIL\] fake_case', '\[KNOWN\]'),
    @('known_bug crash', 'known_bug', "$list`r`nexit /b 3221225477", $null, 1, '\[CRASH\] fake_case', '\[KNOWN\]')
)

$failed = 0
try {
    $n = 0
    foreach ($sc in $scenarios) {
        $n++
        $stub = Join-Path $tmp "stub$n.cmd"
        Set-Content -Path $stub -Value ("@echo off`r`n" + $sc[2]) -Encoding ASCII
        $env:OPENSWBF2_TEST_EXE = $stub
        if ($sc[3]) { $env:OPENSWBF2_LIST_TIMEOUT_MS = $sc[3] }
        try {
            $out = (& powershell -NoProfile -File $run -NoBuild -Smoke None -Suite $sc[1] 2>&1 | Out-String)
            $code = $LASTEXITCODE
        }
        finally {
            Remove-Item Env:\OPENSWBF2_LIST_TIMEOUT_MS -ErrorAction SilentlyContinue
        }
        $why = $null
        if ($code -ne $sc[4]) { $why = "exit $code, expected $($sc[4])" }
        elseif ($out -notmatch $sc[5]) { $why = "output missing /$($sc[5])/" }
        elseif ($sc[6] -and $out -match $sc[6]) { $why = "output unexpectedly matches /$($sc[6])/" }
        if ($why) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $why" }
        else { Write-Host "OK $n $($sc[0])" }
    }
}
finally {
    Remove-Item Env:\OPENSWBF2_TEST_EXE -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
}
. (Join-Path $PSScriptRoot 'smoke.ps1')
$smokeTmp = Join-Path $env:TEMP ("openswbf2-smoke-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $smokeTmp | Out-Null
$smokeScenarios = @(
    @('smoke exit 0 without marker', 'exit /b 0', 5, 'InitFailure'),
    @('smoke fast-fail code', 'exit /b 3221226505', 5, 'GsFailure'),
    @('smoke exit 3', 'exit /b 3', 5, 'Fatal'),
    @('smoke exit 4', 'exit /b 4', 5, 'Fatal'),
    @('smoke marker in log', 'echo OPENSWBF2 RUN COMPLETE>%2', 5, 'Pass'),
    @('smoke timeout', "ping -n 31 127.0.0.1 >nul`r`nexit /b 0", 2, 'TimeOut')
)
try {
    foreach ($sc in $smokeScenarios) {
        $n++
        $stub = Join-Path $smokeTmp "smoke$n.cmd"
        Set-Content -Path $stub -Value ("@echo off`r`n" + $sc[1]) -Encoding ASCII
        $caseDir = Join-Path $smokeTmp "case$n"
        New-Item -ItemType Directory -Path $caseDir | Out-Null
        $r = Invoke-SmokeCase "t$n" $stub $smokeTmp ('-log "' + $caseDir + '\run.log"') $sc[2] $caseDir
        $why = $null
        if ($r.Class -ne $sc[3]) { $why = "class $($r.Class), expected $($sc[3])" }
        elseif ($sc[3] -eq 'TimeOut' -and @(Get-CimInstance Win32_Process -Filter "Name='PING.EXE'" | Where-Object { $_.CommandLine -like '*-n 31 127.0.0.1*' }).Count -gt 0) { $why = 'child ping survived the kill' }
        if ($why) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $why" }
        else { Write-Host "OK $n $($sc[0])" }
    }
    $n++
    if ((Get-SmokeClass $false $false 0 'x OPENSWBF2 RUN COMPLETE y' @('a.dmp')) -ne 'Fatal') { $failed++; Write-Host "FAILED ${n}: smoke marker plus dump must be Fatal" }
    else { Write-Host "OK $n smoke marker plus dump is Fatal" }
    $n++
    $pre = Get-SmokePrecheck $null $smokeTmp
    if ($pre -notmatch 'GameData') { $failed++; Write-Host "FAILED ${n}: smoke precheck without GameData" }
    else { Write-Host "OK $n smoke precheck without GameData" }
}
finally {
    Remove-Item -Recurse -Force $smokeTmp -ErrorAction SilentlyContinue
}
$goodHdr = "    Characteristics`n         102 characteristics`n    DLL characteristics`n            8140 DLL characteristics`n                   Dynamic base`n                   NX compatible`n   1C13000 [   38D38] RVA [size] of Base Relocation Directory`n  .reloc name`n"
$cookieCfg = "            0085C930 Security Cookie`n"
$goodCfg = "$cookieCfg            008327F0 Safe Exception Handler Table`n                 7CF Safe Exception Handler Count`n"
$oldHdr = "    Characteristics`n         103 characteristics`n                   Relocations stripped`n                   NX compatible`n         0 [       0] RVA [size] of Base Relocation Directory`n"
$hardScenarios = @(
    @('hardening good text passes all', $goodHdr, $goodCfg, 736, 0, 736, @('dynamicbase', 'nxcompat', 'reloc', 'safeseh', 'gs', 'sdl'), @()),
    @('hardening old exe fails dynamicbase and reloc', $oldHdr, $goodCfg, 736, 0, 736, @('nxcompat', 'safeseh', 'gs', 'sdl'), @('dynamicbase', 'reloc')),
    @('hardening safeseh count 0 fails', $goodHdr, "$cookieCfg                 0 Safe Exception Handler Count`n", 736, 0, 736, @('dynamicbase', 'nxcompat', 'reloc', 'gs', 'sdl'), @('safeseh')),
    @('hardening missing security cookie fails gs', $goodHdr, "                 7CF Safe Exception Handler Count`n", 736, 0, 736, @('dynamicbase', 'nxcompat', 'reloc', 'safeseh', 'sdl'), @('gs')),
    @('hardening /GS- entries fail gs', $goodHdr, $goodCfg, 736, 3, 736, @('dynamicbase', 'nxcompat', 'reloc', 'safeseh', 'sdl'), @('gs')),
    @('hardening empty tlog fails gs and sdl', $goodHdr, $goodCfg, 0, 0, 0, @('reloc', 'safeseh'), @('gs', 'sdl')),
    @('hardening missing /sdl entry fails sdl', $goodHdr, $goodCfg, 736, 0, 735, @('dynamicbase', 'nxcompat', 'reloc', 'safeseh', 'gs'), @('sdl'))
)
foreach ($sc in $hardScenarios) {
    $n++
    $got = Get-HardeningChecks $sc[1] $sc[2] $sc[3] $sc[4] $sc[5]
    $why = $null
    foreach ($name in $sc[6]) { if (-not ($got | Where-Object { $_.Name -eq $name }).Ok) { $why = "$name should pass" } }
    foreach ($name in $sc[7]) { if (($got | Where-Object { $_.Name -eq $name }).Ok) { $why = "$name should fail" } }
    if ($why) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $why" }
    else { Write-Host "OK $n $($sc[0])" }
}
$n++
$allCases = @(Get-SmokeCases 'All' @('foo1c_ctf TimeOut', 'tat3c_con Fatal', 'bar1c_con', 'quux1c_con Pass') $false $null)
$byName = @{}
foreach ($c in $allCases) { $byName[$c.Name] = $c }
$rejected = 0
foreach ($badLine in 'baz1c_con Bogus', 'qux1c_con GsFailure') {
    try { Get-SmokeCases 'All' @($badLine) $false $null | Out-Null } catch { $rejected++ }
}
$why = $null
if ($byName['foo1c_ctf'].Expect.Class -ne 'TimeOut' -or $byName['foo1c_ctf'].Timeout -ne 0 -or $byName['foo1c_ctf'].Expect.LogRx -ne 'OPENSWBF2 MISSION RUNNING') { $why = 'TimeOut line' }
elseif ($byName['tat3c_con'].Expect.Class -ne 'Fatal' -or $byName['tat3c_con'].Timeout -ne 0) { $why = 'Fatal line' }
elseif (-not $byName['tat3c_con'].Expect.LogRx -or $byName['tat3c_con'].Expect.Code -ne 3 -or $byName['tat3c_con'].Expect.MaxDumps -ne 1) { $why = 'Fatal line must pin the d3dx9 signature' }
elseif ('FATAL: exception 0xC0000005 at 0x658FA553 (d3dx9_43.dll+0x1BA553) build 6ADE5B7C1C4C000' -notmatch $byName['tat3c_con'].Expect.LogRx) { $why = 'Fatal LogRx does not match the recorded tat3 crash' }
elseif ($byName['bar1c_con'].Expect.Class -ne 'Pass' -or $byName['quux1c_con'].Expect.Class -ne 'Pass') { $why = 'missing or Pass class must be Pass' }
elseif ($rejected -ne 2) { $why = 'unknown and GsFailure classes must be rejected' }
elseif (@($allCases | Where-Object { $_.Name -like 'crash-*' }).Count -ne 12 -or -not $byName.ContainsKey('fatal-init') -or $byName['dedicated'].Timeout -ne 0) { $why = 'dedicated or crash cases changed' }
if ($why) { $failed++; Write-Host "FAILED ${n}: all-missions parsing: $why" }
else { Write-Host "OK $n all-missions parsing" }
if ($failed -gt 0) { exit 1 }
exit 0
