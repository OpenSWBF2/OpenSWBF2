param([string]$Configuration = 'Debug')
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'smoke.ps1')
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
    @('tables compare skip is named', 'unit', "$list`r`n$onePass`r`nexit /b 0", $null, 0, '\[SKIP\] tables:compare - no smoke run', $null),
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
        @('asan canary heap overflow is reported', (Get-AsanCanaryRx 'heap-buffer-overflow')),
        @('asan canary red heap overflow is reported', (Get-AsanCanaryRx)),
        @('asan canary red heap use after free is reported', (Get-AsanCanaryRx)),
        @('asan canary memory pool overflow is reported', (Get-AsanCanaryRx)),
        @('asan canary memory pool use after free is reported', (Get-AsanCanaryRx)),
        @('asan canary lua bin overflow is reported', (Get-AsanCanaryRx)),
        @('asan canary lua bin use after free is reported', (Get-AsanCanaryRx)),
        @('asan canary net packet overflow is reported', (Get-AsanCanaryRx -tail '[\s\S]*netBigPackets')),
        @('asan canary net packet use after free is reported', (Get-AsanCanaryRx -tail '[\s\S]*netBigPackets')),
        @('asan canary red stack overflow is reported', (Get-AsanCanaryRx -tail '[\s\S]*sMem')),
        @('asan canary red stack use after scope is reported', (Get-AsanCanaryRx -tail '[\s\S]*sMem')),
        @('asan canary stack allocator overflow is reported', (Get-AsanCanaryRx)),
        @('asan canary stack allocator use after free is reported', (Get-AsanCanaryRx)),
        @('asan canary sub allocator overflow is reported', (Get-AsanCanaryRx)),
        @('asan canary sub allocator use after free is reported', (Get-AsanCanaryRx)),
        @('asan canary list allocator overflow is reported', (Get-AsanCanaryRx)),
        @('asan canary list allocator use after free is reported', (Get-AsanCanaryRx)),
        @('asan canary snd free list overflow is reported', (Get-AsanCanaryRx)),
        @('asan canary snd free list use after free is reported', (Get-AsanCanaryRx)),
        @('asan canary glyph memory overflow is reported', (Get-AsanCanaryRx)),
        @('asan canary glyph memory use after free is reported', (Get-AsanCanaryRx)),
        @('asan canary mimalloc heap slack overflow is reported', (Get-AsanCanaryRx)),
        @('asan canary mimalloc heap prefix underrun is reported', (Get-AsanCanaryRx)),
        @('asan canary mimalloc heap use after destroy is reported', (Get-AsanCanaryRx))
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
    $canaryRx = Get-AsanCanaryRx
    $canaryHit = "ERROR: AddressSanitizer: use-after-poison on address 0x1 at pc 0x2`r`nWRITE of size 1 at 0x1 thread T0`r`n    #0 0x2 in canary C:\x\tests\unit\asan_canary.cpp:45`r`n    #1 0x3 in doctest C:\x\doctest.h:1`r`n"
    $canaryChecks = @(
        @(66, $canaryHit, 'REPORTED'),
        @(66, $canaryHit.Replace("`r`n", "`n"), 'REPORTED'),
        @(66, $canaryHit.Replace('use-after-poison', 'unknown-crash'), 'OTHER'),
        @(66, $canaryHit.Replace('#0 0x2 in canary C:\x\tests\unit\asan_canary.cpp:45', '#0 0x2 in MiReadPrefix C:\x\RedMemoryMi.cpp:50').Replace('#1 0x3 in doctest C:\x\doctest.h:1', '#1 0x3 in canary C:\x\tests\unit\asan_canary.cpp:45'), 'OTHER'),
        @(66, ($canaryHit + "allocated by thread T0 here:`r`n    #0 0x4 in canary C:\x\tests\unit\asan_canary.cpp:44`r`n"), 'REPORTED'),
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
$n++
$peSrc = @((Join-Path $PSScriptRoot "..\bin\Win32\$Configuration\BattlefrontII.exe"), (Join-Path $PSScriptRoot "..\bin\Tests\$Configuration\OpenSWBF2Tests.exe") | Where-Object { Test-Path -LiteralPath $_ })
if ($peSrc.Count -eq 0) { $failed++; Write-Host "FAILED ${n}: pe aslr fixtures: no built $Configuration exe to copy a header from" }
else {
    $fs = [IO.File]::OpenRead($peSrc[0])
    try {
        $peHdr = New-Object byte[] 4096
        $peLen = $fs.Read($peHdr, 0, $peHdr.Length)
    }
    finally { $fs.Dispose() }
    $peE = [BitConverter]::ToInt32($peHdr, 0x3C)
    $peOpt = $peE + 24
    $peCases = @(
        @('clean header', @(), $null),
        @('dynamic base cleared', @(, @(($peOpt + 70), 0xBF, 0)), 'DYNAMIC_BASE is clear'),
        @('relocation directory zeroed', @(@(($peOpt + 136), 0, 0), @(($peOpt + 137), 0, 0), @(($peOpt + 138), 0, 0), @(($peOpt + 139), 0, 0), @(($peOpt + 140), 0, 0), @(($peOpt + 141), 0, 0), @(($peOpt + 142), 0, 0), @(($peOpt + 143), 0, 0)), 'base relocation directory is empty'),
        @('relocs stripped set', @(, @(($peE + 22), 0xFF, 0x01)), 'IMAGE_FILE_RELOCS_STRIPPED is set'),
        @('PE32+ magic', @(@($peOpt, 0, 0x0B), @(($peOpt + 1), 0, 0x02)), 'magic 0x20B is not PE32')
    )
    $why = $null
    $i = 0
    $peTmp = Join-Path $env:TEMP ("openswbf2-pe-" + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $peTmp | Out-Null
    foreach ($pc in $peCases) {
        $h = [byte[]]$peHdr.Clone()
        foreach ($m in $pc[1]) { $h[$m[0]] = [byte](($h[$m[0]] -band $m[1]) -bor $m[2]) }
        $f = Join-Path $peTmp ("pe-aslr-$i.bin")
        $i++
        [IO.File]::WriteAllBytes($f, [byte[]]$h[0..($peLen - 1)])
        $got = Get-PeAslr $f
        if ($null -eq $pc[2]) { if ($null -ne $got) { $why = "$($pc[0]) gave '$got'" } }
        elseif ($null -eq $got -or -not $got.Contains($pc[2])) { $why = "$($pc[0]) gave '$got', expected '$($pc[2])'" }
    }
    Remove-Item -Recurse -Force $peTmp -ErrorAction SilentlyContinue
    if ($why) { $failed++; Write-Host "FAILED ${n}: pe aslr fixtures: $why" }
    else { Write-Host "OK $n pe aslr fixtures flag dynamic base, relocations, relocs stripped and PE32 magic from $(Split-Path $peSrc[0] -Leaf)" }
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
    @('mimalloc-defines without tests tlog checks the game only', $miGood, 'Debug', $true, $false)
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
$addrDir = Join-Path $env:TEMP ("openswbf2-addrorder-" + [guid]::NewGuid().ToString('N'))
try {
    foreach ($d in 'src\Battlefront2\Source', 'src\RedEngineFL') { New-Item -ItemType Directory -Path (Join-Path $addrDir $d) | Out-Null }
    $addrLines = @('int k = (int)this;', 'if ((unsigned int)pA > 1) {}', 'if (1 < (unsigned int)pB) {}', 'if (reinterpret_cast<uintptr_t>(pA) > 1) {}', 'if (1 > reinterpret_cast<int>(pA)) {}', 'bool b = PblPtrAfter(x, y);', 'if (pOther > pObject) {}', 'int c = iCount > 3;', 'pGroup->mThreads.Add(&mNode, this, (int)pOwner);', 'if (ppA > ppB) {}', 'bool e = (const char*)pA > (const char*)pB;', 'bool d = std::less<const void*>()(pA, pB);', 'bool f = obj > other;', 'mList.Add(&mNode, this, (int)iKey);', 'n = (char*)pA - (char*)pB;')
    Set-Content -Encoding ASCII -Path (Join-Path $addrDir 'src\Battlefront2\Source\a.cpp') -Value $addrLines
    Set-Content -Encoding ASCII -Path (Join-Path $addrDir 'src\RedEngineFL\a.cpp') -Value $addrLines
    $bad = @(Get-SourceChecks $addrDir -addrOrderAllow @() -Only 'addr-order')
    $addrWant = (@(1..7) + @(9..12) | ForEach-Object { "src\Battlefront2\Source\a.cpp:$_" }) -join ', '
    $allowed = @(Get-SourceChecks $addrDir -addrOrderAllow @('src\Battlefront2\Source\a.cpp|if (pOther > pObject) {}') -Only 'addr-order')
    $unused = @(Get-SourceChecks $addrDir -addrOrderAllow @('src\Battlefront2\Source\a.cpp|if (pOther > pObject) {}', 'src\Battlefront2\Source\a.cpp|int nope;') -Only 'addr-order')
    if ($bad.Count -ne 1 -or $bad[0].Ok -or $bad[0].Detail -ne $addrWant) { $failed++; Write-Host "FAILED ${n}: addr-order flags pointer order under Battlefront2 and PebbleFL only: $($bad.Detail)" }
    elseif ($allowed[0].Detail -cmatch ':7(,|$)') { $failed++; Write-Host "FAILED ${n}: addr-order allow entry did not suppress its line: $($allowed[0].Detail)" }
    elseif ($unused[0].Detail -cnotmatch 'unused allow src\\Battlefront2\\Source\\a\.cpp\|int nope;') { $failed++; Write-Host "FAILED ${n}: addr-order did not report an unused allow entry: $($unused[0].Detail)" }
    else { Write-Host "OK $n addr-order flags pointer order under Battlefront2 and PebbleFL only and keeps its allow list exact" }
}
finally {
    Remove-Item -Recurse -Force $addrDir -ErrorAction SilentlyContinue
}
$n++
$crtBanDir = Join-Path $env:TEMP ("openswbf2-crtban-" + [guid]::NewGuid().ToString('N'))
try {
    foreach ($d in 'src\Foo', 'src\mimalloc\src', 'src\PebbleFL\Common') { New-Item -ItemType Directory -Path (Join-Path $crtBanDir $d) | Out-Null }
    Set-Content -Encoding ASCII -Path (Join-Path $crtBanDir 'src\Foo\a.cpp') -Value @('void* p = malloc(1);', 'void* q = calloc (1, 2);', 'p = realloc(p, 4);', 'free(p);', 'char* s = _strdup("x");', 'size_t z = _msize(q);', 'pBackend->free(p);', 'mFree(p);', 'RedFreeToHeap(0, p, 0);', 'PblCrtFree(p);', 'int free_count = 0;', 'std::free(p);')
    Set-Content -Encoding ASCII -Path (Join-Path $crtBanDir 'src\mimalloc\src\x.c') -Value @('void* p = malloc(1);')
    Set-Content -Encoding ASCII -Path (Join-Path $crtBanDir 'src\PebbleFL\Common\PblCrtHeap.cpp') -Value @('void* p = malloc(1);', 'free(p);')
    $bad = @(Get-SourceChecks $crtBanDir -Only 'crt-heap-ban')
    Remove-Item -LiteralPath (Join-Path $crtBanDir 'src\Foo\a.cpp')
    $good = @(Get-SourceChecks $crtBanDir -Only 'crt-heap-ban')
    if ($bad.Count -ne 1 -or $bad[0].Ok -or $bad[0].Detail -ne 'src\Foo\a.cpp:1, src\Foo\a.cpp:2, src\Foo\a.cpp:3, src\Foo\a.cpp:4, src\Foo\a.cpp:5, src\Foo\a.cpp:6') { $failed++; Write-Host "FAILED ${n}: crt-heap-ban flags CRT heap calls outside src\mimalloc and PblCrtHeap.cpp: $($bad.Detail)" }
    elseif ($good.Count -ne 1 -or -not $good[0].Ok) { $failed++; Write-Host "FAILED ${n}: crt-heap-ban flags CRT heap calls outside src\mimalloc and PblCrtHeap.cpp: allowed files flagged $($good.Detail)" }
    else { Write-Host "OK $n crt-heap-ban flags CRT heap calls outside src\mimalloc and PblCrtHeap.cpp" }
}
finally {
    Remove-Item -Recurse -Force $crtBanDir -ErrorAction SilentlyContinue
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
$testAllCfg = $script:SmokeDigestConfiguration
$script:SmokeDigestConfiguration = 'Test'
$testAllCases = @(Get-SmokeCases 'All' @('bar1c_con') $false $null)
$script:SmokeDigestConfiguration = $testAllCfg
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
elseif (@($allCases | Where-Object { $_.Name -like 'crash-*' }).Count -ne 42 -or $byName.ContainsKey('crash-buildheapfail') -or -not $byName.ContainsKey('fatal-init') -or $byName['dedicated'].Timeout -ne 0) { $why = 'dedicated or crash cases changed' }
elseif (@($testAllCases | Where-Object { $_.Name -like 'crash-*' }).Count -ne 43 -or @($testAllCases | Where-Object { $_.Name -eq 'crash-buildheapfail' }).Count -ne 1) { $why = 'Test crash cases changed' }
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
    $cfCrt = { param($s, $b, $c, $a, $th, $pv) "[t=1.000] [f=1] LogMemory Inform GameMemory.cpp:1: OPENSWBF2 COMMITPARTS state=$s private=$pv gamecrt=$b heaps=$c threads=$th threadcount=9 arena=$a crtbusy=0 crtcommitted=0 crtlarge=0`r`n" }
    $cfLog = { param($m, $mb, $mc, $ma, $order, $noParts, $mt)
        if (-not $mt) { $mt = @(0, 0, 0, 0) }
        $t = ''
        for ($k = 0; $k -lt 4; $k++) {
            $t += & $cfLine mission $m[$k]
            if (-not $noParts) { $t += & $cfCrt mission $mb[$k] $mc[$k] $ma[$k] $mt[$k] $m[$k] }
            $sState = $(if ($order -and $k -eq 1) { 'mission' } else { 'shell' })
            $t += & $cfLine $sState 60000000
            if (-not $noParts) { $t += & $cfCrt $sState 3000000 2000000 1000000 0 60000000 }
        }
        $t
    }
    $cfB = @(3000000, 3000000, 3000000, 3000000)
    $cfC = @(2000000, 2000000, 2000000, 2000000)
    $cfA = @(1000000, 1000000, 1000000, 1000000)
    $cfFlat = @(100000000, 100000000, 100000000, 100000000)
    $cfP = Test-CommitFlat (& $cfLog @(100000000, 100003000, 100001000, 100004000) $cfB $cfC $cfA $false $false) 1000 4 2
    $cfH = Test-CommitFlat (& $cfLog @(100000000, 100003000, 100001000, 100004001) $cfB $cfC $cfA $false $false) 1000 4 2
    $cfR = Test-CommitFlat (& $cfLog @(100000000, 100000000, 100000000, 100500000) $cfB @(2000000, 2000000, 2000000, 2500000) $cfA $false $false) 1000 4 2
    $cfL = Test-CommitFlat (& $cfLog $cfFlat @(3000000, 3000000, 3000000, 3002000) $cfC $cfA $false $false) 1000 4 2
    $cfN = Test-CommitFlat (& $cfLog @(100000000, 100000000, 100000000, 100262144) $cfB $cfC @(1000000, 1000000, 1000000, 1262144) $false $false) 1000 4 2
    $cfX = Test-CommitFlat (& $cfLog @(100262144, 100200000, 100000000, 100462144) $cfB $cfC @(1262144, 1000000, 1000000, 1262144) $false $false) 1000 4 2
    $cfM = Test-CommitFlat (& $cfLog $cfFlat $cfB $cfC $cfA $false $true) 1000 4 2
    $cfF = Test-CommitFlat (& $cfLog $cfFlat $cfB $cfC $cfA $true $false) 1000 4 2
    $cfZ = Test-CommitFlat (& $cfLog $cfFlat $cfB $cfC $cfA $false $false) 1000 0 0
    $cfS = Test-CommitFlat (& $cfLog $cfFlat $cfB $cfC $cfA $false $false) 1000 4 4
    $cfT = Test-CommitFlat (& $cfLog @(100000000, 100000000, 100000000, 100184320) $cfB $cfC $cfA $false $false @(400000, 400000, 400000, 584320)) 0 4 2
    $cfG = Test-CommitFlat (& $cfLog $cfFlat @(3000000, 3000000, 3000000, 3000001) $cfC $cfA $false $false) 0 4 2
    $cfCfg = $script:SmokeDigestConfiguration
    $script:SmokeDigestConfiguration = 'Test'
    $cfCase = @(Get-SmokeCases 'Standard' @() $false $null | Where-Object { $_.Name -eq 'commit-flat' })
    $rwCase = @(Get-SmokeCases 'Standard' @() $false $null | Where-Object { $_.Name -eq 'repeat-water' })
    $script:SmokeDigestConfiguration = 'Asan'
    $rwCase += @(Get-SmokeCases 'Standard' @() $false $null | Where-Object { $_.Name -eq 'repeat-water' })
    $script:SmokeDigestConfiguration = 'Release'
    $rwRel = @(Get-SmokeCases 'Standard' @() $false $null | Where-Object { $_.Name -eq 'repeat-water' })
    $script:SmokeDigestConfiguration = $cfCfg
    $cfWhy = $null
    if ($cfP -or -not $cfH -or $cfR -or -not $cfL -or $cfN -notmatch ' arena = ' -or $cfX -or $cfM -notmatch 'COMMITPARTS' -or -not $cfF -or $cfZ -cne 'CommitCycles and CommitSettled are required' -or $cfS -cne 'CommitSettled must be below CommitCycles' -or $cfT -or $cfG -notmatch 'rest = ') { $cfWhy = "pass='$cfP' high='$cfH' retention='$cfR' leak='$cfL' newpage='$cfN' cross='$cfX' noparts='$cfM' order='$cfF' zero='$cfZ' settled='$cfS' threads='$cfT' gamecrt='$cfG'" }
    elseif ($cfCase.Count -ne 1 -or $cfCase[0].Expect.CommitFlat -ne 0 -or $cfCase[0].Expect.CommitCycles -ne 16 -or $cfCase[0].Expect.CommitSettled -ne 8 -or $cfCase[0].Expect.SameDigests -ne 17 -or $cfCase[0].Args -notmatch ' -repeatmission 16$' -or $cfCase[0].Timeout -ne 220) { $cfWhy = 'commit-flat case missing or wrong' }
    if ($cfWhy) { $failed++; Write-Host "FAILED ${n}: commit flat: $cfWhy" }
    else { Write-Host "OK $n commit flat fails a new arena or rest high after the settled cycles, counts game CRT bytes in place of heap and thread commit and needs alternating states" }
    $n++
    if ($rwCase.Count -ne 2 -or $rwRel.Count -ne 0 -or @($rwCase | Where-Object { $_.Timeout -ne 23 -or $_.Expect.Class -ne 'TimeOut' -or $_.Expect.SameDigests -ne 2 }).Count -ne 0) { $failed++; Write-Host "FAILED ${n}: repeat-water must run in Test and Asan only, TimeOut 23 with two equal digests" }
    else { Write-Host "OK $n repeat-water runs in Test and Asan with its measured timeout" }
    $n++
    $bareCr = @(Get-ChildItem -LiteralPath $PSScriptRoot -Filter '*.ps1' -Recurse -File | Where-Object { [IO.File]::ReadAllText($_.FullName) -match "`r(?!`n)" } | ForEach-Object { $_.Name })
    if ($bareCr.Count -gt 0) { $failed++; Write-Host "FAILED ${n}: bare carriage return in $($bareCr -join ', ')" }
    else { Write-Host "OK $n no test script holds a bare carriage return" }
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
    $pvDigest = 'OPENSWBF2 SIM DIGEST turns=9 time=1.000 rand=1/2 snd=1.000 state=0x00000042'
    $trAd = Join-Path $digTmp 'trace-a-digest.log'
    $trBd = Join-Path $digTmp 'trace-b-digest.log'
    [IO.File]::WriteAllLines($trAd, [string[]]($linesA + $pvDigest))
    [IO.File]::WriteAllLines($trBd, [string[]]($linesB + $pvDigest))
    $bAd = Get-PerturbBase $trAd
    $pvEq = Get-PerturbVerdict $bAd $trAd 'Pass' 'Pass' $false 9
    $pvSp = Get-PerturbVerdict $bAd $trBd 'Pass' 'Pass' $false 9
    $pvIn = Get-PerturbVerdict $bAd $trE 'Pass' 'Pass' $false 9
    $pvCr = Get-PerturbVerdict $bAd $trBd 'Crash' 'Pass' $false 9
    $trA2 = Join-Path $digTmp 'trace-a2.log'
    $trB2 = Join-Path $digTmp 'trace-b2.log'
    [IO.File]::WriteAllLines($trA2, [string[]](1..1520 | ForEach-Object { 'OPENSWBF2 DIGEST TRACE turn={0} state=0x{0:X8} rand=1/2' -f $_ }))
    [IO.File]::WriteAllLines($trB2, [string[]](1..1510 | ForEach-Object { 'OPENSWBF2 DIGEST TRACE turn={0} state=0x{0:X8} rand=1/2' -f $_ }))
    $cmpLim = Compare-DigestTrace $trA2 $trB2 1500
    $cmpAll = Compare-DigestTrace $trA2 $trB2
    $pvBad = @()
    if ($pvEq.Result -ne 'equal') { $pvBad += "equal=$($pvEq.Result)" }
    if ($pvSp.Result -ne 'split' -or $pvSp.Turn -ne 7) { $pvBad += "split=$($pvSp.Result)/$($pvSp.Turn)" }
    if ($pvIn.Result -ne 'crash' -or $pvIn.Class -ne 'incomplete') { $pvBad += "incomplete=$($pvIn.Result)/$($pvIn.Class)" }
    if ($pvCr.Result -ne 'crash' -or $pvCr.Class -ne 'Crash' -or $pvCr.Turn -ne 7) { $pvBad += "crash=$($pvCr.Result)/$($pvCr.Class)/$($pvCr.Turn)" }
    if ($cmpLim -ne 0 -or $cmpAll -ne -1) { $pvBad += "limit=$cmpLim/$cmpAll" }
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
    $bRep = Get-PerturbBase $repBase
    $vEq = Get-PerturbVerdict $bRep $repEq 'Pass' 'Pass' $true 15
    $vSp = Get-PerturbVerdict $bRep $repSp 'Pass' 'Pass' $true 15
    $vOne = Get-PerturbVerdict $bRep $repOne 'Pass' 'Pass' $true 15
    if ($vEq.Result -ne 'equal') { $fullBad += "repeat equal=$($vEq.Result)/$($vEq.Class)" }
    if ($vSp.Result -ne 'split' -or $vSp.Turn -ne 18) { $fullBad += "repeat split=$($vSp.Result)/$($vSp.Turn)" }
    if ($vOne.Result -ne 'crash' -or $vOne.Class -ne 'incomplete') { $fullBad += "repeat one=$($vOne.Result)/$($vOne.Class)" }
    $serBase = Join-Path $digTmp 'serial-base.log'
    $serRand = Join-Path $digTmp 'serial-rand.log'
    [IO.File]::WriteAllLines($serBase, [string[]](@(1..20 | ForEach-Object { & $trLine $_ 1 }) + $simLine))
    [IO.File]::WriteAllLines($serRand, [string[]](@(1..20 | ForEach-Object { if ($_ -eq 7) { & $trLine $_ 3 } else { & $trLine $_ 1 } }) + $simLine))
    $bSer = Get-PerturbBase $serBase
    $sSer = Get-PerturbVerdict $bSer $serRand 'Pass' 'Pass' $false 15
    $sEq = Get-PerturbVerdict $bSer $serBase 'Pass' 'Pass' $false 15
    $sNoDig = Get-PerturbVerdict $bSer $trA 'Pass' 'Pass' $false 9
    if ($sSer.Result -ne 'split' -or $sSer.Turn -ne 7 -or $sEq.Result -ne 'equal') { $fullBad += "serial=$($sSer.Result)/$($sSer.Turn) same=$($sEq.Result)" }
    if ($sNoDig.Result -ne 'crash' -or $sNoDig.Class -ne 'incomplete') { $fullBad += "serial short=$($sNoDig.Result)/$($sNoDig.Class)" }
    $gv = @(
        (Get-PerturbGroupVerdict 0 54 54), (Get-PerturbGroupVerdict 1 54 54), (Get-PerturbGroupVerdict 0 53 54))
    if (($gv -join ',') -cne 'True,False,False') { $fullBad += "group=$($gv -join ',')" }
    if ($fullBad.Count) { $failed++; Write-Host "FAILED ${n}: perturb full trace: $($fullBad -join '; ')" }
    else { Write-Host "OK $n perturb full trace compares rand and late turns and splits repeat segments" }
    $n++
    $wBad = @()
    $simW = $simLine.Replace('turns=15', 'turns=1500')
    $trRange = { param($from, $to, $odd) @($from..$to | ForEach-Object { if ($_ -eq $odd) { & $trLine $_ 3 } else { & $trLine $_ 1 } }) }
    $wFile = { param($name, $lines) $f = Join-Path $digTmp $name; [IO.File]::WriteAllLines($f, [string[]]$lines); $f }
    $wLong = & $wFile 'whole-long.log' ((& $trRange 1 22 0) + $simLine)
    $vLong = Get-PerturbVerdict $bSer $wLong 'Pass' 'Pass' $false 15
    if ($vLong.Result -ne 'split' -or $vLong.Turn -ne 21) { $wBad += "longer run=$($vLong.Result)/$($vLong.Turn)" }
    $wCut = & $wFile 'whole-cut.log' ((& $trRange 1 20 0) + $simLine + '[t=1.000] [f=1] LogSim Inform SimDigest.cpp:164: OPENSWBF2 DIGEST TRACE turn=21 state=0x0000')
    $wCutRand = & $wFile 'whole-cut-rand.log' ((& $trRange 1 20 0) + $simLine + '[t=1.000] [f=1] LogSim Inform SimDigest.cpp:164: OPENSWBF2 DIGEST TRACE turn=21 state=0x00000015 rand=1/')
    $vCut = Get-PerturbVerdict $bSer $wCut 'Pass' 'Pass' $false 15
    $vCutRand = Get-PerturbVerdict $bSer $wCutRand 'Pass' 'Pass' $false 15
    if ($vCut.Result -ne 'equal' -or $vCutRand.Result -ne 'equal') { $wBad += "truncated last line=$($vCut.Result)/$($vCutRand.Result)" }
    $bTo = Get-PerturbBase (& $wFile 'whole-to-base.log' ((& $trRange 1 1700 0) + $simW))
    $vToEq = Get-PerturbVerdict $bTo (& $wFile 'whole-to-short.log' ((& $trRange 1 1550 1550) + $simW)) 'TimeOut' 'TimeOut' $false
    $vToLong = Get-PerturbVerdict $bTo (& $wFile 'whole-to-long.log' ((& $trRange 1 1800 0) + $simW)) 'TimeOut' 'TimeOut' $false
    $vToSp = Get-PerturbVerdict $bTo (& $wFile 'whole-to-split.log' ((& $trRange 1 1800 1620) + $simW)) 'TimeOut' 'TimeOut' $false
    $vToIn = Get-PerturbVerdict $bTo (& $wFile 'whole-to-incomplete.log' ((& $trRange 1 1400 0) + $simW)) 'TimeOut' 'TimeOut' $false
    if ($vToEq.Result -ne 'equal' -or $vToLong.Result -ne 'equal') { $wBad += "timeout prefix=$($vToEq.Result)/$($vToLong.Result)" }
    if ($vToSp.Result -ne 'split' -or $vToSp.Turn -ne 1620) { $wBad += "timeout split=$($vToSp.Result)/$($vToSp.Turn)" }
    if ($vToIn.Result -ne 'crash' -or $vToIn.Class -ne 'incomplete') { $wBad += "timeout below 1500=$($vToIn.Result)/$($vToIn.Class)" }
    $wSeg1 = (& $trRange 1 1500 0) + $simW + $toMission
    $vRepEq = Get-PerturbVerdict $bTo (& $wFile 'whole-rep-eq.log' ($wSeg1 + (& $trRange 1 1700 0) + $simW)) 'Pass' 'Pass' $true
    $vRepLate = Get-PerturbVerdict $bTo (& $wFile 'whole-rep-late.log' ($wSeg1 + (& $trRange 1 1700 1650) + $simW)) 'Pass' 'Pass' $true
    $vRepShort = Get-PerturbVerdict $bTo (& $wFile 'whole-rep-short.log' ((& $trRange 1 1499 0) + $simW + $toMission + (& $trRange 1 1700 0) + $simW)) 'Pass' 'Pass' $true
    if ($vRepEq.Result -ne 'equal') { $wBad += "repeat whole=$($vRepEq.Result)/$($vRepEq.Class)" }
    if ($vRepLate.Result -ne 'split' -or $vRepLate.Turn -ne 1650) { $wBad += "repeat late split=$($vRepLate.Result)/$($vRepLate.Turn)" }
    if ($vRepShort.Result -ne 'crash' -or $vRepShort.Class -ne 'incomplete') { $wBad += "repeat segment one below 1500=$($vRepShort.Result)/$($vRepShort.Class)" }
    $bShort = Get-PerturbBase (& $wFile 'whole-base-short.log' ((& $trRange 1 1499 0) + $simW))
    if ((Test-PerturbBaseFull $bTo) -ne $true -or (Test-PerturbBaseFull $bShort) -ne $false -or (Test-PerturbBaseFull (Get-PerturbBase $repEq) 15) -ne $false) { $wBad += 'base full check' }
    if ($wBad.Count) { $failed++; Write-Host "FAILED ${n}: perturb whole mission: $($wBad -join '; ')" }
    else { Write-Host "OK $n perturb whole mission splits a longer run, ignores a cut last line, compares a TimeOut prefix and repeat segment two past 1600" }
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
    if (@(Get-PerturbProofMissing $pCases['perturb-geo1c_con-pad-1'] $pLine.Replace('modes=pad', 'modes=pad,fill')).Count -eq 0) { $prBad += 'pad accepted modes=pad,fill' }
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
    if ($allP.Count -ne 63 -or @($allP | Where-Object { $_.Expect.Perturb.Mode -eq 'repeat' }).Count -ne 7) { $prBad += "perturb cases=$($allP.Count), need 63 with 7 repeat" }
    if ($nabRep.Count -ne 1 -or $nabRep[0].Timeout -ne 180 -or $nabRep[0].Expect.Class -ne 'TimeOut') { $prBad += 'nab2g_c repeat not TimeOut 180' }
    if ($prBad.Count) { $failed++; Write-Host "FAILED ${n}: perturb proof: $($prBad -join '; ')" }
    else { Write-Host "OK $n perturb runs must prove their mode, seed, hog and repeat" }
    $n++
    $moBad = @()
    $moRx = 'OPENSWBF2 MIOPTIONS purge_delay=1000 retry_on_oom=0 arena_reserve=67108864 arena_eager_commit=0 reserve_os_memory=0 page_full_retain=-1'
    $moNew = { param($class) @(Add-MiOptionsExpect @(New-Object PSObject -Property @{ Name = 'selftest-case'; Expect = @{ Class = $class } }) -1)[0] }
    foreach ($class in 'Pass', 'TimeOut') {
        if (@((& $moNew $class).Expect.LogRx) -notcontains $moRx) { $moBad += "$class case lacks purge_delay=1000" }
    }
    foreach ($c in @(Add-MiOptionsExpect @(Get-SmokeCases 'Standard' @() $false $null) 0)) { if ($c.Expect.Class -in 'Pass', 'TimeOut' -and @($c.Expect.LogRx) -notcontains $moRx.Replace('purge_delay=1000', 'purge_delay=0')) { $moBad += "$($c.Name) lacks purge_delay=0" } }
    $moPlain = @{}
    foreach ($c in @(Get-SmokeCases 'Crash' @() $false $null)) { $moPlain[$c.Name] = $c }
    foreach ($c in @(Add-MiOptionsExpect @(Get-SmokeCases 'Crash' @() $false $null) -1)) { if ($c.Expect.Class -notin 'Pass', 'TimeOut' -and (@($c.Expect.LogRx) -join "`n") -cne (@($moPlain[$c.Name].Expect.LogRx) -join "`n")) { $moBad += "$($c.Name) fatal expectation changed" } }
    $moOne = & $moNew 'Pass'
    $moLine = "[t=0.000] [f=0] LogCore Inform pcMain.cpp:1: $moRx`r`n"
    $moMiss = { param($case, $text) @(@($case.Expect.LogRx) | Where-Object { $text -cnotmatch $_ }).Count }
    if ((& $moMiss $moOne $moLine) -ne 0) { $moBad += 'mimalloc options log rejected' }
    elseif ((& $moMiss $moOne 'x') -eq 0) { $moBad += 'log without MIOPTIONS accepted' }
    if ($moBad.Count) { $failed++; Write-Host "FAILED ${n}: mimalloc options assertion: $($moBad -join '; ')" }
    else { Write-Host "OK $n Add-MiOptionsExpect asserts the mimalloc options on Pass and TimeOut cases and keeps fatal cases" }
    $n++
    $sdBad = @()
    $sdNew = { param($name) @(Add-SerialTieExpect @(New-Object PSObject -Property @{ Name = $name; Expect = @{ Class = 'Pass' } }))[0] }
    $sdPlain = & $sdNew 'selftest-case'
    $sdFlipLine = $digLine.Replace('state=0x0000ABCD', 'state=0x0000ABCC')
    $sdRes = New-Object PSObject -Property @{ Class = 'Pass'; CaseDir = $digTmp; LogText = ''; Dumps = @() }
    $sdRun = {
        param($case, $text)
        [IO.File]::WriteAllText((Join-Path $digTmp 'run.log'), $text)
        $sdRes.LogText = $text
        Test-SmokeExpect $case $sdRes
    }
    $env:OPENSWBF2_DIGEST_EXPECTED = $flipExp
    if (& $sdRun $sdPlain "[t=1.000] [f=1] $digLine`r`n") { $sdBad += 'plain digest mismatch accepted' }
    if (-not (& $sdRun $sdPlain "[t=1.000] [f=1] $sdFlipLine`r`n")) { $sdBad += 'plain digest equal to the row rejected' }
    $env:OPENSWBF2_DIGEST_EXPECTED = $savedExpected
    if ($sdBad.Count) { $failed++; Write-Host "FAILED ${n}: digest row expectation: $($sdBad -join '; ')" }
    else { Write-Host "OK $n a pass case compares its digest row" }
    $n++
    $stBad = @()
    $stCfg = $script:SmokeDigestConfiguration
    $script:SmokeDigestConfiguration = 'Test'
    $stTest = & $sdNew 'selftest-norow'
    $stTimeOut = @(Add-SerialTieExpect @(New-Object PSObject -Property @{ Name = 'selftest-norow'; Expect = @{ Class = 'TimeOut' } }))[0]
    $script:SmokeDigestConfiguration = $stCfg
    $stTie = "[t=1.000] [f=1] LogSim Inform CollisionObject.h:1: OPENSWBF2 COLLISION serialties=2`r`n"
    $stZero = $stTie.Replace('serialties=2', 'serialties=0')
    $env:OPENSWBF2_DIGEST_EXPECTED = $flipExp
    if (& $sdRun $stTest $stTie) { $stBad += 'case with collision serial ties accepted' }
    if (-not (& $sdRun $stTest $stZero)) { $stBad += 'case with zero collision serial ties rejected' }
    if (& $sdRun $stTest '') { $stBad += 'test build pass case without a serialties line accepted' }
    if (@($stTimeOut.Expect.LogRx) -contains '(?m)OPENSWBF2 COLLISION serialties=0\r?$') { $stBad += 'serialties line required on a timeout case' }
    if (@($stTimeOut.Expect.LogRx) -notcontains '(?s)\A(?!.*OPENSWBF2 COLLISION serialties=[1-9])') { $stBad += 'timeout case lacks the serialties lookahead' }
    if (@(@($sdPlain.Expect.LogRx) -match 'serialties').Count) { $stBad += 'serialties checked outside a test build' }
    $env:OPENSWBF2_DIGEST_EXPECTED = $savedExpected
    if ($stBad.Count) { $failed++; Write-Host "FAILED ${n}: collision serial ties: $($stBad -join '; ')" }
    else { Write-Host "OK $n test builds fail a case that logs collision serial ties" }
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
        elseif ((Get-GuardRowKind @{ Asan = $true }) -ne 'Asan' -or (Get-GuardRowKind @{ Kind = 'Smoke' }) -ne 'Smoke' -or (Get-GuardRowKind @{ Kind = 'Perturb' }) -ne 'Perturb' -or (Get-GuardRowKind @{ Id = 'x' }) -ne 'T1') { $why = 'row kind' }
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
        @{ Id = 'S-PERTURB'; Kind = 'Perturb'; Config = 'Test'; Missions = @('m1'); File = 'src\G\Game.h'; Anchor = 'a'; Replacement = 'b' },
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
    Test-SelCase '.cpp touched selects its rows and the game read sets that contain it' @{ Hashes = (Copy-SelHashes 'src\G\Game.cpp' 'new') } @('S-HARD', 'S-SMOKE', 'S-PERTURB') 'deps changed'
    Test-SelCase 'header touched selects every read set that contains it' @{ Hashes = (Copy-SelHashes 'src\G\Shared.h' 'new') } @('S-LIB', 'S-PIN', 'S-HDR', 'S-ASAN', 'S-CANARY', 'S-HARD', 'S-SMOKE', 'S-PERTURB') 'deps changed'
    Test-SelCase 'test file touched selects only its cases rows' @{ Hashes = (Copy-SelHashes 'tests\unit\t_pin.cpp' 'new') } @('S-PIN') 'deps changed'
    Test-SelCase 'directory literal file touched selects its test rows' @{ Hashes = (Copy-SelHashes 'tests\corpus\c\1.txt' 'new') } @('S-PIN') 'deps changed'
    $r2 = @($selRows | ForEach-Object { $_.Clone() })
    ($r2 | Where-Object { $_.Id -eq 'S-HARD2' }).Anchor = 'z'
    Test-SelCase 'row edited selects only that row' @{ Rows = $r2 } @('S-HARD2') 'row changed'
    Test-SelCase 'vcxproj touched selects every row' @{ Hashes = (Copy-SelHashes 'BattlefrontII.vcxproj' 'new') } $selAll 'global trigger*'
    Test-SelCase 'smoke.ps1 touched selects every row' @{ Hashes = (Copy-SelHashes 'tests\smoke.ps1' 'new') } $selAll 'global trigger*'
    Test-SelCase 'stubs touched selects every row' @{ Hashes = (Copy-SelHashes 'tests\stubs\stub.cpp' 'new') } $selAll 'global trigger*'
    Test-SelCase 'toolset changed selects every row' @{ Toolset = '14.50.1' } $selAll 'toolset'
    Test-SelCase 'no tlog selects every row that needs one' @{ TestsReads = $null; AsanReads = $null; GameReads = $null } @('S-LIB', 'S-PIN', 'S-HDR', 'S-ASAN', 'S-CANARY', 'S-HARD', 'S-HARD2', 'S-SMOKE', 'S-PERTURB') 'no tlog*'
    Test-SelCase 'unmapped path selects the rows that read it' @{ Hashes = (Copy-SelHashes 'src\G\Game.h' $null) } @('S-HARD', 'S-SMOKE', 'S-PERTURB') 'unmapped *'
    Test-SelCase 'unmapped literal selects its test rows' @{ TestLiterals = @{ 'tests\unit\t_pin.cpp' = @('tests\corpus\c\', 'tests\missing\x.txt') } } @('S-PIN') 'unmapped literal*'
    Test-SelCase 'unknown test name selects its rows' @{ TestSources = @{ 'lib works' = 'tests\unit\t_lib.cpp'; 'pin text' = 'tests\unit\t_pin.cpp' } } @('S-HDR', 'S-CANARY') 'unknown test*'
    Test-SelCase 'text-pin row left alone by an unrelated change' @{ Hashes = (Copy-SelHashes 'src\G\Other.cpp' 'new') } @('S-HARD2', 'S-SMOKE', 'S-PERTURB') 'deps changed'
    Test-SelCase 'smoke row depends on every game TU it links with' @{ Hashes = (Copy-SelHashes 'src\X\pcWindow.cpp' 'new') } @('S-PIN', 'S-SMOKE', 'S-PERTURB') 'deps changed'
    Test-SelCase 'perturb row depends on the mission scripts like a smoke row' @{ Hashes = (Copy-SelHashes 'data\scripts\a.lua' 'new') } @('S-SMOKE', 'S-PERTURB') 'deps changed'
    $p = Join-Path $selRoot 'src\G\Lib.h'
    (Get-Item -LiteralPath $p).LastWriteTime = (Get-Date).AddHours(1)
    $stale = Read-ClReadTlog $testsTlog $selRoot
    Test-SelCase 'stale tlog selects every row that needs the stale TU' @{ TestsReads = $stale; AsanReads = $stale } $selT1 'stale tlog*'
}
catch { $failed++; Write-Host "FAILED: selector block threw: $_" }
finally {
    Remove-Item -LiteralPath $selTmp -Recurse -Force -ErrorAction SilentlyContinue
}
$cbTmp = Join-Path $env:TEMP ("openswbf2-cmptables-" + [guid]::NewGuid().ToString('N'))
try {
    $cbBase = Join-Path $PSScriptRoot 'baseline\v143'
    $cbScript = Join-Path $PSScriptRoot 'baseline\compare-tables.ps1'
    $cbFiles = @('cloth-headroom.txt', 'mem-peaks.txt', 'pool-sizes.txt', 'smoke-all-digests.txt')
    $cbText = @{}
    foreach ($fn in $cbFiles) { $cbText[$fn] = [IO.File]::ReadAllLines((Join-Path $cbBase $fn)) }
    function Find-CbRow([string]$fn, [string]$pattern) {
        $lines = $cbText[$fn]
        for ($i = 0; $i -lt $lines.Count; $i++) { if ($lines[$i] -cmatch $pattern) { return $i } }
        throw "no row matching $pattern in $fn"
    }
    function Invoke-CbCase([string]$name, [string]$fn, [scriptblock]$edit) {
        $d = Join-Path $cbTmp $name
        New-Item -ItemType Directory -Force -Path $d | Out-Null
        foreach ($f in $cbFiles) {
            $lines = [System.Collections.Generic.List[string]]::new([string[]]$cbText[$f])
            if ($f -ceq $fn) { & $edit $lines }
            [IO.File]::WriteAllLines((Join-Path $d $f), $lines)
        }
        $out = & { $ErrorActionPreference = 'Continue'; & powershell -NoProfile -ExecutionPolicy Bypass -File $cbScript -Tables $d 2>&1 | Out-String }
        return @{ Code = $LASTEXITCODE; Out = $out }
    }
    function Set-CbField($lines, [int]$row, [int]$col, [long]$delta) {
        $f = $lines[$row] -split "`t"
        $f[$col] = [string]([long]$f[$col] + $delta)
        $lines[$row] = $f -join "`t"
    }
    $cbRuntime = Find-CbRow 'mem-peaks.txt' "^[^`t]+`texit`theap`tRuntime`t"
    $cbLuaHeap = Find-CbRow 'mem-peaks.txt' "^[^`t]+`t[^`t]+`theap`tLua`t"
    $cbLuaBin = Find-CbRow 'mem-peaks.txt' "^[^`t]+`t[^`t]+`tpool`tLua\d+`t"
    $cbCloth = Find-CbRow 'cloth-headroom.txt' "^[^`t]+`t\d+`t"
    $cbPool = Find-CbRow 'pool-sizes.txt' "^[^`t]+`tSetMemoryPoolSize`t[^`t]+`t\d+`t"
    $cbCases = @(
        @{ Name = 'equal'; File = 'mem-peaks.txt'; Edit = { param($l) }; Code = 0; Want = @('\[PASS\] tables:cloth', '\[PASS\] tables:mem-peaks', '\[PASS\] tables:pool-sizes', '\[PASS\] tables:digests') },
        @{ Name = 'runtime'; File = 'mem-peaks.txt'; Edit = { param($l) Set-CbField $l $cbRuntime 7 -1 }; Code = 1; Want = @('\[FAIL\] tables:mem-peaks', '\[PASS\] tables:cloth') },
        @{ Name = 'luaheap'; File = 'mem-peaks.txt'; Edit = { param($l) Set-CbField $l $cbLuaHeap 6 1 }; Code = 1; Want = @('\[FAIL\] tables:mem-peaks') },
        @{ Name = 'luabin'; File = 'mem-peaks.txt'; Edit = { param($l) Set-CbField $l $cbLuaBin 6 -1 }; Code = 1; Want = @('\[FAIL\] tables:mem-peaks') },
        @{ Name = 'clothfree'; File = 'cloth-headroom.txt'; Edit = { param($l) Set-CbField $l $cbCloth 1 1 }; Code = 1; Want = @('\[FAIL\] tables:cloth', '\[PASS\] tables:mem-peaks') },
        @{ Name = 'pool'; File = 'pool-sizes.txt'; Edit = { param($l) Set-CbField $l $cbPool 4 1 }; Code = 1; Want = @('\[FAIL\] tables:pool-sizes', '\[PASS\] tables:mem-peaks') },
        @{ Name = 'missing'; File = 'mem-peaks.txt'; Edit = { param($l) $l.RemoveAt($cbLuaBin) }; Code = 1; Want = @('\[FAIL\] tables:mem-peaks', 'missing from tables') },
        @{ Name = 'extra'; File = 'mem-peaks.txt'; Edit = { param($l) $l.Add("zzz9c_con`texit`theap`tRuntime`t1`t-`t1`t1`t1`t0") }; Code = 1; Want = @('\[FAIL\] tables:mem-peaks', 'zzz9c_con\texit\theap\tRuntime not in baseline') },
        @{ Name = 'pooldup'; File = 'pool-sizes.txt'; Edit = { param($l) $l.Insert($cbPool, $l[$cbPool]) }; Code = 1; Want = @('\[FAIL\] tables:pool-sizes') },
        @{ Name = 'digestnew'; File = 'smoke-all-digests.txt'; Edit = { param($l) $l.Add("zzz9c_con`tnone`tnone") }; Code = 1; Want = @('\[FAIL\] tables:digests', 'zzz9c_con not in baseline') },
        @{ Name = 'dedicated'; File = 'smoke-all-digests.txt'; Edit = { param($l) $i = Find-CbRow 'smoke-all-digests.txt' "^dedicated`t"; $l[$i] = "dedicated`tchanged`tnone" }; Code = 0; Want = @('\[PASS\] tables:digests') },
        @{ Name = 'empty'; File = 'cloth-headroom.txt'; Edit = { param($l) for ($i = $l.Count - 1; $i -ge 0; $i--) { if ($l[$i] -and -not $l[$i].StartsWith('#') -and -not $l[$i].StartsWith("mission`t")) { $l.RemoveAt($i) } } }; Code = 1; Want = @('cloth-headroom\.txt in ', 'has no rows') }
    )
    $cbWhy = $null
    foreach ($c in $cbCases) {
        $r = Invoke-CbCase $c.Name $c.File $c.Edit
        if ($r.Code -ne $c.Code) { $cbWhy = "$($c.Name) gave exit $($r.Code)"; break }
        foreach ($w in $c.Want) { if ($r.Out -notmatch $w) { $cbWhy = "$($c.Name) output lacks '$w'"; break } }
        if ($cbWhy) { break }
    }
    if ($cbWhy) { $failed++; Write-Host "FAILED ${n}: compare-tables: $cbWhy" }
    else { Write-Host "OK $n compare-tables passes equal tables and a changed dedicated row and fails a 1-byte change in a Runtime heap, Lua heap, Lua bin, CLOTH free and pool-sizes row, a missing, extra or duplicated row, a new digest case and an empty table" }
    $n++
    $lbLines = [IO.File]::ReadAllLines((Join-Path $cbBase 'lua-bins.txt'))
    $lbRow = -1
    for ($i = 0; $i -lt $lbLines.Count; $i++) { if ($lbLines[$i] -cmatch "^[^`t]+`t[^`t]+`t\d+`t\d+`t\d+`t\d+`t\d+`t\d+$") { $lbRow = $i; break } }
    $lbWhy = $null
    if ($lbRow -lt 0) { $lbWhy = 'no lua-bins row with a spill count' }
    else {
        $lbSame = Join-Path $cbTmp 'luabins-same'
        $lbSpill = Join-Path $cbTmp 'luabins-spill'
        New-Item -ItemType Directory -Force -Path $lbSame, $lbSpill | Out-Null
        [IO.File]::WriteAllLines((Join-Path $lbSame 'lua-bins.txt'), $lbLines)
        $lbEdit = [string[]]$lbLines.Clone()
        $f = $lbEdit[$lbRow] -split "`t"
        $f[7] = [string]([long]$f[7] + 1)
        $lbEdit[$lbRow] = $f -join "`t"
        [IO.File]::WriteAllLines((Join-Path $lbSpill 'lua-bins.txt'), $lbEdit)
        $lbOk = & { $ErrorActionPreference = 'Continue'; & powershell -NoProfile -ExecutionPolicy Bypass -File $cbScript -Tables $lbSame -LuaBins 2>&1 | Out-String }
        $lbOkCode = $LASTEXITCODE
        $lbBad = & { $ErrorActionPreference = 'Continue'; & powershell -NoProfile -ExecutionPolicy Bypass -File $cbScript -Tables $lbSpill -LuaBins 2>&1 | Out-String }
        $lbBadCode = $LASTEXITCODE
        if ($lbOkCode -ne 0 -or $lbOk -notmatch '\[PASS\] tables:lua-bins' -or $lbOk -match 'tables:mem-peaks') { $lbWhy = "equal table gave exit $lbOkCode" }
        elseif ($lbBadCode -ne 1 -or $lbBad -notmatch '\[FAIL\] tables:lua-bins') { $lbWhy = "spill change gave exit $lbBadCode" }
    }
    if ($lbWhy) { $failed++; Write-Host "FAILED ${n}: compare-tables lua-bins: $lbWhy" }
    else { Write-Host "OK $n compare-tables -LuaBins passes an equal lua-bins table and fails a 1-count spill change" }
    $n++
}
catch { $failed++; Write-Host "FAILED: compare-tables block threw: $_" }
finally {
    Remove-Item -LiteralPath $cbTmp -Recurse -Force -ErrorAction SilentlyContinue
}
if ($failed -gt 0) { exit 1 }
exit 0
