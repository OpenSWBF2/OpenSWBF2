param([string]$Configuration = 'Debug')
$ErrorActionPreference = 'Stop'
$run = Join-Path $PSScriptRoot 'run.ps1'
$tmp = Join-Path $env:TEMP ("openswbf2-selftest-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $tmp | Out-Null

$list = 'echo %* | findstr /C:"list-test-cases" >nul && (echo ==========& echo fake_case& echo ==========& exit /b 0)'
$oneFail = 'echo [doctest] test cases: 1 ^| 0 passed ^| 1 failed'
$onePass = 'echo [doctest] test cases: 1 ^| 1 passed ^| 0 failed'
$asanWrite = 'for /f "tokens=2 delims=''" %%a in ("%ASAN_OPTIONS%") do echo ERROR: AddressSanitizer: heap-buffer-overflow>"%%a.123"'

$scenarios = @(
    @('listing exit non-zero', 'unit', 'exit /b 3', $null, 2, 'Listing failed', $null),
    @('listing zero cases', 'unit', 'exit /b 0', $null, 2, 'listed 0 cases', $null),
    @('listing timeout', 'unit', "ping -n 3 127.0.0.1 >nul`r`nexit /b 0", '1000', 2, 'timed out', $null),
    @('known_bug did not run once', 'known_bug', "$list`r`nexit /b 0", $null, 1, '\[FAIL\] fake_case', '\[KNOWN\]'),
    @('known_bug genuine failure', 'known_bug', "$list`r`n$oneFail`r`nexit /b 1", $null, 0, '\[KNOWN\] fake_case', $null),
    @('known_bug exit 1 without 1-case output', 'known_bug', "$list`r`nexit /b 1", $null, 1, '\[FAIL\] fake_case', '\[KNOWN\]'),
    @('known_bug crash', 'known_bug', "$list`r`nexit /b 3221225477", $null, 1, '\[CRASH\] fake_case', '\[KNOWN\]'),
    @('asan report fails a passing case', 'unit', "$list`r`n$asanWrite`r`n$onePass`r`nexit /b 0", $null, 1, '\[FAIL\] fake_case[\s\S]*heap-buffer-overflow', $null, @('-Configuration', 'Asan')),
    @('asan exit 66 without report is a startup failure', 'unit', "$list`r`nexit /b 66", $null, 1, 'failed before main', $null, @('-Configuration', 'Asan')),
    @('asan stub without Asan has no asan verdict', 'unit', "$list`r`n$asanWrite`r`n$onePass`r`nexit /b 0", $null, 0, '\[PASS\] fake_case', 'heap-buffer-overflow'),
    @('exit 66 without Asan stays a crash', 'unit', "$list`r`nexit /b 66", $null, 1, '\[CRASH\] fake_case 0x00000042', 'failed before main')
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
        $extra = @()
        if ($sc.Count -gt 7) { $extra = $sc[7] }
        try {
            $out = (& powershell -NoProfile -File $run -NoBuild -Smoke None -Suite $sc[1] @extra 2>&1 | Out-String)
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
if ($Configuration -eq 'Asan') {
    $asanCanaries = @(
        @('asan canary heap overflow is reported', 'heap-buffer-overflow'),
        @('asan canary red heap overflow is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary red heap use after free is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary memory pool overflow is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary memory pool use after free is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary lua bin overflow is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary lua bin use after free is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary net packet overflow is reported', 'use-after-poison[\s\S]*netBigPackets'),
        @('asan canary net packet use after free is reported', 'use-after-poison[\s\S]*netBigPackets'),
        @('asan canary red stack overflow is reported', 'use-after-poison[\s\S]*sMem'),
        @('asan canary red stack use after scope is reported', 'use-after-poison[\s\S]*sMem'),
        @('asan canary stack allocator overflow is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary stack allocator use after free is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary sub allocator overflow is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary sub allocator use after free is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary list allocator overflow is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary list allocator use after free is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary snd free list overflow is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary snd free list use after free is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary glyph memory overflow is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary glyph memory use after free is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap')
    )
    foreach ($canary in $asanCanaries) {
        $n++
        $out = (& powershell -NoProfile -File $run -Configuration Asan -NoBuild -Smoke None -Suite asan_canary -Filter $canary[0] 2>&1 | Out-String)
        $code = $LASTEXITCODE
        if ($code -ne 1 -or -not $out.Contains("[FAIL] $($canary[0])") -or $out -notmatch $canary[1]) { $failed++; Write-Host "FAILED ${n}: $($canary[0]): exit $code, expected 1 with [FAIL] and /$($canary[1])/" }
        else { Write-Host "OK $n $($canary[0])" }
    }
    $n++
    $canarySource = Get-Content -Raw (Join-Path $PSScriptRoot 'unit\asan_canary.cpp')
    $canaryCases = ([regex]::Matches($canarySource, 'TEST_CASE\("asan canary')).Count
    if ($canaryCases -ne $asanCanaries.Count) { $failed++; Write-Host "FAILED ${n}: asan canary table has $($asanCanaries.Count) entries, asan_canary.cpp has $canaryCases cases" }
    else { Write-Host "OK $n asan canary table covers all $canaryCases cases" }
}
else { Write-Host 'SKIP canary: needs -Configuration Asan' }
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
    $n++
    $preDir = Join-Path $smokeTmp 'precheck'
    New-Item -ItemType Directory -Force -Path (Join-Path $preDir 'retail\_lvl_pc') | Out-Null
    Set-Content -Path (Join-Path $preDir 'retail\_lvl_pc\shell.lvl') -Value 'x'
    foreach ($f in 'BattlefrontII.exe', 'binkw32.dll', 'Galaxy.dll', 'GalaxyPeer.dll', 'steam_api.dll') { Set-Content -Path (Join-Path $preDir $f) -Value 'x' }
    & cmd /c mklink /J "$(Join-Path $preDir 'data')" "$(Join-Path $preDir 'retail')" | Out-Null
    $pre = Get-SmokePrecheck $smokeTmp $preDir
    & cmd /c rmdir "$(Join-Path $preDir 'data')" | Out-Null
    if ($pre -notmatch 'staged data') { $failed++; Write-Host "FAILED ${n}: smoke precheck rejects a retail data junction" }
    else { Write-Host "OK $n smoke precheck rejects a retail data junction" }
    $n++
    $rtDir = Join-Path $smokeTmp 'runtimes'
    New-Item -ItemType Directory -Force -Path $rtDir | Out-Null
    Set-Content -Path (Join-Path $rtDir 'a.dll') -Value 'x'
    $miss = @(Get-MissingRuntimes $rtDir @('a.dll', 'b.dll'))
    if ($miss.Count -ne 1 -or $miss[0] -ne 'b.dll') { $failed++; Write-Host "FAILED ${n}: runtime preflight names the missing dll: got '$($miss -join ', ')'" }
    else { Write-Host "OK $n runtime preflight names the missing dll" }
    $n++
    $miss = @(Get-MissingRuntimes $rtDir @())
    if ($miss.Count -ne 0) { $failed++; Write-Host "FAILED ${n}: runtime preflight with an empty list: got '$($miss -join ', ')'" }
    else { Write-Host "OK $n runtime preflight with an empty list" }
    $n++
    $ao = $null
    try { $ao = Get-AsanOptions 'C:\x\case' } catch {}
    if ($ao -cne "exitcode=66:log_path='C:\x\case\asan'") { $failed++; Write-Host "FAILED ${n}: asan options quote the log path: got '$ao'" }
    else { Write-Host "OK $n asan options quote the log path" }
    $n++
    $stub = Join-Path $smokeTmp "smoke$n.cmd"
    Set-Content -Path $stub -Value ("@echo off`r`nfor /f ""tokens=2 delims='"" %%a in (""%ASAN_OPTIONS%"") do echo ERROR: AddressSanitizer: heap-buffer-overflow>""%%a.123""`r`necho OPENSWBF2 RUN COMPLETE>%2`r`nexit /b 0") -Encoding ASCII
    $caseDir = Join-Path $smokeTmp "case$n"
    New-Item -ItemType Directory -Path $caseDir | Out-Null
    $why = $null
    try {
        $r = Invoke-SmokeCase "t$n" $stub $smokeTmp ('-log "' + $caseDir + '\run.log"') 5 $caseDir (Get-AsanOptions $caseDir)
        $resText = [IO.File]::ReadAllText((Join-Path $caseDir 'result.txt'))
        if ($r.Class -ne 'Pass') { $why = "class $($r.Class), expected Pass" }
        elseif ($r.Asan -notmatch 'heap-buffer-overflow') { $why = "Asan is '$($r.Asan)'" }
        elseif ($resText -notmatch 'asan=1') { $why = 'result.txt lacks asan=1' }
    }
    catch { $why = $_.Exception.Message }
    if ($why) { $failed++; Write-Host "FAILED ${n}: smoke asan report is surfaced: $why" }
    else { Write-Host "OK $n smoke asan report is surfaced" }
    $n++
    $stub = Join-Path $smokeTmp "smoke$n.cmd"
    Set-Content -Path $stub -Value "@echo off`r`nif defined ASAN_OPTIONS exit /b 5`r`necho OPENSWBF2 RUN COMPLETE>%2`r`nexit /b 0" -Encoding ASCII
    $caseDir = Join-Path $smokeTmp "case$n"
    New-Item -ItemType Directory -Path $caseDir | Out-Null
    $r = Invoke-SmokeCase "t$n" $stub $smokeTmp ('-log "' + $caseDir + '\run.log"') 5 $caseDir
    $resText = [IO.File]::ReadAllText((Join-Path $caseDir 'result.txt'))
    if ($r.Class -ne 'Pass' -or $null -ne $r.Asan -or $resText -notmatch 'asan=0') { $failed++; Write-Host "FAILED ${n}: smoke without asan options: class $($r.Class) asan '$($r.Asan)'" }
    else { Write-Host "OK $n smoke without asan options sets no ASAN_OPTIONS" }
    $n++
    $emptyDir = Join-Path $smokeTmp 'asan-empty'
    New-Item -ItemType Directory -Path $emptyDir | Out-Null
    $why = $null
    try {
        $r66 = Get-AsanReport $emptyDir 66
        $r0 = Get-AsanReport $emptyDir 0
        if ($r66 -notmatch 'failed before main') { $why = "exit 66 gave '$r66'" }
        elseif ($null -ne $r0) { $why = "exit 0 gave '$r0'" }
    }
    catch { $why = $_.Exception.Message }
    if ($why) { $failed++; Write-Host "FAILED ${n}: asan report without a file: $why" }
    else { Write-Host "OK $n asan report without a file" }
    $n++
    $canaryRx = 'use-after-poison[\s\S]*Startup_RedInitHeap'
    $canaryChecks = @(
        @(66, 'ERROR: AddressSanitizer: use-after-poison ... Startup_RedInitHeap', 'REPORTED'),
        @(0, $null, 'CLEAN'),
        @(66, 'ERROR: AddressSanitizer: heap-buffer-overflow', 'OTHER'),
        @(1, $null, 'OTHER')
    )
    $why = $null
    foreach ($cc in $canaryChecks) {
        $got = Get-AsanCanaryState $cc[0] $cc[1] $canaryRx
        if ($got -ne $cc[2]) { $why = "code $($cc[0]) report '$($cc[1])' gave $got, expected $($cc[2])"; break }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: asan canary state classifier: $why" }
    else { Write-Host "OK $n asan canary state classifier" }
    $n++
    $allNames = @(Get-SmokeCases 'All' @() $false $null | ForEach-Object { $_.Name })
    $excl = @($script:SmokeAsanExcluded)
    $missing = @($excl | Where-Object { $allNames -notcontains $_ })
    if ($excl.Count -ne 14 -or $missing.Count -gt 0) { $failed++; Write-Host "FAILED ${n}: asan exclusions: count $($excl.Count), unknown '$($missing -join ', ')'" }
    else { Write-Host "OK $n asan exclusions name real smoke cases" }
}
finally {
    Remove-Item -Recurse -Force $smokeTmp -ErrorAction SilentlyContinue
}
$goodHdr = "    Characteristics`n         102 characteristics`n    DLL characteristics`n            8140 DLL characteristics`n                   Dynamic base`n                   NX compatible`n                   Application can handle large (>2GB) addresses`n   1C13000 [   38D38] RVA [size] of Base Relocation Directory`n  .reloc name`n"
$cookieCfg = "            0085C930 Security Cookie`n"
$goodCfg = "$cookieCfg            008327F0 Safe Exception Handler Table`n                 7CF Safe Exception Handler Count`n"
$oldHdr = "    Characteristics`n         103 characteristics`n                   Relocations stripped`n                   NX compatible`n         0 [       0] RVA [size] of Base Relocation Directory`n"
$hardScenarios = @(
    @('hardening good text passes all', $goodHdr, $goodCfg, 736, 0, 736, @('laa', 'dynamicbase', 'nxcompat', 'reloc', 'safeseh', 'gs', 'sdl'), @()),
    @('hardening old exe fails dynamicbase and reloc', $oldHdr, $goodCfg, 736, 0, 736, @('nxcompat', 'safeseh', 'gs', 'sdl'), @('dynamicbase', 'reloc', 'laa')),
    @('hardening safeseh count 0 fails', $goodHdr, "$cookieCfg                 0 Safe Exception Handler Count`n", 736, 0, 736, @('laa', 'dynamicbase', 'nxcompat', 'reloc', 'gs', 'sdl'), @('safeseh')),
    @('hardening missing security cookie fails gs', $goodHdr, "                 7CF Safe Exception Handler Count`n", 736, 0, 736, @('laa', 'dynamicbase', 'nxcompat', 'reloc', 'safeseh', 'sdl'), @('gs')),
    @('hardening /GS- entries fail gs', $goodHdr, $goodCfg, 736, 3, 736, @('laa', 'dynamicbase', 'nxcompat', 'reloc', 'safeseh', 'sdl'), @('gs')),
    @('hardening empty tlog fails gs and sdl', $goodHdr, $goodCfg, 0, 0, 0, @('laa', 'reloc', 'safeseh'), @('gs', 'sdl')),
    @('hardening missing /sdl entry fails sdl', $goodHdr, $goodCfg, 736, 0, 735, @('laa', 'dynamicbase', 'nxcompat', 'reloc', 'safeseh', 'gs'), @('sdl')),
    @('hardening non-LAA header fails laa', $goodHdr.Replace('Application can handle large (>2GB) addresses', 'Something else'), $goodCfg, 736, 0, 736, @('dynamicbase', 'nxcompat', 'reloc', 'safeseh', 'gs', 'sdl'), @('laa'))
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
function New-FlagBag($total, $an, $il, $io, $extra) {
    $p = @{ Total = $total; GsMinus = 0; Sdl = $total; AlignedNewOff = $an; InlineOff = $il; InlineOn = $io; ThrowingNew = 0; Gl = 0; FpFast = 0; FpContract = 0 }
    if ($extra) { foreach ($k in $extra.Keys) { $p[$k] = $extra[$k] } }
    return New-Object PSObject -Property $p
}
$allFlags = @('flag-alignednew', 'flag-inline', 'flag-alignednew-tests', 'flag-banned')
$flagScenarios = @(
    @('flags all good pass', (New-FlagBag 748 748 0 748 $null), (New-FlagBag 97 97 0 0 $null), $allFlags, @()),
    @('flags alignednew 747/748 fails', (New-FlagBag 748 747 0 748 $null), (New-FlagBag 97 97 0 0 $null), @('flag-inline', 'flag-alignednew-tests', 'flag-banned'), @('flag-alignednew')),
    @('flags one /Zc:inline- fails', (New-FlagBag 748 748 1 748 $null), (New-FlagBag 97 97 0 0 $null), @('flag-alignednew', 'flag-alignednew-tests', 'flag-banned'), @('flag-inline')),
    @('flags missing /Zc:inline fails', (New-FlagBag 748 748 0 747 $null), (New-FlagBag 97 97 0 0 $null), @('flag-alignednew', 'flag-alignednew-tests', 'flag-banned'), @('flag-inline')),
    @('flags tests missing alignednew fails', (New-FlagBag 748 748 0 748 $null), (New-FlagBag 97 0 0 0 $null), @('flag-alignednew', 'flag-inline', 'flag-banned'), @('flag-alignednew-tests')),
    @('flags one /GL fails banned', (New-FlagBag 748 748 0 748 @{ Gl = 1 }), (New-FlagBag 97 97 0 0 $null), @('flag-alignednew', 'flag-inline', 'flag-alignednew-tests'), @('flag-banned')),
    @('flags one tests /fp:fast fails banned', (New-FlagBag 748 748 0 748 $null), (New-FlagBag 97 97 0 0 @{ FpFast = 1 }), @('flag-alignednew', 'flag-inline', 'flag-alignednew-tests'), @('flag-banned')),
    @('flags empty game tlog fails', (New-FlagBag 0 0 0 0 $null), $null, @('flag-banned'), @('flag-alignednew', 'flag-inline'))
)
foreach ($sc in $flagScenarios) {
    $n++
    $got = @(Get-FlagChecks $sc[1] $sc[2])
    $why = $null
    foreach ($name in $sc[3]) { if (-not ($got | Where-Object { $_.Name -eq $name }).Ok) { $why = "$name should pass" } }
    foreach ($name in $sc[4]) { if (($got | Where-Object { $_.Name -eq $name }).Ok) { $why = "$name should fail" } }
    if (-not $sc[2] -and ($got | Where-Object { $_.Name -eq 'flag-alignednew-tests' })) { $why = 'flag-alignednew-tests must be omitted without a tests tlog' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $why" }
    else { Write-Host "OK $n $($sc[0])" }
}
$n++
$tlogDir = Join-Path $env:TEMP ("openswbf2-tlog-" + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path $tlogDir | Out-Null
    Set-Content -Encoding Unicode -Path (Join-Path $tlogDir 'CL.command.1.tlog') -Value @(
        '^C:\X\A.CPP',
        '/c /IC:\X\GL\INC /Zc:alignedNew- /Zc:inline- /sdl /GS C:\X\A.CPP',
        '^C:\X\B.CPP',
        '/c /IC:\X\GL\INC /Zc:alignedNew- /Zc:inline /sdl /GS /Fo"C:\X\GL\B.OBJ" C:\X\B.CPP'
    )
    $tc = Get-ClTlogCounts $tlogDir
    $why = $null
    if ($tc.Total -ne 2) { $why = "Total=$($tc.Total)" }
    elseif ($tc.AlignedNewOff -ne 2) { $why = "AlignedNewOff=$($tc.AlignedNewOff)" }
    elseif ($tc.InlineOff -ne 1) { $why = "InlineOff=$($tc.InlineOff)" }
    elseif ($tc.InlineOn -ne 1) { $why = "InlineOn=$($tc.InlineOn)" }
    elseif ($tc.Gl -ne 0) { $why = "Gl=$($tc.Gl) matched a path" }
    elseif ($tc.Sdl -ne 2 -or $tc.GsMinus -ne 0) { $why = "Sdl=$($tc.Sdl) GsMinus=$($tc.GsMinus)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: UTF-16 tlog flag counts: $why" }
    else { Write-Host "OK $n UTF-16 tlog flag counts" }
}
finally {
    Remove-Item -Recurse -Force $tlogDir -ErrorAction SilentlyContinue
}
$n++
$suiteDir = Join-Path $env:TEMP ("openswbf2-suite-" + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path (Join-Path $suiteDir 'src') | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $suiteDir 'tests\unit') | Out-Null
    Set-Content -Encoding ASCII -Path (Join-Path $suiteDir 'tests\unit\x.cpp') -Value @(
        'TEST_CASE("listed (a)" * doctest::timeout(5) * doctest::test_suite("unit")) {}',
        'TEST_CASE("probe" * doctest::skip()) {}',
        'TEST_CASE("untagged") {}',
        'TEST_CASE("unlisted" * doctest::test_suite("units")) {}',
        'TEST_SUITE("unit") {}'
    )
    $sc = @(Get-SourceChecks $suiteDir | Where-Object { $_.Name -eq 't1-suite' })
    $want = 'tests\unit\x.cpp:3, tests\unit\x.cpp:4, tests\unit\x.cpp:5'
    if ($sc.Count -ne 1 -or $sc[0].Ok -or $sc[0].Detail -ne $want) { $failed++; Write-Host "FAILED ${n}: t1-suite flags untagged and unlisted cases: $($sc.Detail)" }
    else { Write-Host "OK $n t1-suite flags untagged and unlisted cases" }
}
finally {
    Remove-Item -Recurse -Force $suiteDir -ErrorAction SilentlyContinue
}
$n++
$spacerDir = Join-Path $env:TEMP ("openswbf2-spacer-" + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path (Join-Path $spacerDir 'src') | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $spacerDir 'tests') | Out-Null
    Set-Content -Encoding ASCII -Path (Join-Path $spacerDir 'src\x.h') -Value @(
        'unsigned char acAllowed[0x10];',
        'unsigned char acAllowed[0x10];',
        'char mGap[0x20];',
        'char mPad[12];',
        'char acName[64];',
        'char mRun[16 + 0x4];',
        'unsigned char m[2][0x10];',
        'static const unsigned char kTable[0x4] = { 1, 2, 3, 4 };',
        'mutable char mBuf[0x8];',
        'alignas(4) unsigned char mAligned[0x8];',
        'BYTE mByte[0x8];',
        'uint8_t mU8[0x8];',
        'signed char mS[0x8];',
        'unsigned __int8 mI8[0x8];',
        'char a[4], pad2[0x10];',
        'char mTail[sizeof(int) - 1];',
        '// char mComment[0x10];',
        'char* apNames[0x10];',
        'struct S { bool b; volatile char mVol[0x2]; };',
        'char acSized[0x10];'
    )
    $allow = @('src\x.h|acAllowed|[0x10]', 'src\x.h|acSized|[16]', 'src\x.h|mMissing|[0x4]')
    $sc = @(Get-SourceChecks $spacerDir $allow | Where-Object { $_.Name -eq 'char-spacer' })
    $want = ((@(2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 19, 20) | ForEach-Object { "src\x.h:$_" }) + 'unused allow src\x.h|acSized|[16]' + 'unused allow src\x.h|mMissing|[0x4]') -join ', '
    if ($sc.Count -ne 1 -or $sc[0].Ok -or $sc[0].Detail -ne $want) { $failed++; Write-Host "FAILED ${n}: char-spacer flags every spacer form outside the exact allow-list: $($sc.Detail)" }
    else { Write-Host "OK $n char-spacer flags every spacer form outside the exact allow-list" }
}
finally {
    Remove-Item -Recurse -Force $spacerDir -ErrorAction SilentlyContinue
}
$n++
$offsetDir = Join-Path $env:TEMP ("openswbf2-offset-" + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path (Join-Path $offsetDir 'src') | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $offsetDir 'tests') | Out-Null
    Set-Content -Encoding ASCII -Path (Join-Path $offsetDir 'src\x.cpp') -Value @(
        'p = (Foo*)((char*)pObj + 0x10);',
        'x = *(int*)((char*)this + 0x1c);',
        'q = (const Bar*)((unsigned char*)pObj->mRec - 8);',
        'a = (const sockaddr*)(pPeer + 1);',
        'r = reinterpret_cast<Baz*>(pRec + 2);',
        's = reinterpret_cast<int*>(reinterpret_cast<char*>(pObj) + 4);',
        'Read((char*)mBitmap + 1, n);',
        't = (Node*)((char*)pLink - offsetof(Node, mLink));',
        'u = (Node*)((char*)pLink - (int)offsetof(Node, mLink));',
        'v = (int*)(pMe->mCtrl);',
        'w = (char*)pA - (char*)pB;',
        '// c = (Foo*)((char*)pObj + 0x10);',
        'y = (Foo*)((char*)pAllowed + 4);',
        'y = (Foo*)((char*)pAllowed + 4);',
        'z = (Foo*)( (char*)pSpaced  +  4 );',
        'k = (float)(fA + fB);',
        'h = (char*)p + iSize;',
        'n = (IMAGE_NT_HEADERS*)((char*)pBase + pDos->e_lfanew);',
        'm = (IMAGE_NT_HEADERS*)((char*)pBase + 0x3c);'
    )
    $allow = @('src\x.cpp|(Foo*)((char*)pAllowed + 4)', 'src\x.cpp|(Foo*)( (char*)pSpaced + 4 )', 'src\x.cpp|(Foo*)((char*)pMissing + 4)', 'src\y.cpp|(Foo*)((char*)pAllowed + 4)')
    $sc = @(Get-SourceChecks $offsetDir @() $allow | Where-Object { $_.Name -eq 'offset-hack' })
    $want = ((@(1, 2, 3, 4, 5, 6, 7, 14, 19) | ForEach-Object { "src\x.cpp:$_" }) + 'unused allow src\x.cpp|(Foo*)((char*)pMissing + 4)' + 'unused allow src\y.cpp|(Foo*)((char*)pAllowed + 4)') -join ', '
    if ($sc.Count -ne 1 -or $sc[0].Ok -or $sc[0].Detail -ne $want) { $failed++; Write-Host "FAILED ${n}: offset-hack flags every byte-offset form outside the exact allow-list: $($sc.Detail)" }
    else { Write-Host "OK $n offset-hack flags every byte-offset form outside the exact allow-list" }
}
finally {
    Remove-Item -Recurse -Force $offsetDir -ErrorAction SilentlyContinue
}
$n++
$allCases = @(Get-SmokeCases 'All' @('foo1c_ctf TimeOut', 'tat3c_con Fatal', 'bar1c_con', 'quux1c_con Pass', 'foo2c_ctf Pass 240') $false $null)
$byName = @{}
foreach ($c in $allCases) { $byName[$c.Name] = $c }
$rejected = 0
foreach ($badLine in 'baz1c_con Bogus', 'qux1c_con GsFailure', 'foo3c_ctf Pass abc') {
    try { Get-SmokeCases 'All' @($badLine) $false $null | Out-Null } catch { $rejected++ }
}
$why = $null
if ($byName['foo1c_ctf'].Expect.Class -ne 'TimeOut' -or $byName['foo1c_ctf'].Timeout -ne 0 -or $byName['foo1c_ctf'].Expect.LogRx -ne 'OPENSWBF2 MISSION RUNNING') { $why = 'TimeOut line' }
elseif ($byName['tat3c_con'].Expect.Class -ne 'Fatal' -or $byName['tat3c_con'].Timeout -ne 0) { $why = 'Fatal line' }
elseif (-not $byName['tat3c_con'].Expect.LogRx -or $byName['tat3c_con'].Expect.Code -ne 3 -or $byName['tat3c_con'].Expect.MaxDumps -ne 1) { $why = 'Fatal line must pin the d3dx9 signature' }
elseif ('FATAL: exception 0xC0000005 at 0x658FA553 (d3dx9_43.dll+0x1BA553) build 6ADE5B7C1C4C000' -notmatch $byName['tat3c_con'].Expect.LogRx) { $why = 'Fatal LogRx does not match the recorded tat3 crash' }
elseif ($byName['bar1c_con'].Expect.Class -ne 'Pass' -or $byName['quux1c_con'].Expect.Class -ne 'Pass') { $why = 'missing or Pass class must be Pass' }
elseif ($byName['foo2c_ctf'].Expect.Class -ne 'Pass' -or $byName['foo2c_ctf'].Timeout -ne 240 -or $byName['bar1c_con'].Timeout -ne 0 -or $byName['quux1c_con'].Timeout -ne 0) { $why = 'third token must set the case timeout' }
elseif ($rejected -ne 3) { $why = 'unknown classes, GsFailure and non-integer timeouts must be rejected' }
elseif (@($allCases | Where-Object { $_.Name -like 'crash-*' }).Count -ne 40 -or -not $byName.ContainsKey('fatal-init') -or $byName['dedicated'].Timeout -ne 0) { $why = 'dedicated or crash cases changed' }
if ($why) { $failed++; Write-Host "FAILED ${n}: all-missions parsing: $why" }
else { Write-Host "OK $n all-missions parsing" }
$n++
$hrx = $script:SmokeAddrSpaceRx
$okLine = 'OPENSWBF2 ADDRSPACE arena=0x8000F020 arena_end=0x8400F020 laa=1 steam=1 steam_api=0x65740000 Galaxy=0x652E0000 GalaxyPeer=0x00000000 binkw32=0x18000000'
$badLines = @($okLine.Replace('arena=0x8000F020', 'arena=0x00EA7020'), $okLine.Replace('laa=1', 'laa=0'), $okLine.Replace('steam=1', 'steam=0'), $okLine.Replace('Galaxy=0x652E0000', 'Galaxy=0x00000000'), $okLine.Replace('binkw32=0x18000000', 'binkw32=0x00000000'), $okLine.Replace('steam_api=0x65740000', 'steam_api=0x00000000'))
$hogCases = @(Get-SmokeCases 'All' @('foo1c_ctf TimeOut', 'tat3c_con Fatal', 'bar1c_con') $false $null $true)
$hogByName = @{}
foreach ($c in $hogCases) { $hogByName[$c.Name] = $c }
$why = $null
if ($okLine -cnotmatch $hrx) { $why = 'positive line rejected' }
foreach ($b in $badLines) { if (-not $why -and $b -cmatch $hrx) { $why = "bad line accepted: $b" } }
if (-not $why) {
    foreach ($name in 'foo1c_ctf', 'tat3c_con', 'bar1c_con', 'dedicated') {
        $c = $hogByName[$name]
        if (-not $c.Args.Contains('-hogaddrspace')) { $why = "$name lacks hog" }
        elseif (@($c.Expect.LogRx) -notcontains $hrx) { $why = "$name lacks ADDRSPACE rx" }
    }
}
if (-not $why) {
    foreach ($c in $hogCases) {
        if ($c.Name -like 'crash-*' -or $c.Name -eq 'fatal-init') {
            if ($c.Args.Contains('-hogaddrspace') -or @($c.Expect.LogRx) -contains $hrx) { $why = "$($c.Name) must not be hogged" }
        }
    }
}
if (-not $why) {
    $plain = @(Get-SmokeCases 'All' @('bar1c_con') $false $null)
    foreach ($c in $plain) { if ($c.Args.Contains('-hogaddrspace') -or @($c.Expect.LogRx) -contains $hrx) { $why = "$($c.Name) hogged by default" } }
}
if ($why) { $failed++; Write-Host "FAILED ${n}: hog smoke cases: $why" }
else { Write-Host "OK $n hog smoke cases" }
$n++
$psrc = Join-Path $env:TEMP ("openswbf2-parallel-" + [guid]::NewGuid().ToString('N'))
$why = $null
$pbatch = $null
$pdirs = @()
try {
    New-Item -ItemType Directory -Path (Join-Path $psrc 'data') | Out-Null
    Copy-Item -LiteralPath $env:ComSpec -Destination (Join-Path $psrc 'BattlefrontII.exe')
    Set-Content -Path (Join-Path $psrc 'binkw32.dll') -Value 'x'
    Set-Content -Path (Join-Path $psrc 'data\marker.txt') -Value 'x'
    foreach ($w in 1, 2) {
        $d = Join-Path $psrc "workers\w$w"
        New-SmokeWorkerDir $psrc $d
        $pdirs += $d
        $link = Get-Item -LiteralPath (Join-Path $d 'data') -Force
        if (-not (Test-Path -LiteralPath (Join-Path $d 'BattlefrontII.exe')) -or -not (Test-Path -LiteralPath (Join-Path $d 'binkw32.dll'))) { $why = "worker $w lacks exe or dll" }
        elseif (-not ($link.Attributes -band [IO.FileAttributes]::ReparsePoint) -or -not (Test-Path -LiteralPath (Join-Path $d 'data\marker.txt'))) { $why = "worker $w data is not a junction to the source data" }
    }
    if (-not $why) {
        $items = @(
            (New-Object PSObject -Property @{ Name = 'slow'; ExeName = 'BattlefrontII.exe'; ArgLine = '/c ping -n 4 127.0.0.1 >nul'; Timeout = 60; CaseDir = (Join-Path $psrc 'case-slow') }),
            (New-Object PSObject -Property @{ Name = 'fast'; ExeName = 'BattlefrontII.exe'; ArgLine = '/c exit 0'; Timeout = 60; CaseDir = (Join-Path $psrc 'case-fast') })
        )
        $pbatch = Start-SmokeBatch $items $pdirs
        $slow = Receive-SmokeCase $pbatch 0
        $fast = Receive-SmokeCase $pbatch 1
        if ($slow.Name -ne 'slow' -or $fast.Name -ne 'fast') { $why = 'results not in case order' }
        elseif ($slow.WorkDir -eq $fast.WorkDir) { $why = 'concurrent cases shared a worker folder' }
        elseif ($fast.Ms -ge $slow.Ms) { $why = "fast case ($($fast.Ms) ms) did not finish before slow ($($slow.Ms) ms)" }
    }
}
catch { $why = $_.Exception.Message }
finally {
    if ($pbatch) { Stop-SmokeBatch $pbatch }
    foreach ($d in $pdirs) { Remove-SmokeWorkerDir $d }
    Remove-Item -Recurse -Force $psrc -ErrorAction SilentlyContinue
}
if ($why) { $failed++; Write-Host "FAILED ${n}: parallel smoke: $why" }
else { Write-Host "OK $n parallel smoke" }
function New-LaaRec($l, $lr, $wr, $frames = 9274, $dur = 309) {
    return New-Object PSObject -Property @{ Key = "geo1c_con`t ${dur}s`tL=Rep`t28u@$l`P`t ${lr}r`t 0p`tW=CIS`t28u@10 `t ${wr}r`t 0p"; Frames = $frames }
}
$laaBase = 1..5 | ForEach-Object { New-LaaRec 15 36 97 }
$laaCases = @(
    @('laa compare identical passes', { (Compare-LaaRuns $laaBase $laaBase).Ok }, $true),
    @('laa compare frame count differs fails', { $r = Compare-LaaRuns $laaBase (@($laaBase[0..3]) + (New-LaaRec 15 36 97 9275)); (-not $r.Ok) -and (($r.Failures -join ';') -match 'runs differ') }, $true),
    @('laa compare reinforcement differs fails', { (Compare-LaaRuns $laaBase (@($laaBase[0..3]) + (New-LaaRec 15 35 97))).Ok }, $false),
    @('laa compare four runs fails', { (Compare-LaaRuns $laaBase @($laaBase[0..3])).Ok }, $false)
)
foreach ($lc in $laaCases) {
    $n++
    $got = [bool](& $lc[1])
    if ($got -ne $lc[2]) { $failed++; Write-Host "FAILED ${n}: $($lc[0])" }
    else { Write-Host "OK $n $($lc[0])" }
}
$digTmp = Join-Path $env:TEMP ("openswbf2-digest-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $digTmp | Out-Null
$savedExpected = $env:OPENSWBF2_DIGEST_EXPECTED
try {
    $n++
    $digLine = 'OPENSWBF2 SIM DIGEST turns=1500 time=50.000 rand=1/2 snd=49.923 state=0x0000ABCD'
    [IO.File]::WriteAllText((Join-Path $digTmp 'run.log'), "$digLine`r`n")
    $goodExp = Join-Path $digTmp 'good.txt'
    $flipExp = Join-Path $digTmp 'flip.txt'
    [IO.File]::WriteAllText($goodExp, "# header`r`nselftest-case`t$digLine`r`n")
    [IO.File]::WriteAllText($flipExp, "selftest-case`t$($digLine.Replace('state=0x0000ABCD', 'state=0x0000ABCC'))`r`n")
    $digCase = New-Object PSObject -Property @{ Name = 'selftest-case'; Expect = @{ Class = 'Pass' } }
    $digRes = New-Object PSObject -Property @{ Class = 'Pass'; CaseDir = $digTmp; LogText = $digLine; Dumps = @() }
    $env:OPENSWBF2_DIGEST_EXPECTED = $goodExp
    $okGood = Test-SmokeExpect $digCase $digRes
    $env:OPENSWBF2_DIGEST_EXPECTED = $flipExp
    $okFlip = Test-SmokeExpect $digCase $digRes
    if (-not $okGood -or $okFlip) { $failed++; Write-Host "FAILED ${n}: digest compare: matching=$okGood flipped=$okFlip" }
    else { Write-Host "OK $n digest compare fails on a one-bit state flip" }
    $n++
    $trA = Join-Path $digTmp 'trace-a.log'
    $trB = Join-Path $digTmp 'trace-b.log'
    $linesA = 1..9 | ForEach-Object { 'OPENSWBF2 DIGEST TRACE turn={0} state=0x{0:X8} rand=1/2' -f $_ }
    $linesB = 1..9 | ForEach-Object { if ($_ -ge 7) { 'OPENSWBF2 DIGEST TRACE turn={0} state=0x{1:X8} rand=1/2' -f $_, ($_ + 100) } else { 'OPENSWBF2 DIGEST TRACE turn={0} state=0x{0:X8} rand=1/2' -f $_ } }
    [IO.File]::WriteAllLines($trA, [string[]]$linesA)
    [IO.File]::WriteAllLines($trB, [string[]]$linesB)
    $trC = Join-Path $digTmp 'trace-c.log'
    $linesC = 1..9 | ForEach-Object { 'OPENSWBF2 DIGEST TRACE turn={0} state=0x{0:X8} rand=3/4' -f $_ }
    [IO.File]::WriteAllLines($trC, [string[]]$linesC)
    $trE = Join-Path $digTmp 'trace-e.log'
    [IO.File]::WriteAllLines($trE, [string[]]@('no trace here'))
    $split = Compare-DigestTrace $trA $trB
    $same = Compare-DigestTrace $trA $trA
    $randOnly = Compare-DigestTrace $trA $trC
    $empty = Compare-DigestTrace $trA $trE
    if ($split -ne 7 -or $same -ne 0 -or $randOnly -ne 0 -or $empty -ne -1) { $failed++; Write-Host "FAILED ${n}: digest trace split: split=$split same=$same randOnly=$randOnly empty=$empty" }
    else { Write-Host "OK $n digest trace split names turn 7, ignores rand, fails an empty trace" }
    $n++
    $errLine = '[t=1.000] [f=1] LogMemory Error X.cpp:1: boom'
    $errLog = Join-Path $digTmp 'run.log'
    $errRes = New-Object PSObject -Property @{ Class = 'Pass'; CaseDir = $digTmp; LogText = $errLine; Dumps = @() }
    $errCase = New-Object PSObject -Property @{ Name = 'selftest-errorrx'; Expect = @{ Class = 'Pass'; ErrorRx = @('LogMemory Error X\.cpp:\d+: boom') } }
    $missCase = New-Object PSObject -Property @{ Name = 'selftest-errorrx'; Expect = @{ Class = 'Pass'; ErrorRx = @('nomatch') } }
    $plainCase = New-Object PSObject -Property @{ Name = 'selftest-errorrx'; Expect = @{ Class = 'Pass' } }
    [IO.File]::WriteAllText($errLog, "$errLine`r`n")
    $errA = Test-SmokeExpect $errCase $errRes
    $errC = Test-SmokeExpect $missCase $errRes
    $errD = Test-SmokeExpect $plainCase $errRes
    [IO.File]::WriteAllText($errLog, "[t=1.000] [f=1] LogMemory Inform X.cpp:1: fine`r`n")
    $errB = Test-SmokeExpect $errCase $errRes
    if (-not $errA -or $errB -or $errC -or $errD) { $failed++; Write-Host "FAILED ${n}: smoke ErrorRx: matched=$errA none=$errB nomatch=$errC noErrorRx=$errD" }
    else { Write-Host "OK $n smoke ErrorRx expects an Error-driven failure" }
}
finally {
    $env:OPENSWBF2_DIGEST_EXPECTED = $savedExpected
    Remove-Item -Recurse -Force $digTmp -ErrorAction SilentlyContinue
}
$n++
$allLines = @(Get-Content (Join-Path $PSScriptRoot 'smoke\missions-all.txt') | Where-Object { $_.Trim() })
$parAll = @{}
foreach ($c in @(Get-SmokeCases 'All' $allLines $false $null)) { $parAll[$c.Name] = $c }
$parTw = @{}
foreach ($c in @(Get-SmokeCases 'Twice' @() $false $null)) { $parTw[$c.Name] = $c }
$parStd = @{}
foreach ($c in @(Get-SmokeCases 'Standard' @() $false $null)) { $parStd[$c.Name] = $c }
$why = $null
$sfx = ' -logcmds "LogMemory Verbose"'
foreach ($p in @(@('nab2g_c', $parTw, 'twice-digest-1'), @('hot1g_con', $parTw, 'digest-hot1g_con'), @('spa3c_ass', $parTw, 'digest-spa3c_ass'), @('geo1c_con', $parStd, 'geo1c_con'), @('dag1c_ctf', $parStd, 'digest-dag1c_ctf'), @('kas2c_uber', $parStd, 'digest-kas2c_uber'))) {
    if (-not $parAll.ContainsKey($p[0])) { $why = "All case $($p[0]) missing"; break }
    if (-not $p[1].ContainsKey($p[2])) { $why = "case $($p[2]) missing"; break }
    $allArgs = $parAll[$p[0]].Args
    if (-not $allArgs.EndsWith($sfx)) { $why = "All $($p[0]) lacks the LogMemory Verbose switch"; break }
    if ($allArgs.Substring(0, $allArgs.Length - $sfx.Length) -cne $p[1][$p[2]].Args) { $why = "All $($p[0]) args differ from $($p[2])"; break }
}
if (-not $why -and $parAll['nab2g_c'].Timeout -ne 0) { $why = 'All nab2g_c must use the default timeout' }
if (-not $why -and (Get-SmokeCases 'Standard' @() $false $null)[0].Name -cne 'geo1c_con') { $why = 'Standard[0] must be geo1c_con for the ASLR probe' }
if ($why) { $failed++; Write-Host "FAILED ${n}: all/twice digest parity: $why" }
else { Write-Host "OK $n all/twice digest parity" }
$testExe = Join-Path (Split-Path $PSScriptRoot -Parent) "bin\Tests\$Configuration\OpenSWBF2Tests.exe"
foreach ($probe in 'strictness probe emits an undeclared error', 'strictness probe unmatched expectation') {
    $n++
    if (-not (Test-Path $testExe)) { $failed++; Write-Host "FAILED ${n}: ${probe}: test exe not built"; continue }
    $out = (& $testExe "-tc=$probe" -ns 2>&1 | Out-String)
    $code = $LASTEXITCODE
    if ($code -ne 1 -or $out -notmatch '\[UNEXPECTED-LOG\]') { $failed++; Write-Host "FAILED ${n}: ${probe}: exit $code, expected 1 with [UNEXPECTED-LOG]" }
    else { Write-Host "OK $n $probe" }
}
$grTmp = Join-Path $env:TEMP ("openswbf2-selftest-gr-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $grTmp | Out-Null
$grScript = Join-Path $PSScriptRoot 'guard-revert.ps1'
$repo = Split-Path $PSScriptRoot -Parent
$junction = Join-Path $grTmp 'reparse\nested\link'
try {
    $rowLine = "`$GuardRevertRows = @(@{ Id = 'SELFTEST-01'; File = 'x'; Anchor = 'x'; Replacement = 'x'; Tests = @('x') })"
    $grCases = @(
        @('guard-revert exits 1 when the rows file throws', "throw 'selftest rows failure'", 'selftest rows failure'),
        @('guard-revert exits 1 when a step after row selection throws', "function Find-GameDataRoot { throw 'selftest mid-run failure' }", 'selftest mid-run failure')
    )
    foreach ($gc in $grCases) {
        $n++
        $rowsFile = Join-Path $grTmp "rows$n.ps1"
        Set-Content -Path $rowsFile -Value ($rowLine + "`r`n" + $gc[1]) -Encoding ASCII
        $wtBefore = @(& git -C $repo worktree list).Count
        $grOut = Join-Path $grTmp "out$n"
        $out = (& powershell -NoProfile -Command "& '$grScript' -RowsFile '$rowsFile' -OutDir '$grOut' -ProvenFile '$(Join-Path $grTmp "proven$n.tsv")'; exit `$LASTEXITCODE" 2>&1 | Out-String)
        $code = $LASTEXITCODE
        $wtAfter = @(& git -C $repo worktree list).Count
        $why = $null
        if ($code -ne 1) { $why = "exit $code, expected 1" }
        elseif ($out -notmatch 'ERROR:' -or $out -notmatch [regex]::Escape($gc[2])) { $why = "output lacks ERROR: $($gc[2])" }
        elseif ($wtAfter -ne $wtBefore) { $why = "worktree count went from $wtBefore to $wtAfter" }
        if ($why) { $failed++; Write-Host "FAILED ${n}: $($gc[0]): $why" }
        else { Write-Host "OK $n $($gc[0])" }
    }
    $n++
    $midMark = Join-Path $repo ("selftest-midrun-" + [guid]::NewGuid().ToString('N') + '.txt')
    $midProven = Join-Path $grTmp "proven$n.tsv"
    $rowsFile = Join-Path $grTmp "rows$n.ps1"
    Set-Content -Path $rowsFile -Encoding ASCII -Value @(
        $rowLine,
        'function Assert-Runtimes { }',
        'function Find-GameDataRoot { }',
        'function Find-MSBuild { ''msbuild'' }',
        'function Build-Tests { $true }',
        'function Set-Anchor { $null }',
        '$script:SelftestRuns = 0',
        'function Run-Case {',
        '    $script:SelftestRuns++',
        '    $state = ''GREEN''',
        "    if (`$script:SelftestRuns -gt 1) { Set-Content -LiteralPath '$midMark' -Value 'x'; `$state = 'RED' }",
        '    New-Object PSObject -Property @{ State = $state; Detail = ''''; Code = 0; Text = ''''; Report = $null }',
        '}'
    )
    $wtBefore = @(& git -C $repo worktree list).Count
    try {
        $out = (& powershell -NoProfile -Command "& '$grScript' -RowsFile '$rowsFile' -OutDir '$(Join-Path $grTmp "out$n")' -ProvenFile '$midProven'; exit `$LASTEXITCODE" 2>&1 | Out-String)
        $code = $LASTEXITCODE
    }
    finally {
        Remove-Item -LiteralPath $midMark -Force -ErrorAction SilentlyContinue
    }
    $wtAfter = @(& git -C $repo worktree list).Count
    $why = $null
    if ($code -ne 1) { $why = "exit $code, expected 1" }
    elseif ($out -notmatch 'PROOFS NOT RECORDED: main tree changed during the run') { $why = 'no PROOFS NOT RECORDED line for the mid-run change' }
    elseif (Test-Path -LiteralPath $midProven) { $why = 'a proof was recorded after the main tree changed' }
    elseif ($out -notmatch 'SELFTEST-01\s+control=GREEN\s+reverted=RED') { $why = 'the row did not run to RED, so the case proves nothing' }
    elseif ($wtAfter -ne $wtBefore) { $why = "worktree count went from $wtBefore to $wtAfter" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: guard-revert records no proof when the main tree changes mid-run: $why" }
    else { Write-Host "OK $n guard-revert records no proof when the main tree changes mid-run" }
    $n++
    $wtBefore = @(& git -C $repo worktree list).Count
    $grWt =Join-Path $repo 'artifacts\guard-revert\wt'
    $grDirs = @(Get-ChildItem -LiteralPath $grWt -Directory -Filter 'gr-*' -ErrorAction SilentlyContinue | ForEach-Object { $_.Name })
    $grOut = Join-Path $grTmp "out$n"
    $out = @(& powershell -NoProfile -Command "& '$grScript' -ListSelected -OutDir '$grOut' -ProvenFile '$(Join-Path $grTmp "proven$n.tsv")'; exit `$LASTEXITCODE" 2>&1 | ForEach-Object { "$_" })
    $code = $LASTEXITCODE
    $wtAfter = @(& git -C $repo worktree list).Count
    $grNew = @(Get-ChildItem -LiteralPath $grWt -Directory -Filter 'gr-*' -ErrorAction SilentlyContinue | Where-Object { $grDirs -notcontains $_.Name })
    $GuardRevertRows = $null
    . (Join-Path $PSScriptRoot 'guardrevert\rows.ps1')
    $listIds = @($out | Where-Object { $_ -match '^(?:SELECT|CARRY) ' } | ForEach-Object { $_.Split(' ')[1] })
    $rowIds = @($GuardRevertRows | ForEach-Object { $_.Id })
    $idDiff = @(Compare-Object @($rowIds | Sort-Object) @($listIds | Sort-Object) -CaseSensitive)
    $why = $null
    if ($code -ne 0) { $why = "exit $code" }
    elseif ($listIds.Count -ne $rowIds.Count) { $why = "$($listIds.Count) list lines for $($rowIds.Count) rows" }
    elseif ($idDiff.Count) { $why = "listed ids differ from rows.ps1: $($idDiff[0].InputObject)" }
    elseif ($wtAfter -ne $wtBefore) { $why = "worktree count went from $wtBefore to $wtAfter" }
    elseif ($grNew.Count) { $why = "new scratch tree $($grNew[0].Name)" }
    elseif (Test-Path -LiteralPath $grOut) { $why = 'the run created its output directory, so it got past the build step' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: guard-revert -ListSelected: $why" }
    else { Write-Host "OK $n guard-revert -ListSelected lists every row once and builds nothing" }
    $n++
    $rpTarget = Join-Path $grTmp 'target'
    New-Item -ItemType Directory -Force -Path (Split-Path $junction -Parent) | Out-Null
    New-Item -ItemType Directory -Force -Path $rpTarget | Out-Null
    $sentinel = Join-Path $rpTarget 'sentinel.txt'
    Set-Content -Path $sentinel -Value 'x'
    & cmd /c mklink /J "$junction" "$rpTarget" | Out-Null
    $plain = Join-Path $grTmp 'plain'
    New-Item -ItemType Directory -Force -Path (Join-Path $plain 'sub') | Out-Null
    Set-Content -Path (Join-Path $plain 'sub\f.txt') -Value 'x'
    $hits = @(Get-ReparsePoints (Join-Path $grTmp 'reparse'))
    $none = @(Get-ReparsePoints $plain)
    & cmd /c rmdir "$junction" | Out-Null
    $why = $null
    if ($hits.Count -ne 1 -or $hits[0] -ne $junction) { $why = "junction scan got '$($hits -join ', ')'" }
    elseif ($none.Count -ne 0) { $why = "plain scan got '$($none -join ', ')'" }
    elseif (-not (Test-Path -LiteralPath $sentinel)) { $why = 'sentinel file is gone' }
    elseif (Test-Path -LiteralPath $junction) { $why = 'junction still exists after rmdir' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: reparse point scan: $why" }
    else { Write-Host "OK $n reparse point scan reports the junction and not its target" }
}
finally {
    if (Test-Path -LiteralPath $junction) { & cmd /c rmdir "$junction" | Out-Null }
    if (-not (Test-Path -LiteralPath $junction)) { Remove-Item -LiteralPath $grTmp -Recurse -Force -ErrorAction SilentlyContinue }
}
$stTmp = Join-Path $env:TEMP ("openswbf2-selftest-staged-" + [guid]::NewGuid().ToString('N'))
$stLinks = @()
try {
    $stTargetA = Join-Path $stTmp 'retail\AAA'
    $stTargetB = Join-Path $stTmp 'retail\BBB'
    $stTargetC = Join-Path $stTmp 'retail\CCC'
    foreach ($t in $stTargetA, $stTargetB, $stTargetC) {
        New-Item -ItemType Directory -Force -Path $t | Out-Null
        Set-Content -Path (Join-Path $t 'sentinel.txt') -Value 'x'
    }
    $staged = Join-Path $stTmp 'staged'
    New-Item -ItemType Directory -Force -Path (Join-Path $staged 'data\_lvl_pc') | Out-Null
    Set-Content -Path (Join-Path $staged 'data\_lvl_pc\shell.lvl') -Value 'x'
    Set-Content -Path (Join-Path $staged 'BattlefrontII.exe') -Value 'x'
    $stLinks += Join-Path $staged 'data\_lvl_pc\AAA'
    & cmd /c mklink /J "$($stLinks[-1])" "$stTargetA" | Out-Null
    $stLinks += Join-Path $staged 'data\BBB'
    & cmd /c mklink /J "$($stLinks[-1])" "$stTargetB" | Out-Null
    $stSelf = Join-Path $stTmp 'selflink'
    $stLinks += $stSelf
    & cmd /c mklink /J "$stSelf" "$stTargetC" | Out-Null

    $n++
    $why = $null
    try { Remove-StagedOutput $staged } catch { $why = "threw: $_" }
    if (-not $why) {
        if (Test-Path -LiteralPath $staged) { $why = 'staged folder still exists' }
        elseif (-not (Test-Path -LiteralPath (Join-Path $stTargetA 'sentinel.txt'))) { $why = '_lvl_pc junction target sentinel is gone' }
        elseif (-not (Test-Path -LiteralPath (Join-Path $stTargetB 'sentinel.txt'))) { $why = 'data junction target sentinel is gone' }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: staged cleanup keeps junction targets: $why" }
    else { Write-Host "OK $n staged cleanup removes the folder and keeps junction targets" }

    $n++
    $why = $null
    try { Remove-StagedOutput (Join-Path $stTmp 'missing') } catch { $why = "threw: $_" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: staged cleanup of a missing folder: $why" }
    else { Write-Host "OK $n staged cleanup of a missing folder is a no-op" }

    $n++
    $why = $null
    try { Remove-StagedOutput $stSelf } catch { $why = "threw: $_" }
    if (-not $why) {
        if (Test-Path -LiteralPath $stSelf) { $why = 'junction still exists' }
        elseif (-not (Test-Path -LiteralPath (Join-Path $stTargetC 'sentinel.txt'))) { $why = 'junction target sentinel is gone' }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: staged cleanup of a junction folder: $why" }
    else { Write-Host "OK $n staged cleanup of a junction folder removes only the junction" }

    $n++
    $why = $null
    $stArt = Join-Path $stTmp 'artifacts'
    $stWorker = Join-Path $stArt 'tests\run\smoke-workers\w1'
    New-Item -ItemType Directory -Force -Path $stWorker | Out-Null
    Set-Content -Path (Join-Path $stWorker 'result.txt') -Value 'x'
    $stLinks += Join-Path $stWorker 'data'
    & cmd /c mklink /J "$($stLinks[-1])" "$stTargetA" | Out-Null
    try { Remove-Junctions $stArt } catch { $why = "threw: $_" }
    if (-not $why) {
        if (Test-Path -LiteralPath $stLinks[-1]) { $why = 'artifact junction still exists' }
        elseif (-not (Test-Path -LiteralPath (Join-Path $stWorker 'result.txt'))) { $why = 'artifact file next to the junction is gone' }
        elseif (-not (Test-Path -LiteralPath (Join-Path $stTargetA 'sentinel.txt'))) { $why = 'junction target sentinel is gone' }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: artifact junction cleanup: $why" }
    else { Write-Host "OK $n artifact junction cleanup removes only the junctions and keeps the artifacts" }
}
finally {
    foreach ($l in $stLinks) { if (Test-Path -LiteralPath $l) { & cmd /c rmdir "$l" | Out-Null } }
    $stLeft = @($stLinks | Where-Object { Test-Path -LiteralPath $_ })
    if ($stLeft.Count -eq 0 -and (Test-Path -LiteralPath $stTmp)) { Remove-Item -LiteralPath $stTmp -Recurse -Force -ErrorAction SilentlyContinue }
}
. (Join-Path $PSScriptRoot 'guardrevert\select.ps1')
$selTmp = Join-Path $env:TEMP ("openswbf2-selftest-sel-" + [guid]::NewGuid().ToString('N'))
function Write-SelResult([string]$Name, $Why) {
    $script:n++
    if ($Why) { $script:failed++; Write-Host "FAILED $($script:n): selector: ${Name}: $Why" }
    else { Write-Host "OK $($script:n) selector: $Name" }
}
function Invoke-SelCase($Over) {
    $in = @{}
    foreach ($k in $selBase.Keys) { $in[$k] = $selBase[$k] }
    foreach ($k in $Over.Keys) { $in[$k] = $Over[$k] }
    $r = @{}
    foreach ($x in @(Select-GuardRows $in)) { if ($null -ne $x) { $r[$x.Id] = $x } }
    $r
}
function Test-SelCase([string]$Name, $Over, [string[]]$Want, [string]$Reason) {
    $why = $null
    try {
        $r = Invoke-SelCase $Over
        $sel = @($r.Values | Where-Object { $_.Action -eq 'SELECT' } | ForEach-Object { $_.Id } | Sort-Object)
        $exp = @($Want | Sort-Object)
        if ($r.Count -ne $selRows.Count) { $why = "$($r.Count) results for $($selRows.Count) rows" }
        elseif (($sel -join ',') -ne ($exp -join ',')) { $why = "selected '$($sel -join ',')', expected '$($exp -join ',')'" }
        elseif ($Reason) {
            $bad = @($r.Values | Where-Object { $_.Action -eq 'SELECT' -and $_.Reason -notlike $Reason })
            if ($bad.Count) { $why = "$($bad[0].Id) reason '$($bad[0].Reason)', expected '$Reason'" }
        }
        if (-not $why) {
            $c = @($r.Values | Where-Object { $_.Action -eq 'CARRY' -and $_.Commit -ne 'c0ffee1' })
            if ($c.Count) { $why = "$($c[0].Id) carried without its proof commit" }
        }
    }
    catch { $why = "threw: $_" }
    Write-SelResult $Name $why
}
function Copy-SelHashes([string]$Path, $Value) {
    $h = @{}
    foreach ($k in $selHashes.Keys) { $h[$k] = $selHashes[$k] }
    if ($null -eq $Value) { $h.Remove($Path) } else { $h[$Path] = $Value }
    $h
}
try {
    New-Item -ItemType Directory -Path $selTmp | Out-Null
    $why = $null
    try {
        $pt = Join-Path $selTmp 'parse.tlog'
        Set-Content -Encoding Unicode -Path $pt -Value @('^C:\R\SRC\A.CPP', 'C:\R\SRC\A.H', 'C:\PROGRAM FILES\X.H', '^C:\R\SRC\B.CPP|C:\R\SRC\C.CPP', 'C:\R\SRC\A.H')
        $pr = Read-ClReadTlog $pt 'C:\R\'
        if ($null -eq $pr -or $null -eq $pr.Sets) { $why = 'no result' }
        elseif ($pr.Sets.Count -ne 3) { $why = "$($pr.Sets.Count) TUs, expected 3" }
        else {
            foreach ($tu in 'src\A.cpp', 'src\B.cpp', 'src\C.cpp') {
                if (-not $pr.Sets.ContainsKey($tu)) { $why = "no TU $tu"; break }
                $s = $pr.Sets[$tu]
                if ($s.Count -ne 2 -or -not $s.Contains($tu) -or -not $s.Contains('src\A.h')) { $why = "$tu reads '$(@($s) -join ', ')'"; break }
            }
        }
    }
    catch { $why = "threw: $_" }
    Write-SelResult 'UTF-16 tlog parses blocks, multi-source groups and drops out-of-repo paths' $why

    $why = $null
    try {
        $h1 = Get-GuardRowHash @{ Id = 'A'; File = 'f'; Anchor = "x`r`ny"; Tests = @('t1', 't2') }
        $h2 = Get-GuardRowHash @{ Tests = @('t1', 't2'); Anchor = "x`ny"; File = 'f'; Id = 'A' }
        $h3 = Get-GuardRowHash @{ Id = 'A'; File = 'f'; Anchor = "x`ny"; Tests = @('t2', 't1') }
        $h4 = Get-GuardRowHash @{ Id = 'A'; File = 'f'; Anchor = "x`ny"; Tests = @('t1', 't2'); Asan = $true }
        if ($h1 -notmatch '^[0-9a-f]{64}$') { $why = "hash '$h1' is not lower-case SHA-256 hex" }
        elseif ($h1 -ne $h2) { $why = 'key order or CRLF changed the hash' }
        elseif ($h1 -eq $h3) { $why = 'Tests order did not change the hash' }
        elseif ($h1 -eq $h4) { $why = 'an extra key did not change the hash' }
        elseif ((Get-GuardRowKind @{ Asan = $true }) -ne 'Asan' -or (Get-GuardRowKind @{ Kind = 'Smoke' }) -ne 'Smoke' -or (Get-GuardRowKind @{ Id = 'x' }) -ne 'T1') { $why = 'row kind' }
    }
    catch { $why = "threw: $_" }
    Write-SelResult 'row hash folds CRLF, ignores key order and covers Tests order and every key' $why

    $selRoot = Join-Path $selTmp 'repo'
    $selFiles = @('src\G\Game.cpp', 'src\G\Game.h', 'src\G\Lib.cpp', 'src\G\Lib.h', 'src\G\Shared.h', 'src\X\pcWindow.cpp', 'src\G\Other.cpp', 'tests\unit\t_lib.cpp', 'tests\unit\t_pin.cpp', 'tests\unit\t_other.cpp', 'tests\stubs\stub.cpp', 'tests\corpus\c\1.txt', 'data\scripts\a.lua', 'tests\smoke\s.txt', 'BattlefrontII.vcxproj', 'tests\smoke.ps1')
    $old = (Get-Date).AddDays(-1)
    $selHashes = @{}
    $i = 0
    foreach ($f in $selFiles) {
        $i++
        $p = Join-Path $selRoot $f
        New-Item -ItemType Directory -Force -Path (Split-Path $p -Parent) | Out-Null
        Set-Content -Path $p -Value 'x'
        (Get-Item -LiteralPath $p).LastWriteTime = $old
        $selHashes[$f] = "hash$i"
    }
    $R = $selRoot.ToUpperInvariant() + '\'
    $testsTlog = Join-Path $selTmp 'tests.tlog'
    Set-Content -Encoding Unicode -Path $testsTlog -Value @("^${R}SRC\G\LIB.CPP", "${R}SRC\G\LIB.H", "${R}SRC\G\SHARED.H", "^${R}TESTS\UNIT\T_LIB.CPP", "${R}SRC\G\LIB.H", "^${R}TESTS\UNIT\T_PIN.CPP", "^${R}TESTS\UNIT\T_OTHER.CPP", "^${R}TESTS\STUBS\STUB.CPP")
    $gameTlog = Join-Path $selTmp 'game.tlog'
    Set-Content -Encoding Unicode -Path $gameTlog -Value @("^${R}SRC\G\GAME.CPP", "${R}SRC\G\GAME.H", "${R}SRC\G\SHARED.H", "^${R}SRC\G\LIB.CPP", "${R}SRC\G\LIB.H", "${R}SRC\G\SHARED.H", "^${R}SRC\X\PCWINDOW.CPP", "^${R}SRC\G\OTHER.CPP")
    $selTests = Read-ClReadTlog $testsTlog $selRoot
    $selGame = Read-ClReadTlog $gameTlog $selRoot
    $selRows = @(
        @{ Id = 'S-LIB'; File = 'src\G\Lib.cpp'; Anchor = 'a'; Replacement = 'b'; Tests = @('lib works') },
        @{ Id = 'S-PIN'; File = 'src\X\pcWindow.cpp'; Anchor = 'a'; Replacement = 'b'; Tests = @('pin text') },
        @{ Id = 'S-HDR'; File = 'src\G\Lib.h'; Anchor = 'a'; Replacement = 'b'; Tests = @('other case') },
        @{ Id = 'S-ASAN'; Asan = $true; File = 'src\G\Lib.cpp'; Anchor = 'a'; Replacement = 'b'; Tests = @('lib works') },
        @{ Id = 'S-CANARY'; Kind = 'AsanCanary'; File = 'src\G\Shared.h'; Anchor = 'a'; Replacement = 'b'; Tests = @('other case'); Report = 'r' },
        @{ Id = 'S-HARD'; Kind = 'Hardening'; Check = 'c'; File = 'src\G\Game.cpp'; Anchor = 'a'; Replacement = 'b' },
        @{ Id = 'S-HARD2'; Kind = 'Hardening'; Check = 'c'; File = 'src\G\Other.cpp'; Anchor = 'a'; Replacement = 'b' },
        @{ Id = 'S-SMOKE'; Kind = 'Smoke'; Case = 'crash-x'; File = 'src\G\Game.h'; Anchor = 'a'; Replacement = 'b' },
        @{ Id = 'S-TLOG'; Kind = 'Tlog'; Project = 'game'; Check = 'f'; File = 'BattlefrontII.vcxproj'; Anchor = 'a'; Replacement = 'b' }
    )
    $selAll = @($selRows | ForEach-Object { $_.Id })
    $selT1 = @('S-LIB', 'S-PIN', 'S-HDR', 'S-ASAN', 'S-CANARY')
    $selBase = @{
        Rows = $selRows; Proofs = @{}; Hashes = $selHashes; TestsReads = $selTests; AsanReads = $selTests; GameReads = $selGame
        TestsProjectGameTUs = @('src\G\Lib.cpp')
        TestSources = @{ 'lib works' = 'tests\unit\t_lib.cpp'; 'pin text' = 'tests\unit\t_pin.cpp'; 'other case' = 'tests\unit\t_other.cpp' }
        TestLiterals = @{ 'tests\unit\t_pin.cpp' = @('tests\corpus\c\') }
        Toolset = '14.44.35207'
    }
    Test-SelCase 'missing record selects every row' @{} $selAll 'no record'
    $selProofs = @{}
    foreach ($x in (Invoke-SelCase @{}).Values) {
        $selProofs[$x.Id] = @{ RowHash = $x.RowHash; DepDigest = $x.DepDigest; TriggerDigest = $x.TriggerDigest; Toolset = '14.44.35207'; Commit = 'c0ffee1'; Date = '2026-11-04' }
    }
    $selBase.Proofs = $selProofs
    Test-SelCase 'unchanged inputs carry every row with its proof commit' @{} @() $null
    $p2 = @{}
    foreach ($k in $selProofs.Keys) { if ($k -ne 'S-HARD2') { $p2[$k] = $selProofs[$k] } }
    Test-SelCase 'one missing record selects only that row' @{ Proofs = $p2 } @('S-HARD2') 'no record'
    Test-SelCase '.cpp touched selects its rows and the game read sets that contain it' @{ Hashes = (Copy-SelHashes 'src\G\Game.cpp' 'new') } @('S-HARD', 'S-SMOKE') 'deps changed'
    Test-SelCase 'header touched selects every read set that contains it' @{ Hashes = (Copy-SelHashes 'src\G\Shared.h' 'new') } @('S-LIB', 'S-PIN', 'S-HDR', 'S-ASAN', 'S-CANARY', 'S-HARD', 'S-SMOKE') 'deps changed'
    Test-SelCase 'test file touched selects only its cases rows' @{ Hashes = (Copy-SelHashes 'tests\unit\t_pin.cpp' 'new') } @('S-PIN') 'deps changed'
    Test-SelCase 'directory literal file touched selects its test rows' @{ Hashes = (Copy-SelHashes 'tests\corpus\c\1.txt' 'new') } @('S-PIN') 'deps changed'
    $r2 = @($selRows | ForEach-Object { $_.Clone() })
    ($r2 | Where-Object { $_.Id -eq 'S-HARD2' }).Anchor = 'z'
    Test-SelCase 'row edited selects only that row' @{ Rows = $r2 } @('S-HARD2') 'row changed'
    Test-SelCase 'vcxproj touched selects every row' @{ Hashes = (Copy-SelHashes 'BattlefrontII.vcxproj' 'new') } $selAll 'global trigger*'
    Test-SelCase 'smoke.ps1 touched selects every row' @{ Hashes = (Copy-SelHashes 'tests\smoke.ps1' 'new') } $selAll 'global trigger*'
    Test-SelCase 'stubs touched selects every row' @{ Hashes = (Copy-SelHashes 'tests\stubs\stub.cpp' 'new') } $selAll 'global trigger*'
    Test-SelCase 'toolset changed selects every row' @{ Toolset = '14.50.1' } $selAll 'toolset'
    Test-SelCase 'no tlog selects every row that needs one' @{ TestsReads = $null; AsanReads = $null; GameReads = $null } @('S-LIB', 'S-PIN', 'S-HDR', 'S-ASAN', 'S-CANARY', 'S-HARD', 'S-HARD2', 'S-SMOKE') 'no tlog*'
    Test-SelCase 'unmapped path selects the rows that read it' @{ Hashes = (Copy-SelHashes 'src\G\Game.h' $null) } @('S-HARD', 'S-SMOKE') 'unmapped *'
    Test-SelCase 'unmapped literal selects its test rows' @{ TestLiterals = @{ 'tests\unit\t_pin.cpp' = @('tests\corpus\c\', 'tests\missing\x.txt') } } @('S-PIN') 'unmapped literal*'
    Test-SelCase 'unknown test name selects its rows' @{ TestSources = @{ 'lib works' = 'tests\unit\t_lib.cpp'; 'pin text' = 'tests\unit\t_pin.cpp' } } @('S-HDR', 'S-CANARY') 'unknown test*'
    Test-SelCase 'text-pin row left alone by an unrelated change' @{ Hashes = (Copy-SelHashes 'src\G\Other.cpp' 'new') } @('S-HARD2', 'S-SMOKE') 'deps changed'
    Test-SelCase 'smoke row depends on every game TU it links with' @{ Hashes = (Copy-SelHashes 'src\X\pcWindow.cpp' 'new') } @('S-PIN', 'S-SMOKE') 'deps changed'
    $p = Join-Path $selRoot 'src\G\Lib.h'
    (Get-Item -LiteralPath $p).LastWriteTime = (Get-Date).AddHours(1)
    $stale = Read-ClReadTlog $testsTlog $selRoot
    Test-SelCase 'stale tlog selects every row that needs the stale TU' @{ TestsReads = $stale; AsanReads = $stale } $selT1 'stale tlog*'
}
catch { $failed++; Write-Host "FAILED: selector block threw: $_" }
finally {
    Remove-Item -LiteralPath $selTmp -Recurse -Force -ErrorAction SilentlyContinue
}
if ($failed -gt 0) { exit 1 }
exit 0
