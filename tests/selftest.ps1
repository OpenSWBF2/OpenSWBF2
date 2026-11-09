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
        @('asan canary glyph memory use after free is reported', 'use-after-poison[\s\S]*Startup_RedInitHeap'),
        @('asan canary mimalloc heap slack overflow is reported', 'use-after-poison[\s\S]*asan_canary\.cpp'),
        @('asan canary mimalloc heap prefix underrun is reported', 'use-after-poison[\s\S]*asan_canary\.cpp'),
        @('asan canary mimalloc heap use after destroy is reported', 'use-after-poison[\s\S]*asan_canary\.cpp')
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
    $exclCfg = $script:SmokeDigestConfiguration
    $script:SmokeDigestConfiguration = 'Test'
    $allNames += @(Get-SmokeCases 'Standard' @() $false $null | ForEach-Object { $_.Name })
    $script:SmokeDigestConfiguration = $exclCfg
    $excl = @($script:SmokeAsanExcluded)
    $missing = @($excl | Where-Object { $allNames -notcontains $_ })
    if ($excl.Count -ne 17 -or $missing.Count -gt 0) { $failed++; Write-Host "FAILED ${n}: asan exclusions: count $($excl.Count), unknown '$($missing -join ', ')'" }
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
$serialDir = Join-Path $env:TEMP ("openswbf2-serial-" + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path (Join-Path $serialDir 'src\LuaFL') | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $serialDir 'tests') | Out-Null
    Set-Content -Encoding ASCII -Path (Join-Path $serialDir 'src\x.cpp') -Value @(
        'static int sThreadActivationKey = 0;',
        'int a = sThreadActivationKey;',
        'unsigned b = pObj->mCollisionSerial;',
        'x = sCollisionSerialNext + 1;',
        'k = NextThreadActivationKey();',
        'int mCollisionSerialTies = 0;',
        'q = pA->mCollisionSerial == pB->mCollisionSerial;'
    )
    Set-Content -Encoding ASCII -Path (Join-Path $serialDir 'src\LuaFL\l.cpp') -Value @(
        'h = sCollisionSerialNext;'
    )
    $allow = @('src\x.cpp|static int sThreadActivationKey = 0;', 'src\x.cpp|int stale = sThreadActivationKey;')
    $sc = @(Get-SourceChecks $serialDir @() @() @() $allow | Where-Object { $_.Name -eq 'serial-value' })
    $want = ((@('src\LuaFL\l.cpp:1') + (@(2, 3, 4, 7) | ForEach-Object { "src\x.cpp:$_" })) + 'unused allow src\x.cpp|int stale = sThreadActivationKey;') -join ', '
    if ($sc.Count -ne 1 -or $sc[0].Ok -or $sc[0].Detail -ne $want) { $failed++; Write-Host "FAILED ${n}: serial-value flags every serial read outside the exact allow-list: $($sc.Detail)" }
    else { Write-Host "OK $n serial-value flags every serial read outside the exact allow-list" }
}
finally {
    Remove-Item -Recurse -Force $serialDir -ErrorAction SilentlyContinue
}
$n++
$handleDir = Join-Path $env:TEMP ("openswbf2-handle-" + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path (Join-Path $handleDir 'src\LuaFL') | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $handleDir 'src\PebbleFL\Common') | Out-Null
    Set-Content -Encoding ASCII -Path (Join-Path $handleDir 'src\x.cpp') -Value @(
        'int a = PBL_HANDLE_ID(p);',
        'return a ? b - 1 : 0xdddddddd;',
        'h.mSavedHandleId = 0;',
        'return (float)(pOwner->PblHandled::GetHandleId() & 0xf);',
        'if ((unsigned int)pOld != 0xDDDDDDDDu)',
        'if (object->GetHandleId() == (unsigned int)debugId)',
        'if (t->GetTracker() != (Tracker*)0xdddddddd)',
        'if (state->handle.GetHandleId() == netHostTraceHandleId)',
        'if (ptr->GetHandleId() != id)',
        'h = p;',
        'ok = d.mGameObject->GetHandleId() == d.mGameObjectId;',
        'x = GameObjectHandleId(p);',
        'if (id != ptr->GetHandleId())',
        'if (p->GetHandleId() == uSaved)',
        'if (p->GetHandleId() != h.GetHandleId())',
        'if (p->GetHandleId() == saved.id_)',
        'if (o->GetGameObject() && o->GetGameObject()->PblHandled::GetHandleId())',
        'p = reinterpret_cast<Tracker*>(0xdddddddd);',
        'if (static_cast<unsigned int>(v) == 0xDDDDDDDD)',
        'if (v != 0xDDDDDDDDUL)',
        'if (p->GetHandleId())',
        'if (p->GetHandleId() == 0)',
        'if (object->GetHandleId() == 110)',
        'if (!mSkip && !(PblHandled::GetHandleId() & 1) == even)',
        'if (GetIndex(i, 1, s->PblHandled::GetHandleId()) && ok)',
        'if ((int)v.GetHandleId() % 30 != b)',
        'if (pObject->GetHandleId() == (unsigned int)debugWriteHandleId)',
        'if (!map.FindId(pObject->GetHandleId()))'
    )
    Set-Content -Encoding ASCII -Path (Join-Path $handleDir 'src\PebbleFL\Common\PblHandle.h') -Value @(
        'unsigned int mHandleId;'
    )
    Set-Content -Encoding ASCII -Path (Join-Path $handleDir 'src\LuaFL\l.cpp') -Value @(
        'int a = PBL_HANDLE_ID(p);'
    )
    $sc = @(Get-SourceChecks $handleDir -Only 'handle-raw')
    $want = (@(1, 3, 5, 7, 9, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22) | ForEach-Object { "src\x.cpp:$_" }) -join ', '
    if ($sc.Count -ne 1 -or $sc[0].Ok -or $sc[0].Detail -ne $want) { $failed++; Write-Host "FAILED ${n}: handle-raw flags raw handle reads and freed-fill compares outside PblHandle: $($sc.Detail)" }
    else { Write-Host "OK $n handle-raw flags raw handle reads and freed-fill compares outside PblHandle" }
}
finally {
    Remove-Item -Recurse -Force $handleDir -ErrorAction SilentlyContinue
}
$miGood = '/c /ISRC\MIMALLOC\INCLUDE /D MI_DEBUG=2 /D MI_SECURE=0 /D MI_NO_GETENV=1 /D MI_USE_RTLGENRANDOM /GS C:\X\SRC\MIMALLOC\SRC\STATIC.C'
$miRelease = $miGood.Replace('MI_DEBUG=2 /D MI_SECURE=0', 'MI_DEBUG=0 /D MI_SECURE=4')
$miScenarios = @(
    @('mimalloc-defines good static.c line passes', $miGood, 'Debug', $true, $true),
    @('mimalloc-defines quoted asan line passes', '/c /fsanitize=address /D "MI_DEBUG=0" /D "MI_SECURE=0" /D "MI_TRACK_ASAN=1" /D "MI_NO_GETENV=1" /D "MI_USE_RTLGENRANDOM" C:\X\SRC\MIMALLOC\SRC\STATIC.C', 'Asan', $true, $true),
    @('mimalloc-defines missing MI_NO_GETENV fails', $miGood.Replace(' /D MI_NO_GETENV=1', ''), 'Debug', $false, $true),
    @('mimalloc-defines asan without MI_TRACK_ASAN fails', $miGood.Replace('/c ', '/c /fsanitize=address '), 'Debug', $false, $true),
    @('mimalloc-defines MI_TRACK_ASAN without asan fails', $miGood.Replace('/c ', '/c /D MI_TRACK_ASAN=1 '), 'Debug', $false, $true),
    @('mimalloc-defines release line passes', $miRelease, 'Release', $true, $true),
    @('mimalloc-defines release with MI_SECURE=0 fails', $miRelease.Replace('MI_SECURE=4', 'MI_SECURE=0'), 'Release', $false, $true),
    @('mimalloc-defines debug values under release fail', $miGood, 'Release', $false, $true),
    @('mimalloc-defines missing tests tlog fails', $miGood, 'Debug', $false, $false)
)
foreach ($sc in $miScenarios) {
    $n++
    $miDir = Join-Path $env:TEMP ("openswbf2-midef-" + [guid]::NewGuid().ToString('N'))
    try {
        New-Item -ItemType Directory -Path $miDir | Out-Null
        Set-Content -Encoding Unicode -Path (Join-Path $miDir 'CL.command.1.tlog') -Value @(
            '^C:\X\A.CPP',
            '/c /Zc:alignedNew- /Zc:inline /sdl /GS C:\X\A.CPP',
            '^C:\X\SRC\MIMALLOC\SRC\STATIC.C',
            $sc[1]
        )
        $bag = Get-ClTlogCounts $miDir
        $testsBag = $null
        if ($sc[4]) { $testsBag = $bag }
        $got = @(Get-FlagChecks $bag $testsBag $sc[2] $sc[2] | Where-Object { $_.Name -eq 'mimalloc-defines' })
        if ($got.Count -ne 1 -or $got[0].Ok -ne $sc[3]) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $($got.Detail)" }
        else { Write-Host "OK $n $($sc[0])" }
    }
    finally {
        Remove-Item -Recurse -Force $miDir -ErrorAction SilentlyContinue
    }
}
$n++
$miApiDir = Join-Path $env:TEMP ("openswbf2-miapi-" + [guid]::NewGuid().ToString('N'))
try {
    foreach ($d in 'src\Foo', 'src\mimalloc\src', 'src\RedEngineFL\Memory') { New-Item -ItemType Directory -Path (Join-Path $miApiDir $d) | Out-Null }
    Set-Content -Encoding ASCII -Path (Join-Path $miApiDir 'src\Foo\a.cpp') -Value @('void* p = mi_malloc(1);', 'mi_heap_t* h = mi_heap_new();', 'void* q = mi_heap_malloc (h, 1);', 'mi_free(q);', 'int v = MI_VERSION;', 'int mi_count = 0;', 'mi_option_set(mi_option_verbose, 1);')
    Set-Content -Encoding ASCII -Path (Join-Path $miApiDir 'src\mimalloc\src\x.c') -Value @('void* p = mi_malloc(1);')
    Set-Content -Encoding ASCII -Path (Join-Path $miApiDir 'src\RedEngineFL\Memory\RedMemoryMi.cpp') -Value @('void* p = mi_malloc(1);')
    $bad = @(Get-SourceChecks $miApiDir -Only 'mi-api-ban')
    Remove-Item -LiteralPath (Join-Path $miApiDir 'src\Foo\a.cpp')
    $good = @(Get-SourceChecks $miApiDir -Only 'mi-api-ban')
    if ($bad.Count -ne 1 -or $bad[0].Ok -or $bad[0].Detail -ne 'src\Foo\a.cpp:1, src\Foo\a.cpp:2, src\Foo\a.cpp:3, src\Foo\a.cpp:4, src\Foo\a.cpp:7') { $failed++; Write-Host "FAILED ${n}: mi-api-ban flags mimalloc calls outside src\mimalloc and RedMemoryMi.cpp: $($bad.Detail)" }
    elseif ($good.Count -ne 1 -or -not $good[0].Ok) { $failed++; Write-Host "FAILED ${n}: mi-api-ban flags mimalloc calls outside src\mimalloc and RedMemoryMi.cpp: allowed files flagged $($good.Detail)" }
    else { Write-Host "OK $n mi-api-ban flags mimalloc calls outside src\mimalloc and RedMemoryMi.cpp" }
}
finally {
    Remove-Item -Recurse -Force $miApiDir -ErrorAction SilentlyContinue
}
$n++
$impGood = "Dump of file C:\X\BattlefrontII.exe`n`n  Section contains the following imports:`n`n    KERNEL32.dll`n              1000 Import Address Table`n                 1A0 GetTickCount`n`n    api-ms-win-crt-environment-l1-1-0.dll`n                   0 getenv`n"
$impBad = $impGood + "`n    bcrypt.dll`n                   0 BCryptGenRandom`n"
$okGood = @(Get-ImportChecks $impGood | Where-Object { $_.Name -eq 'import-modules' })
$okBad = @(Get-ImportChecks $impBad | Where-Object { $_.Name -eq 'import-modules' })
$okEmpty = @(Get-ImportChecks '' | Where-Object { $_.Name -eq 'import-modules' })
if ($okGood.Count -ne 1 -or -not $okGood[0].Ok -or $okBad[0].Ok -or $okBad[0].Detail -notmatch 'bcrypt\.dll' -or $okEmpty[0].Ok -or $okEmpty[0].Detail -notmatch 'kernel32\.dll not found') { $failed++; Write-Host "FAILED ${n}: import-modules allows only listed DLLs: good=$($okGood.Ok) bad=$($okBad.Ok) empty=$($okEmpty.Ok)" }
else { Write-Host "OK $n import-modules allows only listed DLLs" }
$n++
$allCases = @(Get-SmokeCases 'All' @('foo1c_ctf TimeOut', 'tat3c_con Fatal', 'bar1c_con', 'quux1c_con Pass', 'foo2c_con Pass 240', 'foo7c_ctf Fatal', 'foo5c_ass TimeOut', 'dag1c_ctf Pass', 'spa3c_ass Pass') $false $null)
$byName = @{}
foreach ($c in $allCases) { $byName[$c.Name] = $c }
$rejected = 0
foreach ($badLine in 'baz1c_con Bogus', 'qux1c_con GsFailure', 'foo3c_ctf Pass abc', 'foo4c_1flag Pass', 'foo6c_ass') {
    try { Get-SmokeCases 'All' @($badLine) $false $null | Out-Null } catch { $rejected++ }
}
$why = $null
$nvRx = '(?m)OPENSWBF2 NOVICTORY\r?$'
$nvLog = "[t=0.000] [f=0] LogCore Inform pcMain.cpp:1: OPENSWBF2 NOVICTORY`r`n"
if ($byName['foo1c_ctf'].Expect.Class -ne 'TimeOut' -or $byName['foo1c_ctf'].Timeout -ne 0 -or (@($byName['foo1c_ctf'].Expect.LogRx) -join '|') -ne ('OPENSWBF2 MISSION RUNNING|' + $nvRx)) { $why = 'TimeOut line' }
elseif ($byName['foo5c_ass'].Expect.Class -ne 'TimeOut' -or $byName['foo5c_ass'].Timeout -ne 0 -or (@($byName['foo5c_ass'].Expect.LogRx) -join '|') -ne ('OPENSWBF2 MISSION RUNNING|' + $nvRx)) { $why = 'assault TimeOut line' }
elseif (-not $byName['foo1c_ctf'].Args.EndsWith(' -novictory') -or -not $byName['foo5c_ass'].Args.EndsWith(' -novictory')) { $why = 'flag and assault cases must run with -novictory' }
elseif ($byName['foo7c_ctf'].Expect.Class -ne 'Fatal' -or $byName['foo7c_ctf'].Args.Contains('-novictory') -or @($byName['foo7c_ctf'].Expect.LogRx) -contains $nvRx) { $why = 'only TimeOut flag and assault cases get -novictory' }
elseif ($nvLog -cnotmatch $nvRx -or $nvLog.Replace('NOVICTORY', 'NOVICTORYX') -cmatch $nvRx -or $nvLog.Replace('NOVICTORY', 'NOVICTORY on') -cmatch $nvRx) { $why = 'NOVICTORY rx must pin the whole line' }
elseif ($byName['bar1c_con'].Args.Contains('-novictory') -or @($byName['bar1c_con'].Expect.LogRx) -contains $nvRx) { $why = 'con cases must not get -novictory' }
elseif ($byName['dag1c_ctf'].Args.Contains('-novictory') -or @($byName['dag1c_ctf'].Expect.LogRx) -contains $nvRx -or $byName['spa3c_ass'].Args.Contains('-novictory') -or @($byName['spa3c_ass'].Expect.LogRx) -contains $nvRx) { $why = 'digest cases must not get -novictory' }
elseif (@(Get-SmokeCases 'All' @('quux1c_con Pass') $false $null)[0].Expect.LogRx) { $why = 'flag case leaked its LogRx into the shared Pass expectation' }
elseif ($byName['tat3c_con'].Expect.Class -ne 'Fatal' -or $byName['tat3c_con'].Timeout -ne 0) { $why = 'Fatal line' }
elseif (-not $byName['tat3c_con'].Expect.LogRx -or $byName['tat3c_con'].Expect.Code -ne 3 -or $byName['tat3c_con'].Expect.MaxDumps -ne 1) { $why = 'Fatal line must pin the d3dx9 signature' }
elseif ('FATAL: exception 0xC0000005 at 0x658FA553 (d3dx9_43.dll+0x1BA553) build 6ADE5B7C1C4C000' -notmatch $byName['tat3c_con'].Expect.LogRx) { $why = 'Fatal LogRx does not match the recorded tat3 crash' }
elseif ($byName['bar1c_con'].Expect.Class -ne 'Pass' -or $byName['quux1c_con'].Expect.Class -ne 'Pass') { $why = 'missing or Pass class must be Pass' }
elseif ($byName['foo2c_con'].Expect.Class -ne 'Pass' -or $byName['foo2c_con'].Timeout -ne 240 -or $byName['bar1c_con'].Timeout -ne 0 -or $byName['quux1c_con'].Timeout -ne 0) { $why = 'third token must set the case timeout' }
elseif ($rejected -ne 5) { $why = 'unknown classes, GsFailure, non-integer timeouts and Pass flag or assault lines without a digest row must be rejected' }
elseif (@($allCases | Where-Object { $_.Name -like 'crash-*' }).Count -ne 42 -or -not $byName.ContainsKey('fatal-init') -or $byName['dedicated'].Timeout -ne 0) { $why = 'dedicated or crash cases changed' }
if ($why) { $failed++; Write-Host "FAILED ${n}: all-missions parsing: $why" }
else { Write-Host "OK $n all-missions parsing" }
$n++
$hrx = $script:SmokeAddrSpaceRx
$okLine = 'OPENSWBF2 ADDRSPACE arena=0x82000000 arena_end=0x86000000 laa=1 steam=1 steam_api=0x65740000 Galaxy=0x652E0000 GalaxyPeer=0x00000000 binkw32=0x18000000'
$badLines = @($okLine.Replace('arena=0x82000000', 'arena=0x02000000'), $okLine.Replace('laa=1', 'laa=0'), $okLine.Replace('steam=1', 'steam=0'), $okLine.Replace('Galaxy=0x652E0000', 'Galaxy=0x00000000'), $okLine.Replace('binkw32=0x18000000', 'binkw32=0x00000000'), $okLine.Replace('steam_api=0x65740000', 'steam_api=0x00000000'))
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
    $repLog = Join-Path $digTmp 'run.log'
    $repCase = New-Object PSObject -Property @{ Name = 'selftest-repeat'; Expect = @{ Class = 'Pass'; SameDigests = 2 } }
    $repRes = New-Object PSObject -Property @{ Class = 'Pass'; CaseDir = $digTmp; LogText = ''; Dumps = @() }
    $digOther = $digLine.Replace('state=0x0000ABCD', 'state=0x0000ABCE')
    [IO.File]::WriteAllText($repLog, "[t=1.000] [f=1] $digLine`r`n[t=2.000] [f=2] $digLine`r`n")
    $repSame = Test-SmokeExpect $repCase $repRes
    [IO.File]::WriteAllText($repLog, "[t=1.000] [f=1] $digLine`r`n[t=2.000] [f=2] $digOther`r`n")
    $repDiff = Test-SmokeExpect $repCase $repRes
    [IO.File]::WriteAllText($repLog, "[t=1.000] [f=1] $digLine`r`n")
    $repOne = Test-SmokeExpect $repCase $repRes
    $repCfg = $script:SmokeDigestConfiguration
    $script:SmokeDigestConfiguration = 'Test'
    $repStd = @(Get-SmokeCases 'Standard' @() $false $null | Where-Object { $_.Name -eq 'repeat-digest' })
    $script:SmokeDigestConfiguration = $repCfg
    $repWhy = $null
    if (-not $repSame -or $repDiff -or $repOne) { $repWhy = "same=$repSame differ=$repDiff one=$repOne" }
    elseif ($repStd.Count -ne 1 -or $repStd[0].Expect.SameDigests -ne 2) { $repWhy = 'repeat-digest does not require two equal digests' }
    if ($repWhy) { $failed++; Write-Host "FAILED ${n}: repeat digest compare: $repWhy" }
    else { Write-Host "OK $n repeat digest compare fails on two different digests" }
    [IO.File]::WriteAllText($repLog, "$digLine`r`n")
    $n++
    $cfLine = { param($s, $v) "[t=1.000] [f=1] LogMemory Inform GameState.cpp:1: OPENSWBF2 COMMIT state=$s bytes=$v`r`n" }
    $cfPass = (& $cfLine mission 100000000) + (& $cfLine shell 60000000) + (& $cfLine mission 101000000) + (& $cfLine shell 60500000) + (& $cfLine mission 102048576)
    $cfHigh = (& $cfLine mission 100000000) + (& $cfLine shell 60000000) + (& $cfLine mission 101000000) + (& $cfLine shell 60500000) + (& $cfLine mission 102048577)
    $cfFour = (& $cfLine mission 100000000) + (& $cfLine shell 60000000) + (& $cfLine mission 101000000) + (& $cfLine shell 60500000)
    $cfP = Test-CommitFlat $cfPass 1048576
    $cfH = Test-CommitFlat $cfHigh 1048576
    $cfF = Test-CommitFlat $cfFour 1048576
    $cfCfg = $script:SmokeDigestConfiguration
    $script:SmokeDigestConfiguration = 'Test'
    $cfCase = @(Get-SmokeCases 'Standard' @() $false $null | Where-Object { $_.Name -eq 'commit-flat' })
    $script:SmokeDigestConfiguration = $cfCfg
    $cfWhy = $null
    if ($cfP -or -not $cfH -or -not $cfF) { $cfWhy = "pass='$cfP' high='$cfH' four='$cfF'" }
    elseif ($cfCase.Count -ne 1 -or $cfCase[0].Expect.CommitFlat -ne 1048576 -or $cfCase[0].Expect.SameDigests -ne 3 -or $cfCase[0].Args -notmatch ' -repeatmission 2$') { $cfWhy = 'commit-flat case missing or wrong' }
    if ($cfWhy) { $failed++; Write-Host "FAILED ${n}: commit flat: $cfWhy" }
    else { Write-Host "OK $n commit flat bounds m3-m2 and s2-s1 and needs five states" }
    $n++
    $memA = "[t=1.000] [f=1] LogMemory Inform x.cpp:1: OPENSWBF2 MEMREPORT source=exit`r`n[t=1.000] [f=1] LogMemory Inform x.cpp:1: OPENSWBF2 MEMREPORT heap=Runtime cap=9 peak=5 used=4 allocs=3 failed=0`r`n"
    $memB = "[t=2.000] [f=2] LogMemory Inform x.cpp:1: OPENSWBF2 MEMREPORT source=turn1500`r`n[t=2.000] [f=2] LogMemory Inform x.cpp:1: OPENSWBF2 MEMREPORT heap=Lua cap=9 peak=8 used=7 allocs=6 failed=0`r`n[t=2.000] [f=2] LogMemory Inform x.cpp:1: OPENSWBF2 MEMREPORT heap=Runtime cap=9 peak=14 used=13 allocs=12 failed=0`r`n"
    [IO.File]::WriteAllText($repLog, $memA + $memB + $memA + $memB.Replace('peak=14 used=13', 'peak=24 used=23'))
    $memGot = @(Get-Turn1500Runtime $digTmp)
    if (($memGot -join ',') -cne 'peak=14 used=13,peak=24 used=23') { $failed++; Write-Host "FAILED ${n}: turn 1500 Runtime report: $($memGot -join ',')" }
    else { Write-Host "OK $n turn 1500 Runtime report reads only the turn1500 Runtime lines" }
    [IO.File]::WriteAllText($repLog, "$digLine`r`n")
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
    $pvEq = Get-PerturbVerdict $trA $trA 'Pass' 'Pass' 9
    $pvSp = Get-PerturbVerdict $trA $trB 'Pass' 'Pass' 9
    $pvIn = Get-PerturbVerdict $trA $trE 'Pass' 'Pass' 9
    $pvCr = Get-PerturbVerdict $trA $trB 'Crash' 'Pass' 9
    $trA2 = Join-Path $digTmp 'trace-a2.log'
    $trB2 = Join-Path $digTmp 'trace-b2.log'
    [IO.File]::WriteAllLines($trA2, [string[]](1..1520 | ForEach-Object { 'OPENSWBF2 DIGEST TRACE turn={0} state=0x{0:X8} rand=1/2' -f $_ }))
    [IO.File]::WriteAllLines($trB2, [string[]](1..1510 | ForEach-Object { 'OPENSWBF2 DIGEST TRACE turn={0} state=0x{0:X8} rand=1/2' -f $_ }))
    $cmpLim = Compare-DigestTrace $trA2 $trB2 1500
    $cmpAll = Compare-DigestTrace $trA2 $trB2
    $pvLim = Get-PerturbVerdict $trA2 $trB2 'Pass' 'Pass'
    $pvBad = @()
    if ($pvEq.Result -ne 'equal') { $pvBad += "equal=$($pvEq.Result)" }
    if ($pvSp.Result -ne 'split' -or $pvSp.Turn -ne 7) { $pvBad += "split=$($pvSp.Result)/$($pvSp.Turn)" }
    if ($pvIn.Result -ne 'crash' -or $pvIn.Class -ne 'incomplete') { $pvBad += "incomplete=$($pvIn.Result)/$($pvIn.Class)" }
    if ($pvCr.Result -ne 'crash' -or $pvCr.Class -ne 'Crash' -or $pvCr.Turn -ne 7) { $pvBad += "crash=$($pvCr.Result)/$($pvCr.Class)/$($pvCr.Turn)" }
    if ($cmpLim -ne 0 -or $cmpAll -ne -1 -or $pvLim.Result -ne 'equal') { $pvBad += "limit=$cmpLim/$cmpAll/$($pvLim.Result)" }
    if ($pvBad.Count) { $failed++; Write-Host "FAILED ${n}: perturb verdict: $($pvBad -join ' ')" }
    else { Write-Host "OK $n perturb verdict names equal, split turn, incomplete and crash" }
    $n++
    $fullBad = @()
    $trF = Join-Path $digTmp 'trace-f.log'
    [IO.File]::WriteAllLines($trF, [string[]]($linesA | ForEach-Object { if ($_ -match 'turn=7 ') { $_.Replace('rand=1/2', 'rand=5/2') } else { $_ } }))
    $mA = Get-DigestTrace $trA
    $mF = Get-DigestTrace $trF
    $fRand = Compare-DigestTraceFull $mA $mF
    $fRandOld = Compare-DigestTrace $trA $trF
    if ($fRand -ne 7 -or $fRandOld -ne 0) { $fullBad += "rand=$fRand/$fRandOld" }
    $mL = @{}
    $mM = @{}
    foreach ($t in 1..1600) {
        $mL[$t] = 'state=0x{0:X8} rand=1/2' -f $t
        $mM[$t] = $mL[$t]
    }
    $mM[1550] = 'state=0x00000001 rand=1/2'
    $fLate = Compare-DigestTraceFull $mL $mM
    $mShort = @{}
    foreach ($t in 1..1599) { $mShort[$t] = $mL[$t] }
    $fMiss = Compare-DigestTraceFull $mL $mShort
    $fEmpty = Compare-DigestTraceFull $mL @{}
    $fSame = Compare-DigestTraceFull $mL $mL
    if ($fLate -ne 1550 -or $fMiss -ne -1 -or $fEmpty -ne -1 -or $fSame -ne 0) { $fullBad += "late=$fLate miss=$fMiss empty=$fEmpty same=$fSame" }
    $trLine = { param($t, $r) "[t=1.000] [f=1] LogSim Inform SimDigest.cpp:164: OPENSWBF2 DIGEST TRACE turn=$t state=0x{0:X8} rand=$r/2" -f $t }
    $simLine = '[t=2.000] [f=2] LogSim Inform GameLoop.cpp:1523: OPENSWBF2 SIM DIGEST turns=15 time=1.000 rand=1/2 snd=1.000 state=0x00000042'
    $toMission = '[t=3.000] [f=3] LogSim Inform GameState.cpp:1347: OPENSWBF2 REPEAT to_mission mission=geo1c_con'
    $repBase = Join-Path $digTmp 'repeat-base.log'
    [IO.File]::WriteAllLines($repBase, [string[]](@(1..20 | ForEach-Object { & $trLine $_ 1 }) + $simLine))
    $repEq = Join-Path $digTmp 'repeat-eq.log'
    $seg1 = @(1..15 | ForEach-Object { & $trLine $_ 1 }) + $simLine + $toMission
    [IO.File]::WriteAllLines($repEq, [string[]]($seg1 + @(1..20 | ForEach-Object { & $trLine $_ 1 }) + $simLine))
    $repSp = Join-Path $digTmp 'repeat-split.log'
    [IO.File]::WriteAllLines($repSp, [string[]]($seg1 + @(1..20 | ForEach-Object { if ($_ -eq 18) { & $trLine $_ 3 } else { & $trLine $_ 1 } }) + $simLine))
    $repOne = Join-Path $digTmp 'repeat-one.log'
    [IO.File]::WriteAllLines($repOne, [string[]]($seg1[0..15]))
    $repSegs = @(Get-DigestTraceSegments $repEq)
    if ($repSegs.Count -ne 2 -or $repSegs[1].Count -ne 20 -or -not $repSegs[1].ContainsKey(1) -or $repSegs[0].Count -ne 15) { $fullBad += "segments=$($repSegs.Count)" }
    if (@(Get-DigestTraceSegments (Join-Path $digTmp 'no-such.log')).Count -ne 0) { $fullBad += 'missing log has segments' }
    $vEq = Get-PerturbVerdict $repBase $repEq 'Pass' 'Pass' 15 'serial' $true
    $vSp = Get-PerturbVerdict $repBase $repSp 'Pass' 'Pass' 15 'serial' $true
    $vOne = Get-PerturbVerdict $repBase $repOne 'Pass' 'Pass' 15 'serial' $true
    $vAddr = Get-PerturbVerdict $repBase $repSp 'Pass' 'Pass' 15 'address' $true
    if ($vEq.Result -ne 'equal') { $fullBad += "repeat equal=$($vEq.Result)/$($vEq.Class)" }
    if ($vSp.Result -ne 'split' -or $vSp.Turn -ne 18) { $fullBad += "repeat split=$($vSp.Result)/$($vSp.Turn)" }
    if ($vOne.Result -ne 'crash' -or $vOne.Class -ne 'incomplete') { $fullBad += "repeat one=$($vOne.Result)/$($vOne.Class)" }
    if ($vAddr.Result -ne 'equal') { $fullBad += "repeat address=$($vAddr.Result)" }
    $serBase = Join-Path $digTmp 'serial-base.log'
    $serRand = Join-Path $digTmp 'serial-rand.log'
    [IO.File]::WriteAllLines($serBase, [string[]](@(1..20 | ForEach-Object { & $trLine $_ 1 }) + $simLine))
    [IO.File]::WriteAllLines($serRand, [string[]](@(1..20 | ForEach-Object { if ($_ -eq 7) { & $trLine $_ 3 } else { & $trLine $_ 1 } }) + $simLine))
    $sSer = Get-PerturbVerdict $serBase $serRand 'Pass' 'Pass' 15 'serial'
    $sAdr = Get-PerturbVerdict $serBase $serRand 'Pass' 'Pass' 15
    $sEq = Get-PerturbVerdict $serBase $serBase 'Pass' 'Pass' 15 'serial'
    $sNoDig = Get-PerturbVerdict $serBase $trA 'Pass' 'Pass' 9 'serial'
    if ($sSer.Result -ne 'split' -or $sSer.Turn -ne 7 -or $sAdr.Result -ne 'equal' -or $sEq.Result -ne 'equal') { $fullBad += "serial=$($sSer.Result)/$($sSer.Turn) address=$($sAdr.Result) same=$($sEq.Result)" }
    if ($sNoDig.Result -ne 'crash' -or $sNoDig.Class -ne 'incomplete') { $fullBad += "serial short=$($sNoDig.Result)/$($sNoDig.Class)" }
    $gv = @(
        (Get-PerturbGroupVerdict 'serial' 0 66 66), (Get-PerturbGroupVerdict 'serial' 1 66 66), (Get-PerturbGroupVerdict 'serial' 0 65 66),
        (Get-PerturbGroupVerdict 'address' 3 66 66), (Get-PerturbGroupVerdict 'address' 0 66 66), (Get-PerturbGroupVerdict 'address' 3 65 66),
        (Get-PerturbGroupVerdict '' 0 66 66), (Get-PerturbGroupVerdict 'Serial' 0 66 66))
    if (($gv -join ',') -cne 'True,False,False,True,False,False,False,False') { $fullBad += "group=$($gv -join ',')" }
    if ($fullBad.Count) { $failed++; Write-Host "FAILED ${n}: perturb full trace: $($fullBad -join '; ')" }
    else { Write-Host "OK $n perturb full trace compares rand and late turns, splits repeat segments and follows the sim order" }
    $n++
    $savedPm = $env:OPENSWBF2_PERTURB_MISSIONS
    $env:OPENSWBF2_PERTURB_MISSIONS = 'geo1c_con'
    $pCases = @{}
    foreach ($c in (Get-SmokeCases 'Perturb' @() $false $null)) { $pCases[$c.Name] = $c }
    $env:OPENSWBF2_PERTURB_MISSIONS = $savedPm
    $addr = 'OPENSWBF2 ADDRSPACE arena=0x82000000 arena_end=0x86000000 laa=1 steam=1 steam_api=0x65740000 Galaxy=0x652E0000 GalaxyPeer=0x00000000 binkw32=0x18000000'
    $pLine = "[t=0.100] [f=0] LogCore Inform pcMain.cpp:1: OPENSWBF2 PERTURB seed=1 modes=pad`r`n"
    $prBad = @()
    if (@(Get-PerturbProofMissing $pCases['perturb-geo1c_con-pad-1'] "x`r`n$pLine").Count -ne 0) { $prBad += 'pad line rejected' }
    if (@(Get-PerturbProofMissing $pCases['perturb-geo1c_con-pad-1'] 'x').Count -eq 0) { $prBad += 'pad without PERTURB line accepted' }
    if (@(Get-PerturbProofMissing $pCases['perturb-geo1c_con-pad-1'] $pLine.Replace('modes=pad', 'modes=pad,top')).Count -eq 0) { $prBad += 'pad accepted modes=pad,top' }
    if (@(Get-PerturbProofMissing $pCases['perturb-geo1c_con-pad-2'] $pLine).Count -eq 0) { $prBad += 'seed 2 accepted seed=1' }
    if (@(Get-PerturbProofMissing $pCases['perturb-geo1c_con-base'] 'x').Count -ne 0) { $prBad += 'plain base rejected' }
    if (@(Get-PerturbProofMissing $pCases['perturb-geo1c_con-base'] $pLine).Count -eq 0) { $prBad += 'base with PERTURB line accepted' }
    if (@(Get-PerturbProofMissing $pCases['perturb-geo1c_con-hog'] $addr).Count -ne 0) { $prBad += 'hog with ADDRSPACE rejected' }
    if (@(Get-PerturbProofMissing $pCases['perturb-geo1c_con-hog'] 'x').Count -eq 0) { $prBad += 'hog without ADDRSPACE accepted' }
    if (@(Get-PerturbProofMissing $pCases['perturb-geo1c_con-hog'] "$addr`r`n$pLine").Count -eq 0) { $prBad += 'hog with PERTURB line accepted' }
    $rShell = "[t=1.000] [f=1] LogSim Inform GameLoop.cpp:1535: OPENSWBF2 REPEAT to_shell mission=geo1c_con`r`n"
    $rMission = "[t=2.000] [f=2] LogSim Inform GameState.cpp:1347: OPENSWBF2 REPEAT to_mission mission=geo1c_con`r`n"
    $rCase = $pCases['perturb-geo1c_con-repeat']
    if (-not $rCase -or $rCase.Args -notmatch ' -repeatmission$' -or $rCase.Timeout -ne 300) { $prBad += 'repeat case missing or wrong' }
    elseif (@(Get-PerturbProofMissing $rCase "$rShell$rMission").Count -ne 0) { $prBad += 'repeat with both REPEAT lines rejected' }
    elseif (@(Get-PerturbProofMissing $rCase $rShell).Count -eq 0) { $prBad += 'repeat without to_mission accepted' }
    elseif (@(Get-PerturbProofMissing $rCase $rMission).Count -eq 0) { $prBad += 'repeat without to_shell accepted' }
    elseif (@(Get-PerturbProofMissing $rCase "$rShell$rMission$pLine").Count -eq 0) { $prBad += 'repeat with PERTURB line accepted' }
    $savedPm = $env:OPENSWBF2_PERTURB_MISSIONS
    $env:OPENSWBF2_PERTURB_MISSIONS = ''
    $allP = @(Get-SmokeCases 'Perturb' @() $false $null)
    $env:OPENSWBF2_PERTURB_MISSIONS = $savedPm
    $nabRep = @($allP | Where-Object { $_.Name -eq 'perturb-nab2g_c-repeat' })
    if ($allP.Count -ne 66 -or @($allP | Where-Object { $_.Expect.Perturb.Mode -eq 'repeat' }).Count -ne 6) { $prBad += "perturb cases=$($allP.Count), need 66 with 6 repeat" }
    if ($nabRep.Count -ne 1 -or $nabRep[0].Timeout -ne 180 -or $nabRep[0].Expect.Class -ne 'TimeOut') { $prBad += 'nab2g_c repeat not TimeOut 180' }
    if ($prBad.Count) { $failed++; Write-Host "FAILED ${n}: perturb proof: $($prBad -join '; ')" }
    else { Write-Host "OK $n perturb runs must prove their mode, seed, hog and repeat" }
    $n++
    $soBad = @()
    $soPlain = @(Get-SmokeCases 'Standard' @() $false $null)
    $soSerial = @(Add-SimOrderExpect @(Get-SmokeCases 'Standard' @() $false $null) 'serial')
    $soCrash = @(Add-SimOrderExpect @(Get-SmokeCases 'Crash' @() $false $null) 'serial')
    $soRx = '(?m)OPENSWBF2 SIMORDER mode=serial\r?$'
    foreach ($c in $soPlain) { if (@($c.Expect.LogRx) -contains $soRx) { $soBad += "$($c.Name) asserts serial without -SimOrder" } }
    foreach ($c in $soSerial) { if (@($c.Expect.LogRx) -notcontains $soRx) { $soBad += "$($c.Name) lacks the serial assertion" } }
    foreach ($c in $soCrash) { if ($c.Expect.Class -notin 'Pass', 'TimeOut' -and @($c.Expect.LogRx) -contains $soRx) { $soBad += "$($c.Name) asserts serial on a crash case" } }
    $soAdded = @($soSerial[0].Expect.LogRx | Where-Object { @($soPlain[0].Expect.LogRx) -notcontains $_ })
    $soGood = "[t=0.000] [f=0] LogCore Inform pcMain.cpp:1: OPENSWBF2 SIMORDER mode=serial`r`n"
    $soWarn = "$soGood[t=0.000] [f=0] LogCore Warning pcMain.cpp:1: -simorder needs address or serial, ignored`r`n"
    $soMiss = { param($text) @($soAdded | Where-Object { $text -cnotmatch $_ }).Count }
    if ($soAdded.Count -ne 2) { $soBad += "added $($soAdded.Count) rx, want 2" }
    elseif ((& $soMiss $soGood) -ne 0) { $soBad += 'serial log rejected' }
    elseif ((& $soMiss $soGood.Replace('mode=serial', 'mode=address')) -eq 0) { $soBad += 'address log accepted under -SimOrder serial' }
    elseif ((& $soMiss $soWarn) -eq 0) { $soBad += 'rejected -simorder warning accepted' }
    if ($soBad.Count) { $failed++; Write-Host "FAILED ${n}: simorder assertion: $($soBad -join '; ')" }
    else { Write-Host "OK $n -SimOrder asserts its SIMORDER line on every non-crash case" }
    $n++
    $alBad = @()
    $alMiRx = 'OPENSWBF2 MIOPTIONS purge_delay=1000 retry_on_oom=0 arena_reserve=67108864 arena_eager_commit=0 reserve_os_memory=0 page_full_retain=-1'
    $alArenaRx = '(?s)\A(?!.*OPENSWBF2 MIOPTIONS)'
    $alCrash = @(Get-SmokeCases 'Crash' @() $false $null)
    $alDef = @(Add-AllocatorExpect @(Get-SmokeCases 'Standard' @() $false $null) '' -1)
    $alZero = @(Add-AllocatorExpect @(Get-SmokeCases 'Standard' @() $false $null) 'mimalloc' 0)
    $alMiCrash = @(Add-AllocatorExpect @(Get-SmokeCases 'Crash' @() $false $null) '' -1)
    $alArena = @(Add-AllocatorExpect @(Get-SmokeCases 'Crash' @() $false $null) 'arena' -1)
    foreach ($c in $alDef) { if (@($c.Expect.LogRx) -notcontains $alMiRx) { $alBad += "$($c.Name) lacks purge_delay=1000" } }
    foreach ($c in $alZero) { if (@($c.Expect.LogRx) -notcontains $alMiRx.Replace('purge_delay=1000', 'purge_delay=0')) { $alBad += "$($c.Name) lacks purge_delay=0" } }
    if (@($alMiCrash | Where-Object { $_.Name -eq 'crash-midoublefree' }).Count -ne 1) { $alBad += 'mimalloc lacks crash-midoublefree' }
    if (@($alArena | Where-Object { $_.Name -eq 'crash-midoublefree' }).Count -ne 0) { $alBad += 'arena keeps crash-midoublefree' }
    foreach ($c in $alArena) { if ($c.Expect.Class -in 'Pass', 'TimeOut' -and @($c.Expect.LogRx) -notcontains $alArenaRx) { $alBad += "$($c.Name) lacks the arena rx" } }
    $alPlain = @{}
    foreach ($c in $alCrash) { $alPlain[$c.Name] = $c }
    foreach ($c in @($alMiCrash + $alArena)) { if ($c.Expect.Class -notin 'Pass', 'TimeOut' -and (@($c.Expect.LogRx) -join "`n") -cne (@($alPlain[$c.Name].Expect.LogRx) -join "`n")) { $alBad += "$($c.Name) fatal expectation changed" } }
    $alOne = @(Add-AllocatorExpect @(New-Object PSObject -Property @{ Name = 'selftest-case'; Expect = @{ Class = 'Pass' } }) '' -1)[0]
    $alLine = "[t=0.000] [f=0] LogCore Inform pcMain.cpp:1: $alMiRx`r`n"
    $alWarn = "$alLine[t=0.000] [f=0] LogCore Warning pcMain.cpp:1: -allocator needs arena or mimalloc, using mimalloc`r`n"
    $alMiss = { param($case, $text) @(@($case.Expect.LogRx) | Where-Object { $text -cnotmatch $_ }).Count }
    if ((& $alMiss $alOne $alLine) -ne 0) { $alBad += 'mimalloc options log rejected' }
    elseif ((& $alMiss $alOne 'x') -eq 0) { $alBad += 'log without MIOPTIONS accepted' }
    elseif ((& $alMiss $alOne $alWarn) -eq 0) { $alBad += 'rejected -allocator warning accepted' }
    if ($alBad.Count) { $failed++; Write-Host "FAILED ${n}: allocator assertion: $($alBad -join '; ')" }
    else { Write-Host "OK $n -Allocator asserts the mimalloc options or their absence and keeps fatal cases" }
    $n++
    $sdBad = @()
    $sdNew = { param($name, $mode) @(Add-SimOrderExpect @(New-Object PSObject -Property @{ Name = $name; Expect = @{ Class = 'Pass' } }) $mode)[0] }
    $sdSerial = & $sdNew 'selftest-case' 'serial'
    $sdAddress = & $sdNew 'selftest-case' 'address'
    $sdPlain = & $sdNew 'selftest-case' ''
    $sdNoRow = & $sdNew 'selftest-norow' 'address'
    $sdHead = "[t=0.000] [f=0] LogCore Inform pcMain.cpp:1: OPENSWBF2 SIMORDER mode=serial`r`n"
    $sdAddrHead = $sdHead.Replace('mode=serial', 'mode=address')
    $sdFlipLine = $digLine.Replace('state=0x0000ABCD', 'state=0x0000ABCC')
    $sdRes = New-Object PSObject -Property @{ Class = 'Pass'; CaseDir = $digTmp; LogText = ''; Dumps = @() }
    $sdRun = {
        param($case, $text)
        [IO.File]::WriteAllText((Join-Path $digTmp 'run.log'), $text)
        $sdRes.LogText = $text
        Test-SmokeExpect $case $sdRes
    }
    $env:OPENSWBF2_DIGEST_EXPECTED = $flipExp
    if (-not $sdAddress.Expect.ContainsKey('NoDigestRow')) { $sdBad += 'address case lacks NoDigestRow' }
    if ($sdSerial.Expect.ContainsKey('NoDigestRow') -or $sdPlain.Expect.ContainsKey('NoDigestRow')) { $sdBad += 'NoDigestRow set outside address' }
    if (& $sdRun $sdSerial "$sdHead[t=1.000] [f=1] $digLine`r`n") { $sdBad += 'serial digest mismatch accepted' }
    if (-not (& $sdRun $sdSerial "$sdHead[t=1.000] [f=1] $sdFlipLine`r`n")) { $sdBad += 'serial digest equal to the row rejected' }
    if (-not (& $sdRun $sdAddress "$sdAddrHead[t=1.000] [f=1] $digLine`r`n")) { $sdBad += 'address digest mismatch rejected' }
    if (& $sdRun $sdAddress $sdAddrHead) { $sdBad += 'address case without a digest line accepted' }
    if (& $sdRun $sdPlain "[t=1.000] [f=1] $digLine`r`n") { $sdBad += 'plain digest mismatch accepted' }
    if (-not (& $sdRun $sdPlain "[t=1.000] [f=1] $sdFlipLine`r`n")) { $sdBad += 'plain digest equal to the row rejected' }
    if (-not (& $sdRun $sdNoRow $sdAddrHead)) { $sdBad += 'address case without a digest row needs a digest line' }
    $env:OPENSWBF2_DIGEST_EXPECTED = $savedExpected
    if ($sdBad.Count) { $failed++; Write-Host "FAILED ${n}: digest row expectation: $($sdBad -join '; ')" }
    else { Write-Host "OK $n plain and -SimOrder serial compare the digest row, -SimOrder address checks only the digest line" }
    $n++
    $stBad = @()
    $stCfg = $script:SmokeDigestConfiguration
    $script:SmokeDigestConfiguration = 'Test'
    $stTest = & $sdNew 'selftest-norow' 'serial'
    $stPlain = & $sdNew 'selftest-norow' ''
    $stAddr = & $sdNew 'selftest-norow' 'address'
    $stTimeOut = @(Add-SimOrderExpect @(New-Object PSObject -Property @{ Name = 'selftest-norow'; Expect = @{ Class = 'TimeOut' } }) 'serial')[0]
    $stPlainTimeOut = @(Add-SimOrderExpect @(New-Object PSObject -Property @{ Name = 'selftest-norow'; Expect = @{ Class = 'TimeOut' } }) '')[0]
    $script:SmokeDigestConfiguration = $stCfg
    $stTie = "$sdHead[t=1.000] [f=1] LogSim Inform CollisionObject.h:1: OPENSWBF2 COLLISION serialties=2`r`n"
    $stZero = $stTie.Replace('serialties=2', 'serialties=0')
    $stPlainTie = $stTie.Replace($sdHead, '')
    $stPlainZero = $stZero.Replace($sdHead, '')
    $env:OPENSWBF2_DIGEST_EXPECTED = $flipExp
    if (& $sdRun $stTest $stTie) { $stBad += 'serial case with collision serial ties accepted' }
    if (-not (& $sdRun $stTest $stZero)) { $stBad += 'serial case with zero collision serial ties rejected' }
    if (& $sdRun $stTest $sdHead) { $stBad += 'test build serial pass case without a serialties line accepted' }
    if (& $sdRun $stPlain $stPlainTie) { $stBad += 'plain case with collision serial ties accepted' }
    if (-not (& $sdRun $stPlain $stPlainZero)) { $stBad += 'plain case with zero collision serial ties rejected' }
    if (& $sdRun $stPlain '') { $stBad += 'test build plain pass case without a serialties line accepted' }
    if (@(@($stPlain.Expect.LogRx) -match 'SIMORDER').Count) { $stBad += 'plain case asserts a SIMORDER line' }
    foreach ($t in @($stTimeOut, $stPlainTimeOut)) {
        if (@($t.Expect.LogRx) -contains '(?m)OPENSWBF2 COLLISION serialties=0\r?$') { $stBad += 'serialties line required on a timeout case' }
        if (@($t.Expect.LogRx) -notcontains '(?s)\A(?!.*OPENSWBF2 COLLISION serialties=[1-9])') { $stBad += 'timeout case lacks the serialties lookahead' }
    }
    if (@(@($sdSerial.Expect.LogRx) -match 'serialties').Count -or @(@($sdPlain.Expect.LogRx) -match 'serialties').Count) { $stBad += 'serialties checked outside a test build' }
    if (@(@($stAddr.Expect.LogRx) -match 'serialties').Count) { $stBad += 'serialties checked under -SimOrder address' }
    if (-not (& $sdRun $stAddr $stTie.Replace('mode=serial', 'mode=address'))) { $stBad += 'collision serial ties failed an address case' }
    $env:OPENSWBF2_DIGEST_EXPECTED = $savedExpected
    if ($stBad.Count) { $failed++; Write-Host "FAILED ${n}: collision serial ties: $($stBad -join '; ')" }
    else { Write-Host "OK $n plain and -SimOrder serial test builds fail a case that logs collision serial ties" }
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
$srTmp = Join-Path $env:TEMP ("openswbf2-samerun-" + [guid]::NewGuid().ToString('N'))
try {
    $srCase = Join-Path $srTmp 'case'
    $srRef = Join-Path $srTmp 'ref'
    New-Item -ItemType Directory -Force -Path $srCase, $srRef | Out-Null
    $srWrite = {
        param($dir, $state, $free)
        [IO.File]::WriteAllText((Join-Path $dir 'run.log'), "[t=1.000] [f=1] OPENSWBF2 SIM DIGEST turns=1 state=$state`r`n[t=1.000] [f=1] OPENSWBF2 CLOTH free=$free enabled=1 low=131072 high=262144`r`n")
    }
    & $srWrite $srCase '0xA' 100
    & $srWrite $srRef '0xA' 100
    $srEqual = Test-SameRunAs $srCase $srRef
    & $srWrite $srRef '0xA' 101
    $srCloth = Test-SameRunAs $srCase $srRef
    & $srWrite $srRef '0xB' 100
    $srDigest = Test-SameRunAs $srCase $srRef
    $srMissing = Test-SameRunAs $srCase (Join-Path $srTmp 'absent')
    if ($srEqual -or $srCloth -cnotmatch 'CLOTH' -or $srDigest -cnotmatch 'digest' -or $srMissing -cnotmatch 'reference') { $failed++; Write-Host "FAILED ${n}: same run compare: equal='$srEqual' cloth='$srCloth' digest='$srDigest' missing='$srMissing'" }
    else { Write-Host "OK $n same run compare fails on a CLOTH, digest or missing reference difference" }
}
finally {
    Remove-Item -Recurse -Force $srTmp -ErrorAction SilentlyContinue
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
$cbTmp = Join-Path $env:TEMP ("openswbf2-cmpbackend-" + [guid]::NewGuid().ToString('N'))
try {
    $cbBase = Join-Path $PSScriptRoot 'baseline\v143'
    $cbScript = Join-Path $PSScriptRoot 'baseline\compare-backend.ps1'
    $cbSlack = @{}
    foreach ($l in [IO.File]::ReadAllLines((Join-Path $cbBase 'cloth-slack.txt'))) {
        if ($l -and -not $l.StartsWith('#') -and -not $l.StartsWith("mission`t")) { $f = $l -split "`t"; $cbSlack[$f[0]] = [long]$f[1] }
    }
    $cbGood = Join-Path $cbTmp 'good'
    $cbBad = Join-Path $cbTmp 'bad'
    $cbUsed = Join-Path $cbTmp 'used'
    foreach ($d in $cbGood, $cbBad, $cbUsed) {
        New-Item -ItemType Directory -Force -Path $d | Out-Null
        Copy-Item -LiteralPath (Join-Path $cbBase 'smoke-all-digests.txt') -Destination $d
    }
    $cbPeaks = [IO.File]::ReadAllLines((Join-Path $cbBase 'mem-peaks.txt'))
    $cbPeakRow = -1
    for ($i = 0; $i -lt $cbPeaks.Count; $i++) { if ($cbPeaks[$i] -match "^[^`t]+`texit`theap`tRuntime`t") { $cbPeakRow = $i; break } }
    $cbPeakLow = @($cbPeaks)
    $cbUsedLow = @($cbPeaks)
    $f = $cbPeaks[$cbPeakRow] -split "`t"
    $f[6] = [string]([long]$f[6] - 1024)
    $cbPeakLow[$cbPeakRow] = $f -join "`t"
    $f = $cbPeaks[$cbPeakRow] -split "`t"
    $f[7] = [string]([long]$f[7] - 512)
    $cbUsedLow[$cbPeakRow] = $f -join "`t"
    [IO.File]::WriteAllLines((Join-Path $cbGood 'mem-peaks.txt'), $cbPeakLow)
    [IO.File]::WriteAllLines((Join-Path $cbBad 'mem-peaks.txt'), $cbPeaks)
    [IO.File]::WriteAllLines((Join-Path $cbUsed 'mem-peaks.txt'), $cbUsedLow)
    $cbRows = @()
    $cbHigh = @()
    $cbChanged = $false
    foreach ($l in [IO.File]::ReadAllLines((Join-Path $cbBase 'cloth-headroom.txt'))) {
        if (-not $l -or $l.StartsWith('#') -or $l.StartsWith("mission`t")) { $cbRows += $l; $cbHigh += $l; continue }
        $f = $l -split "`t"
        if ($f[1] -ne '-') { $f[1] = [string]([long]$f[1] + $cbSlack[$f[0]]) }
        $cbRows += ($f -join "`t")
        if (-not $cbChanged) { $f[4] = [string]([long]$f[4] + 16); $cbChanged = $true }
        $cbHigh += ($f -join "`t")
    }
    [IO.File]::WriteAllLines((Join-Path $cbGood 'cloth-headroom.txt'), $cbRows)
    [IO.File]::WriteAllLines((Join-Path $cbBad 'cloth-headroom.txt'), $cbHigh)
    [IO.File]::WriteAllLines((Join-Path $cbUsed 'cloth-headroom.txt'), $cbRows)
    $cbLuaTables = Join-Path $cbTmp 'luatables'
    $cbLuaArena = Join-Path $cbTmp 'luaarena'
    foreach ($d in $cbLuaTables, $cbLuaArena) { New-Item -ItemType Directory -Force -Path $d | Out-Null }
    Copy-Item -LiteralPath (Join-Path $cbBase 'smoke-all-digests.txt') -Destination $cbLuaTables
    [IO.File]::WriteAllLines((Join-Path $cbLuaTables 'cloth-headroom.txt'), $cbRows)
    $cbLuaPeaks = @($cbPeaks)
    $cbLuaHeap = $false
    $cbLuaPool = $false
    for ($i = 0; $i -lt $cbLuaPeaks.Count; $i++) {
        if (-not $cbLuaHeap -and $cbLuaPeaks[$i] -match "^[^`t]+`t[^`t]+`theap`tLua`t") {
            $f = $cbLuaPeaks[$i] -split "`t"
            $f[8] = [string]([long]$f[8] + 7)
            $cbLuaPeaks[$i] = $f -join "`t"
            $cbLuaHeap = $true
        }
        elseif (-not $cbLuaPool -and $cbLuaPeaks[$i] -match "^[^`t]+`t[^`t]+`tpool`tLua8`t") {
            $f = $cbLuaPeaks[$i] -split "`t"
            $f[4] = [string]([long]$f[4] + 11)
            $cbLuaPeaks[$i] = $f -join "`t"
            $cbLuaPool = $true
        }
    }
    [IO.File]::WriteAllLines((Join-Path $cbLuaTables 'mem-peaks.txt'), $cbLuaPeaks)
    [IO.File]::WriteAllLines((Join-Path $cbLuaArena 'mem-peaks.txt'), $cbLuaPeaks)
    $cbOutGood = (& powershell -NoProfile -ExecutionPolicy Bypass -File $cbScript -Tables $cbGood -ArenaTables $cbBase 2>&1 | Out-String)
    $cbCodeGood = $LASTEXITCODE
    $cbOutBad = (& powershell -NoProfile -ExecutionPolicy Bypass -File $cbScript -Tables $cbBad -ArenaTables $cbBase 2>&1 | Out-String)
    $cbCodeBad = $LASTEXITCODE
    $cbOutUsed = (& powershell -NoProfile -ExecutionPolicy Bypass -File $cbScript -Tables $cbUsed -ArenaTables $cbBase 2>&1 | Out-String)
    $cbCodeUsed = $LASTEXITCODE
    $cbOutLuaBoth = (& powershell -NoProfile -ExecutionPolicy Bypass -File $cbScript -Tables $cbLuaTables -ArenaTables $cbLuaArena 2>&1 | Out-String)
    $cbCodeLuaBoth = $LASTEXITCODE
    $cbOutLuaOne = (& powershell -NoProfile -ExecutionPolicy Bypass -File $cbScript -Tables $cbLuaTables -ArenaTables $cbBase 2>&1 | Out-String)
    $cbCodeLuaOne = $LASTEXITCODE
    $cbWhy = $null
    if ($cbCodeUsed -ne 1 -or $cbOutUsed -notmatch 'FAIL. backend:mem-peaks' -or $cbOutUsed -notmatch 'used gap 512 used bound 496') { $cbWhy = "used gap past the slack gave exit $cbCodeUsed" }
    elseif ($cbCodeGood -ne 0 -or $cbOutGood -notmatch '\[PASS\] backend:cloth missions=109') { $cbWhy = "slack-raised tables gave exit $cbCodeGood" }
    elseif ($cbCodeBad -ne 1 -or $cbOutBad -notmatch '\[FAIL\] backend:cloth' -or $cbOutBad -notmatch '\[PASS\] backend:mem-peaks') { $cbWhy = "changed high gave exit $cbCodeBad" }
    elseif (-not $cbLuaHeap -or -not $cbLuaPool) { $cbWhy = "fixture found no Lua heap or Lua8 pool row" }
    elseif ($cbCodeLuaBoth -ne 0 -or $cbOutLuaBoth -notmatch '\[PASS\] backend:mem-peaks rows=' -or $cbOutLuaBoth -notmatch '\[PASS\] backend:mem-peaks-lua rows=') { $cbWhy = "Lua rows changed on both backends gave exit $cbCodeLuaBoth" }
    elseif ($cbCodeLuaOne -ne 1 -or $cbOutLuaOne -notmatch '\[FAIL\] backend:mem-peaks-lua' -or $cbOutLuaOne -notmatch '\[PASS\] backend:mem-peaks rows=') { $cbWhy = "Lua rows changed on one backend gave exit $cbCodeLuaOne" }
    if ($cbWhy) { $failed++; Write-Host "FAILED ${n}: compare-backend: $cbWhy" }
    else { Write-Host "OK $n compare-backend passes slack-raised CLOTH free and a lower peak, fails a changed high and a used gap past the slack, ignores Lua rows against the archive and compares them between backends" }
    $n++
}
catch { $failed++; Write-Host "FAILED: compare-backend block threw: $_" }
finally {
    Remove-Item -LiteralPath $cbTmp -Recurse -Force -ErrorAction SilentlyContinue
}
if ($failed -gt 0) { exit 1 }
exit 0
