param([string]$Configuration = 'Debug', [ValidateSet('Win32', 'x64')][string]$Platform = 'Win32')
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'smoke.ps1')
$script:SmokePlatform = $Platform
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
        $extra = @()
        if ($sc.Count -gt 7) { $extra = $sc[7] }
        $out = (& powershell -NoProfile -File $run -Platform $Platform -NoBuild -Smoke None -Suite $sc[1] @extra 2>&1 | Out-String)
        $code = $LASTEXITCODE
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
$skipTmp = Join-Path $env:TEMP ("openswbf2-t1skip-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $skipTmp | Out-Null
$skipFixtures = @(
    @('t1 platform skip: on x64 both math corpus cases print SKIP and do not run', 'x64', @('math corpus', 'math corpus x87 pc24'), 0, @('\[SKIP\] math corpus - Phase 48\r?\n', '\[SKIP\] math corpus x87 pc24 - Phase 48\r?\n'), @('\[(?:PASS|FAIL|CRASH|STALL)\] math corpus', 't1-skip:'), @(), @('math corpus', 'math corpus x87 pc24')),
    @('t1 platform skip: on Win32 both math corpus cases run', 'Win32', @('math corpus', 'math corpus x87 pc24'), 1, @('\[FAIL\] math corpus \(', '\[FAIL\] math corpus x87 pc24 \('), @('\[SKIP\] math corpus', 't1-skip:'), @('math corpus', 'math corpus x87 pc24'), @()),
    @('t1 platform skip: on x64 a case not named in the list is not skipped', 'x64', @('math corpus', 'math corpus x87 pc24', 'math corpus x87', 'MATH CORPUS'), 1, @('\[SKIP\] math corpus - Phase 48\r?\n', '\[SKIP\] math corpus x87 pc24 - Phase 48\r?\n', '\[FAIL\] math corpus x87 \(', '\[FAIL\] MATH CORPUS \('), @('\[SKIP\] math corpus x87 -', '\[SKIP\] MATH CORPUS'), @('math corpus x87', 'MATH CORPUS'), @('math corpus', 'math corpus x87 pc24')),
    @('t1 platform skip: on x64 a listed name the tests exe lacks fails the run', 'x64', @('math corpus'), 1, @('\[FAIL\] t1-skip:math corpus x87 pc24 - not a test case of ', '\[SKIP\] math corpus - Phase 48\r?\n'), @('t1-skip:math corpus -'), @(), @('math corpus'))
)
try {
    foreach ($sf in $skipFixtures) {
        $n++
        $callLog = Join-Path $skipTmp "calls$n.txt"
        $corpus = (@($sf[2]) | ForEach-Object { "    echo $_" }) -join "`r`n"
        $stub = Join-Path $skipTmp "skip$n.cmd"
        $body = "@echo off`r`n>>""$callLog"" echo %*`r`necho %* | findstr /C:""list-test-cases"" >nul || goto run`r`necho ==========`r`necho case %2`r`nif ""%2""==""corpus"" (`r`n$corpus`r`n)`r`necho ==========`r`nexit /b 0`r`n:run`r`necho %* | findstr /I /C:""math corpus"" >nul && (echo [doctest] test cases: 1 ^| 0 passed ^| 1 failed& exit /b 1)`r`n$onePass`r`nexit /b 0"
        Set-Content -Path $stub -Value $body -Encoding ASCII
        $env:OPENSWBF2_TEST_EXE = $stub
        $out = (& powershell -NoProfile -File $run -Platform $sf[1] -NoBuild -Smoke None 2>&1 | Out-String)
        $code = $LASTEXITCODE
        $calls = @()
        if (Test-Path -LiteralPath $callLog) { $calls = @(Get-Content -LiteralPath $callLog | ForEach-Object { $_.Trim() }) }
        $why = $null
        if ($code -ne $sf[3]) { $why = "exit $code, expected $($sf[3])" }
        foreach ($rx in $sf[4]) { if (-not $why -and $out -cnotmatch $rx) { $why = "output missing /$rx/" } }
        foreach ($rx in $sf[5]) { if (-not $why -and $out -cmatch $rx) { $why = "output unexpectedly matches /$rx/" } }
        foreach ($nm in $sf[6]) { if (-not $why -and $calls -cnotcontains """-tc=$nm""") { $why = "case '$nm' did not run" } }
        foreach ($nm in $sf[7]) { if (-not $why -and $calls -ccontains """-tc=$nm""") { $why = "case '$nm' ran" } }
        if ($why) { $failed++; Write-Host "FAILED ${n}: $($sf[0]): $why" }
        else { Write-Host "OK $n $($sf[0])" }
    }
}
finally {
    Remove-Item Env:\OPENSWBF2_TEST_EXE -ErrorAction SilentlyContinue
    Remove-Item -Recurse -Force $skipTmp -ErrorAction SilentlyContinue
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
        $out = (& powershell -NoProfile -File $run -Platform $Platform -Configuration Asan -NoBuild -Smoke None -Suite asan_canary -Filter $canary[0] 2>&1 | Out-String)
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
    @('smoke exit 0 without marker', 'exit /b 0', 'InitFailure'),
    @('smoke fast-fail code', 'exit /b 3221226505', 'GsFailure'),
    @('smoke exit 3', 'exit /b 3', 'Fatal'),
    @('smoke exit 4', 'exit /b 4', 'Fatal'),
    @('smoke marker in log', 'echo OPENSWBF2 RUN COMPLETE>%2', 'Pass'),
    @('smoke process without a progress section is killed at its first stall', "ping -n 31 127.0.0.1 >nul`r`nexit /b 0", 'Stall')
)
$savedStallSeconds = $script:StallSeconds
$script:StallSeconds = 3
try {
    foreach ($sc in $smokeScenarios) {
        $n++
        $stub = Join-Path $smokeTmp "smoke$n.cmd"
        Set-Content -Path $stub -Value ("@echo off`r`n" + $sc[1]) -Encoding ASCII
        $caseDir = Join-Path $smokeTmp "case$n"
        New-Item -ItemType Directory -Path $caseDir | Out-Null
        $r = Invoke-SmokeCase "t$n" $stub $smokeTmp ('-log "' + $caseDir + '\run.log"') $caseDir
        $why = $null
        if ($r.Class -ne $sc[2]) { $why = "class $($r.Class), expected $($sc[2])" }
        elseif ($r.Stall -ne [int]($sc[2] -eq 'Stall')) { $why = "stall=$($r.Stall)" }
        elseif ($sc[2] -eq 'Stall' -and @(Get-CimInstance Win32_Process -Filter "Name='PING.EXE'" | Where-Object { $_.CommandLine -like '*-n 31 127.0.0.1*' }).Count -gt 0) { $why = 'child ping survived the kill' }
        elseif ($sc[2] -eq 'Stall' -and ([IO.File]::ReadAllText((Join-Path $caseDir 'result.txt')) -notmatch "stall=1\r\nsilent_max_s=3\.\d\r\nturns=-\r\nsteps=-")) { $why = 'result.txt lacks stall=1, silent_max_s=3.x, turns=- and steps=-' }
        if ($why) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $why" }
        else { Write-Host "OK $n $($sc[0])" }
    }
    $stallFixtures = @(
        @('stall clock: changed turns or steps reset the silence', @{ Progress = '1/1'; Seen = $true; CpuMs = 100.0; SilenceMs = 2500.0; SilentMaxMs = 2500.0; Stalled = $false; Kill = $false }, @(@{ Progress = '1/2'; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 150.0; IntervalMs = 1000.0 }), 0.0, 2500.0, $false, $false),
        @('stall clock: unchanged progress with the main thread in Wait adds the whole interval', @{ Progress = '1/1'; Seen = $true; CpuMs = 100.0; SilenceMs = 0.0; SilentMaxMs = 0.0; Stalled = $false; Kill = $false }, @(@{ Progress = '1/1'; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 150.0; IntervalMs = 1000.0 }), 1000.0, 1000.0, $false, $false),
        @('stall clock: unchanged progress with the main thread Running adds its CPU time, not the interval', @{ Progress = '1/1'; Seen = $true; CpuMs = 100.0; SilenceMs = 0.0; SilentMaxMs = 0.0; Stalled = $false; Kill = $false }, @(@{ Progress = '1/1'; State = 'Running'; WaitReason = $null; CpuMs = 400.0; IntervalMs = 1000.0 }), 300.0, 300.0, $false, $false),
        @('stall clock: a Ready main thread adds only its CPU time', @{ Progress = '1/1'; Seen = $true; CpuMs = 100.0; SilenceMs = 500.0; SilentMaxMs = 500.0; Stalled = $false; Kill = $false }, @(@{ Progress = '1/1'; State = 'Ready'; WaitReason = $null; CpuMs = 100.0; IntervalMs = 1000.0 }, @{ Progress = '1/1'; State = 'Ready'; WaitReason = $null; CpuMs = 300.0; IntervalMs = 1000.0 }), 700.0, 700.0, $false, $false),
        @('stall clock: a Standby main thread adds only its CPU time', @{ Progress = '1/1'; Seen = $true; CpuMs = 100.0; SilenceMs = 0.0; SilentMaxMs = 0.0; Stalled = $false; Kill = $false }, @(@{ Progress = '1/1'; State = 'Standby'; WaitReason = $null; CpuMs = 100.0; IntervalMs = 1000.0 }), 0.0, 0.0, $false, $false),
        @('stall clock: a Transition main thread adds only its CPU time', @{ Progress = '1/1'; Seen = $true; CpuMs = 100.0; SilenceMs = 0.0; SilentMaxMs = 0.0; Stalled = $false; Kill = $false }, @(@{ Progress = '1/1'; State = 'Transition'; WaitReason = $null; CpuMs = 150.0; IntervalMs = 1000.0 }), 50.0, 50.0, $false, $false),
        @('stall clock: a waiting main thread while another thread of the process is Ready adds only the process CPU time', @{ Progress = '1/1'; Seen = $true; CpuMs = 100.0; ProcCpuMs = 100.0; SilenceMs = 0.0; SilentMaxMs = 0.0; Stalled = $false; Kill = $false }, @(@{ Progress = '1/1'; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 100.0; ProcCpuMs = 250.0; OtherReady = $true; IntervalMs = 1000.0 }, @{ Progress = '1/1'; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 100.0; ProcCpuMs = 260.0; OtherReady = $false; IntervalMs = 1000.0 }), 1150.0, 1150.0, $false, $false),
        @('stall clock: a main thread waiting for a page adds only its CPU time', @{ Progress = '1/1'; Seen = $true; CpuMs = 100.0; SilenceMs = 0.0; SilentMaxMs = 0.0; Stalled = $false; Kill = $false }, @(@{ Progress = '1/1'; State = 'Wait'; WaitReason = 'PageIn'; CpuMs = 120.0; IntervalMs = 1000.0 }, @{ Progress = '1/1'; State = 'Wait'; WaitReason = 'FreePage'; CpuMs = 120.0; IntervalMs = 1000.0 }), 20.0, 20.0, $false, $false),
        @('stall clock: without a section progress counts as unchanged and the first stall kills', (New-StallState), @(@{ Progress = $null; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 0.0; IntervalMs = 1000.0 }, @{ Progress = $null; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 0.0; IntervalMs = 1000.0 }, @{ Progress = $null; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 0.0; IntervalMs = 1000.0 }), 0.0, 3000.0, $true, $true),
        @('stall clock: without a section silence below the window is no stall', (New-StallState), @(@{ Progress = $null; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 0.0; IntervalMs = 1000.0 }, @{ Progress = $null; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 0.0; IntervalMs = 1000.0 }), 2000.0, 2000.0, $false, $false),
        @('stall clock: with a section the first stall is reported once and does not kill', @{ Progress = '1/1'; Seen = $true; CpuMs = 0.0; SilenceMs = 0.0; SilentMaxMs = 0.0; Stalled = $false; Kill = $false }, @(@{ Progress = '1/1'; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 0.0; IntervalMs = 2000.0 }, @{ Progress = '1/1'; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 0.0; IntervalMs = 1500.0 }, @{ Progress = '1/1'; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 0.0; IntervalMs = 1000.0 }), 1000.0, 3500.0, $true, $false),
        @('stall clock: after the stall a second window of silence kills', @{ Progress = '1/1'; Seen = $true; CpuMs = 0.0; SilenceMs = 0.0; SilentMaxMs = 3000.0; Stalled = $true; Kill = $false }, @(@{ Progress = '1/1'; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 0.0; IntervalMs = 1000.0 }, @{ Progress = '1/1'; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 0.0; IntervalMs = 1000.0 }, @{ Progress = '1/1'; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 0.0; IntervalMs = 1000.0 }), 0.0, 3000.0, $true, $true),
        @('stall clock: silent max keeps the largest silence across a reset', @{ Progress = '1/1'; Seen = $true; CpuMs = 0.0; SilenceMs = 0.0; SilentMaxMs = 0.0; Stalled = $false; Kill = $false }, @(@{ Progress = '1/1'; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 0.0; IntervalMs = 2000.0 }, @{ Progress = '1/2'; State = 'Wait'; WaitReason = 'UserRequest'; CpuMs = 0.0; IntervalMs = 1000.0 }, @{ Progress = '1/2'; State = 'Running'; WaitReason = $null; CpuMs = 500.0; IntervalMs = 1000.0 }), 500.0, 2000.0, $false, $false)
    )
    foreach ($sf in $stallFixtures) {
        $n++
        $st = $sf[1]
        foreach ($smp in $sf[2]) { $st = Step-StallClock $st $smp }
        if ($st.SilenceMs -ne $sf[3] -or $st.SilentMaxMs -ne $sf[4] -or $st.Stalled -ne $sf[5] -or $st.Kill -ne $sf[6]) { $failed++; Write-Host "FAILED ${n}: $($sf[0]): silence=$($st.SilenceMs) max=$($st.SilentMaxMs) stalled=$($st.Stalled) kill=$($st.Kill), expected $($sf[3]) $($sf[4]) $($sf[5]) $($sf[6])" }
        else { Write-Host "OK $n $($sf[0])" }
    }
    $psExe = Join-Path $env:windir 'System32\WindowsPowerShell\v1.0\powershell.exe'
    $sectionHead = "param(`$log)`r`n`$m = [IO.MemoryMappedFiles.MemoryMappedFile]::CreateNew(""Local\OpenSWBF2Progress.`$PID"", 4096)`r`n`$v = `$m.CreateViewAccessor(0, 12)`r`n"
    $spinExe = Join-Path $smokeTmp 'stallstub.exe'
    Add-Type -OutputAssembly $spinExe -OutputType ConsoleApplication -ReferencedAssemblies 'System.Core' -TypeDefinition @'
public static class StallStub
{
    public static int Main(string[] a)
    {
        var m = System.IO.MemoryMappedFiles.MemoryMappedFile.CreateNew("Local\\OpenSWBF2Progress." + System.Diagnostics.Process.GetCurrentProcess().Id, 4096);
        var v = m.CreateViewAccessor(0, 12);
        v.Write(4, 1);
        bool t1 = a[0].StartsWith("-tc=");
        string mode = t1 ? a[0].Substring(4) : a[1];
        if (t1) { System.Console.WriteLine("[RUN] " + mode); }
        if (mode == "quick")
        {
            v.Write(0, 7);
            System.IO.File.WriteAllText(a[0], "OPENSWBF2 RUN COMPLETE\r\n");
            System.Threading.Thread.Sleep(150);
            return 0;
        }
        if (mode == "flood")
        {
            var w = new System.IO.StreamWriter(System.Console.OpenStandardOutput());
            string pad = new string('x', 90);
            for (int i = 0; i < 40000; i++) { w.WriteLine("line " + i.ToString("D5") + " " + pad); }
            w.WriteLine("[doctest] test cases: 1 | 1 passed | 0 failed");
            w.Flush();
            return 0;
        }
        if (mode == "answer")
        {
            while (v.ReadInt32(8) == 0) { System.Threading.Thread.Sleep(100); }
            if (t1)
            {
                System.Console.WriteLine("[STALL] " + mode + " label");
                return 124;
            }
            System.IO.File.WriteAllText(a[0], "FATAL: hang (supervisor saw no progress, last frame 1)\r\n");
            return 5;
        }
        long x = 0;
        while (true) { x++; }
    }
}
'@
    $stubRuns = @(
        @('stall stub: steady progress keeps a run alive past the window', $psExe, ($sectionHead + "for (`$i = 1; `$i -le 40; `$i++) { `$v.Write(4, [int]`$i); Start-Sleep -Milliseconds 200 }`r`nSet-Content -LiteralPath `$log -Value 'OPENSWBF2 RUN COMPLETE'`r`nexit 0"), 'Pass', 0, $null),
        @('stall stub: a section without progress and a main thread burning CPU is killed after two windows', $spinExe, 'spin', 'Stall', 1, $null),
        @('stall stub: a stub that answers the stall flag is classified by its own exit', $spinExe, 'answer', 'Fatal', 1, @{ Class = 'Fatal'; Code = 5; LogRx = 'FATAL: hang \(supervisor saw no progress' })
    )
    foreach ($stubRun in $stubRuns) {
        $n++
        $sr = @($stubRun[0], $null, $stubRun[3], $stubRun[4], $stubRun[5])
        $caseDir = Join-Path $smokeTmp "case$n"
        New-Item -ItemType Directory -Path $caseDir | Out-Null
        if ($stubRun[1] -eq $psExe) {
            $stub = Join-Path $smokeTmp "stall$n.ps1"
            Set-Content -Path $stub -Value $stubRun[2] -Encoding ASCII
            $stubArgs = '-NoProfile -ExecutionPolicy Bypass -File "' + $stub + '" "' + $caseDir + '\run.log"'
        }
        else { $stubArgs = '"' + $caseDir + '\run.log" ' + $stubRun[2] }
        $r = Invoke-SmokeCase "t$n" $stubRun[1] $smokeTmp $stubArgs $caseDir
        $why = $null
        if ($r.Class -ne $sr[2]) { $why = "class $($r.Class), expected $($sr[2])" }
        elseif ($r.Stall -ne $sr[3]) { $why = "stall=$($r.Stall), expected $($sr[3])" }
        elseif ($r.Steps -eq '-') { $why = 'steps were not read from the section' }
        elseif ($sr[4]) {
            $xc = New-Object PSObject -Property @{ Name = "t$n"; Expect = $sr[4] }
            $xs = New-Object PSObject -Property @{ Name = "t$n"; Expect = ($sr[4].Clone()) }
            $xs.Expect.Stall = 1
            if (Test-SmokeExpect $xc $r) { $why = 'Test-SmokeExpect passed a stall=1 result without Expect.Stall' }
            elseif (-not (Test-SmokeExpect $xs $r)) { $why = 'Test-SmokeExpect failed a stall=1 result with Expect.Stall = 1' }
            elseif (-not (Test-SmokeExpect $xc (New-Object PSObject -Property @{ Class = 'Fatal'; Code = $r.Code; Dumps = @(); LogText = $r.LogText; CaseDir = $r.CaseDir; Stall = 0 }))) { $why = 'Test-SmokeExpect failed the same result with stall=0' }
        }
        if ($why) { $failed++; Write-Host "FAILED ${n}: $($sr[0]): $why" }
        else { Write-Host "OK $n $($sr[0])" }
    }
    $t1Runs = @(
        @('t1 stall: a tests exe that answers the stall flag is STALL with its own [STALL] line in run.ps1 and RED stall in guard-revert', 'answer', 'answer label', $false),
        @('t1 stall: a tests exe that never answers the stall flag is killed, STALL in run.ps1 and RED stall in guard-revert', 'spin', 'no answer to the stall flag', $true)
    )
    foreach ($tr in $t1Runs) {
        $n++
        $res = Invoke-T1Exe $spinExe $smokeTmp ('"-tc=' + $tr[1] + '"') $null
        $tok = Get-T1Token $res
        $gs = Get-GuardT1State $res $false
        $why = $null
        if (-not $res.Stalled) { $why = 'the detector reported no stall' }
        elseif ($res.Killed -ne $tr[3]) { $why = "killed=$($res.Killed), expected $($tr[3])" }
        elseif (-not $tr[3] -and $res.Code -ne 124) { $why = "exit $($res.Code), expected the exe's own 124" }
        elseif ($res.Text -notmatch "\[RUN\] $($tr[1])") { $why = 'stdout lacks the [RUN] line' }
        elseif ($tok.Token -ne 'STALL' -or $tok.Detail -ne $tr[2]) { $why = "run.ps1 token $($tok.Token) detail '$($tok.Detail)'" }
        elseif ($gs.State -ne 'RED' -or $gs.Detail -ne 'stall' -or $gs.Code -ne 124) { $why = "guard-revert state $($gs.State) detail '$($gs.Detail)' code $($gs.Code)" }
        if ($why) { $failed++; Write-Host "FAILED ${n}: $($tr[0]): $why" }
        else { Write-Host "OK $n $($tr[0])" }
    }
    $n++
    $caseDir = Join-Path $smokeTmp "case$n"
    New-Item -ItemType Directory -Path $caseDir | Out-Null
    $r = Invoke-SmokeCase "t$n" $spinExe $smokeTmp ('"' + $caseDir + '\run.log" quick') $caseDir
    if ($r.Class -ne 'Pass' -or $r.Turns -ne '7' -or $r.Stall -ne 0) { $failed++; Write-Host "FAILED ${n}: a process that exits within the first second still has its progress read: class $($r.Class) turns '$($r.Turns)' stall $($r.Stall)" }
    else { Write-Host "OK $n a process that exits within the first second still has its progress read" }
    $n++
    $res = Invoke-T1Exe $spinExe $smokeTmp '"-tc=flood"' $null
    $tok = Get-T1Token $res
    $lastLine = [regex]::Match($res.Text, '(line \d{5}) x+\r?\n\[doctest\] test cases: 1 \| 1 passed \| 0 failed')
    $why = $null
    if ($tok.Token -ne 'PASS') { $why = "token $($tok.Token)" }
    elseif ($res.Omitted -le 0 -or $res.Text -notmatch "\[OUTPUT TRUNCATED: $($res.Omitted) bytes omitted\]") { $why = "omitted $($res.Omitted), no truncation marker" }
    elseif ($res.Text.Length -gt $script:OutputHeadBytes + $script:OutputTailBytes + 100) { $why = "kept $($res.Text.Length) characters" }
    elseif (-not $res.Text.StartsWith('[RUN] flood')) { $why = 'the head lost the [RUN] line' }
    elseif (-not $lastLine.Success -or $lastLine.Groups[1].Value -cne 'line 39999') { $why = 'the tail lost the last line or the summary' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: t1 output past the kept head and tail is cut with a marker and the verdict survives: $why" }
    else { Write-Host "OK $n t1 output past the kept head and tail is cut with a marker and the verdict survives" }
    $n++
    Initialize-OutputCapture
    $capWhy = $null
    foreach ($cc in @(@('0123456789', "012`r`n[OUTPUT TRUNCATED: 3 bytes omitted]`r`n6789", 3), @('0123456', '0123456', 0), @('01', '01', 0), @('', '', 0))) {
        $ms = New-Object IO.MemoryStream (, [Text.Encoding]::ASCII.GetBytes($cc[0]))
        $cap = [OpenSWBF2OutputCapture]::new($ms, [Text.Encoding]::ASCII, 3, 4)
        if ($cap.Text -cne $cc[1] -or $cap.Omitted -ne $cc[2] -or -not $cap.Complete) { $capWhy = "'$($cc[0])' gave '$($cap.Text)' omitted $($cap.Omitted) complete $($cap.Complete)"; break }
    }
    if ($capWhy) { $failed++; Write-Host "FAILED ${n}: output capture keeps head and tail and counts the cut bytes: $capWhy" }
    else { Write-Host "OK $n output capture keeps head and tail and counts the cut bytes" }
    $script:StallSeconds = $savedStallSeconds
    $n++
    $t1Pure = @(
        @(@{ Code = [uint32]124; Text = '[STALL] x y'; Stalled = $false; Killed = $false }, 'CRASH', '0x0000007C', 'RED', 'crash 0x0000007C'),
        @(@{ Code = [uint32]0; Text = '[doctest] test cases: 1 | 1 passed | 0 failed'; Stalled = $true; Killed = $false }, 'PASS', '', 'GREEN', ''),
        @(@{ Code = [uint32]1; Text = ''; Stalled = $true; Killed = $false }, 'FAIL', '', 'RED', ''),
        @(@{ Code = [uint32]1; Text = '[STALL] x y'; Stalled = $true; Killed = $true }, 'STALL', 'x y', 'RED', 'stall')
    )
    $why = $null
    foreach ($tp in $t1Pure) {
        $res = New-Object PSObject -Property $tp[0]
        $tok = Get-T1Token $res
        $gs = Get-GuardT1State $res $false
        if ($tok.Token -ne $tp[1] -or $tok.Detail -ne $tp[2] -or $gs.State -ne $tp[3] -or $gs.Detail -ne $tp[4]) { $why = "code=$($tp[0].Code) stalled=$($tp[0].Stalled) killed=$($tp[0].Killed): $($tok.Token) '$($tok.Detail)' / $($gs.State) '$($gs.Detail)'"; break }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: t1 verdict is the exe's own output unless the detector stalled it: $why" }
    else { Write-Host "OK $n t1 verdict is the exe's own output unless the detector stalled it" }
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
    $rtW = @(Get-PlatformRuntimes @('a.dll') 'Win32') -join ','
    $rtX = @(Get-PlatformRuntimes @('a.dll') 'x64') -join ','
    if ($rtW -cne 'a.dll' -or $rtX -cne 'a.dll,vcruntime140_1.dll') { $failed++; Write-Host "FAILED ${n}: runtime list per platform: Win32 '$rtW', x64 '$rtX'" }
    else { Write-Host "OK $n x64 runtime preflight adds vcruntime140_1.dll and the Win32 list is unchanged" }
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
        $r = Invoke-SmokeCase "t$n" $stub $smokeTmp ('-log "' + $caseDir + '\run.log"') $caseDir (Get-AsanOptions $caseDir)
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
    $r = Invoke-SmokeCase "t$n" $stub $smokeTmp ('-log "' + $caseDir + '\run.log"') $caseDir
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
    if ($excl.Count -ne 18 -or $missing.Count -gt 0) { $failed++; Write-Host "FAILED ${n}: asan exclusions: count $($excl.Count), unknown '$($missing -join ', ')'" }
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
$exeConfigs = @(@('Debug', $Configuration) | Select-Object -Unique)
foreach ($peCfg in $exeConfigs) {
    $n++
    $peSrc = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot "..\bin\$Platform\$peCfg\BattlefrontII.exe"))
    if (-not (Test-Path -LiteralPath $peSrc)) { $failed++; Write-Host "FAILED ${n}: pe aslr fixtures: $peSrc not built" }
    else {
        $fs = [IO.File]::OpenRead($peSrc)
        try {
            $peHdr = New-Object byte[] 4096
            $peLen = $fs.Read($peHdr, 0, $peHdr.Length)
        }
        finally { $fs.Dispose() }
        $peE = [BitConverter]::ToInt32($peHdr, 0x3C)
        $peOpt = $peE + 24
        $peMagic = [BitConverter]::ToUInt16($peHdr, $peOpt)
        $peReloc = $peOpt + 136
        if ($peMagic -eq 0x20B) { $peReloc = $peOpt + 152 }
        $peCases = @(
            @('clean header', @(), $null),
            @('dynamic base cleared', @(, @(($peOpt + 70), 0xBF, 0)), 'DYNAMIC_BASE is clear'),
            @('relocation directory zeroed', @(0..7 | ForEach-Object { , @(($peReloc + $_), 0, 0) }), 'base relocation directory is empty'),
            @('relocs stripped set', @(, @(($peE + 22), 0xFF, 0x01)), 'IMAGE_FILE_RELOCS_STRIPPED is set'),
            @('ROM magic', @(@($peOpt, 0, 0x07), @(($peOpt + 1), 0, 0x01)), 'magic 0x107 is not PE32 or PE32+')
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
        else { Write-Host ("OK $n pe aslr fixtures flag dynamic base, relocations, relocs stripped and a bad magic from $(Split-Path $peSrc -Leaf) ($peCfg, magic 0x{0:X})" -f $peMagic) }
    }
}
function New-FlagBag($total, $an, $il, $io, $extra) {
    $p = @{ Total = $total; GsMinus = 0; Sdl = $total; AlignedNewOff = $an; InlineOff = $il; InlineOn = $io; ThrowingNew = 0; Gl = 0; FpFast = 0; FpContract = 0; We4311 = $total; We4312 = $total; We4302 = $total; We4477 = $total; WeAll = $total; WeMissing = @() }
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
        'm = (IMAGE_NT_HEADERS*)((char*)pBase + 0x3c);',
        'g = ((Damageable*)((char*)pUnit->GetGameObject() + 320))->Vanish();',
        'e = (Node*)(pList->Head(1) + 2);',
        'c = (char*)pRec->Buf()[0] + 4;',
        'b = Get(a)->Value() + 2;'
    )
    $allow = @('src\x.cpp|(Foo*)((char*)pAllowed + 4)', 'src\x.cpp|(Foo*)( (char*)pSpaced + 4 )', 'src\x.cpp|(Foo*)((char*)pMissing + 4)', 'src\y.cpp|(Foo*)((char*)pAllowed + 4)')
    $sc = @(Get-SourceChecks $offsetDir @() $allow | Where-Object { $_.Name -eq 'offset-hack' })
    $want = ((@(1, 2, 3, 4, 5, 6, 7, 14, 19, 20, 21, 22) | ForEach-Object { "src\x.cpp:$_" }) + 'unused allow src\x.cpp|(Foo*)((char*)pMissing + 4)' + 'unused allow src\y.cpp|(Foo*)((char*)pAllowed + 4)') -join ', '
    if ($sc.Count -ne 1 -or $sc[0].Ok -or $sc[0].Detail -ne $want) { $failed++; Write-Host "FAILED ${n}: offset-hack flags every byte-offset form outside the exact allow-list: $($sc.Detail)" }
    else { Write-Host "OK $n offset-hack flags every byte-offset form outside the exact allow-list" }
}
finally {
    Remove-Item -Recurse -Force $offsetDir -ErrorAction SilentlyContinue
}
$n++
$sizeDir = Join-Path $env:TEMP ("openswbf2-sizelit-" + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path (Join-Path $sizeDir 'src\a'), (Join-Path $sizeDir 'src\LuaFL') | Out-Null
    Set-Content -Encoding ASCII -Path (Join-Path $sizeDir 'src\a\x.cpp') -Value @(
        'MemoryPool Foo::sMemoryPool("Foo", 0x30);',
        'MemoryPool Bar::sMemoryPool("Bar", sizeof(Bar));',
        'MemoryPool Baz::sMemoryPool("Baz", 0);',
        'FreeList Foo::smFreeList(0xcc, 1, true, 4);',
        'FreeList Bar::smFreeList(sizeof(Bar), 1, true, 4);',
        'gPool.Setup(uCount, 0xFD0, 0);',
        'Agent::sMemoryPool.Setup(600, 0x358, Agent::sMemoryPool.GetGrow());',
        'gPool.Setup(Count(), sizeof(Foo), 0);',
        'pPool->Create(2, 16, 0);',
        'mLock.Create(1, 1);',
        'Foo::SetupMemoryPool(16, 0x40, Foo::sMemoryPool.GetGrow());',
        'mValue.Setup(1.0f, 2.0f, 3.0f);',
        '// gPool.Setup(uCount, 0xFD0, 0);',
        'MemoryPool kAllowed("allowed", 16);'
    )
    Set-Content -Encoding ASCII -Path (Join-Path $sizeDir 'src\LuaFL\l.cpp') -Value 'MemoryPool Vendor::sMemoryPool("Vendor", 8);'
    $allow = @('src\a\x.cpp|MemoryPool kAllowed("allowed", 16)|x64:safe:raw blocks', 'src\a\x.cpp|MemoryPool kMissing("missing", 16)|x64:safe:raw blocks', 'src\a\x.cpp|.Create(9, 9, 9)')
    $c = Get-X64SizeLiteralCheck $sizeDir $allow
    $want = (@('allow without an x64:safe reason src\a\x.cpp|.Create(9, 9, 9)') + (@(1, 4, 6, 7, 9, 11) | ForEach-Object { "src\a\x.cpp:$_" }) + 'unused allow src\a\x.cpp|MemoryPool kMissing("missing", 16)') -join ', '
    $why = $null
    if ($c.Name -ne 'x64-size-literal' -or $c.Ok -or $c.Detail -ne $want) { $why = "ok=$($c.Ok): $($c.Detail)" }
    Set-Content -Encoding ASCII -Path (Join-Path $sizeDir 'src\a\x.cpp') -Value @('MemoryPool Bar::sMemoryPool("Bar", sizeof(Bar));', 'gPool.Setup(uCount, sizeof(Foo), 0);')
    $clean = Get-X64SizeLiteralCheck $sizeDir @()
    if (-not $why -and (-not $clean.Ok -or $clean.Detail)) { $why = "clean tree: ok=$($clean.Ok): $($clean.Detail)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: x64-size-literal flags every literal pool size outside the exact allow-list: $why" }
    else { Write-Host "OK $n x64-size-literal flags every literal pool size outside the exact allow-list" }
}
finally {
    Remove-Item -Recurse -Force $sizeDir -ErrorAction SilentlyContinue
}
$n++
$verdictDir = Join-Path $env:TEMP ("openswbf2-verdict-" + [guid]::NewGuid().ToString('N'))
try {
    New-Item -ItemType Directory -Path (Join-Path $verdictDir 'src') | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $verdictDir 'tests') | Out-Null
    Set-Content -Encoding ASCII -Path (Join-Path $verdictDir 'src\x.cpp') -Value @(
        'unsigned char acSafe[0x10];',
        'unsigned char acBare[0x10];',
        'char mOr[0x10 | 0x1];',
        'q = (Foo*)((char*)pA + (iA | iB));'
    )
    $bad = @(Get-SourceChecks $verdictDir @('src\x.cpp|acSafe|[0x10]|x64:safe:byte buffer', 'src\x.cpp|acBare|[0x10]', 'src\x.cpp|mOr|[0x10|0x1]|x64:unsafe:bits') @('src\x.cpp|(Foo*)((char*)pA + (iA | iB))|x64:fixed:', 'src\x.cpp|(Foo*)((char*)pA + 4)|x64:safe: ') -Only 'allow-x64-verdict')
    $wantBad = 'no verdict src\x.cpp|acBare|[0x10], no verdict src\x.cpp|mOr|[0x10|0x1]|x64:unsafe:bits, no verdict src\x.cpp|(Foo*)((char*)pA + (iA | iB))|x64:fixed:, no verdict src\x.cpp|(Foo*)((char*)pA + 4)|x64:safe: '
    $good = @(Get-SourceChecks $verdictDir @('src\x.cpp|acSafe|[0x10]|x64:safe:byte buffer', 'src\x.cpp|acBare|[0x10]|x64:fixed:sized by sizeof', 'src\x.cpp|mOr|[0x10|0x1]|x64:safe:bit mask') @('src\x.cpp|(Foo*)((char*)pA + (iA | iB))|x64:fixed:offset in bytes') -Only @('allow-x64-verdict', 'char-spacer', 'offset-hack'))
    $why = $null
    if ($bad.Count -ne 1 -or $bad[0].Ok -or $bad[0].Detail -cne $wantBad) { $why = "missing, malformed and empty verdicts: $($bad.Detail)" }
    elseif ($good.Count -ne 3 -or @($good | Where-Object { -not $_.Ok }).Count) { $why = "all verdicts, keys with an inner |: $(@($good | ForEach-Object { '{0}={1} {2}' -f $_.Name, $_.Ok, $_.Detail }) -join '; ')" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: allow-x64-verdict: $why" }
    else { Write-Host "OK $n allow-x64-verdict fails a missing, malformed or empty verdict, passes when every entry has one, and keeps a key with an inner | whole" }
}
finally {
    Remove-Item -Recurse -Force $verdictDir -ErrorAction SilentlyContinue
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
$libGood = '/c /Zc:alignedNew- /Zc:inline /sdl /GS C:\X\LIB\B.CPP'
$libScenarios = @(
    @('library tlog all good passes', $libGood, @()),
    @('library tlog /Zc:inline- fails flag-inline', $libGood.Replace('/Zc:inline ', '/Zc:inline- '), @('flag-inline')),
    @('library tlog /GS- fails flag-banned and gs', $libGood.Replace('/GS ', '/GS- '), @('flag-banned', 'gs')),
    @('library tlog without /sdl fails sdl', $libGood.Replace('/sdl ', ''), @('sdl')),
    @('library tlog without /Zc:alignedNew- fails flag-alignednew', $libGood.Replace('/Zc:alignedNew- ', ''), @('flag-alignednew')),
    @('missing mimalloc tlog dir fails mimalloc-defines', $libGood, @('mimalloc-defines', 'flag-alignednew', 'flag-inline'))
)
foreach ($sc in $libScenarios) {
    $n++
    $libRoot = Join-Path $env:TEMP ("openswbf2-libtlog-" + [guid]::NewGuid().ToString('N'))
    try {
        $exeTl = Join-Path $libRoot 'BattlefrontII.tlog'
        $libTl = Join-Path $libRoot 'lib\SoundFL\SoundFL.tlog'
        $miTl = Join-Path $libRoot 'lib\mimalloc\mimalloc.tlog'
        New-Item -ItemType Directory -Force -Path $exeTl, $libTl | Out-Null
        Set-Content -Encoding Unicode -Path (Join-Path $exeTl 'CL.command.1.tlog') -Value @('^C:\X\A.CPP', '/c /Zc:alignedNew- /Zc:inline /sdl /GS C:\X\A.CPP')
        Set-Content -Encoding Unicode -Path (Join-Path $libTl 'CL.command.1.tlog') -Value @('^C:\X\LIB\B.CPP', $sc[1])
        if ($sc[0] -notmatch 'missing mimalloc') {
            New-Item -ItemType Directory -Force -Path $miTl | Out-Null
            Set-Content -Encoding Unicode -Path (Join-Path $miTl 'CL.command.1.tlog') -Value @('^C:\X\SRC\MIMALLOC\SRC\STATIC.C', '/c /Zc:alignedNew- /Zc:inline /sdl /GS /D MI_DEBUG=2 /D MI_SECURE=0 /D MI_NO_GETENV=1 /D MI_USE_RTLGENRANDOM C:\X\SRC\MIMALLOC\SRC\STATIC.C')
        }
        $bag = Get-ClTlogCounts @($exeTl, $libTl, $miTl)
        $got = @(Get-FlagChecks $bag $null 'Debug' 'Debug') + @(Get-HardeningChecks $goodHdr $goodCfg $bag.Total $bag.GsMinus $bag.Sdl)
        $why = $null
        foreach ($c in $got) {
            if (@('flag-alignednew', 'flag-inline', 'flag-banned', 'mimalloc-defines', 'gs', 'sdl') -notcontains $c.Name) { continue }
            $want = $sc[2] -notcontains $c.Name
            if ($c.Ok -ne $want) { $why = "$($c.Name) ok=$($c.Ok): $($c.Detail)" }
        }
        if ($why) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $why" }
        else { Write-Host "OK $n $($sc[0])" }
    }
    finally {
        Remove-Item -Recurse -Force $libRoot -ErrorAction SilentlyContinue
    }
}
$weGood = '/c /Zc:alignedNew- /Zc:inline /sdl /GS /we4311 /we"4312" /we4302 /we4477 C:\X\A.CPP'
$weLuac = @('^C:\X\L.CPP', $weGood.Replace('A.CPP', 'L.CPP'))
$weScenarios = @(
    @('x64-truncation passes plain and quoted /we on every line', 'x64', @('^C:\X\A.CPP', $weGood), $true, $true, $null, $weLuac),
    @('x64-truncation fails a line without /we4477 and names it', 'x64', @('^C:\X\A.CPP', $weGood, '^C:\X\B.CPP', $weGood.Replace(' /we4477', '').Replace('A.CPP', 'B.CPP')), $true, $false, 'first missing C:\X\B.CPP', $weLuac),
    @('x64-truncation fails a /we44770 lookalike', 'x64', @('^C:\X\A.CPP', $weGood.Replace('/we4477', '/we44770')), $true, $false, 'first missing C:\X\A.CPP', $weLuac),
    @('x64-truncation fails without a tests tlog', 'x64', @('^C:\X\A.CPP', $weGood), $false, $false, 'tests cl entries=0', $weLuac),
    @('x64-truncation fails a LuaCompiler line without /we4302 and names it', 'x64', @('^C:\X\A.CPP', $weGood), $true, $false, 'luac cl entries=1 /we4311=1 /we4312=1 /we4302=0 /we4477=1 all four=0 first missing C:\X\L.CPP', @('^C:\X\L.CPP', $weGood.Replace(' /we4302', '').Replace('A.CPP', 'L.CPP'))),
    @('x64-truncation fails without a LuaCompiler tlog', 'x64', @('^C:\X\A.CPP', $weGood), $true, $false, 'luac cl entries=0', $null),
    @('x64-truncation is not emitted on Win32', 'Win32', @('^C:\X\A.CPP', $weGood), $true, $null, $null, $null)
)
foreach ($sc in $weScenarios) {
    $n++
    $weDir = Join-Path $env:TEMP ("openswbf2-we-" + [guid]::NewGuid().ToString('N'))
    $wePlat = $script:SmokePlatform
    try {
        New-Item -ItemType Directory -Path $weDir | Out-Null
        Set-Content -Encoding Unicode -Path (Join-Path $weDir 'CL.command.1.tlog') -Value $sc[2]
        $bag = Get-ClTlogCounts $weDir
        $testsBag = $null
        if ($sc[3]) { $testsBag = New-FlagBag 97 97 0 0 $null }
        $luacBag = $null
        if ($sc[6]) {
            $weLuacDir = Join-Path $weDir 'luac'
            New-Item -ItemType Directory -Path $weLuacDir | Out-Null
            Set-Content -Encoding Unicode -Path (Join-Path $weLuacDir 'CL.command.1.tlog') -Value $sc[6]
            $luacBag = Get-ClTlogCounts $weLuacDir
        }
        $script:SmokePlatform = $sc[1]
        $got = @(Get-FlagChecks $bag $testsBag 'Debug' 'Debug' $luacBag | Where-Object { $_.Name -eq 'x64-truncation' })
        $why = $null
        if ($null -eq $sc[4]) { if ($got.Count) { $why = 'emitted' } }
        elseif ($got.Count -ne 1) { $why = "rows=$($got.Count)" }
        elseif ($got[0].Ok -ne $sc[4]) { $why = "ok=$($got[0].Ok): $($got[0].Detail)" }
        elseif ($sc[5] -and $got[0].Detail.IndexOf($sc[5]) -lt 0) { $why = "detail lacks '$($sc[5])': $($got[0].Detail)" }
        if ($why) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $why" }
        else { Write-Host "OK $n $($sc[0])" }
    }
    finally {
        $script:SmokePlatform = $wePlat
        Remove-Item -Recurse -Force $weDir -ErrorAction SilentlyContinue
    }
}
$objScenarios = @(
    @('x64-objects passes when every unit is built and linked', @('One', 'Two'), @('One', 'Two'), $true, $null),
    @('x64-objects fails a unit with no object and names it', @('One'), @('One', 'Two'), $false, 'TWO.OBJ not built'),
    @('x64-objects fails a stale object the link did not read', @('One', 'Two'), @('One'), $false, 'TWO.OBJ not linked')
)
foreach ($sc in $objScenarios) {
    $n++
    $objRoot = Join-Path $env:TEMP ("openswbf2-x64obj-" + [guid]::NewGuid().ToString('N'))
    $objPlat = $script:SmokePlatform
    try {
        $objInt = Join-Path $objRoot 'obj\x64\Test'
        New-Item -ItemType Directory -Force -Path (Join-Path $objInt 'src\a'), (Join-Path $objInt 'BattlefrontII.tlog') | Out-Null
        Set-Content -Encoding UTF8 -Path (Join-Path $objRoot 'BattlefrontII.vcxproj') -Value @(
            '<?xml version="1.0" encoding="utf-8"?>',
            '<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">',
            '  <ItemGroup>',
            '    <ClCompile Include="src\a\One.cpp" />',
            '    <ClCompile Include="Two.cpp" />',
            '  </ItemGroup>',
            '</Project>'
        )
        $objPath = @{ One = (Join-Path $objInt 'src\a\ONE.OBJ'); Two = (Join-Path $objInt 'TWO.OBJ') }
        foreach ($o in $sc[1]) { Set-Content -Path $objPath[$o] -Value 'obj' -Encoding ASCII }
        Set-Content -Encoding Unicode -Path (Join-Path $objInt 'BattlefrontII.tlog\link.command.1.tlog') -Value @(('^' + ((@($sc[2] | ForEach-Object { $objPath[$_] }) + (Join-Path $objInt 'BATTLEFRONTII.RES')) -join '|')), '/OUT:"X.EXE"')
        $script:SmokePlatform = 'x64'
        $c = Get-X64ObjectsCheck $objRoot 'Test'
        $why = $null
        if ($c.Ok -ne $sc[3]) { $why = "ok=$($c.Ok): $($c.Detail)" }
        elseif ($c.Detail -notmatch '^units=2 ') { $why = "units: $($c.Detail)" }
        elseif ($sc[4] -and $c.Detail.IndexOf($sc[4], [StringComparison]::OrdinalIgnoreCase) -lt 0) { $why = "detail lacks '$($sc[4])': $($c.Detail)" }
        if ($why) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $why" }
        else { Write-Host "OK $n $($sc[0])" }
    }
    finally {
        $script:SmokePlatform = $objPlat
        Remove-Item -Recurse -Force $objRoot -ErrorAction SilentlyContinue
    }
}
$n++
$xhRoot = Join-Path $env:TEMP ("openswbf2-x64hard-" + [guid]::NewGuid().ToString('N'))
try {
    $xhTests = Join-Path $xhRoot 'tests'
    New-Item -ItemType Directory -Force -Path $xhTests | Out-Null
    Get-ChildItem -LiteralPath $PSScriptRoot -Force | Where-Object { $_.Name -ne 'third_party' } | Copy-Item -Destination $xhTests -Recurse
    $xhInt = Join-Path $xhRoot 'obj\x64\Test'
    $xhTlog = Join-Path $xhInt 'BattlefrontII.tlog'
    $xhTt = Join-Path $xhRoot 'obj\Tests\x64\Test\OpenSWBF2Tests.tlog'
    New-Item -ItemType Directory -Force -Path $xhTlog, $xhTt, (Join-Path $xhRoot 'bin\x64\Test'), (Join-Path $xhRoot 'src') | Out-Null
    Set-Content -Encoding UTF8 -Path (Join-Path $xhRoot 'BattlefrontII.vcxproj') -Value @('<?xml version="1.0" encoding="utf-8"?>', '<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003"><ItemGroup><ClCompile Include="A.cpp" /></ItemGroup></Project>')
    Set-Content -Path (Join-Path $xhInt 'A.obj') -Value 'obj' -Encoding ASCII
    Set-Content -Path (Join-Path $xhRoot 'bin\x64\Test\BattlefrontII.exe') -Value 'not a pe' -Encoding ASCII
    Set-Content -Encoding Unicode -Path (Join-Path $xhTlog 'CL.command.1.tlog') -Value @('^C:\X\A.CPP', $weGood)
    Set-Content -Encoding Unicode -Path (Join-Path $xhTt 'CL.command.1.tlog') -Value @('^C:\X\T.CPP', $weGood.Replace('A.CPP', 'T.CPP'))
    $xhLc = Join-Path $xhInt 'LuaCompiler\LuaCompiler.tlog'
    New-Item -ItemType Directory -Force -Path $xhLc | Out-Null
    Set-Content -Encoding Unicode -Path (Join-Path $xhLc 'CL.command.1.tlog') -Value $weLuac
    Set-Content -Encoding Unicode -Path (Join-Path $xhTlog 'link.command.1.tlog') -Value @(('^' + (Join-Path $xhInt 'A.OBJ')), '/OUT:"X.EXE"')
    $xhOut = & { $ErrorActionPreference = 'Continue'; & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $xhTests 'run.ps1') -Platform x64 -Configuration Test -NoBuild -Hardening 2>&1 | ForEach-Object { "$_" } }
    $xhRows = @($xhOut | Where-Object { $_ -cmatch '^\[(PASS|FAIL|SKIP)\] hardening:' })
    $xhBin = @(@(Get-HardeningChecks '' '' 0 0 0) + @(Get-MiCrtHookCheck '') + @(Get-ImportChecks '') | Where-Object { $_.Name -ne 'sdl' } | ForEach-Object { $_.Name }) + @('aslr-pe', 'aslr-pe-tests')
    $why = $null
    foreach ($bn in $xhBin) {
        $rows = @($xhRows | Where-Object { $_ -cmatch ('^\[(PASS|FAIL|SKIP)\] hardening:' + [regex]::Escape($bn) + '(?= |$)') })
        if (-not $why -and ($rows.Count -ne 1 -or $rows[0] -cne "[SKIP] hardening:$bn - Phase 52")) { $why = "$bn rows: $($rows -join ' | ')" }
    }
    $xhTlogNames = @('sdl', 'flag-alignednew', 'flag-inline', 'flag-alignednew-tests', 'flag-banned', 'x64-truncation', 'mimalloc-defines', 'x64-objects')
    $dupes = @($xhRows | ForEach-Object { ($_ -split ' ')[1] } | Where-Object { $xhTlogNames -contains $_.Substring('hardening:'.Length) } | Group-Object | Where-Object { $_.Count -ne 1 } | ForEach-Object { $_.Name })
    if (-not $why -and @($xhRows | Where-Object { $_ -like '*hardening:dumpbin*' }).Count) { $why = 'dumpbin ran on x64' }
    if (-not $why -and $dupes.Count) { $why = "rows repeated: $($dupes -join ', ')" }
    if (-not $why) {
        foreach ($want in '[PASS] hardening:x64-truncation', '[PASS] hardening:x64-objects') { if ($xhRows -cnotcontains $want) { $why = "no $want" } }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: x64 hardening runs no Win32 binary check: $why"; $xhOut | ForEach-Object { Write-Host "    $_" } }
    else { Write-Host "OK $n x64 hardening with an x64 exe present runs no Win32 binary check and passes x64-truncation and x64-objects" }
}
finally {
    Remove-Item -Recurse -Force $xhRoot -ErrorAction SilentlyContinue
}
$n++
$mapGood = @(
    ' 0002:0000156c 00000004H .CRT$XIB                DATA',
    ' 0002:00001578 00000004H .CRT$XLB                DATA',
    ' 0002:0000157c 00000004H .CRT$XLY                DATA',
    ' 0002:0000156c       __mi_crt_callback_init     0085456c     static.obj',
    ' 0002:00001578       __mi_tls_callback_pre      00854578     static.obj',
    ' 0002:0000157c       __mi_tls_callback_post     0085457c     static.obj'
) -join "`r`n"
$hookCases = @(
    @('good map', $mapGood, $true),
    @('no __mi_crt_callback_init', ($mapGood -replace '(?m)^.*__mi_crt_callback_init.*\r?\n', ''), $false),
    @('XIB symbol outside .CRT$XIB', $mapGood.Replace('0002:0000156c       __mi_crt_callback_init', '0002:00001570       __mi_crt_callback_init'), $false),
    @('XLY symbol from another object', $mapGood.Replace('0085457c     static.obj', '0085457c     other.obj'), $false),
    @('no map', $null, $false)
)
$why = $null
foreach ($hc in $hookCases) {
    $r = Get-MiCrtHookCheck $hc[1]
    if ($r.Name -ne 'mimalloc-crt-hooks' -or $r.Ok -ne $hc[2]) { $why = "$($hc[0]) ok=$($r.Ok) $($r.Detail)" }
}
if ($why) { $failed++; Write-Host "FAILED ${n}: mimalloc-crt-hooks map fixtures: $why" }
else { Write-Host "OK $n mimalloc-crt-hooks passes the three static.obj hooks and fails a missing, misplaced or foreign hook" }
$n++
$layRoot = 'C:\X'
$layFlags = @{ 'C:\X\SRC\A.CPP' = '/c /Zc:inline C:\X\SRC\A.CPP' }
$lay = Resolve-LayoutPairs @(@('Alpha', 'C:\X\src\A.cpp'), @('Beta', 'C:\X\src\RedEngineFL\B.cpp')) $layFlags $layRoot
if ($lay.Work.Count -ne 1 -or $lay.Work[0].Class -ne 'Alpha' -or $lay.Problems.Count -ne 1 -or $lay.Problems[0] -ne 'Beta@src\RedEngineFL\B.cpp not in any game tlog') { $failed++; Write-Host "FAILED ${n}: layout pairs with a TU in no game tlog: work=$($lay.Work.Count) problems=$($lay.Problems -join '; ')" }
else { Write-Host "OK $n layout capture reports a class whose TU is in no game tlog as a problem" }
$n++
. (Join-Path $PSScriptRoot 'guardrevert\select.ps1')
$gpRoot = Split-Path $PSScriptRoot -Parent
$gp = @(Get-GameProjects $gpRoot $Configuration)
$gpHave = @($gp | ForEach-Object { $_.Vcxproj.ToLowerInvariant() } | Sort-Object)
$gpWant = @(@((Join-Path $gpRoot 'BattlefrontII.vcxproj')) + @(Get-ChildItem -LiteralPath (Join-Path $gpRoot 'src') -Recurse -Filter '*.vcxproj' -File | ForEach-Object { $_.FullName }) | ForEach-Object { $_.ToLowerInvariant() } | Sort-Object)
$gpWhy = $null
if (($gpHave -join '|') -ne ($gpWant -join '|')) { $gpWhy = "projects $($gpHave -join ', ') differ from disk $($gpWant -join ', ')" }
elseif (@($gp | Select-Object -ExpandProperty TlogDir -Unique).Count -ne $gp.Count) { $gpWhy = 'two projects share a tlog dir' }
else {
    foreach ($p in $gp) {
        $rel = $p.Vcxproj.Substring($gpRoot.Length + 1)
        foreach ($t in @($rel, "$rel.filters")) {
            if ((Test-Path -LiteralPath (Join-Path $gpRoot $t)) -and $script:GuardGlobalTriggers -notcontains $t) { $gpWhy = "$t is not a guard-revert global trigger" }
        }
    }
    if ($script:GuardGlobalTriggers -notcontains 'OpenSWBF2.Game.props') { $gpWhy = 'OpenSWBF2.Game.props is not a guard-revert global trigger' }
}
if ($gpWhy) { $failed++; Write-Host "FAILED ${n}: Get-GameProjects covers every game project: $gpWhy" }
else { Write-Host "OK $n Get-GameProjects returns the exe and every src vcxproj, and each is a guard-revert trigger" }
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
$newSizeDir = Join-Path $env:TEMP ("openswbf2-newsize-" + [guid]::NewGuid().ToString('N'))
try {
    foreach ($d in 'src\Foo', 'tests\stubs', 'tests\third_party') { New-Item -ItemType Directory -Path (Join-Path $newSizeDir $d) | Out-Null }
    Set-Content -Encoding ASCII -Path (Join-Path $newSizeDir 'src\Foo\a.cpp') -Value @('void* Foo::operator new(size_t uSize);')
    Set-Content -Encoding ASCII -Path (Join-Path $newSizeDir 'tests\stubs\s.cpp') -Value @('int x;', 'void* AIPath::operator new(unsigned int uSize) { return 0; }')
    Set-Content -Encoding ASCII -Path (Join-Path $newSizeDir 'tests\third_party\t.h') -Value @('void* operator new(unsigned int n);')
    $bad = @(Get-SourceChecks $newSizeDir -Only 'new-size-t')
    Set-Content -Encoding ASCII -Path (Join-Path $newSizeDir 'tests\stubs\s.cpp') -Value @('int x;', 'void* AIPath::operator new(size_t uSize) { return 0; }')
    $good = @(Get-SourceChecks $newSizeDir -Only 'new-size-t')
    if ($bad.Count -ne 1 -or $bad[0].Ok -or $bad[0].Detail -ne 'tests\stubs\s.cpp:2') { $failed++; Write-Host "FAILED ${n}: new-size-t flags operator new(unsigned int in tests outside tests\third_party: $($bad.Detail)" }
    elseif ($good.Count -ne 1 -or -not $good[0].Ok) { $failed++; Write-Host "FAILED ${n}: new-size-t flags operator new(unsigned int in tests outside tests\third_party: size_t flagged $($good.Detail)" }
    else { Write-Host "OK $n new-size-t flags operator new(unsigned int in tests outside tests\third_party" }
}
finally {
    Remove-Item -Recurse -Force $newSizeDir -ErrorAction SilentlyContinue
}
$n++
$readersDir = Join-Path $env:TEMP ("openswbf2-readers-" + [guid]::NewGuid().ToString('N'))
try {
    foreach ($d in 'tests\baseline', 'tests\unit') { New-Item -ItemType Directory -Path (Join-Path $readersDir $d) | Out-Null }
    Copy-Item -LiteralPath (Join-Path (Split-Path $PSScriptRoot -Parent) 'tests\baseline\v143') -Destination (Join-Path $readersDir 'tests\baseline') -Recurse
    foreach ($f in 'tests\baseline\compare-tables.ps1', 'tests\baseline\capture-vs2013.ps1', 'tests\unit\memory_baseline.cpp') { Copy-Item -LiteralPath (Join-Path (Split-Path $PSScriptRoot -Parent) $f) -Destination (Join-Path $readersDir $f) }
    $good = @(Get-SourceChecks $readersDir -Only 'baseline-readers')
    Set-Content -Encoding ASCII -Path (Join-Path $readersDir 'tests\baseline\v143\extra-table.txt') -Value 'x'
    $bad = @(Get-SourceChecks $readersDir -Only 'baseline-readers')
    if ($good.Count -ne 1 -or -not $good[0].Ok) { $failed++; Write-Host "FAILED ${n}: baseline-readers fails a v143 file with no reader: unmodified copy flagged $($good.Detail)" }
    elseif ($bad.Count -ne 1 -or $bad[0].Ok -or $bad[0].Detail -cne 'extra-table.txt has no reader') { $failed++; Write-Host "FAILED ${n}: baseline-readers fails a v143 file with no reader: $($bad.Detail)" }
    else { Write-Host "OK $n baseline-readers fails a v143 file with no reader" }
}
finally {
    Remove-Item -Recurse -Force $readersDir -ErrorAction SilentlyContinue
}
$n++
$mapDir = Join-Path $env:TEMP ("openswbf2-mapcmp-" + [guid]::NewGuid().ToString('N'))
try {
    $mapFiles = @('initializers.txt', 'initializers-di.txt', 'initializers-discarded.txt')
    $mapBase = Join-Path $mapDir 'base'
    New-Item -ItemType Directory -Path $mapBase | Out-Null
    foreach ($f in $mapFiles) { Copy-Item -LiteralPath (Join-Path (Split-Path $PSScriptRoot -Parent) "tests\baseline\v143\$f") -Destination $mapBase }
    $mapCmp = Join-Path $PSScriptRoot 'baseline\compare-tables.ps1'
    $mapK = 0
    $mapRun = {
        param([string]$file, [string[]]$rows)
        $script:mapK++
        $d = Join-Path $mapDir "t$script:mapK"
        New-Item -ItemType Directory -Path $d | Out-Null
        foreach ($f in $mapFiles) { Copy-Item -LiteralPath (Join-Path $mapBase $f) -Destination $d }
        if ($file) { [IO.File]::WriteAllLines((Join-Path $d $file), $rows) }
        $o = & powershell -NoProfile -ExecutionPolicy Bypass -File $mapCmp -Tables $d -Baseline $mapBase -Map 2>&1 | Out-String
        return @($LASTEXITCODE, $o)
    }
    $mapWhy = $null
    $r = & $mapRun '' @()
    if ($r[0] -ne 0 -or $r[1] -notmatch '\[PASS\] map:initializers-discarded lines=') { $mapWhy = "unchanged copy: exit $($r[0])" }
    foreach ($f in $mapFiles) {
        if ($mapWhy) { break }
        $label = 'map:' + [IO.Path]::GetFileNameWithoutExtension($f)
        $rows = @([IO.File]::ReadAllLines((Join-Path $mapBase $f)))
        $a = @($rows[0] -split "`t", 2)
        $b2 = @($rows[1] -split "`t", 2)
        $swapped = @($rows)
        $swapped[0] = $a[0] + "`t" + $b2[1]
        $swapped[1] = $b2[0] + "`t" + $a[1]
        $added = @($rows) + @("$($rows.Count)`tnew`tnew`tnew.obj")
        $removed = @($rows[0..($rows.Count - 2)])
        foreach ($m in @(@('reordered', $swapped), @('added', $added), @('removed', $removed))) {
            $r = & $mapRun $f $m[1]
            if ($r[0] -ne 1 -or $r[1] -notmatch ('\[FAIL\] ' + [regex]::Escape($label) + ' lines=')) { $mapWhy = "$($m[0]) row in $f not caught: exit $($r[0])"; break }
        }
    }
    if ($mapWhy) { $failed++; Write-Host "FAILED ${n}: map compare fails a reordered, added or removed initializer: $mapWhy" }
    else { Write-Host "OK $n map compare fails a reordered, added or removed initializer" }
}
finally {
    Remove-Item -Recurse -Force $mapDir -ErrorAction SilentlyContinue
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
$allCases = @(Get-SmokeCases 'All' @('foo1c_ctf TimeOut', 'tat3c_con Fatal', 'bar1c_con', 'quux1c_con Pass', 'foo7c_ctf Fatal', 'foo5c_ass TimeOut', 'dag1c_ctf Pass', 'spa3c_ass Pass') $false $null)
$testAllCfg = $script:SmokeDigestConfiguration
$script:SmokeDigestConfiguration = 'Test'
$testAllCases = @(Get-SmokeCases 'All' @('bar1c_con') $false $null)
$script:SmokeDigestConfiguration = $testAllCfg
$byName = @{}
foreach ($c in $allCases) { $byName[$c.Name] = $c }
$rejected = 0
foreach ($badLine in 'baz1c_con Bogus', 'qux1c_con GsFailure', 'foo3c_ctf Pass abc', 'foo4c_1flag Pass', 'foo6c_ass', 'foo2c_con Pass 240', 'foo8c_c TimeOut 120') {
    try { Get-SmokeCases 'All' @($badLine) $false $null | Out-Null } catch { $rejected++ }
}
$why = $null
$nvRx = '(?m)OPENSWBF2 NOVICTORY\r?$'
$nvLog = "[t=0.000] [f=0] LogCore Inform pcMain.cpp:1: OPENSWBF2 NOVICTORY`r`n"
$soakRx = 'OPENSWBF2 MISSION RUNNING|OPENSWBF2 SIM DIGEST turns=1500 |OPENSWBF2 SOAK turns=|'
if ($byName['foo1c_ctf'].Expect.Class -ne 'Pass' -or (@($byName['foo1c_ctf'].Expect.LogRx) -join '|') -ne ($soakRx + $nvRx)) { $why = 'TimeOut line' }
elseif ($byName['foo5c_ass'].Expect.Class -ne 'Pass' -or (@($byName['foo5c_ass'].Expect.LogRx) -join '|') -ne ($soakRx + $nvRx)) { $why = 'assault TimeOut line' }
elseif (-not $byName['foo1c_ctf'].Args.EndsWith(' -novictory -soakturns 9300') -or -not $byName['foo5c_ass'].Args.EndsWith(' -novictory -soakturns 9300')) { $why = 'flag and assault cases must run with -novictory and end at the short soak count' }
elseif ($byName['foo7c_ctf'].Expect.Class -ne 'Fatal' -or $byName['foo7c_ctf'].Args.Contains('-novictory') -or @($byName['foo7c_ctf'].Expect.LogRx) -contains $nvRx) { $why = 'only TimeOut flag and assault cases get -novictory' }
elseif ($nvLog -cnotmatch $nvRx -or $nvLog.Replace('NOVICTORY', 'NOVICTORYX') -cmatch $nvRx -or $nvLog.Replace('NOVICTORY', 'NOVICTORY on') -cmatch $nvRx) { $why = 'NOVICTORY rx must pin the whole line' }
elseif ($byName['bar1c_con'].Args.Contains('-novictory') -or @($byName['bar1c_con'].Expect.LogRx) -contains $nvRx) { $why = 'con cases must not get -novictory' }
elseif ($byName['dag1c_ctf'].Args.Contains('-novictory') -or @($byName['dag1c_ctf'].Expect.LogRx) -contains $nvRx -or $byName['spa3c_ass'].Args.Contains('-novictory') -or @($byName['spa3c_ass'].Expect.LogRx) -contains $nvRx) { $why = 'digest cases must not get -novictory' }
elseif ((@(@(Get-SmokeCases 'All' @('quux1c_con Pass') $false $null)[0].Expect.LogRx) -join '|') -cne $script:SmokeCapRx) { $why = 'flag case leaked its LogRx into the shared Pass expectation' }
elseif ($byName['tat3c_con'].Expect.Class -ne 'Fatal') { $why = 'Fatal line' }
elseif (-not $byName['tat3c_con'].Expect.LogRx -or $byName['tat3c_con'].Expect.Code -ne 3 -or $byName['tat3c_con'].Expect.MaxDumps -ne 1) { $why = 'Fatal line must pin the d3dx9 signature' }
elseif ('FATAL: exception 0xC0000005 at 0x658FA553 (d3dx9_43.dll+0x1BA553) build 6ADE5B7C1C4C000' -notmatch @($byName['tat3c_con'].Expect.LogRx)[0]) { $why = 'Fatal LogRx does not match the recorded tat3 crash' }
elseif ($byName['bar1c_con'].Expect.Class -ne 'Pass' -or $byName['quux1c_con'].Expect.Class -ne 'Pass') { $why = 'missing or Pass class must be Pass' }
elseif ($rejected -ne 7) { $why = 'unknown classes, GsFailure, any third token and Pass flag or assault lines without a digest row must be rejected' }
elseif (@($allCases | Where-Object { $_.Name -like 'crash-*' }).Count -ne 44 -or $byName.ContainsKey('crash-buildheapfail') -or -not $byName.ContainsKey('fatal-init')) { $why = 'dedicated or crash cases changed' }
elseif (@($testAllCases | Where-Object { $_.Name -like 'crash-*' }).Count -ne 45 -or @($testAllCases | Where-Object { $_.Name -eq 'crash-buildheapfail' }).Count -ne 1) { $why = 'Test crash cases changed' }
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
        New-Item -ItemType Directory -Path (Join-Path $d 'SaveGames') | Out-Null
        Set-Content -Path (Join-Path $d 'SaveGames\Player 1.profile') -Value 'x'
        Set-Content -Path (Join-Path $d 'display.ini') -Value '[Display]'
        Set-Content -Path (Join-Path $d 'chat1.txt') -Value 'x'
        Copy-Item -LiteralPath $env:ComSpec -Destination (Join-Path $d 'BattlefrontII-alt.exe')
    }
    if (-not $why) {
        $items = @(
            (New-Object PSObject -Property @{ Name = 'slow'; ExeName = 'BattlefrontII.exe'; ArgLine = '/c ping -n 4 127.0.0.1 >nul'; CaseDir = (Join-Path $psrc 'case-slow') }),
            (New-Object PSObject -Property @{ Name = 'fast'; ExeName = 'BattlefrontII.exe'; ArgLine = '/c exit 0'; CaseDir = (Join-Path $psrc 'case-fast') }),
            (New-Object PSObject -Property @{ Name = 'alt'; ExeName = 'BattlefrontII-alt.exe'; ArgLine = '/c exit 0'; CaseDir = (Join-Path $psrc 'case-alt') })
        )
        $pbatch = Start-SmokeBatch $items $pdirs
        $slow = Receive-SmokeCase $pbatch 0
        $fast = Receive-SmokeCase $pbatch 1
        $alt = Receive-SmokeCase $pbatch 2
        if ($slow.Name -ne 'slow' -or $fast.Name -ne 'fast' -or $alt.Name -ne 'alt') { $why = 'results not in case order' }
        elseif ($slow.WorkDir -eq $fast.WorkDir) { $why = 'concurrent cases shared a worker folder' }
        elseif ($fast.Ms -ge $slow.Ms) { $why = "fast case ($($fast.Ms) ms) did not finish before slow ($($slow.Ms) ms)" }
        elseif ($alt.WorkDir -ne $fast.WorkDir -or $alt.Code -ne 0) { $why = "batch exe not kept across cases in one worker: alt ran in $($alt.WorkDir) exit $($alt.Code)" }
        foreach ($d in $pdirs) {
            if ($why) { break }
            $left = @(Get-ChildItem -LiteralPath $d -Force | ForEach-Object { $_.Name } | Sort-Object)
            if (($left -join ',') -ne ((@('BattlefrontII-alt.exe', 'BattlefrontII.exe', 'binkw32.dll', 'data', 'steam_appid.txt') | Sort-Object) -join ',') -or -not (Test-Path -LiteralPath (Join-Path $d 'data\marker.txt'))) { $why = "worker $d not cleared before its case: $($left -join ',')" }
        }
        if (-not $why) {
            $pl = Join-Path $psrc 'outside'
            New-Item -ItemType Directory -Path $pl | Out-Null
            Set-Content -Path (Join-Path $pl 'keep.txt') -Value 'x'
            New-Item -ItemType Directory -Path (Join-Path $pdirs[0] 'nested') | Out-Null
            & cmd /c mklink /J "$(Join-Path $pdirs[0] 'nested\link')" "$pl" | Out-Null
            $refused = $false
            try { Clear-SmokeWorkerDir $pdirs[0] } catch { $refused = $_.Exception.Message -like 'worker folder holds a reparse point*' }
            & cmd /c rmdir "$(Join-Path $pdirs[0] 'nested\link')" | Out-Null
            if (-not $refused -or -not (Test-Path -LiteralPath (Join-Path $pl 'keep.txt'))) { $why = 'worker clear did not refuse a nested junction' }
        }
    }
}
catch { $why = $_.Exception.Message }
finally {
    if ($pbatch) { Stop-SmokeBatch $pbatch }
    foreach ($d in $pdirs) { Remove-HarnessTree $d }
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
    $digRes = New-Object PSObject -Property @{ Class = 'Pass'; CaseDir = $digTmp; LogText = $digLine; Dumps = @(); Turns = '1500' }
    $env:OPENSWBF2_DIGEST_EXPECTED = $goodExp
    $okGood = Test-SmokeExpect $digCase $digRes
    $env:OPENSWBF2_DIGEST_EXPECTED = $flipExp
    $okFlip = Test-SmokeExpect $digCase $digRes
    if (-not $okGood -or $okFlip) { $failed++; Write-Host "FAILED ${n}: digest compare: matching=$okGood flipped=$okFlip" }
    else { Write-Host "OK $n digest compare fails on a one-bit state flip" }
    $soakX = @(Get-SmokeCases 'Twice' @() $false $null | Where-Object { $_.Name -eq 'twice-digest-1' })[0].Expect
    $soakCase = New-Object PSObject -Property @{ Name = 'selftest-soak'; Expect = $soakX }
    $soakRun = '[t=1.000] [f=62] OPENSWBF2 MISSION RUNNING nab2g_c'
    $soakDig = "[t=2.000] [f=1561] $digLine"
    $soakEnd = '[t=2.100] [f=1562] OPENSWBF2 SOAK turns=1501'
    $soakDone = '[t=2.200] [f=1562] OPENSWBF2 RUN COMPLETE'
    foreach ($sf in @(@('soak case without its SOAK line fails', @($soakRun, $soakDig, $soakDone), $false), @('soak case without its digest line fails', @($soakRun, $soakEnd, $soakDone), $false), @('soak case with its digest and SOAK lines passes', @($soakRun, $soakDig, $soakEnd, $soakDone), $true))) {
        $n++
        $soakText = ($sf[1] -join "`r`n") + "`r`n"
        [IO.File]::WriteAllText((Join-Path $digTmp 'run.log'), $soakText)
        $soakRes = New-Object PSObject -Property @{ Class = (Get-SmokeClass $false $false 0 $soakText @()); CaseDir = $digTmp; LogText = $soakText; Dumps = @(); Turns = '1501' }
        $soakOk = Test-SmokeExpect $soakCase $soakRes
        if ($soakRes.Class -ne 'Pass' -or $soakOk -ne $sf[2]) { $failed++; Write-Host "FAILED ${n}: $($sf[0]): class=$($soakRes.Class) ok=$soakOk" }
        else { Write-Host "OK $n $($sf[0])" }
    }
    $n++
    $shapeWhy = $null
    $shapeShort = @(Get-SmokeCases 'All' @('foo9c_c TimeOut', 'bar1c_con Pass') $false $null $false 'Short')
    $shapeDefault = @(Get-SmokeCases 'All' @('foo9c_c TimeOut') $false $null)[0]
    $shapeLong = @(Get-SmokeCases 'All' @('foo9c_c TimeOut', 'bar1c_con Pass') $false $null $false 'Long')
    $shapeRej = 0
    try { Get-SmokeCases 'All' @('foo9c_c TimeOut') $false $null $false 'Medium' | Out-Null } catch { $shapeRej++ }
    foreach ($sh in @(@($shapeShort[0], 9300), @($shapeDefault, 9300), @($shapeLong[0], 617300))) {
        $c = $sh[0]
        if ($shapeWhy) { break }
        if ($c.Name -ne 'foo9c_c' -or $c.Expect.Class -ne 'Pass' -or -not $c.Args.EndsWith(" -soakturns $($sh[1])") -or @($c.Expect.LogRx) -notcontains 'OPENSWBF2 SIM DIGEST turns=1500 ' -or @($c.Expect.LogRx) -notcontains 'OPENSWBF2 SOAK turns=') { $shapeWhy = "TimeOut case for count $($sh[1]) is Class=$($c.Expect.Class) Args=$($c.Args)" }
    }
    if (-not $shapeWhy -and ($script:SoakShortTurns -ne 9300 -or $script:SoakLongTurns -ne 617300)) { $shapeWhy = "soak counts are $script:SoakShortTurns and $script:SoakLongTurns" }
    foreach ($pc in $shapeShort[1], $shapeLong[1]) { if (-not $shapeWhy -and (-not $pc.Args.EndsWith(" -soakturns $script:SmokeTurnCap") -or @($pc.Expect.LogRx) -contains 'OPENSWBF2 SOAK turns=' -or @($pc.Expect.LogRx) -notcontains $script:SmokeCapRx)) { $shapeWhy = "a Pass mission does not carry only the work cap: $($pc.Args)" } }
    if (-not $shapeWhy -and $shapeRej -ne 1) { $shapeWhy = 'an unknown soak was accepted' }
    if ($shapeWhy) { $failed++; Write-Host "FAILED ${n}: All TimeOut soak case shape: $shapeWhy" }
    else { Write-Host "OK $n All TimeOut missions are Pass cases ending at -soakturns 9300 (Short) or 617300 (Long) with the digest and SOAK lines" }
    $allSoakCase = $shapeShort[0]
    $allSoakDig = "[t=2.000] [f=1500] $digLine"
    $allSoakDig1501 = "[t=2.000] [f=1501] $($digLine.Replace('turns=1500 ', 'turns=1501 '))"
    $allSoakEnd = '[t=2.300] [f=9300] OPENSWBF2 SOAK turns=9300'
    foreach ($sf in @(@('All TimeOut case that exits early without its SOAK line fails', @($soakRun, $allSoakDig, $soakDone), $false), @('All TimeOut case without its turn-1500 digest line fails', @($soakRun, $allSoakEnd, $soakDone), $false), @('All TimeOut case with a digest at another turn fails', @($soakRun, $allSoakDig1501, $allSoakEnd, $soakDone), $false), @('All TimeOut case with its turn-1500 digest and SOAK lines passes', @($soakRun, $allSoakDig, $allSoakEnd, $soakDone), $true))) {
        $n++
        $soakText = ($sf[1] -join "`r`n") + "`r`n"
        [IO.File]::WriteAllText((Join-Path $digTmp 'run.log'), $soakText)
        $soakRes = New-Object PSObject -Property @{ Class = (Get-SmokeClass $false $false 0 $soakText @()); CaseDir = $digTmp; LogText = $soakText; Dumps = @(); Turns = '9300' }
        $soakOk = Test-SmokeExpect $allSoakCase $soakRes
        if ($soakRes.Class -ne 'Pass' -or $soakOk -ne $sf[2]) { $failed++; Write-Host "FAILED ${n}: $($sf[0]): class=$($soakRes.Class) ok=$soakOk" }
        else { Write-Host "OK $n $($sf[0])" }
    }
    $n++
    $soakWhy = $null
    $soakCfg = $script:SmokeDigestConfiguration
    $script:SmokeDigestConfiguration = 'Test'
    $soakStd = @(Get-SmokeCases 'Standard' @() $false $null | Where-Object { $_.Name -in 'novictory-con', 'repeat-water' })
    $script:SmokeDigestConfiguration = $soakCfg
    $soakAll = @($soakStd) + @(Get-SmokeCases 'Twice' @() $false $null | Where-Object { $_.Name -like 'twice-digest-*' }) + @(Get-SmokeCases 'Soak' @() $false $null)
    if ($soakAll.Count -ne 5) { $soakWhy = "expected 5 soak cases, got $($soakAll.Count)" }
    foreach ($c in $soakAll) {
        if (-not $soakWhy -and ($c.Expect.Class -ne 'Pass' -or $c.Args -notmatch ' -soakturns \d+' -or @($c.Expect.LogRx) -notcontains 'OPENSWBF2 SOAK turns=')) { $soakWhy = "$($c.Name) must be Pass, pass -soakturns and require the SOAK line" }
    }
    $soakStdNames = @()
    foreach ($g in 'None', 'Standard', 'Twice', 'Dedicated', 'Crash', 'All', 'Perturb') { $soakStdNames += @(Get-SmokeCases $g @('nab2g_c TimeOut') $false $null | ForEach-Object { $_.Name }) }
    if (-not $soakWhy -and $soakStdNames -contains 'asan-soak') { $soakWhy = 'asan-soak is in a group other than Soak' }
    if ($soakWhy) { $failed++; Write-Host "FAILED ${n}: soak cases: $soakWhy" }
    else { Write-Host "OK $n soak cases are Pass, require their SOAK line and asan-soak runs only in Soak" }
    $n++
    $capWhy = $null
    $capCfg = $script:SmokeDigestConfiguration
    $capEnv = $env:OPENSWBF2_DIGEST_EXPECTED
    $env:OPENSWBF2_DIGEST_EXPECTED = $savedExpected
    $capMissions = @(Get-Content (Join-Path $PSScriptRoot 'smoke\missions-all.txt') | Where-Object { $_.Trim() })
    $capSeen = 0
    $capTo = @($capMissions | Where-Object { @($_.Trim() -split '\s+')[1] -eq 'TimeOut' }).Count
    $capReq = 0
    foreach ($cfg in 'Release', 'Test', 'Asan') {
        $script:SmokeDigestConfiguration = $cfg
        foreach ($g in 'Standard', 'Twice', 'Dedicated', 'Crash', 'All', 'Perturb', 'Soak') {
            foreach ($sk in 'Short', 'Long') {
                foreach ($hg in $false, $true) {
                    foreach ($c in @(Get-SmokeCases $g $capMissions $false $null $hg $sk)) {
                        $capSeen++
                        if ($capWhy) { continue }
                        $req = @(@($c.Expect.LogRx) | Where-Object { $_ -ceq 'OPENSWBF2 SOAK turns=' -or $_ -ceq '(?m)OPENSWBF2 SOAK turns=1\r?$' }).Count -gt 0
                        $forbid = @($c.Expect.LogRx) -contains $script:SmokeCapRx
                        if ($req) { $capReq++ }
                        if ($c.Expect.Class -eq 'TimeOut') { $capWhy = "$cfg $g $sk $($c.Name) expects TimeOut" }
                        elseif ([regex]::Matches($c.Args, ' -soakturns \d+').Count -ne 1) { $capWhy = "$cfg $g $sk $($c.Name) does not carry exactly one -soakturns: $($c.Args)" }
                        elseif ($req -eq $forbid) { $capWhy = "$cfg $g $sk $($c.Name) requires=$req forbids=$forbid the SOAK line" }
                        elseif ($forbid -and -not $c.Args.EndsWith(" -soakturns $script:SmokeTurnCap")) { $capWhy = "$cfg $g $sk $($c.Name) forbids the SOAK line but does not end at the work cap: $($c.Args)" }
                        elseif ((@($c.PSObject.Properties.Name | Sort-Object) -join ',') -cne 'Args,Expect,Name,Twice') { $capWhy = "$cfg $g $sk $($c.Name) carries fields other than Name, Args, Expect and Twice: $(@($c.PSObject.Properties.Name) -join ',')" }
                        elseif (($c.Args -replace '^.*(?:^| )-hangtimeout (\S+)(?: .*)?$', '$1') -cne $(if ($c.Name -eq 'crash-hang') { '5' } else { '0' }) -or [regex]::Matches($c.Args, '(?:^| )-hangtimeout ').Count -ne 1) { $capWhy = "$cfg $g $sk $($c.Name) does not carry exactly one -hangtimeout 0 (crash-hang: 5): $($c.Args)" }
                    }
                }
            }
        }
    }
    $script:SmokeDigestConfiguration = $capCfg
    $env:OPENSWBF2_DIGEST_EXPECTED = $capEnv
    if (-not $capWhy -and ($capTo -lt 1 -or $capReq -lt 12 * $capTo)) { $capWhy = "$capReq cases require SOAK, expected at least 12 x $capTo TimeOut missions" }
    if (-not $capWhy -and $script:SmokeTurnCap -ne $script:SoakLongTurns) { $capWhy = "work cap is $script:SmokeTurnCap" }
    if ($capWhy) { $failed++; Write-Host "FAILED ${n}: work cap: $capWhy" }
    else { Write-Host "OK $n no case expects TimeOut; every case carries one -soakturns and either requires the SOAK line or forbids it at the work cap, runs supervised and carries only Name, Args, Expect and Twice ($capSeen cases)" }
    $capSrc = @(Get-SmokeCases 'Standard' @() $false $null | Where-Object { $_.Name -eq 'digest-kas2c_uber' })[0]
    $capCase = New-Object PSObject -Property @{ Name = 'selftest-cap'; Args = $capSrc.Args; Expect = $capSrc.Expect }
    $capEnd = "[t=9.000] [f=9274] LogSim Inform GameLoop.cpp:1: OPENSWBF2 RUN COMPLETE geo1c_con`r`n"
    $capOk = "[t=1.000] [f=1] LogCore Inform pcMain.cpp:1: OPENSWBF2 PACING fps=30 vsync=off`r`n$capEnd"
    $capRun = $capOk.Replace($capEnd, "[t=8.000] [f=617560] LogSim Inform GameLoop.cpp:1: OPENSWBF2 SOAK turns=617300`r`n$capEnd")
    foreach ($sf in @(@('self-ending case that ends by itself passes', $capOk, $true), @('self-ending case that reaches the work cap fails', $capRun, $false))) {
        $n++
        [IO.File]::WriteAllText((Join-Path $digTmp 'run.log'), $sf[1])
        $capRes = New-Object PSObject -Property @{ Class = (Get-SmokeClass $false $false 0 $sf[1] @()); CaseDir = $digTmp; LogText = $sf[1]; Dumps = @(); Turns = '617300' }
        $capGot = Test-SmokeExpect $capCase $capRes
        if ($capRes.Class -ne 'Pass' -or $capGot -ne $sf[2]) { $failed++; Write-Host "FAILED ${n}: $($sf[0]): class=$($capRes.Class) ok=$capGot" }
        else { Write-Host "OK $n $($sf[0])" }
    }
    $tuCase = New-Object PSObject -Property @{ Name = 'selftest-turns'; Expect = @{ Class = 'Pass' } }
    $tuSoak = "[t=1.000] [f=1500] $digLine`r`n[t=2.000] [f=1501] OPENSWBF2 SOAK turns=1501`r`n[t=3.000] [f=3002] OPENSWBF2 SOAK turns=3002`r`n"
    $tuDig = "[t=1.000] [f=1500] $digLine`r`n"
    foreach ($sf in @(@('result turns below the last logged SOAK count fails', $tuSoak, '3001', $false), @('result turns at the last logged SOAK count passes', $tuSoak, '3002', $true), @('result without progress turns fails a logged SOAK count', $tuSoak, '-', $false), @('result turns below the logged SIM DIGEST count fails', $tuDig, '1499', $false), @('result turns at the logged SIM DIGEST count passes', $tuDig, '1500', $true))) {
        $n++
        [IO.File]::WriteAllText((Join-Path $digTmp 'run.log'), $sf[1])
        $tuRes = New-Object PSObject -Property @{ Class = 'Pass'; CaseDir = $digTmp; LogText = $sf[1]; Dumps = @(); Turns = $sf[2] }
        $tuGot = Test-SmokeExpect $tuCase $tuRes
        if ($tuGot -ne $sf[3]) { $failed++; Write-Host "FAILED ${n}: $($sf[0]): turns=$($sf[2]) ok=$tuGot" }
        else { Write-Host "OK $n $($sf[0])" }
    }
    $n++
    $csWhy = $null
    $csAll = @(Get-SmokeCases 'Crash' @() $false $null)
    $cs = @($csAll | Where-Object { $_.Name -eq 'crash-stall' })
    $ch = @($csAll | Where-Object { $_.Name -eq 'crash-hang' })
    if ($cs.Count -ne 1 -or $ch.Count -ne 1) { $csWhy = "crash-stall $($cs.Count), crash-hang $($ch.Count)" }
    else {
        $csX = $cs[0].Expect
        if ($csX.Class -ne 'Fatal' -or $csX.Code -ne 5 -or $csX.MinDumps -ne 1 -or $csX.MaxDumps -ne 1 -or $csX.Stall -ne 1 -or @($csX.LogRx) -cnotcontains 'FATAL: hang \(supervisor saw no progress') { $csWhy = 'crash-stall must expect Fatal, exit 5, one dump, a stall and the supervisor FATAL line' }
        elseif (-not $cs[0].Args.Contains(' -crashtest hang ') -or [regex]::Matches($cs[0].Args, '(?:^| )-hangtimeout 0(?: |$)').Count -ne 1 -or [regex]::Matches($cs[0].Args, '(?:^| )-hangtimeout ').Count -ne 1) { $csWhy = "crash-stall must run -crashtest hang supervised by the tail only: $($cs[0].Args)" }
        elseif ($ch[0].Expect.ContainsKey('Stall')) { $csWhy = 'crash-hang must not expect a stall' }
        elseif ($script:SmokeAsanExcluded -cnotcontains 'crash-stall') { $csWhy = 'crash-stall is not excluded from ASan' }
    }
    if ($csWhy) { $failed++; Write-Host "FAILED ${n}: crash-stall case: $csWhy" }
    else { Write-Host "OK $n crash-stall is a supervised hang that must end in the game's own stall dump" }
    $n++
    $pcWhy = $null
    foreach ($pn in 'pacing-realtime', 'pacing-realtime-uncapped') {
        $pc = @(Get-SmokeCases 'Standard' @() $false $null | Where-Object { $_.Name -eq $pn })
        if ($pc.Count -ne 1 -or $pc[0].Expect.Class -ne 'Pass' -or -not $pc[0].Args.EndsWith(' -soakturns 300') -or @($pc[0].Expect.LogRx) -notcontains 'OPENSWBF2 SOAK turns=' -or @($pc[0].Expect.LogRx) -notcontains 'OPENSWBF2 MISSION RUNNING') { $pcWhy = "$pn must be Pass, end at -soakturns 300 and require MISSION RUNNING and SOAK" }
    }
    if ($pcWhy) { $failed++; Write-Host "FAILED ${n}: real-time pacing cases: $pcWhy" }
    else { Write-Host "OK $n real-time pacing cases end at -soakturns 300 with the SOAK line" }
    $n++
    $suWhy = $null
    $suRx = '(?m)OPENSWBF2 SOAK turns=1\r?$'
    $suWant = 'cursor-sw,cursor-sw-ini,display-badini,display-switches,fps-0,fps-30,pacing-waitquery-ini,window-borderless-headless'
    $suCfg = $script:SmokeDigestConfiguration
    foreach ($cfg in 'Release', 'Test', 'Asan') {
        $script:SmokeDigestConfiguration = $cfg
        $suStd = @(Get-SmokeCases 'Standard' @() $false $null)
        $suNames = (@($suStd | Where-Object { $_.Args -cmatch '(?:^| )-soakturns 1(?: |$)' } | ForEach-Object { $_.Name } | Sort-Object) -join ',')
        if ($suNames -cne $suWant) { $suWhy = "$cfg cases ending at -soakturns 1 are '$suNames'"; break }
        foreach ($c in @($suStd | Where-Object { ($suWant -split ',') -ccontains $_.Name })) {
            if (-not $c.Args.EndsWith(' -soakturns 1') -or @($c.Expect.LogRx) -cnotcontains $suRx -or $c.Expect.Class -ne 'Pass') { $suWhy = "$cfg $($c.Name) must be Pass, end at -soakturns 1 and require the SOAK turns=1 line"; break }
        }
        if ($suWhy) { break }
        $suGeo = @($suStd | Where-Object { $_.Name -eq 'geo1c_con' })[0]
        $suDup = @($suStd | Where-Object { $_.Name -ne 'geo1c_con' -and $_.Args -ceq $suGeo.Args } | ForEach-Object { $_.Name })
        if ($suDup.Count) { $suWhy = "$cfg $($suDup -join ',') repeats the geo1c_con run"; break }
        foreach ($rx in 'OPENSWBF2 PACING fps=80 vsync=off interval=0x80000000 waitquery=0 timer=none', 'OPENSWBF2 GAMMA path=ramp reason=norender', '(?s)\A(?!.*OPENSWBF2 GAMMA path=pass)', 'OPENSWBF2 CURSOR mode=hw image=skipped') {
            if (@($suGeo.Expect.LogRx) -cnotcontains $rx) { $suWhy = "$cfg geo1c_con lacks the start-up line $rx"; break }
        }
        if ($suWhy) { break }
    }
    $script:SmokeDigestConfiguration = $suCfg
    if (-not $suWhy) {
        $suSrc = @(Get-SmokeCases 'Standard' @() $false $null | Where-Object { $_.Name -eq 'fps-30' })[0]
        $suCase = New-Object PSObject -Property @{ Name = 'selftest-startup'; Args = $suSrc.Args; Expect = $suSrc.Expect }
        $suPace = "[t=0.020] [f=0] LogRender Inform pcRedRenderer.cpp:1: OPENSWBF2 PACING fps=30 vsync=off`r`n"
        $suDone = "[t=2.700] [f=2] LogSim Inform GameLoop.cpp:680: OPENSWBF2 RUN COMPLETE geo1c_con`r`n"
        foreach ($sf in @(@('turn 1', "$suPace[t=2.690] [f=2] LogSim Inform GameLoop.cpp:1397: OPENSWBF2 SOAK turns=1`r`n$suDone", '1', $true), @('no SOAK line', "$suPace$suDone", '1', $false), @('SOAK at turn 10', "$suPace[t=2.690] [f=11] LogSim Inform GameLoop.cpp:1397: OPENSWBF2 SOAK turns=10`r`n$suDone", '10', $false), @('no PACING line', "[t=2.690] [f=2] LogSim Inform GameLoop.cpp:1397: OPENSWBF2 SOAK turns=1`r`n$suDone", '1', $false))) {
            [IO.File]::WriteAllText((Join-Path $digTmp 'run.log'), $sf[1])
            $suRes = New-Object PSObject -Property @{ Class = (Get-SmokeClass $false $false 0 $sf[1] @()); CaseDir = $digTmp; LogText = $sf[1]; Dumps = @(); Turns = $sf[2] }
            $suGot = Test-SmokeExpect $suCase $suRes
            if ($suRes.Class -ne 'Pass' -or $suGot -ne $sf[3]) { $suWhy = "fps-30 with $($sf[0]): class=$($suRes.Class) ok=$suGot, expected $($sf[3])"; break }
        }
    }
    if ($suWhy) { $failed++; Write-Host "FAILED ${n}: start-up cases: $suWhy" }
    else { Write-Host "OK $n the eight start-up cases end at -soakturns 1 and pass only with the SOAK turns=1 line; geo1c_con carries the default pacing, gamma and cursor lines and no other case repeats its run" }
    $n++
    $amWhy = $null
    $amCfg = $script:SmokeDigestConfiguration
    $amTw = @{}
    foreach ($xC in @(Get-SmokeCases 'Twice' @() $false $null)) { $amTw[$xC.Name] = $xC }
    foreach ($xCfg in 'Release', 'Test', 'Asan') {
        $script:SmokeDigestConfiguration = $xCfg
        $amStd = @(Get-SmokeCases 'Standard' @() $false $null)
        $amHave = @($amStd | Where-Object { $_.Name -like 'asan-*' } | ForEach-Object { $_.Name } | Sort-Object) -join ','
        $amExp = $(if ($xCfg -eq 'Asan') { 'asan-hot1g_con,asan-spa3c_ass' } else { '' })
        if ($amHave -cne $amExp) { $amWhy = "$xCfg Standard asan cases are '$amHave', expected '$amExp'"; break }
        foreach ($xM in @($amStd | Where-Object { $_.Name -like 'asan-*' })) {
            $amTwin = $amTw['digest-' + $xM.Name.Substring(5)]
            if (-not $amTwin -or $xM.Args -cne $amTwin.Args -or $xM.Expect.Class -ne 'Pass' -or $xM.Twice -ne 0) { $amWhy = "$($xM.Name) must be a Pass case with the args of $('digest-' + $xM.Name.Substring(5))"; break }
        }
        if ($amWhy) { break }
    }
    $script:SmokeDigestConfiguration = $amCfg
    if (-not $amWhy -and @($script:SmokeAsanExcluded | Where-Object { $_ -like 'asan-*' }).Count) { $amWhy = 'an asan- case is excluded from ASan' }
    if ($amWhy) { $failed++; Write-Host "FAILED ${n}: ASan digest missions: $amWhy" }
    else { Write-Host "OK $n the Asan Standard group, and only it, runs asan-hot1g_con and asan-spa3c_ass with the Twice digest-case args" }
    $n++
    $psWhy = $null
    $psSaved = $env:OPENSWBF2_PERTURB_MISSIONS
    $psList = @('geo1c_con', 'nab2g_c', 'hot1g_con', 'spa3c_ass', 'dag1c_ctf', 'kas2c_uber', 'tan1g_con')
    try {
        $env:OPENSWBF2_PERTURB_MISSIONS = ''
        $psAll = @(Get-SmokeCases 'Perturb' @() $false $null)
        for ($psI = 0; $psI -lt $psList.Count -and -not $psWhy; $psI++) {
            $xM = $psList[$psI]
            $psSeed = 1 + ($psI % 2)
            $psMine = @($psAll | Where-Object { $_.Expect.Perturb.Mission -eq $xM })
            $psModes = (@($psMine | ForEach-Object { $_.Expect.Perturb.Mode }) | Sort-Object) -join ','
            if ($psModes -cne 'base,fill,hog,pad,pool,repeat') { $psWhy = "$xM runs modes '$psModes'"; break }
            foreach ($xC in $psMine) {
                $xP = $xC.Expect.Perturb
                $psWant = $(if ($xP.Mode -in 'pad', 'fill', 'pool') { $psSeed } else { 0 })
                if ($xP.Seed -ne $psWant) { $psWhy = "$($xC.Name) has seed $($xP.Seed), expected $psWant"; break }
                if ($psWant -gt 0 -and ($xC.Name -cne "perturb-$xM-$($xP.Mode)-$psWant" -or -not $xC.Args.Contains(" -perturbheap ${psWant}:$($xP.Mode)"))) { $psWhy = "$($xC.Name) does not run -perturbheap ${psWant}:$($xP.Mode)"; break }
            }
            if ($psWhy) { break }
            $env:OPENSWBF2_PERTURB_MISSIONS = $xM
            $psOne = @(Get-SmokeCases 'Perturb' @() $false $null)
            $env:OPENSWBF2_PERTURB_MISSIONS = ''
            if ((@($psOne | ForEach-Object { "$($_.Name)|$($_.Args)" }) -join ';') -cne (@($psMine | ForEach-Object { "$($_.Name)|$($_.Args)" }) -join ';')) { $psWhy = "a run filtered to $xM differs from the full run" }
        }
        if (-not $psWhy -and $psAll.Count -ne 42) { $psWhy = "$($psAll.Count) cases, expected 42" }
    }
    catch { $psWhy = "threw: $_" }
    finally { $env:OPENSWBF2_PERTURB_MISSIONS = $psSaved }
    if ($psWhy) { $failed++; Write-Host "FAILED ${n}: perturb seeds: $psWhy" }
    else { Write-Host "OK $n each Perturb mission runs every mode once, pad/fill/pool seeds alternate 1 and 2 down the full list, and a filtered run gets the same cases" }
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
    elseif ($cfCase.Count -ne 1 -or $cfCase[0].Expect.CommitFlat -ne 0 -or $cfCase[0].Expect.CommitCycles -ne 16 -or $cfCase[0].Expect.CommitSettled -ne 8 -or $cfCase[0].Expect.SameDigests -ne 17 -or -not $cfCase[0].Args.EndsWith(" -repeatmission 16 -soakturns $script:SmokeTurnCap")) { $cfWhy = 'commit-flat case missing or wrong' }
    if ($cfWhy) { $failed++; Write-Host "FAILED ${n}: commit flat: $cfWhy" }
    else { Write-Host "OK $n commit flat fails a new arena or rest high after the settled cycles, counts game CRT bytes in place of heap and thread commit and needs alternating states" }
    $n++
    if ($rwCase.Count -ne 2 -or $rwRel.Count -ne 0 -or @($rwCase | Where-Object { $_.Expect.Class -ne 'Pass' -or $_.Expect.SameDigests -ne 2 -or -not $_.Args.EndsWith(' -soakturns 1501') -or @($_.Expect.LogRx) -notcontains 'OPENSWBF2 SOAK turns=' }).Count -ne 0) { $failed++; Write-Host "FAILED ${n}: repeat-water must run in Test and Asan only, end on -soakturns 1501 with two equal digests and the SOAK line" }
    else { Write-Host "OK $n repeat-water runs in Test and Asan and ends on its second digest" }
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
    $vToEq = Get-PerturbVerdict $bTo (& $wFile 'whole-to-eq.log' ((& $trRange 1 1700 0) + $simW)) 'Pass' 'Pass' $false
    $vToShort = Get-PerturbVerdict $bTo (& $wFile 'whole-to-short.log' ((& $trRange 1 1550 0) + $simW)) 'Pass' 'Pass' $false
    $vToSp = Get-PerturbVerdict $bTo (& $wFile 'whole-to-split.log' ((& $trRange 1 1700 1620) + $simW)) 'Pass' 'Pass' $false
    $vToIn = Get-PerturbVerdict $bTo (& $wFile 'whole-to-incomplete.log' ((& $trRange 1 1400 0) + $simW)) 'Pass' 'Pass' $false
    if ($vToEq.Result -ne 'equal') { $wBad += "same count=$($vToEq.Result)" }
    if ($vToShort.Result -ne 'crash' -or $vToShort.Class -ne 'incomplete') { $wBad += "run shorter than base=$($vToShort.Result)/$($vToShort.Class)" }
    if ($vToSp.Result -ne 'split' -or $vToSp.Turn -ne 1620) { $wBad += "count split=$($vToSp.Result)/$($vToSp.Turn)" }
    if ($vToIn.Result -ne 'crash' -or $vToIn.Class -ne 'incomplete') { $wBad += "count below 1500=$($vToIn.Result)/$($vToIn.Class)" }
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
    else { Write-Host "OK $n perturb whole mission splits a longer run, ignores a cut last line, fails a run shorter than its base and compares repeat segment two past 1600" }
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
    if (@(Get-PerturbProofMissing $pCases['perturb-geo1c_con-base'] 'x').Count -ne 0) { $prBad += 'plain base rejected' }
    if (@(Get-PerturbProofMissing $pCases['perturb-geo1c_con-base'] $pLine).Count -eq 0) { $prBad += 'base with PERTURB line accepted' }
    if (@(Get-PerturbProofMissing $pCases['perturb-geo1c_con-hog'] $addr).Count -ne 0) { $prBad += 'hog with ADDRSPACE rejected' }
    if (@(Get-PerturbProofMissing $pCases['perturb-geo1c_con-hog'] 'x').Count -eq 0) { $prBad += 'hog without ADDRSPACE accepted' }
    if (@(Get-PerturbProofMissing $pCases['perturb-geo1c_con-hog'] "$addr`r`n$pLine").Count -eq 0) { $prBad += 'hog with PERTURB line accepted' }
    $rShell = "[t=1.000] [f=1] LogSim Inform GameLoop.cpp:1535: OPENSWBF2 REPEAT to_shell mission=geo1c_con`r`n"
    $rMission = "[t=2.000] [f=2] LogSim Inform GameState.cpp:1347: OPENSWBF2 REPEAT to_mission mission=geo1c_con`r`n"
    $rCase = $pCases['perturb-geo1c_con-repeat']
    if (-not $rCase -or -not $rCase.Args.EndsWith(" -repeatmission -soakturns $script:SmokeTurnCap")) { $prBad += 'repeat case missing or wrong' }
    elseif (@(Get-PerturbProofMissing $rCase "$rShell$rMission").Count -ne 0) { $prBad += 'repeat with both REPEAT lines rejected' }
    elseif (@(Get-PerturbProofMissing $rCase $rShell).Count -eq 0) { $prBad += 'repeat without to_mission accepted' }
    elseif (@(Get-PerturbProofMissing $rCase $rMission).Count -eq 0) { $prBad += 'repeat without to_shell accepted' }
    elseif (@(Get-PerturbProofMissing $rCase "$rShell$rMission$pLine").Count -eq 0) { $prBad += 'repeat with PERTURB line accepted' }
    $savedPm = $env:OPENSWBF2_PERTURB_MISSIONS
    $env:OPENSWBF2_PERTURB_MISSIONS = ''
    $allP = @(Get-SmokeCases 'Perturb' @() $false $null)
    $env:OPENSWBF2_PERTURB_MISSIONS = $savedPm
    $nabRep = @($allP | Where-Object { $_.Name -eq 'perturb-nab2g_c-repeat' })
    if ($allP.Count -ne 42 -or @($allP | Where-Object { $_.Expect.Perturb.Mode -eq 'repeat' }).Count -ne 7) { $prBad += "perturb cases=$($allP.Count), need 42 with 7 repeat" }
    $nabPad2 = @($allP | Where-Object { $_.Name -eq 'perturb-nab2g_c-pad-2' })
    if ($nabPad2.Count -ne 1) { $prBad += 'perturb-nab2g_c-pad-2 missing' }
    elseif (@(Get-PerturbProofMissing $nabPad2[0] "$pLine[t=9.000] [f=100001] OPENSWBF2 SOAK turns=100000`r`n").Count -eq 0) { $prBad += 'seed 2 accepted seed=1' }
    elseif (@(Get-PerturbProofMissing $nabPad2[0] "$($pLine.Replace('seed=1', 'seed=2'))[t=9.000] [f=100001] OPENSWBF2 SOAK turns=100000`r`n").Count -ne 0) { $prBad += 'seed 2 rejected seed=2' }
    $nabAll = @($allP | Where-Object { $_.Expect.Perturb.Mission -eq 'nab2g_c' })
    if ($nabRep.Count -ne 1 -or $nabAll.Count -ne 6) { $prBad += "nab2g_c cases=$($nabAll.Count), need 6 with one repeat" }
    foreach ($c in $nabAll) { if ($c.Expect.Class -ne 'Pass' -or -not $c.Args.EndsWith(" -soakturns $script:PerturbSoakTurns") -or @($c.Expect.LogRx) -notcontains 'OPENSWBF2 SOAK turns=') { $prBad += "$($c.Name) must be Pass and end at -soakturns 100000 with the SOAK line" } }
    if ($script:PerturbSoakTurns -ne 100000) { $prBad += "perturb soak count is $script:PerturbSoakTurns" }
    if (@(Get-PerturbProofMissing $nabRep[0] "$($rShell.Replace('geo1c_con', 'nab2g_c'))$($rMission.Replace('geo1c_con', 'nab2g_c'))").Count -eq 0) { $prBad += 'nab2g_c repeat without its SOAK line accepted' }
    if ($prBad.Count) { $failed++; Write-Host "FAILED ${n}: perturb proof: $($prBad -join '; ')" }
    else { Write-Host "OK $n perturb runs must prove their mode, seed, hog and repeat" }
    $n++
    $moBad = @()
    $moRx = 'OPENSWBF2 MIOPTIONS purge_delay=1000 retry_on_oom=0 arena_reserve=67108864 arena_eager_commit=0 reserve_os_memory=0 page_full_retain=-1'
    $moNew = { param($class) @(Add-MiOptionsExpect @(New-Object PSObject -Property @{ Name = 'selftest-case'; Expect = @{ Class = $class } }) -1)[0] }
    if (@((& $moNew 'Pass').Expect.LogRx) -notcontains $moRx) { $moBad += 'Pass case lacks purge_delay=1000' }
    foreach ($c in @(Add-MiOptionsExpect @(Get-SmokeCases 'Standard' @() $false $null) 0)) { if ($c.Expect.Class -eq 'Pass' -and @($c.Expect.LogRx) -notcontains $moRx.Replace('purge_delay=1000', 'purge_delay=0')) { $moBad += "$($c.Name) lacks purge_delay=0" } }
    $moPlain = @{}
    foreach ($c in @(Get-SmokeCases 'Crash' @() $false $null)) { $moPlain[$c.Name] = $c }
    foreach ($c in @(Add-MiOptionsExpect @(Get-SmokeCases 'Crash' @() $false $null) -1)) { if ($c.Expect.Class -ne 'Pass' -and (@($c.Expect.LogRx) -join "`n") -cne (@($moPlain[$c.Name].Expect.LogRx) -join "`n")) { $moBad += "$($c.Name) fatal expectation changed" } }
    $moOne = & $moNew 'Pass'
    $moLine = "[t=0.000] [f=0] LogCore Inform pcMain.cpp:1: $moRx`r`n"
    $moMiss = { param($case, $text) @(@($case.Expect.LogRx) | Where-Object { $text -cnotmatch $_ }).Count }
    if ((& $moMiss $moOne $moLine) -ne 0) { $moBad += 'mimalloc options log rejected' }
    elseif ((& $moMiss $moOne 'x') -eq 0) { $moBad += 'log without MIOPTIONS accepted' }
    if ($moBad.Count) { $failed++; Write-Host "FAILED ${n}: mimalloc options assertion: $($moBad -join '; ')" }
    else { Write-Host "OK $n Add-MiOptionsExpect asserts the mimalloc options on Pass cases and keeps fatal cases" }
    $n++
    $sdBad = @()
    $sdNew = { param($name) @(Add-SerialTieExpect @(New-Object PSObject -Property @{ Name = $name; Expect = @{ Class = 'Pass' } }))[0] }
    $sdPlain = & $sdNew 'selftest-case'
    $sdFlipLine = $digLine.Replace('state=0x0000ABCD', 'state=0x0000ABCC')
    $sdRes = New-Object PSObject -Property @{ Class = 'Pass'; CaseDir = $digTmp; LogText = ''; Dumps = @(); Turns = '1500' }
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
    $stFatal = @(Add-SerialTieExpect @(New-Object PSObject -Property @{ Name = 'selftest-norow'; Expect = @{ Class = 'Fatal' } }))[0]
    $script:SmokeDigestConfiguration = $stCfg
    $stTie = "[t=1.000] [f=1] LogSim Inform CollisionObject.h:1: OPENSWBF2 COLLISION serialties=2`r`n"
    $stZero = $stTie.Replace('serialties=2', 'serialties=0')
    $env:OPENSWBF2_DIGEST_EXPECTED = $flipExp
    if (& $sdRun $stTest $stTie) { $stBad += 'case with collision serial ties accepted' }
    if (-not (& $sdRun $stTest $stZero)) { $stBad += 'case with zero collision serial ties rejected' }
    if (& $sdRun $stTest '') { $stBad += 'test build pass case without a serialties line accepted' }
    if ($stFatal.Expect.ContainsKey('LogRx')) { $stBad += 'serialties checked on a fatal case' }
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
    $allArgs = $parAll[$p[0]].Args -creplace ' -soakturns \d+', ''
    if (-not $allArgs.EndsWith($sfx)) { $why = "All $($p[0]) lacks the LogMemory Verbose switch"; break }
    if ($allArgs.Substring(0, $allArgs.Length - $sfx.Length) -cne ($p[1][$p[2]].Args -creplace ' -soakturns \d+', '')) { $why = "All $($p[0]) args differ from $($p[2])"; break }
}
if (-not $why -and (-not $parAll['nab2g_c'].Args.EndsWith(" -soakturns $script:SoakShortTurns"))) { $why = 'All nab2g_c must end at the short soak count' }
if (-not $why -and (Get-SmokeCases 'Standard' @() $false $null)[0].Name -cne 'geo1c_con') { $why = 'Standard[0] must be geo1c_con for the ASLR probe' }
if ($why) { $failed++; Write-Host "FAILED ${n}: all/twice digest parity: $why" }
else { Write-Host "OK $n all/twice digest parity" }
foreach ($probeCfg in $exeConfigs) {
    $testExe = Join-Path (Split-Path $PSScriptRoot -Parent) "bin\Tests\$Platform\$probeCfg\OpenSWBF2Tests.exe"
    foreach ($probe in 'strictness probe emits an undeclared error', 'strictness probe unmatched expectation') {
        $n++
        if (-not (Test-Path -LiteralPath $testExe)) { $failed++; Write-Host "FAILED ${n}: ${probe}: $testExe not built"; continue }
        $out = (& $testExe "-tc=$probe" -ns 2>&1 | Out-String)
        $code = $LASTEXITCODE
        if ($code -ne 1 -or $out -notmatch '\[UNEXPECTED-LOG\]') { $failed++; Write-Host "FAILED ${n}: ${probe} ($probeCfg): exit $code, expected 1 with [UNEXPECTED-LOG]" }
        else { Write-Host "OK $n $probe ($probeCfg)" }
    }
}
$grTmp = Join-Path $env:TEMP ("openswbf2-selftest-gr-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $grTmp | Out-Null
$grScript = Join-Path $PSScriptRoot 'guard-revert.ps1'
$repo = Split-Path $PSScriptRoot -Parent
$junction = Join-Path $grTmp 'reparse\nested\link'
$grWtRoot = Join-Path $repo ('artifacts\guard-revert\selftest-wt-' + [guid]::NewGuid().ToString('N'))
$grWtArg = "-WorktreeRoot '$grWtRoot'"
$rsWorktrees = { @(& git -C $repo worktree list --porcelain | Where-Object { $_ -like 'worktree *' } | ForEach-Object { $_.Substring(9).Replace('/', '\') }) }
$grOutside = { @(& $rsWorktrees | Where-Object { -not $_.StartsWith($grWtRoot, [StringComparison]::OrdinalIgnoreCase) }) }
try {
    $rowLine = "`$GuardRevertRows = @(@{ Id = 'SELFTEST-01'; Platform = '$Platform'; File = 'x'; Anchor = 'x'; Replacement = 'x'; Tests = @('x') })"
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
        $out = (& powershell -NoProfile -Command "& '$grScript' -Platform $Platform -RowsFile '$rowsFile' -OutDir '$grOut' -ProvenFile '$(Join-Path $grTmp "proven$n.tsv")' $grWtArg; exit `$LASTEXITCODE" 2>&1 | Out-String)
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
        'function Set-Anchor { $script:SelftestRow = $true; $null }',
        'function Run-Case {',
        '    $state = ''GREEN''',
        "    if (`$script:SelftestRow) { Set-Content -LiteralPath '$midMark' -Value 'x'; `$state = 'RED' }",
        '    New-Object PSObject -Property @{ State = $state; Detail = ''''; Code = 0; Text = ''''; Report = $null }',
        '}'
    )
    $wtBefore = @(& $grOutside).Count
    try {
        $out = (& powershell -NoProfile -Command "& '$grScript' -Platform $Platform -RowsFile '$rowsFile' -OutDir '$(Join-Path $grTmp "out$n")' -ProvenFile '$midProven' $grWtArg; exit `$LASTEXITCODE" 2>&1 | Out-String)
        $code = $LASTEXITCODE
    }
    finally {
        Remove-Item -LiteralPath $midMark -Force -ErrorAction SilentlyContinue
    }
    $wtAfter = @(& $grOutside).Count
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
    $grWt = Join-Path "$repo-worktrees" $Platform
    $grDirs = @(Get-ChildItem -LiteralPath $grWt -Directory -Filter 'w*' -ErrorAction SilentlyContinue | ForEach-Object { $_.Name })
    $grOut = Join-Path $grTmp "out$n"
    $out = @(& powershell -NoProfile -Command "& '$grScript' -Platform $Platform -ListSelected -OutDir '$grOut' -ProvenFile '$(Join-Path $grTmp "proven$n.tsv")'; exit `$LASTEXITCODE" 2>&1 | ForEach-Object { "$_" })
    $code = $LASTEXITCODE
    $wtAfter = @(& git -C $repo worktree list).Count
    $grNew = @(Get-ChildItem -LiteralPath $grWt -Directory -Filter 'w*' -ErrorAction SilentlyContinue | Where-Object { $grDirs -notcontains $_.Name })
    $GuardRevertRows = $null
    . (Join-Path $PSScriptRoot 'guardrevert\rows.ps1')
    $listIds = @($out | Where-Object { $_ -match '^(?:SELECT|CARRY) ' } | ForEach-Object { $_.Split(' ')[1] })
    $rowIds = @(Select-GuardPlatformRows @($GuardRevertRows) $Platform | ForEach-Object { $_.Id })
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
    $pfRows = Join-Path $grTmp "rows$n.ps1"
    Set-Content -Path $pfRows -Encoding ASCII -Value "`$GuardRevertRows = @(@{ Id = 'SELFTEST-PW'; File = 'x'; Anchor = 'x'; Replacement = 'x'; Tests = @('x') }, @{ Id = 'SELFTEST-PX'; Platform = 'x64'; File = 'x'; Anchor = 'x'; Replacement = 'x'; Tests = @('x') })"
    $pfGot = @{}
    $why = $null
    foreach ($pfPlat in 'Win32', 'x64') {
        $pfOut = @(& powershell -NoProfile -Command "& '$grScript' -Platform $pfPlat -RowsFile '$pfRows' -ListSelected -OutDir '$(Join-Path $grTmp "out$n-$pfPlat")' -ProvenFile '$(Join-Path $grTmp "proven$n.tsv")'; exit `$LASTEXITCODE" 2>&1 | ForEach-Object { "$_" })
        if ($LASTEXITCODE -ne 0) { $why = "$pfPlat exit ${LASTEXITCODE}: $($pfOut -join ' / ')"; break }
        $pfGot[$pfPlat] = @($pfOut | Where-Object { $_ -match '^(?:SELECT|CARRY) ' } | ForEach-Object { $_.Split(' ')[1] }) -join ','
    }
    if (-not $why -and $pfGot['Win32'] -cne 'SELFTEST-PW') { $why = "Win32 run listed '$($pfGot['Win32'])', expected only the row with no Platform" }
    elseif (-not $why -and $pfGot['x64'] -cne 'SELFTEST-PX') { $why = "x64 run listed '$($pfGot['x64'])', expected only the row marked x64" }
    if (-not $why) {
        $pfNamed = @(& powershell -NoProfile -Command "& '$grScript' -Platform x64 -RowsFile '$pfRows' -Row SELFTEST-PW -ListSelected -OutDir '$(Join-Path $grTmp "out$n-named")' -ProvenFile '$(Join-Path $grTmp "proven$n.tsv")'; exit `$LASTEXITCODE" 2>&1 | ForEach-Object { "$_" })
        $pfNamedCode = $LASTEXITCODE
        $pfW32Rows = Join-Path $grTmp "rows$n-w32.ps1"
        Set-Content -Path $pfW32Rows -Encoding ASCII -Value "`$GuardRevertRows = @(@{ Id = 'SELFTEST-PW'; File = 'x'; Anchor = 'x'; Replacement = 'x'; Tests = @('x') })"
        $pfNone = @(& powershell -NoProfile -Command "& '$grScript' -Platform x64 -RowsFile '$pfW32Rows' -OutDir '$(Join-Path $grTmp "out$n-none")' -ProvenFile '$(Join-Path $grTmp "proven$n.tsv")'; exit `$LASTEXITCODE" 2>&1 | ForEach-Object { "$_" })
        $pfNoneCode = $LASTEXITCODE
        if ($pfNamedCode -ne 2 -or ($pfNamed -join "`n") -notmatch 'No rows selected on x64') { $why = "-Row naming a Win32 row on x64: exit $pfNamedCode, $($pfNamed -join ' / ')" }
        elseif ($pfNoneCode -ne 0 -or ($pfNone -join "`n") -notmatch '(?m)^No rows for platform x64\.$' -or ($pfNone -join "`n") -notmatch '(?m)^selected 0 carried 0$') { $why = "x64 run with no x64 rows: exit $pfNoneCode, $($pfNone -join ' / ')" }
        elseif (Test-Path -LiteralPath (Join-Path $grTmp "out$n-none")) { $why = 'the x64 run with no x64 rows created its output directory' }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: guard-revert row platform filter: $why" }
    else { Write-Host "OK $n guard-revert drops an x64 row from a Win32 run and keeps only it for an x64 run; a platform with no rows selects nothing and exits 0, and -Row naming another platform's row exits 2" }
    $n++
    $why = $null
    try {
        . (Join-Path $PSScriptRoot 'gate\keys.ps1')
        $grAst = [Management.Automation.Language.Parser]::ParseFile($grScript, [ref]$null, [ref]$null)
        $grDefs = @($grAst.FindAll({ param($a) ($a -is [Management.Automation.Language.FunctionDefinitionAst] -and @('Read-GuardProofs', 'Merge-GuardProofs') -contains $a.Name) -or ($a -is [Management.Automation.Language.AssignmentStatementAst] -and $a.Left.Extent.Text -ceq '$script:ProofColumns') }, $false))
        if ($grDefs.Count -ne 3) { throw "found $($grDefs.Count) of the 3 proof definitions in guard-revert.ps1" }
        foreach ($d in $grDefs) { . ([scriptblock]::Create($d.Extent.Text)) }
        $ProvenFile = Join-Path $grTmp "proven$n.tsv"
        $script:PartialFile = Join-Path $grTmp "proven$n.partial.tsv"
        $pfLine = { param([string]$id, [string]$plat, [string]$commit) @($id, 'rh', 'dd', 'td', 'ts', 'tree', $commit, 'RED', 'GREEN', 'Debug/Release', $plat, 'date') -join "`t" }
        [IO.File]::WriteAllText($ProvenFile, ((@(($script:ProofColumns -join "`t"), (& $pfLine 'P-1' 'Win32' 'w1'), (& $pfLine 'P-2' 'Win32' 'w2')) -join "`r`n") + "`r`n"))
        [IO.File]::WriteAllText($script:PartialFile, (& $pfLine 'P-1' 'x64' 'x1') + "`r`n")
        Merge-GuardProofs
        $m1 = Read-GuardProofs $ProvenFile
        [IO.File]::WriteAllText($script:PartialFile, (& $pfLine 'P-1' 'Win32' 'w3') + "`r`n")
        Merge-GuardProofs
        $m2 = Read-GuardProofs $ProvenFile
        $real = Join-Path $PSScriptRoot 'guardrevert\proven.tsv'
        $realLines = @([IO.File]::ReadAllLines($real) | Select-Object -Skip 1 | Where-Object { $_ }).Count
        $realCount = (Read-GuardProofs $real).Count
        if ($m1.Count -ne 3 -or $m1["P-1`tWin32"].Commit -cne 'w1' -or $m1["P-1`tx64"].Commit -cne 'x1' -or $m1["P-2`tWin32"].Commit -cne 'w2') { $why = "after the x64 merge: $(@($m1.Keys | Sort-Object | ForEach-Object { "$($_ -replace "`t", '/')=$($m1[$_].Commit)" }) -join ' ')" }
        elseif ($m2.Count -ne 3 -or $m2["P-1`tWin32"].Commit -cne 'w3' -or $m2["P-1`tx64"].Commit -cne 'x1') { $why = "after the Win32 merge: $(@($m2.Keys | Sort-Object | ForEach-Object { "$($_ -replace "`t", '/')=$($m2[$_].Commit)" }) -join ' ')" }
        elseif ($realCount -ne $realLines) { $why = "proven.tsv has $realLines data lines but reads as $realCount (Id, Platform) proofs" }
    }
    catch { $why = "threw: $_" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: guard proofs keyed by (Id, Platform): $why" }
    else { Write-Host "OK $n a Win32 and an x64 proof for one Id coexist after a merge, each merge replaces only its own platform, and proven.tsv reads back every line ($realLines)" }
    $rsTest = 'lua next order ignores allocation padding under serial order'
    $rsWrite = {
        param([string]$path, [string]$countFile, [int]$rowCount, [string]$r1)
        $defs = @(1..$rowCount | ForEach-Object {
            $rep = 'x'
            if ($_ -eq 1) { $rep = $r1 }
            "    @{ Id = 'SELFTEST-R$_'; Platform = '$Platform'; File = 'src\LuaFL\src\ltable.cpp'; Anchor = 'SELFTEST-R$_'; Replacement = '$rep'; Tests = @('$rsTest') }"
        })
        Set-Content -Path $path -Encoding ASCII -Value @(
            '$GuardRevertRows = @(',
            ($defs -join ",`r`n"),
            ')',
            'function Assert-Runtimes { }',
            'function Find-GameDataRoot { }',
            'function Find-MSBuild { ''msbuild'' }',
            'function Build-Tests { $true }',
            'function Set-Anchor([string]$path, [string]$anchor, [string]$replacement) { $script:SelftestRow = $anchor; $null }',
            'function Run-Case {',
            '    $state = ''GREEN''',
            '    if ($script:SelftestRow) {',
            "        `$lock = New-Object System.Threading.Mutex(`$false, 'Local\OpenSWBF2-selftest-count-$([guid]::NewGuid().ToString('N'))')",
            '        [void]$lock.WaitOne()',
            '        try {',
            "            `$done = @(Get-Content -LiteralPath '$countFile' -ErrorAction SilentlyContinue)",
            '            $kill = $env:OPENSWBF2_SELFTEST_KILL -and $done.Count -ge [int]$env:OPENSWBF2_SELFTEST_KILL',
            "            if (-not `$kill) { Add-Content -LiteralPath '$countFile' -Value `$script:SelftestRow }",
            '        }',
            '        finally { $lock.ReleaseMutex(); $lock.Dispose() }',
            '        if ($kill) { [Environment]::Exit(3) }',
            '        $state = ''RED''',
            '    }',
            '    New-Object PSObject -Property @{ State = $state; Detail = ''''; Code = 0; Text = ''''; Report = $null }',
            '}'
        )
    }
    $rsRun = {
        param([string]$rowsFile, [string]$outDir, [string]$proven, [string]$kill, [bool]$resume)
        $dirArg = "-OutDir '$outDir'"
        if ($resume) { $dirArg = "-Resume '$outDir'" }
        $env:OPENSWBF2_SELFTEST_KILL = $kill
        try {
            $o = @(& powershell -NoProfile -Command "& '$grScript' -Platform $Platform -RowsFile '$rowsFile' $dirArg -ProvenFile '$proven' -Workers 1 $grWtArg; exit `$LASTEXITCODE" 2>&1 | ForEach-Object { "$_" })
            $c = $LASTEXITCODE
        }
        finally { $env:OPENSWBF2_SELFTEST_KILL = $null }
        New-Object PSObject -Property @{ Out = $o; Code = $c }
    }
    $rsLedger = {
        param([string]$path)
        $lines = @([IO.File]::ReadAllLines($path) | Where-Object { $_ })
        $cols = $lines[0].Split("`t")
        @($lines | Select-Object -Skip 1 | ForEach-Object {
            $f = $_.Split("`t")
            $o = [ordered]@{}
            for ($i = 0; $i -lt $cols.Count; $i++) { $o[$cols[$i]] = $f[$i] }
            [pscustomobject]$o
        })
    }
    $rsClean = -not ((& git -C $repo status --porcelain --untracked-files=normal -- . ':!tests/guardrevert/proven.tsv') -join '')
    $n++
    $rsOut = Join-Path $grTmp "out$n"
    $rsCount = Join-Path $grTmp "count$n.txt"
    $rsProven = Join-Path $grTmp "proven$n.tsv"
    $rsRows = Join-Path $grTmp "rows$n.ps1"
    & $rsWrite $rsRows $rsCount 4 'x'
    $wtBefore = @(& $grOutside)
    $r1 = & $rsRun $rsRows $rsOut $rsProven '2' $false
    $led = @()
    if (Test-Path -LiteralPath (Join-Path $rsOut 'ledger.tsv')) { $led = @(& $rsLedger (Join-Path $rsOut 'ledger.tsv')) }
    $done1 = @(Get-Content -LiteralPath $rsCount -ErrorAction SilentlyContinue)
    $why = $null
    if ($r1.Code -ne 1 -or ($r1.Out -join "`n") -notmatch 'rows worker w1 exited 3') { $why = "exit $($r1.Code), expected 1 naming the worker killed with exit 3" }
    elseif ($led.Count -ne 2) { $why = "ledger has $($led.Count) rows, expected 2" }
    elseif (($led | ForEach-Object { "$($_.Id)=$($_.Outcome)" }) -join ' ' -cne 'SELFTEST-R1=PASS SELFTEST-R2=PASS') { $why = "ledger rows are $(($led | ForEach-Object { "$($_.Id)=$($_.Outcome)" }) -join ' ')" }
    elseif (($done1 -join ' ') -cne 'SELFTEST-R1 SELFTEST-R2') { $why = "rows run before the kill: $($done1 -join ' ')" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: guard-revert killed after two rows leaves a two-row ledger: $why" }
    else { Write-Host "OK $n guard-revert killed after two rows leaves a two-row ledger" }
    $n++
    $rsW1 = Join-Path $grWtRoot "$Platform\w1"
    $rsKeep = Join-Path $rsW1 'obj\selftest-keep.txt'
    $rsStray = Join-Path $rsW1 'selftest-stray.txt'
    $rsTracked = Join-Path $rsW1 '.editorconfig'
    $rsTrackedText = $null
    if (Test-Path -LiteralPath $rsTracked) {
        $rsTrackedText = [IO.File]::ReadAllText($rsTracked)
        New-Item -ItemType Directory -Force -Path (Split-Path $rsKeep -Parent) | Out-Null
        Set-Content -LiteralPath $rsKeep -Value 'x'
        Set-Content -LiteralPath $rsStray -Value 'x'
        [IO.File]::AppendAllText($rsTracked, 'selftest edit')
    }
    $rsResumeProven = "$rsProven.resume"
    $r2 = & $rsRun $rsRows $rsOut $rsResumeProven '' $true
    $why = $null
    if ($null -eq $rsTrackedText) { $why = "no persistent worktree at $rsW1 after the first run" }
    elseif (-not (Test-Path -LiteralPath $rsKeep)) { $why = 'the ignored obj file was removed, so the reuse cleans ignored files' }
    elseif (Test-Path -LiteralPath $rsStray) { $why = 'the untracked file survived the reuse' }
    elseif ([IO.File]::ReadAllText($rsTracked) -cne $rsTrackedText) { $why = 'the edited tracked file was not restored' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: guard-revert reuses its worktree: $why" }
    else { Write-Host "OK $n guard-revert reuses its worktree, restores tracked files, drops untracked ones and keeps ignored obj" }
    $n++
    $done2 = @(Get-Content -LiteralPath $rsCount -ErrorAction SilentlyContinue | Select-Object -Skip $done1.Count)
    $resumed = @($r2.Out | Where-Object { $_ -match '^RESUMED ' } | ForEach-Object { $_.Split(' ')[1] })
    $sumIds = @()
    if (Test-Path -LiteralPath (Join-Path $rsOut 'summary.txt')) { $sumIds = @(Get-Content -LiteralPath (Join-Path $rsOut 'summary.txt') | Where-Object { $_ -match '^SELFTEST-R\d' } | ForEach-Object { $_.Split(' ')[0] } | Sort-Object) }
    $why = $null
    if ($r2.Code -ne 0) { $why = "exit $($r2.Code): $((@($r2.Out | Where-Object { $_ -match '^ERROR' }) + @($r2.Out | Select-Object -Last 3)) -join ' / ')" }
    elseif (($done2 -join ' ') -cne 'SELFTEST-R3 SELFTEST-R4') { $why = "resume ran: $($done2 -join ' ')" }
    elseif (($resumed -join ' ') -cne 'SELFTEST-R1 SELFTEST-R2') { $why = "RESUMED lines: $($resumed -join ' ')" }
    elseif (($sumIds -join ' ') -cne 'SELFTEST-R1 SELFTEST-R2 SELFTEST-R3 SELFTEST-R4') { $why = "summary.txt rows: $($sumIds -join ' ')" }
    elseif ($rsClean -and (-not (Test-Path -LiteralPath $rsResumeProven) -or (@((& $rsLedger $rsResumeProven) | ForEach-Object { $_.Id }) -join ' ') -cne 'SELFTEST-R1 SELFTEST-R2 SELFTEST-R3 SELFTEST-R4')) { $why = 'proven file lacks the resumed rows' }
    elseif (@(& $grOutside).Count -ne $wtBefore.Count) { $why = 'worktree count changed' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: guard-revert -Resume runs only rows 3 and 4: $why" }
    else { Write-Host "OK $n guard-revert -Resume runs only rows 3 and 4 and counts all four" }
    $n++
    $rsOut = Join-Path $grTmp "out$n"
    $rsCount = Join-Path $grTmp "count$n.txt"
    $rsProven = Join-Path $grTmp "proven$n.tsv"
    $rsRows = Join-Path $grTmp "rows$n.ps1"
    & $rsWrite $rsRows $rsCount 10 'x'
    $wtBefore = @(& $grOutside)
    $r1 = & $rsRun $rsRows $rsOut $rsProven '9' $false
    $done1 = @(Get-Content -LiteralPath $rsCount -ErrorAction SilentlyContinue)
    $why = $null
    if ($r1.Code -ne 1 -or $done1.Count -ne 9) { $why = "first run exit $($r1.Code) after $($done1.Count) rows, expected 1 after 9" }
    else {
        & $rsWrite $rsRows $rsCount 10 'y'
        $ledPath = Join-Path $rsOut 'ledger.tsv'
        $ledLines = [IO.File]::ReadAllLines($ledPath)
        $cols = $ledLines[0].Split("`t")
        $tamper = @{ 'SELFTEST-R2' = @('DepDigest', '-'); 'SELFTEST-R3' = @('Outcome', 'FAIL'); 'SELFTEST-R4' = @('Tree', 'x'); 'SELFTEST-R5' = @('Toolset', 'x'); 'SELFTEST-R6' = @('Config', 'x'); 'SELFTEST-R7' = @('TriggerDigest', 'x'); 'SELFTEST-R8' = @('DepDigest', 'x'); 'SELFTEST-R9' = @('Platform', (@('Win32', 'x64') | Where-Object { $_ -cne $Platform })) }
        for ($i = 1; $i -lt $ledLines.Count; $i++) {
            $f = $ledLines[$i].Split("`t")
            if ($tamper.ContainsKey($f[0])) { $f[[array]::IndexOf($cols, $tamper[$f[0]][0])] = $tamper[$f[0]][1] }
            $ledLines[$i] = $f -join "`t"
        }
        [IO.File]::WriteAllText($ledPath, (($ledLines -join "`r`n") + "`r`n"))
        $r2 = & $rsRun $rsRows $rsOut "$rsProven.resume" '' $true
        $done2 = @(Get-Content -LiteralPath $rsCount -ErrorAction SilentlyContinue | Select-Object -Skip $done1.Count)
        $resumed = @($r2.Out | Where-Object { $_ -match '^RESUMED ' } | ForEach-Object { $_.Split(' ')[1] })
        if ($r2.Code -ne 0) { $why = "resume exit $($r2.Code): $((@($r2.Out | Where-Object { $_ -match '^ERROR' }) + @($r2.Out | Select-Object -Last 3)) -join ' / ')" }
        elseif ($resumed.Count) { $why = "resumed $($resumed -join ' ') despite a changed row, a '-' digest, a failed outcome or a changed key field" }
        elseif (($done2 -join ' ') -cne ((1..10 | ForEach-Object { "SELFTEST-R$_" }) -join ' ')) { $why = "resume ran: $($done2 -join ' ')" }
        elseif (@(& $grOutside).Count -ne $wtBefore.Count) { $why = 'worktree count changed' }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: guard-revert -Resume re-runs changed, '-', failed and mismatched rows: $why" }
    else { Write-Host "OK $n guard-revert -Resume re-runs a changed row, a '-' digest, a failed outcome and each mismatched key field" }
    $n++
    $pwOut = Join-Path $grTmp "out$n"
    $pwCount = Join-Path $grTmp "count$n.txt"
    $pwRows = Join-Path $grTmp "rows$n.ps1"
    & $rsWrite $pwRows $pwCount 6 'x'
    $pwRun = @(& powershell -NoProfile -Command "& '$grScript' -Platform $Platform -RowsFile '$pwRows' -OutDir '$pwOut' -ProvenFile '$(Join-Path $grTmp "proven$n.tsv")' -Workers 3 $grWtArg; exit `$LASTEXITCODE" 2>&1 | ForEach-Object { "$_" })
    $pwCode = $LASTEXITCODE
    $why = $null
    $pwIds = (1..6 | ForEach-Object { "SELFTEST-R$_" }) -join ' '
    $pwLed = @()
    if (Test-Path -LiteralPath (Join-Path $pwOut 'ledger.tsv')) { $pwLed = @(& $rsLedger (Join-Path $pwOut 'ledger.tsv')) }
    $pwDone = @(Get-Content -LiteralPath $pwCount -ErrorAction SilentlyContinue | Sort-Object { [int]($_ -replace '\D', '') })
    $pwSum = @()
    if (Test-Path -LiteralPath (Join-Path $pwOut 'summary.txt')) { $pwSum = @(Get-Content -LiteralPath (Join-Path $pwOut 'summary.txt') | Where-Object { $_ -match '^SELFTEST-R\d' } | ForEach-Object { $_.Split(' ')[0] }) }
    $pwLogs = @(1..3 | Where-Object { Test-Path -LiteralPath (Join-Path $pwOut "worker-rows-w$_.log") })
    $pwTrees = @(1..3 | Where-Object { [IO.File]::Exists((Join-Path $grWtRoot "$Platform\w$_\.git")) })
    if ($pwCode -ne 0) { $why = "exit ${pwCode}: $((@($pwRun | Where-Object { $_ -match '^ERROR' }) + @($pwRun | Select-Object -Last 3)) -join ' / ')" }
    elseif (($pwDone -join ' ') -cne $pwIds) { $why = "rows run: $($pwDone -join ' ')" }
    elseif ((@($pwLed | Where-Object { $_.Outcome -ceq 'PASS' } | ForEach-Object { $_.Id } | Sort-Object { [int]($_ -replace '\D', '') }) -join ' ') -cne $pwIds -or $pwLed.Count -ne 6) { $why = "ledger: $(@($pwLed | ForEach-Object { "$($_.Id)=$($_.Outcome)" }) -join ' ')" }
    elseif (($pwSum -join ' ') -cne $pwIds) { $why = "summary.txt rows in order: $($pwSum -join ' ')" }
    elseif ($pwLogs.Count -ne 3 -or $pwTrees.Count -ne 3) { $why = "$($pwLogs.Count) row worker logs and $($pwTrees.Count) worktrees, expected 3 each" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: guard-revert -Workers 3: $why" }
    else { Write-Host "OK $n guard-revert -Workers 3 runs each row once in three worktrees with one ledger line per row" }
    $n++
    $ldRun = {
        param([string]$row)
        $mark = Join-Path $grTmp "loaded$n.txt"
        $rows = Join-Path $grTmp "rows$n.ps1"
        Set-Content -Path $rows -Encoding ASCII -Value @(
            "`$GuardRevertRows = @($row)",
            "function Get-SelftestLoaded([string]`$at) { Add-Content -LiteralPath '$mark' -Value (`"`$at smoke=`$([bool](Get-Command Invoke-SmokeCase -ErrorAction SilentlyContinue)) cases=`$([bool](Get-Command Get-SmokeCases -ErrorAction SilentlyContinue)) t1=`$([bool](Get-Command Invoke-T1Exe -ErrorAction SilentlyContinue))`") }",
            'function Assert-Runtimes { Get-SelftestLoaded main }',
            'function Find-GameDataRoot { }',
            'function Find-MSBuild { ''msbuild'' }',
            'function Build-Tests { $true }',
            'function Build-Game { $true }',
            'function Set-Anchor { $null }',
            'function Run-Case { Get-SelftestLoaded worker; New-Object PSObject -Property @{ State = ''GREEN''; Detail = ''''; Code = 0; Text = ''''; Report = $null } }',
            'function Get-GuardHardChecks { Get-SelftestLoaded worker; @(New-Object PSObject -Property @{ Name = ''c''; Ok = $true; Detail = '''' }) }'
        )
        $r = & $rsRun $rows (Join-Path $grTmp "out$n") (Join-Path $grTmp "proven$n.tsv") '' $false
        $seen = @(Get-Content -LiteralPath $mark -ErrorAction SilentlyContinue | Sort-Object -Unique)
        New-Object PSObject -Property @{ Seen = $seen; Out = $r.Out }
    }
    $ld = & $ldRun "@{ Id = 'SELFTEST-LD1'; Platform = '$Platform'; File = 'x'; Anchor = 'x'; Replacement = 'x'; Tests = @('x') }"
    $why = $null
    if (($ld.Seen -join ' | ') -cne 'main smoke=False cases=False t1=True | worker smoke=False cases=False t1=True') { $why = "loaded: $($ld.Seen -join ' | '); $((@($ld.Out | Where-Object { $_ -match '^ERROR' }) + @($ld.Out | Select-Object -Last 3)) -join ' / ')" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: a T1-only guard-revert run loads common.ps1 and not smoke.ps1: $why" }
    else { Write-Host "OK $n a T1-only guard-revert run loads common.ps1 and not smoke.ps1 in the main process or its worker" }
    $n++
    $ld = & $ldRun "@{ Id = 'SELFTEST-LD2'; Platform = '$Platform'; Kind = 'Hardening'; Check = 'c'; File = 'x'; Anchor = 'x'; Replacement = 'x' }"
    $why = $null
    if (($ld.Seen -join ' | ') -cne 'main smoke=True cases=True t1=True | worker smoke=True cases=True t1=True') { $why = "loaded: $($ld.Seen -join ' | '); $((@($ld.Out | Where-Object { $_ -match '^ERROR' }) + @($ld.Out | Select-Object -Last 3)) -join ' / ')" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: a guard-revert run with a Hardening row loads smoke.ps1: $why" }
    else { Write-Host "OK $n a guard-revert run with a Hardening row loads smoke.ps1 in the main process and its worker" }
    $n++
    $lcMark = Join-Path $grTmp "luac$n.txt"
    $lcRows = Join-Path $grTmp "rows$n.ps1"
    Set-Content -Path $lcRows -Encoding ASCII -Value @(
        "`$GuardRevertRows = @(@{ Id = 'SELFTEST-LC1'; Platform = '$Platform'; Luac = `$true; File = 'x'; Anchor = 'x'; Replacement = 'x'; Tests = @('luac case') }, @{ Id = 'SELFTEST-LC2'; Platform = '$Platform'; File = 'x'; Anchor = 'x'; Replacement = 'x'; Tests = @('plain case') })",
        'function Assert-Runtimes { }',
        'function Find-GameDataRoot { }',
        'function Find-MSBuild { ''msbuild'' }',
        "function msbuild { Add-Content -LiteralPath '$lcMark' -Value ((Split-Path `$args[0] -Leaf) + ' ' + `$args[1] + ' ' + `$args[2]); `$global:LASTEXITCODE = 0 }",
        "function Build-Tests([string]`$wt, [string]`$logPath, [string]`$config) { Add-Content -LiteralPath '$lcMark' -Value ('tests ' + `$config); `$true }",
        'function Set-Anchor { $script:SelftestRow = $true; $null }',
        'function Run-Case {',
        '    $state = ''GREEN''',
        '    if ($script:SelftestRow) { $state = ''RED'' }',
        '    New-Object PSObject -Property @{ State = $state; Detail = ''''; Code = 0; Text = ''''; Report = $null }',
        '}'
    )
    $lc = & $rsRun $lcRows (Join-Path $grTmp "out$n") (Join-Path $grTmp "proven$n.tsv") '' $false
    $lcSeen = @(Get-Content -LiteralPath $lcMark -ErrorAction SilentlyContinue)
    $lcWant = @{
        'LuaCompiler.vcxproj /p:Configuration=Debug /p:Platform=Win32' = 2
        'LuaCompiler.vcxproj /p:Configuration=Debug /p:Platform=x64' = 2
        'tests Debug' = 4
    }
    $why = $null
    if ($lc.Code -ne 0) { $why = "exit $($lc.Code): $((@($lc.Out | Where-Object { $_ -match '^ERROR' }) + @($lc.Out | Select-Object -Last 3)) -join ' / ')" }
    elseif (-not (($lc.Out -join "`n") -match 'SELFTEST-LC1\s+control=GREEN\s+reverted=RED') -or -not (($lc.Out -join "`n") -match 'SELFTEST-LC2\s+control=GREEN\s+reverted=RED')) { $why = 'a row did not run control GREEN and reverted RED' }
    elseif ($lcSeen.Count -ne 8) { $why = "$($lcSeen.Count) builds: $($lcSeen -join ' | ')" }
    else {
        foreach ($k in $lcWant.Keys) {
            $got = @($lcSeen | Where-Object { $_ -ceq $k }).Count
            if ($got -ne $lcWant[$k]) { $why = "$got builds of '$k', expected $($lcWant[$k]): $($lcSeen -join ' | ')"; break }
        }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: a Luac guard row builds LuaCompiler for Win32 and x64 in its control and reverted builds: $why" }
    else { Write-Host "OK $n a Luac guard row builds LuaCompiler for Win32 and x64 in its control and reverted builds, and a row without Luac builds only the tests" }
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
    foreach ($w in @(Get-ChildItem -LiteralPath (Join-Path $grWtRoot $Platform) -Directory -ErrorAction SilentlyContinue)) { Remove-HarnessTree $w.FullName }
    if (Test-Path -LiteralPath $grWtRoot) { Remove-HarnessTree $grWtRoot }
    & git -C $repo worktree prune 2>&1 | Out-Null
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
$hxTmp = Join-Path $env:TEMP ("openswbf2-selftest-harness-" + [guid]::NewGuid().ToString('N'))
$hxLinks = @()
try {
    $hxData = Join-Path $hxTmp 'src\data'
    $hxOutside = Join-Path $hxTmp 'outside'
    foreach ($t in $hxData, $hxOutside) {
        New-Item -ItemType Directory -Force -Path $t | Out-Null
        Set-Content -Path (Join-Path $t 'sentinel.txt') -Value 'x'
    }
    $hxSentinels = { (Test-Path -LiteralPath (Join-Path $hxData 'sentinel.txt')) -and (Test-Path -LiteralPath (Join-Path $hxOutside 'sentinel.txt')) }
    $hxMake = {
        param([string]$dir, [bool]$worktree, [bool]$second)
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
        Set-Content -Path (Join-Path $dir 'BattlefrontII.exe') -Value 'x'
        if ($worktree) { Set-Content -Path (Join-Path $dir '.git') -Value 'gitdir: C:/nowhere/.git/worktrees/x' }
        $l = Join-Path $dir 'data'
        & cmd /c mklink /J "$l" "$hxData" | Out-Null
        $script:hxLinks += $l
        if ($second) {
            New-Item -ItemType Directory -Force -Path (Join-Path $dir 'nested') | Out-Null
            $l = Join-Path $dir 'nested\link'
            & cmd /c mklink /J "$l" "$hxOutside" | Out-Null
            $script:hxLinks += $l
        }
    }
    $hxRefused = {
        param([string]$dir)
        try { Remove-HarnessTree $dir; return 'no refusal' }
        catch { if ($_.Exception.Message -notlike 'REFUSING to remove*') { return "threw: $($_.Exception.Message)" } }
        if (-not (Test-Path -LiteralPath $dir)) { return 'folder is gone' }
        if (-not (& $hxSentinels)) { return 'a junction target sentinel is gone' }
        return $null
    }

    $n++
    $why = $null
    $hxWorker = Join-Path $hxTmp 'worker'
    & $hxMake $hxWorker $false $false
    try { Remove-HarnessTree $hxWorker } catch { $why = "threw: $($_.Exception.Message)" }
    if (-not $why) {
        if (Test-Path -LiteralPath $hxWorker) { $why = 'worker folder still exists' }
        elseif (-not (& $hxSentinels)) { $why = 'data junction target sentinel is gone' }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: Remove-HarnessTree removes a worker and its data junction: $why" }
    else { Write-Host "OK $n Remove-HarnessTree removes a worker, unlinks its data junction and keeps the target" }

    $n++
    $hxWorker2 = Join-Path $hxTmp 'worker2'
    & $hxMake $hxWorker2 $false $true
    $why = & $hxRefused $hxWorker2
    if (-not $why -and -not (Test-Path -LiteralPath (Join-Path $hxWorker2 'data\sentinel.txt'))) { $why = 'the data junction was unlinked before the refusal' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: Remove-HarnessTree refuses a worker with a second junction: $why" }
    else { Write-Host "OK $n Remove-HarnessTree refuses a worker with a second junction and touches nothing" }

    $n++
    $hxWt = Join-Path $hxTmp 'worktree'
    & $hxMake $hxWt $true $false
    $why = & $hxRefused $hxWt
    if ($why) { $failed++; Write-Host "FAILED ${n}: Remove-HarnessTree refuses a worktree holding a junction: $why" }
    else { Write-Host "OK $n Remove-HarnessTree refuses a worktree holding a junction" }

    $n++
    $hxSelf = Join-Path $hxTmp 'selflink'
    & cmd /c mklink /J "$hxSelf" "$hxOutside" | Out-Null
    $hxLinks += $hxSelf
    $why = & $hxRefused $hxSelf
    $hxRepo = Join-Path $hxTmp 'repo'
    New-Item -ItemType Directory -Force -Path (Join-Path $hxRepo '.git') | Out-Null
    if (-not $why) { $why = & $hxRefused $hxRepo }
    if (-not $why) {
        try { Remove-HarnessTree (Join-Path $hxTmp 'missing') } catch { $why = "missing folder threw: $($_.Exception.Message)" }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: Remove-HarnessTree refuses a junction folder and a .git folder: $why" }
    else { Write-Host "OK $n Remove-HarnessTree refuses a junction folder and a folder with .git, and skips a missing one" }

    $n++
    $why = $null
    $hxPaths = @(
        (Get-WorkerPath 'C:\r' 'Win32' 'Release' 'smoke' 1),
        (Get-WorkerPath 'C:\r' 'x64' 'Release' 'smoke' 1),
        (Get-WorkerPath 'C:\r' 'Win32' 'Test' 'smoke' 1),
        (Get-WorkerPath 'C:\r' 'Win32' 'Release' 'guard' 1),
        (Get-WorkerPath 'C:\r' 'Win32' 'Release' 'smoke' 2)
    )
    if (@($hxPaths | Sort-Object -Unique).Count -ne 5) { $why = "paths not distinct: $($hxPaths -join ', ')" }
    elseif ($hxPaths[0] -cne 'C:\r\artifacts\smoke-workers\Win32\Release\smoke\w1') { $why = "unexpected layout: $($hxPaths[0])" }
    if (-not $why) {
        $hxBad = @(
            @('index above WorkerIndexMax', { Get-WorkerPath 'C:\r' 'Win32' 'Release' 'smoke' ($script:WorkerIndexMax + 1) }),
            @('index 0', { Get-WorkerPath 'C:\r' 'Win32' 'Release' 'smoke' 0 }),
            @('unknown kind', { Get-WorkerPath 'C:\r' 'Win32' 'Release' 'build' 1 }),
            @('unknown platform', { Get-WorkerPath 'C:\r' 'ARM64' 'Release' 'smoke' 1 }),
            @('unknown configuration', { Get-WorkerPath 'C:\r' 'Win32' 'Profile' 'smoke' 1 })
        )
        foreach ($b in $hxBad) {
            $threw = $false
            try { & $b[1] | Out-Null } catch { $threw = $true }
            if (-not $threw) { $why = "$($b[0]) did not throw"; break }
        }
    }
    if (-not $why -and $script:WorkerIndexMax -ne [Environment]::ProcessorCount) { $why = "WorkerIndexMax is $($script:WorkerIndexMax), expected the logical processor count" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: worker paths: $why" }
    else { Write-Host "OK $n worker paths are distinct per platform, configuration, kind and index, and stay inside 1..WorkerIndexMax" }

    $n++
    $why = $null
    $hxA = @(Get-WorkerPaths 'C:\r' 'Win32' 'Release' 'smoke' 0 8)
    $hxB = @(Get-WorkerPaths 'C:\r' 'Win32' 'Release' 'smoke' 8 8)
    if ($hxA.Count -ne 8 -or $hxB.Count -ne 8) { $why = "got $($hxA.Count) and $($hxB.Count) paths, expected 8 and 8" }
    elseif (@($hxA + $hxB | Sort-Object -Unique).Count -ne 16) { $why = "base 0 and base 8 share a folder: $(@($hxA | Where-Object { $hxB -contains $_ }) -join ', ')" }
    elseif ($hxA[0] -cne (Get-WorkerPath 'C:\r' 'Win32' 'Release' 'smoke' 1) -or $hxB[-1] -cne (Get-WorkerPath 'C:\r' 'Win32' 'Release' 'smoke' 16)) { $why = "ranges are $($hxA[0])..$($hxB[-1]), expected w1..w16" }
    if (-not $why) {
        foreach ($b in @(@(-1, 1), @(($script:WorkerIndexMax - 7), 8), @(0, 0))) {
            $threw = $false
            try { Get-WorkerPaths 'C:\r' 'Win32' 'Release' 'smoke' $b[0] $b[1] | Out-Null } catch { $threw = $true }
            if (-not $threw) { $why = "base $($b[0]) count $($b[1]) did not throw"; break }
        }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: worker ranges: $why" }
    else { Write-Host "OK $n worker ranges for base 0 and base 8 at 8 workers are disjoint, and a range outside 1..WorkerIndexMax throws" }

    $n++
    $why = $null
    $hxSavedData = $env:SWBF2_GAMEDATA
    $env:SWBF2_GAMEDATA = Join-Path $hxTmp 'no-gamedata'
    try {
        foreach ($wb in @($script:WorkerIndexMax, -1)) {
            $out = (& powershell -NoProfile -Command "& '$run' -Platform $Platform -NoBuild -Smoke Standard -Jobs 1 -WorkerBase $wb; exit `$LASTEXITCODE" 2>&1 | Out-String)
            $code = $LASTEXITCODE
            if ($code -ne 2 -or $out -notmatch "-WorkerBase ${wb}: workers -?\d+\.\.-?\d+ are outside 1\.\.$($script:WorkerIndexMax)") { $why = "-WorkerBase $wb gave exit $code, expected 2 naming the range"; break }
        }
    }
    finally { $env:SWBF2_GAMEDATA = $hxSavedData }
    if ($why) { $failed++; Write-Host "FAILED ${n}: run.ps1 -WorkerBase bounds: $why" }
    else { Write-Host "OK $n run.ps1 exits 2 naming the range when -WorkerBase overflows or is negative" }

    $n++
    $why = $null
    $hxBan = @(('worktree\s+remove\s+' + '--force'), ('git\b[^\r\n]*\bclean\s+' + '-[a-z]*x'))
    foreach ($f in @(Get-ChildItem -LiteralPath $PSScriptRoot -Recurse -File | Where-Object { $_.Extension -in '.ps1', '.md', '.yml', '.cmd' })) {
        $text = [IO.File]::ReadAllText($f.FullName)
        foreach ($rx in $hxBan) { if ($text -match $rx) { $why = "$($f.FullName) matches /$rx/"; break } }
        if ($why) { break }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: no forced worktree removal or clean -x: $why" }
    else { Write-Host "OK $n nothing under tests forces a worktree removal or cleans ignored files" }
}
catch { $failed++; Write-Host "FAILED ${n}: harness tree fixtures threw: $($_.Exception.Message)" }
finally {
    foreach ($l in $hxLinks) { if (Test-Path -LiteralPath $l) { & cmd /c rmdir "$l" | Out-Null } }
    $hxLeft = @($hxLinks | Where-Object { Test-Path -LiteralPath $_ })
    if ($hxLeft.Count -eq 0 -and (Test-Path -LiteralPath $hxTmp)) { Remove-Item -LiteralPath $hxTmp -Recurse -Force -ErrorAction SilentlyContinue }
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
    $selFiles = @('src\G\Game.cpp', 'src\G\Game.h', 'src\G\Lib.cpp', 'src\G\Lib.h', 'src\G\Shared.h', 'src\X\pcWindow.cpp', 'src\G\Other.cpp', 'tests\unit\t_lib.cpp', 'tests\unit\t_pin.cpp', 'tests\unit\t_other.cpp', 'tests\stubs\stub.cpp', 'tests\corpus\c\1.txt', 'data\scripts\a.lua', 'tests\smoke\s.txt', 'BattlefrontII.vcxproj', 'tests\smoke.ps1', 'tests\common.ps1', 'tests\smoke\cases.ps1', 'tests\run.ps1', 'tests\unit\t_base.cpp', 'tests\smoke\digest-expected.txt')
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
    Set-Content -Encoding Unicode -Path $testsTlog -Value @("^${R}SRC\G\LIB.CPP", "${R}SRC\G\LIB.H", "${R}SRC\G\SHARED.H", "^${R}TESTS\UNIT\T_LIB.CPP", "${R}SRC\G\LIB.H", "^${R}TESTS\UNIT\T_PIN.CPP", "^${R}TESTS\UNIT\T_OTHER.CPP", "^${R}TESTS\UNIT\T_BASE.CPP", "^${R}TESTS\STUBS\STUB.CPP")
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
        @{ Id = 'S-SMOKE2'; Kind = 'Smoke'; Case = 'crash-y'; File = 'src\G\Other.cpp'; Anchor = 'a'; Replacement = 'b' },
        @{ Id = 'S-PERTURB'; Kind = 'Perturb'; Config = 'Test'; Missions = @('m1'); File = 'src\G\Game.h'; Anchor = 'a'; Replacement = 'b' },
        @{ Id = 'S-TLOG'; Kind = 'Tlog'; Project = 'game'; Check = 'f'; File = 'BattlefrontII.vcxproj'; Anchor = 'a'; Replacement = 'b' },
        @{ Id = 'S-BASE'; File = 'src\G\Lib.cpp'; Anchor = 'a'; Replacement = 'b'; Tests = @('base table') }
    )
    $selAll = @($selRows | ForEach-Object { $_.Id })
    $selT1 = @('S-LIB', 'S-PIN', 'S-HDR', 'S-ASAN', 'S-CANARY', 'S-BASE')
    $selScript = @('S-HARD', 'S-HARD2', 'S-SMOKE', 'S-SMOKE2', 'S-PERTURB', 'S-TLOG')
    $selBase = @{
        Rows = $selRows; Proofs = @{}; Hashes = $selHashes; TestsReads = $selTests; AsanReads = $selTests; GameReads = $selGame
        TestsProjectGameTUs = @('src\G\Lib.cpp')
        TestSources = @{ 'lib works' = 'tests\unit\t_lib.cpp'; 'pin text' = 'tests\unit\t_pin.cpp'; 'other case' = 'tests\unit\t_other.cpp'; 'base table' = 'tests\unit\t_base.cpp' }
        TestLiterals = @{ 'tests\unit\t_pin.cpp' = @('tests\corpus\c\'); 'tests\unit\t_base.cpp' = @('tests\smoke\digest-expected.txt') }
        CaseTexts = @{ 'S-SMOKE' = "case`tcrash-x"; 'S-SMOKE2' = "case`tcrash-y"; 'S-PERTURB' = "case`tperturb-m1" }
        CaseData = 'cases data'
        Toolset = '14.44.35207'
        Platform = 'Win32'
    }
    Test-SelCase 'missing record selects every row' @{} $selAll 'no record'
    $selProofs = @{}
    foreach ($x in (Invoke-SelCase @{}).Values) {
        $selProofs["$($x.Id)`tWin32"] = @{ RowHash = $x.RowHash; DepDigest = $x.DepDigest; TriggerDigest = $x.TriggerDigest; Toolset = '14.44.35207'; Platform = 'Win32'; Commit = 'c0ffee1'; Date = '2026-11-04' }
    }
    $selBase.Proofs = $selProofs
    Test-SelCase 'unchanged inputs carry every row with its proof commit' @{} @() $null
    $p2 = @{}
    foreach ($k in $selProofs.Keys) { if ($k -cne "S-HARD2`tWin32") { $p2[$k] = $selProofs[$k] } }
    Test-SelCase 'one missing record selects only that row' @{ Proofs = $p2 } @('S-HARD2') 'no record'
    Test-SelCase '.cpp touched selects its rows and the game read sets that contain it' @{ Hashes = (Copy-SelHashes 'src\G\Game.cpp' 'new') } @('S-HARD', 'S-SMOKE', 'S-SMOKE2', 'S-PERTURB') 'deps changed'
    Test-SelCase 'header touched selects every read set that contains it' @{ Hashes = (Copy-SelHashes 'src\G\Shared.h' 'new') } @('S-LIB', 'S-PIN', 'S-HDR', 'S-ASAN', 'S-CANARY', 'S-BASE', 'S-HARD', 'S-SMOKE', 'S-SMOKE2', 'S-PERTURB') 'deps changed'
    Test-SelCase 'test file touched selects only its cases rows' @{ Hashes = (Copy-SelHashes 'tests\unit\t_pin.cpp' 'new') } @('S-PIN') 'deps changed'
    Test-SelCase 'directory literal file touched selects its test rows' @{ Hashes = (Copy-SelHashes 'tests\corpus\c\1.txt' 'new') } @('S-PIN') 'deps changed'
    $r2 = @($selRows | ForEach-Object { $_.Clone() })
    ($r2 | Where-Object { $_.Id -eq 'S-HARD2' }).Anchor = 'z'
    Test-SelCase 'row edited selects only that row' @{ Rows = $r2 } @('S-HARD2') 'row changed'
    Test-SelCase 'vcxproj touched selects every row' @{ Hashes = (Copy-SelHashes 'BattlefrontII.vcxproj' 'new') } $selAll 'global trigger*'
    Test-SelCase 'smoke.ps1 logic touched selects every smoke-script row and no T1 row' @{ Hashes = (Copy-SelHashes 'tests\smoke.ps1' 'new') } $selScript 'deps changed'
    Test-SelCase 'common.ps1 touched selects every row' @{ Hashes = (Copy-SelHashes 'tests\common.ps1' 'new') } $selAll 'global trigger*'
    Test-SelCase 'run.ps1 touched selects no guard row' @{ Hashes = (Copy-SelHashes 'tests\run.ps1' 'new') } @() $null
    Test-SelCase 'smoke\cases.ps1 bytes reach rows only through case data and case texts' @{ Hashes = (Copy-SelHashes 'tests\smoke\cases.ps1' 'new') } @() $null
    Test-SelCase 'case data changed selects every smoke-script row and no T1 row' @{ CaseData = 'cases data changed' } $selScript 'deps changed'
    Test-SelCase 'one case text changed selects only the rows naming that case' @{ CaseTexts = @{ 'S-SMOKE' = "case`tcrash-x changed"; 'S-SMOKE2' = "case`tcrash-y"; 'S-PERTURB' = "case`tperturb-m1" } } @('S-SMOKE') 'deps changed'
    Test-SelCase 'perturb case text changed selects only the perturb row' @{ CaseTexts = @{ 'S-SMOKE' = "case`tcrash-x"; 'S-SMOKE2' = "case`tcrash-y"; 'S-PERTURB' = "case`tperturb-m1 changed" } } @('S-PERTURB') 'deps changed'
    Test-SelCase 'a case name with no case text is a doubt' @{ CaseTexts = @{ 'S-SMOKE' = "case`tcrash-x"; 'S-PERTURB' = "case`tperturb-m1" } } @('S-SMOKE2') 'unknown case crash-y'
    Test-SelCase 'no case data is a doubt for every smoke-script row' @{ CaseData = $null } $selScript 'unknown case*'
    Test-SelCase 'an expectation file a T1 test names selects only that test row' @{ Hashes = (Copy-SelHashes 'tests\smoke\digest-expected.txt' 'new') } @('S-BASE') 'deps changed'
    Test-SelCase 'stubs touched selects every row' @{ Hashes = (Copy-SelHashes 'tests\stubs\stub.cpp' 'new') } $selAll 'global trigger*'
    Test-SelCase 'toolset changed selects every row' @{ Toolset = '14.50.1' } $selAll 'toolset'
    $pX64 = @{}; $pNone = @{}
    foreach ($k in $selProofs.Keys) { $a = $selProofs[$k].Clone(); $a.Platform = 'x64'; $pX64[$k] = $a; $b = $selProofs[$k].Clone(); $b.Remove('Platform'); $pNone[$k] = $b }
    Test-SelCase 'platform: a proof equal but for Platform x64 does not carry for Win32' @{ Proofs = $pX64 } $selAll 'platform'
    Test-SelCase 'platform: a proof with no Platform does not carry' @{ Proofs = $pNone } $selAll 'platform'
    Test-SelCase 'platform: a Win32 proof carries for Win32' @{ Platform = 'Win32' } @() $null
    Test-SelCase 'platform: a decision with no platform carries nothing, not even a proof with no Platform' @{ Platform = ''; Proofs = $pNone } $selAll 'platform'
    $why = $null
    try {
        $dW = Invoke-SelCase @{}
        $dX = Invoke-SelCase @{ Platform = 'x64' }
        $same = @($selAll | Where-Object { $dW[$_].DepDigest -ceq $dX[$_].DepDigest })
        if ($same.Count) { $why = "equal sources gave one DepDigest on Win32 and x64 for $($same -join ',')" }
    }
    catch { $why = "threw: $_" }
    Write-SelResult 'platform: equal sources on two platforms give different DepDigests' $why
    $why = $null
    try {
        $pOnlyX64 = @{}
        foreach ($x in (Invoke-SelCase @{ Platform = 'x64' }).Values) {
            $pOnlyX64["$($x.Id)`tx64"] = @{ RowHash = $x.RowHash; DepDigest = $x.DepDigest; TriggerDigest = $x.TriggerDigest; Toolset = '14.44.35207'; Platform = 'x64'; Commit = 'c0ffee1'; Date = '2026-11-04' }
        }
        $pBoth = @{}
        foreach ($k in @($selProofs.Keys) + @($pOnlyX64.Keys)) { $pBoth[$k] = $(if ($selProofs.ContainsKey($k)) { $selProofs[$k] } else { $pOnlyX64[$k] }) }
        $wOnX = Invoke-SelCase @{ Proofs = $pOnlyX64 }
        $xOnX = Invoke-SelCase @{ Platform = 'x64'; Proofs = $pOnlyX64 }
        $wBoth = Invoke-SelCase @{ Proofs = $pBoth }
        $xBoth = Invoke-SelCase @{ Platform = 'x64'; Proofs = $pBoth }
        $bad = @($selAll | Where-Object { $wOnX[$_].Action -ne 'SELECT' -or $wOnX[$_].Reason -ne 'no record' })
        if ($bad.Count) { $why = "the x64 proof of $($bad[0]) gave the Win32 row $($wOnX[$bad[0]].Action) '$($wOnX[$bad[0]].Reason)'" }
        elseif (@($selAll | Where-Object { $xOnX[$_].Action -ne 'CARRY' }).Count) { $why = 'an x64 proof did not carry its own x64 row, so the case proves nothing' }
        elseif (@($selAll | Where-Object { $wBoth[$_].Action -ne 'CARRY' -or $xBoth[$_].Action -ne 'CARRY' }).Count) { $why = 'with both proofs present a row did not carry on its own platform' }
    }
    catch { $why = "threw: $_" }
    Write-SelResult 'platform: an x64 proof never satisfies a Win32 row, and Win32 and x64 proofs for one Id each carry their own platform' $why
    $why = $null
    try {
        $why = & {
            . (Join-Path $PSScriptRoot 'gate\keys.ps1')
            $GuardRevertRows = $null
            . (Join-Path $PSScriptRoot 'guardrevert\rows.ps1')
            $rpProofs = Read-KeyedRecords (Join-Path $PSScriptRoot 'guardrevert\proven.tsv') @('Id', 'Platform')
            $rpSteps = Read-StepRecords (Join-Path $PSScriptRoot 'guardrevert\steps.tsv')
            $rpX = @($rpProofs.Values | Where-Object { $_.Platform -ceq 'x64' })
            $rpXSteps = @($rpSteps.Values | Where-Object { $_.Platform -ceq 'x64' })
            if ($rpX.Count -eq 0) { return 'proven.tsv has no x64 record, so the case proves nothing' }
            if ($rpXSteps.Count -eq 0) { return 'steps.tsv has no x64 record, so the case proves nothing' }
            foreach ($p in $rpX) {
                $own = $rpProofs["$($p.Id)`tx64"]
                if (-not $own -or $own.Platform -cne 'x64') { return "the x64 proof of $($p.Id) is not found under (Id, x64)" }
                $w = $rpProofs["$($p.Id)`tWin32"]
                if ($w -and $w.Platform -cne 'Win32') { return "a Win32 lookup of $($p.Id) returned its $($w.Platform) proof" }
            }
            foreach ($s in $rpXSteps) {
                $own = $rpSteps["$($s.Step)`tx64`t$($s.Config)`t$($s.Tier)"]
                if (-not $own -or $own.Platform -cne 'x64') { return "the x64 record of $($s.Step) $($s.Config) $($s.Tier) is not found under its x64 key" }
                $w = $rpSteps["$($s.Step)`tWin32`t$($s.Config)`t$($s.Tier)"]
                if ($w -and $w.Platform -cne 'Win32') { return "a Win32 lookup of step $($s.Step) $($s.Config) $($s.Tier) returned its $($w.Platform) record" }
            }
            $xIds = @($rpX | ForEach-Object { $_.Id })
            $xRows = @(Select-GuardPlatformRows @($GuardRevertRows) 'x64' | Where-Object { $xIds -ccontains $_.Id -and -not $rpProofs.ContainsKey("$($_.Id)`tWin32") })
            if ($xRows.Count -eq 0) { return 'no x64 row in rows.ps1 has an x64 proof only' }
            $asWin32 = @($xRows | ForEach-Object { $c = $_.Clone(); $c.Remove('Platform'); $c })
            $in = @{}
            foreach ($k in $selBase.Keys) { $in[$k] = $selBase[$k] }
            $in.Proofs = $rpProofs
            $in.Rows = $asWin32
            $in.Platform = 'Win32'
            $onW = @(Select-GuardRows $in | Where-Object { $null -ne $_ })
            $in.Rows = $xRows
            $in.Platform = 'x64'
            $onX = @(Select-GuardRows $in | Where-Object { $null -ne $_ })
            $badW = @($onW | Where-Object { $_.Action -ne 'SELECT' -or $_.Reason -ne 'no record' })
            if ($onW.Count -ne $xRows.Count -or $onX.Count -ne $xRows.Count) { return "$($onW.Count) Win32 and $($onX.Count) x64 decisions for $($xRows.Count) rows" }
            if ($badW.Count) { return "the real x64 proof of $($badW[0].Id) gave its Win32 lookup $($badW[0].Action) '$($badW[0].Reason)'" }
            $noX = @($onX | Where-Object { $_.Reason -eq 'no record' })
            if ($noX.Count) { return "the real x64 proof of $($noX[0].Id) was not found for its own x64 row, so the case proves nothing" }
            $script:rpCounts = "$($xRows.Count) x64 rows, $($rpX.Count) x64 proofs, $($rpXSteps.Count) x64 step records"
            return $null
        }
    }
    catch { $why = "threw: $_" }
    Write-SelResult "platform: real proven.tsv and steps.tsv x64 records are never returned for a Win32 lookup ($script:rpCounts)" $why
    $why = $null
    try {
        $prRows = @(@{ Id = 'R-W' }, @{ Id = 'R-X'; Platform = 'x64' }, @{ Id = 'R-W2'; Platform = 'Win32' })
        $gw = @(Select-GuardPlatformRows $prRows 'Win32' | ForEach-Object { $_.Id }) -join ','
        $gx = @(Select-GuardPlatformRows $prRows 'x64' | ForEach-Object { $_.Id }) -join ','
        if ($gw -cne 'R-W,R-W2') { $why = "Win32 kept '$gw'" }
        elseif ($gx -cne 'R-X') { $why = "x64 kept '$gx'" }
    }
    catch { $why = "threw: $_" }
    Write-SelResult 'platform: a row with no Platform is Win32, and each run keeps only its own platform''s rows' $why
    Test-SelCase 'no tlog selects every row that needs one' @{ TestsReads = $null; AsanReads = $null; GameReads = $null } @('S-LIB', 'S-PIN', 'S-HDR', 'S-ASAN', 'S-CANARY', 'S-BASE', 'S-HARD', 'S-HARD2', 'S-SMOKE', 'S-SMOKE2', 'S-PERTURB') 'no tlog*'
    Test-SelCase 'unmapped path selects the rows that read it' @{ Hashes = (Copy-SelHashes 'src\G\Game.h' $null) } @('S-HARD', 'S-SMOKE', 'S-SMOKE2', 'S-PERTURB') 'unmapped *'
    Test-SelCase 'unmapped literal selects its test rows' @{ TestLiterals = @{ 'tests\unit\t_pin.cpp' = @('tests\corpus\c\', 'tests\missing\x.txt'); 'tests\unit\t_base.cpp' = @('tests\smoke\digest-expected.txt') } } @('S-PIN') 'unmapped literal*'
    Test-SelCase 'unknown test name selects its rows' @{ TestSources = @{ 'lib works' = 'tests\unit\t_lib.cpp'; 'pin text' = 'tests\unit\t_pin.cpp'; 'base table' = 'tests\unit\t_base.cpp' } } @('S-HDR', 'S-CANARY') 'unknown test*'
    Test-SelCase 'text-pin row left alone by an unrelated change' @{ Hashes = (Copy-SelHashes 'src\G\Other.cpp' 'new') } @('S-HARD2', 'S-SMOKE', 'S-SMOKE2', 'S-PERTURB') 'deps changed'
    Test-SelCase 'smoke row depends on every game TU it links with' @{ Hashes = (Copy-SelHashes 'src\X\pcWindow.cpp' 'new') } @('S-PIN', 'S-SMOKE', 'S-SMOKE2', 'S-PERTURB') 'deps changed'
    Test-SelCase 'perturb row depends on the mission scripts like a smoke row' @{ Hashes = (Copy-SelHashes 'data\scripts\a.lua' 'new') } @('S-SMOKE', 'S-SMOKE2', 'S-PERTURB') 'deps changed'
    $p = Join-Path $selRoot 'src\G\Lib.h'
    (Get-Item -LiteralPath $p).LastWriteTime = (Get-Date).AddHours(1)
    $stale = Read-ClReadTlog $testsTlog $selRoot
    Test-SelCase 'stale tlog selects every row that needs the stale TU' @{ TestsReads = $stale; AsanReads = $stale } $selT1 'stale tlog*'
}
catch { $failed++; Write-Host "FAILED: selector block threw: $_" }
finally {
    Remove-Item -LiteralPath $selTmp -Recurse -Force -ErrorAction SilentlyContinue
}
$n++
$vtScript = Join-Path $env:TEMP ("openswbf2-vstools-" + [guid]::NewGuid().ToString('N') + '.ps1')
try {
    Set-Content -Path $vtScript -Encoding ASCII -Value @(
        ". '$(Join-Path $PSScriptRoot 'vstools.ps1')'",
        "`$vsTools = Find-VsTools '$Platform'",
        "`$m = Find-MSBuild",
        "if (`$m -and (Test-Path -LiteralPath `$m)) { 'MSBUILD-FOUND' }"
    )
    $vtOut = (& powershell -NoProfile -ExecutionPolicy Bypass -File $vtScript 2>&1 | Out-String)
    if ($vtOut -notmatch 'MSBUILD-FOUND') { $failed++; Write-Host "FAILED ${n}: a caller variable named vsTools breaks the vstools cache: $($vtOut.Trim())" }
    else { Write-Host "OK $n a caller variable named vsTools leaves the vstools cache intact" }
}
finally { Remove-Item -LiteralPath $vtScript -Force -ErrorAction SilentlyContinue }
$n++
$cbTmp =Join-Path $env:TEMP ("openswbf2-cmptables-" + [guid]::NewGuid().ToString('N'))
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
    $hdLines = [IO.File]::ReadAllLines((Join-Path $cbBase 'hardening.txt'))
    $hdCases = @(
        @{ Name = 'same'; Edit = { param($l) }; Code = 0; Want = '\[PASS\] tables:hardening' },
        @{ Name = 'fail'; Edit = { param($l) $l[0] = $l[0].Replace('[PASS]', '[FAIL]') + ' - x' }; Code = 1; Want = '\[FAIL\] tables:hardening' },
        @{ Name = 'skip'; Edit = { param($l) $l[$l.Count - 1] = $l[$l.Count - 1].Replace('[PASS]', '[SKIP]') }; Code = 1; Want = '\[FAIL\] tables:hardening' },
        @{ Name = 'missing'; Edit = { param($l) $l.RemoveAt(0) }; Code = 1; Want = 'missing from tables' },
        @{ Name = 'extra'; Edit = { param($l) $l.Add('[PASS] hardening:zzz-new') }; Code = 1; Want = 'zzz-new not in baseline' }
    )
    $hdWhy = $null
    foreach ($c in $hdCases) {
        $hd = Join-Path $cbTmp "hd-$($c.Name)"
        New-Item -ItemType Directory -Force -Path $hd | Out-Null
        $hl = [System.Collections.Generic.List[string]]::new([string[]]$hdLines)
        & $c.Edit $hl
        [IO.File]::WriteAllLines((Join-Path $hd 'hardening.txt'), $hl)
        $out = & { $ErrorActionPreference = 'Continue'; & powershell -NoProfile -ExecutionPolicy Bypass -File $cbScript -Tables $hd -Hardening 2>&1 | Out-String }
        if ($LASTEXITCODE -ne $c.Code) { $hdWhy = "$($c.Name) gave exit $LASTEXITCODE"; break }
        if ($out -notmatch $c.Want) { $hdWhy = "$($c.Name) output lacks '$($c.Want)'"; break }
        if ($out -match 'tables:(?:lua-bins|mem-peaks)') { $hdWhy = "$($c.Name) ran another table"; break }
    }
    if ($hdWhy) { $failed++; Write-Host "FAILED ${n}: compare-tables hardening: $hdWhy" }
    else { Write-Host "OK $n compare-tables -Hardening passes an equal hardening table and fails a FAIL or SKIP row, a missing row and an extra row" }
    $n++
    $otLines = [IO.File]::ReadAllLines((Join-Path $cbBase 'offthread.txt'))
    $otSound = [Array]::IndexOf($otLines, '## sound-on')
    $otRow = -1
    for ($i = 0; $i -lt $otLines.Count; $i++) { if ($otLines[$i] -cmatch "^[^`t#]+`t0`t0`t0$") { $otRow = $i; break } }
    $otWhy = $null
    if ($otSound -lt 0 -or $otRow -lt 0 -or $otRow -gt $otSound) { $otWhy = 'no main row or no ## sound-on in offthread.txt' }
    else {
        $otSrcBase = "$(($otLines[$otRow] -split "`t")[0])`t111`tRuntime`t16`t1`tFoo::Bar"
        $otCases = @(
            @{ Name = 'same'; Edit = { param($l) }; Code = 0; Want = '\[PASS\] tables:offthread' },
            @{ Name = 'soundon'; Edit = { param($l) for ($i = $otSound + 1; $i -lt $l.Count; $i++) { $l[$i] = $l[$i] -replace 'sound=opened', 'sound=not opened' -replace "`t0`t0`t0$", "`t9`t9`t9" }; $l.Add("zzz9c_xl`t1`t1`t1") }; Code = 0; Want = '\[PASS\] tables:offthread' },
            @{ Name = 'changed'; Edit = { param($l) $l[$otRow] = $l[$otRow] -replace "`t0`t0`t0$", "`t1`t1`t0" }; Code = 1; Want = '\[FAIL\] tables:offthread' },
            @{ Name = 'missing'; Edit = { param($l) $l.RemoveAt($otRow) }; Code = 1; Want = 'missing from tables' },
            @{ Name = 'extra'; Edit = { param($l) $l.Insert($otSound, "zzz9c_con`t0`t0`t0") }; Code = 1; Want = 'zzz9c_con not in baseline' },
            @{ Name = 'tid'; Base = { param($l) $l.Insert($otRow + 1, $otSrcBase) }; Edit = { param($l) $l.Insert($otRow + 1, ($otSrcBase -replace "`t111`t", "`t222`t")) }; Code = 0; Want = '\[PASS\] tables:offthread' },
            @{ Name = 'srcsize'; Base = { param($l) $l.Insert($otRow + 1, $otSrcBase) }; Edit = { param($l) $l.Insert($otRow + 1, ($otSrcBase -replace "`t16`t", "`t32`t")) }; Code = 1; Want = '\[FAIL\] tables:offthread' }
        )
        foreach ($c in $otCases) {
            $tb = Join-Path $cbTmp "ot-$($c.Name)"
            $bb = Join-Path $cbTmp "otb-$($c.Name)"
            New-Item -ItemType Directory -Force -Path $tb, $bb | Out-Null
            $bl = [System.Collections.Generic.List[string]]::new([string[]]$otLines)
            if ($c.Base) { & $c.Base $bl }
            [IO.File]::WriteAllLines((Join-Path $bb 'offthread.txt'), $bl)
            $tl = [System.Collections.Generic.List[string]]::new([string[]]$otLines)
            & $c.Edit $tl
            [IO.File]::WriteAllLines((Join-Path $tb 'offthread.txt'), $tl)
            $out = & { $ErrorActionPreference = 'Continue'; & powershell -NoProfile -ExecutionPolicy Bypass -File $cbScript -Tables $tb -Baseline $bb -OffThread 2>&1 | Out-String }
            if ($LASTEXITCODE -ne $c.Code) { $otWhy = "$($c.Name) gave exit $LASTEXITCODE"; break }
            if ($out -notmatch $c.Want) { $otWhy = "$($c.Name) output lacks '$($c.Want)'"; break }
            if ($out -match 'tables:(?:lua-bins|mem-peaks)') { $otWhy = "$($c.Name) ran another table"; break }
        }
    }
    if ($otWhy) { $failed++; Write-Host "FAILED ${n}: compare-tables offthread: $otWhy" }
    else { Write-Host "OK $n compare-tables -OffThread compares the main section only, ignores the source tid and fails a changed, missing or extra row and a changed source row" }
    $n++
}
catch { $failed++; Write-Host "FAILED: compare-tables block threw: $_" }
finally {
    Remove-Item -LiteralPath $cbTmp -Recurse -Force -ErrorAction SilentlyContinue
}
. (Join-Path $PSScriptRoot 'gate\keys.ps1')
$kTmp = Join-Path $env:TEMP ('openswbf2-keys-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $kTmp | Out-Null
try {
    $roMake = {
        param([string]$Dir, [scriptblock]$Edit)
        $cases = [ordered]@{
            'geo1c_con' = @{ Result = [ordered]@{ class = 'Pass'; exit = '0x00000000'; elapsed_ms = '9000'; dumps = '0'; asan = '0'; workdir = 'w1'; stdout_complete = '1'; stderr_complete = '1'; stall = '0'; silent_max_s = '0.4'; turns = '9300'; steps = '2600000' }; Log = @('[t=1.000] [f=10] LogSim Inform GameLoop.cpp:1: OPENSWBF2 SIM DIGEST turns=1500 time=50.000 rand=1/2 snd=1.000 state=0x11111111', '[t=2.000] [f=20] LogSim Inform GameLoop.cpp:2: OPENSWBF2 DIGEST TRACE turn=1600 state=0x22222222 a', '[t=3.000] [f=30] LogSim Inform GameLoop.cpp:3: OPENSWBF2 SOAK turns=9300', '[t=3.000] [f=30] LogSim Inform GameLoop.cpp:4: OPENSWBF2 RUN COMPLETE geo1c_con') }
            'crash-stall' = @{ Result = [ordered]@{ class = 'Fatal'; exit = '0x00000005'; elapsed_ms = '64000'; dumps = '1'; asan = '0'; workdir = 'w2'; stdout_complete = '1'; stderr_complete = '1'; stall = '1'; silent_max_s = '60.1'; turns = '0'; steps = '2700000' }; Log = @('FATAL: hang (supervisor saw no progress, last frame 1) build 6AFA77281EC5000 dump C:\w\BattlefrontII-20261116-063453-73484.dmp') }
        }
        $lines = @('[PASS] hardening:new-size-t', '[PASS] smoke:geo1c_con class=Pass exit=0x00000000 (9000 ms)', '[PASS] smoke:crash-stall class=Fatal exit=0x00000005 (64000 ms)', '[PASS] handle registry lookups probe a bounded number of slots (12 ms)', '[PASS] tables:compare', '', 'Results', '  PASS     5')
        if ($Edit) { & $Edit $cases ([ref]$lines) }
        New-Item -ItemType Directory -Force -Path $Dir | Out-Null
        [IO.File]::WriteAllText((Join-Path $Dir 'results.txt'), ($lines -join "`r`n") + "`r`n")
        foreach ($c in $cases.Keys) {
            $cd = Join-Path $Dir "smoke\$c"
            New-Item -ItemType Directory -Force -Path $cd | Out-Null
            [IO.File]::WriteAllText((Join-Path $cd 'result.txt'), (@($cases[$c].Result.Keys | ForEach-Object { "$_=$($cases[$c].Result[$_])" }) -join "`r`n") + "`r`n")
            [IO.File]::WriteAllText((Join-Path $cd 'run.log'), ($cases[$c].Log -join "`r`n") + "`r`n")
        }
    }
    $roBase = Join-Path $kTmp 'ro-base'
    & $roMake $roBase $null
    $roCases = @(
        @('two runs that differ only in times, silence and steps are equal', $true, '', { param($c, $l) $c['geo1c_con'].Result.elapsed_ms = '27000'; $c['geo1c_con'].Result.silent_max_s = '2.7'; $c['geo1c_con'].Result.steps = '2400000'; $c['geo1c_con'].Result.workdir = 'w9'; $c['geo1c_con'].Log = @($c['geo1c_con'].Log | ForEach-Object { $_ -replace '^\[t=[\d.]+\] \[f=\d+\]', '[t=9.999] [f=99]' }); $c['crash-stall'].Log = @('FATAL: hang (supervisor saw no progress, last frame 0) build 7BFB00000000000 dump C:\v\BattlefrontII-20261117-010101-1.dmp'); $l.Value = @($l.Value | ForEach-Object { $_ -replace '\(\d+ ms\)', '(77777 ms)' }) }),
        @('a changed SIM DIGEST line is a difference naming the case and the line', $false, '^smoke\\geo1c_con sim_digest: .*state=0x11111111.*state=0x33333333', { param($c, $l) $c['geo1c_con'].Log[0] = $c['geo1c_con'].Log[0].Replace('0x11111111', '0x33333333') }),
        @('a FATAL line with another reason is a difference', $false, "^smoke\\crash-stall fatal: A 'FATAL: hang \(supervisor saw no progress, last frame #\)' B 'FATAL: hang \(no frame update for # s, last frame #\)'$", { param($c, $l) $c['crash-stall'].Log[0] = $c['crash-stall'].Log[0].Replace('supervisor saw no progress', 'no frame update for 60 s') }),
        @('a changed class is a difference', $false, "^smoke\\crash-stall class: A 'Fatal' B 'Stall'", { param($c, $l) $c['crash-stall'].Result.class = 'Stall' }),
        @('a case missing from one side is a difference', $false, '^smoke\\crash-stall: case only in A$', { param($c, $l) $c.Remove('crash-stall'); $l.Value = @($l.Value | Where-Object { $_ -notmatch 'crash-stall' }) }),
        @('a tables line PASS on one side and FAIL on the other is a difference', $false, "^results\.txt: A x0 B x1 '\[FAIL\] tables:compare exit 1'$", { param($c, $l) $l.Value = @($l.Value | ForEach-Object { $_ -replace '^\[PASS\] tables:compare$', '[FAIL] tables:compare exit 1' }) }),
        @('changed turns, SOAK line or digest trace end are differences', $false, '^smoke\\geo1c_con digest_trace_end: ', { param($c, $l) $c['geo1c_con'].Result.turns = '9301'; $c['geo1c_con'].Log[1] = $c['geo1c_con'].Log[1].Replace('turn=1600', 'turn=1601'); $c['geo1c_con'].Log[2] = $c['geo1c_con'].Log[2].Replace('9300', '9301') })
    )
    $roI = 0
    foreach ($rc in $roCases) {
        $roI++
        $roDir = Join-Path $kTmp "ro-$roI"
        & $roMake $roDir $rc[3]
        $ro = Compare-RunOutcome $roBase $roDir
        $why = $null
        if ($ro.Equal -ne $rc[1]) { $why = "Equal=$($ro.Equal): $($ro.Differences -join ' | ')" }
        elseif ($rc[2] -and -not @($ro.Differences | Where-Object { $_ -match $rc[2] }).Count) { $why = "no difference matches /$($rc[2])/: $($ro.Differences -join ' | ')" }
        elseif ($rc[0] -like 'changed turns*' -and @($ro.Differences | Where-Object { $_ -match '^smoke\\geo1c_con (turns|soak|digest_trace_sha): ' }).Count -ne 3) { $why = "turns, soak and trace hash not all named: $($ro.Differences -join ' | ')" }
        if ($why) { $failed++; Write-Host "FAILED ${n}: run outcome compare: $($rc[0]): $why" }
        else { Write-Host "OK $n run outcome compare: $($rc[0])" }
        $n++
    }
    $kTree = Get-TreeFileHashes $repo
    $kBase = @{ step = 'selftest-step'; platform = 'Win32'; config = 'Release'; tier = 'plan'; toolset = '14.44.35207'; cl = '19.44.35230'; ucrt = '10.0.26100.1'; steam = 'yes'; overlay = 'off'; gamedata = 'g0'; exe = @{ 'BattlefrontII.exe' = 'e0' }; dlls = @{ 'steam_api.dll' = 'w0'; 'Galaxy.dll' = 'w1' }; vendor = @{ 'steam_api.dll' = 'v0'; 'd3dx9_43.dll' = 'x0' }; Kind = 'run'; JudgeKinds = @('smoke'); Files = $kTree }
    $kKey = {
        param([scriptblock]$Edit, [string[]]$Kinds, [string]$Kind)
        $i = $kBase.Clone()
        foreach ($c in 'exe', 'dlls', 'vendor', 'Files') { $i[$c] = $kBase[$c].Clone() }
        if ($Kinds) { $i.JudgeKinds = $Kinds }
        if ($Kind) { $i.Kind = $Kind }
        if ($Edit) { & $Edit $i }
        Get-StepKey $i
    }
    $kPlain = & $kKey
    if (-not $kPlain.Valid -or $kPlain.RunKey -notmatch '^[0-9a-f]{64}$' -or $kPlain.JudgeKey -notmatch '^[0-9a-f]{64}$' -or $kPlain.RunKey -ceq $kPlain.JudgeKey) { $failed++; Write-Host "FAILED ${n}: complete step key input: valid=$($kPlain.Valid) run=$($kPlain.RunKey) judge=$($kPlain.JudgeKey)" }
    else { Write-Host "OK $n complete step key input gives a valid RunKey and JudgeKey" }
    $n++
    $kInputs = [ordered]@{
        'exe normalised hash' = { param($i) $i.exe['BattlefrontII.exe'] = 'e1' }
        'worker DLL hash' = { param($i) $i.dlls['Galaxy.dll'] = 'w2' }
        'tests\run.ps1 hash' = { param($i) $i.Files['tests\run.ps1'] = 'changed' }
        'case data tests\smoke\missions.txt hash' = { param($i) $i.Files['tests\smoke\missions.txt'] = 'changed' }
        'GameData manifest hash' = { param($i) $i.gamedata = 'g1' }
        'cl version' = { param($i) $i.cl = '19.44.35231' }
        'ucrtbase version' = { param($i) $i.ucrt = '10.0.26100.2' }
        'toolset version' = { param($i) $i.toolset = '14.44.35208' }
        'vendor steam_api.dll hash' = { param($i) $i.vendor['steam_api.dll'] = 'v1' }
        'd3dx9_43.dll hash' = { param($i) $i.vendor['d3dx9_43.dll'] = 'x1' }
        'Steam running' = { param($i) $i.steam = 'no' }
        'overlay state' = { param($i) $i.overlay = 'on' }
        'configuration' = { param($i) $i.config = 'Debug' }
        'tier' = { param($i) $i.tier = 'milestone' }
        'platform' = { param($i) $i.platform = 'x64' }
        'step name' = { param($i) $i.step = 'other-step' }
    }
    foreach ($name in $kInputs.Keys) {
        $k = & $kKey $kInputs[$name]
        if ($k.RunKey -ceq $kPlain.RunKey -or -not $k.Valid) { $failed++; Write-Host "FAILED ${n}: step key input '$name' left RunKey equal or made the key invalid" }
        else { Write-Host "OK $n step key input '$name' changes RunKey" }
        $n++
    }
    $kAllKinds = @(@('smoke'), @('smoke-all'), @('t1'), @('hardening'), @('compare'), @('guard'), @('t1', 'hardening', 'smoke'))
    foreach ($f in @('tests\smoke\missions-all.txt', 'tests\smoke\missions.txt', 'tests\smoke\display-bad.ini', 'tests\smoke\display-swcursor.ini', 'tests\smoke\display-waitquery.ini')) {
        $why = $null
        if (-not $kTree.ContainsKey($f)) { $why = "$f is not in the tree table" }
        foreach ($kinds in $kAllKinds) {
            $a = & $kKey $null $kinds 'run'
            $b = & $kKey ([scriptblock]::Create("param(`$i) `$i.Files['$f'] = 'changed'")) $kinds 'run'
            if ($a.RunKey -ceq $b.RunKey) { $why = "RunKey equal for kinds $($kinds -join '+')" }
        }
        if ($why) { $failed++; Write-Host "FAILED ${n}: run input ${f}: $why" }
        else { Write-Host "OK $n run input $f changes RunKey for every step kind" }
        $n++
    }
    $kRunCases = @(
        @('tests\smoke\digest-expected.txt', @('smoke-all'), 'an All step'),
        @('tests\smoke\digest-expected.txt', @('t1'), 'a T1 step'),
        @('tests\smoke\digest-expected.txt', @('t1', 'hardening', 'smoke'), 'a T1+hardening+smoke step'),
        @('tests\baseline\v143\pool-sizes.txt', @('t1'), 'a T1 step'),
        @('tests\baseline\v143\mem-peaks.txt', @('t1'), 'a T1 step'),
        @('tests\baseline\v143\lua-bins.txt', @('t1'), 'a T1 step'),
        @('tests\baseline\v143\cloth-headroom.txt', @('t1'), 'a T1 step'),
        @('tests\baseline\v143\CAUSES.md', @('t1'), 'a T1 step'),
        @('tests\baseline\v143\CAUSES.md', @('smoke'), 'a smoke step'),
        @('tests\baseline\v143\MANIFEST.md', @('compare'), 'a compare step'),
        @('tests\baseline\vs2013\MANIFEST.md', @('compare'), 'a compare step'),
        @('README.md', @('smoke'), 'a smoke step'),
        @('tests\corpus\README.md', @('smoke'), 'a smoke step, because the guard selector hashes it under the corpus test literal'),
        @('tests\third_party\doctest\README.md', @('smoke'), 'a smoke step, because the guard selector hashes it under the tests\third_party\ trigger')
    )
    foreach ($c in $kRunCases) {
        $a = & $kKey $null $c[1] 'run'
        $b = & $kKey ([scriptblock]::Create("param(`$i) `$i.Files['$($c[0])'] = 'changed'")) $c[1] 'run'
        if (-not $kTree.ContainsKey($c[0]) -or $a.RunKey -ceq $b.RunKey) { $failed++; Write-Host "FAILED ${n}: $($c[0]) did not change RunKey for $($c[2])" }
        else { Write-Host "OK $n $($c[0]) changes RunKey for $($c[2])" }
        $n++
    }
    foreach ($kind in @($script:JudgeOnlyFiles.Keys | Sort-Object)) {
        foreach ($e in $script:JudgeOnlyFiles[$kind]) {
            $a = & $kKey $null @($kind) 'run'
            $b = & $kKey ([scriptblock]::Create("param(`$i) `$i.Files['$($e.Path)'] = 'changed'")) @($kind) 'run'
            if (-not $kTree.ContainsKey($e.Path) -or $a.RunKey -cne $b.RunKey -or $a.JudgeKey -ceq $b.JudgeKey) { $failed++; Write-Host "FAILED ${n}: judge-only $kind $($e.Path) ($($e.Reader)): in tree=$($kTree.ContainsKey($e.Path)) run equal=$($a.RunKey -ceq $b.RunKey) judge equal=$($a.JudgeKey -ceq $b.JudgeKey)" }
            else { Write-Host "OK $n judge-only $kind $($e.Path) ($($e.Reader)) changes only JudgeKey" }
            $n++
        }
    }
    $kMixCases = @(
        @('tests\smoke\digest-expected.txt', @('smoke', 'hardening'), $true, 'a smoke+hardening step'),
        @('tests\smoke\expected-errors.txt', @('smoke', 'hardening'), $true, 'a smoke+hardening step'),
        @('tests\smoke\expected-errors.txt', @('smoke-all', 'hardening'), $true, 'an All+hardening step'),
        @('tests\baseline\v143\hardening.txt', @('smoke', 'hardening'), $true, 'a smoke+hardening step'),
        @('tests\smoke\digest-expected.txt', @('hardening'), $false, 'a hardening-only step, whose verdict does not read it'),
        @('tests\smoke\digest-expected.txt', @('smoke-all', 'hardening'), $false, 'an All+hardening step'),
        @('tests\baseline\v143\pool-sizes.txt', @('smoke', 'hardening'), $false, 'a smoke+hardening step, because baseline-readers lists tests\baseline\v143')
    )
    foreach ($c in $kMixCases) {
        $a = & $kKey $null $c[1] 'run'
        $b = & $kKey ([scriptblock]::Create("param(`$i) `$i.Files['$($c[0])'] = 'changed'")) $c[1] 'run'
        $judgeOnly = ($a.RunKey -ceq $b.RunKey -and $a.JudgeKey -cne $b.JudgeKey)
        $runInput = ($a.RunKey -cne $b.RunKey)
        $what = $(if ($c[2]) { 'changes only JudgeKey' } else { 'changes RunKey' })
        if (-not $kTree.ContainsKey($c[0]) -or ($c[2] -and -not $judgeOnly) -or (-not $c[2] -and -not $runInput)) { $failed++; Write-Host "FAILED ${n}: $($c[0]) for $($c[3]) should $($what -replace '^changes', 'change')" }
        else { Write-Host "OK $n $($c[0]) $what for $($c[3])" }
        $n++
    }
    $kUnreadScan = {
        param([string]$Text)
        $hits = @()
        foreach ($k in $script:KindRunUnread.Keys) { foreach ($p in $script:KindRunUnread[$k]) { if ($Text.IndexOf([IO.Path]::GetFileName($p), [StringComparison]::OrdinalIgnoreCase) -ge 0) { $hits += "$k reads $p" } } }
        return $hits
    }
    $kRunText = [IO.File]::ReadAllText((Join-Path $repo 'tests\run.ps1'))
    $kHardStart = $kRunText.IndexOf('$hardDir = ')
    $kHardEnd = $kRunText.IndexOf('if ($smokeActive) {', $kHardStart)
    $kHardText = $kRunText.Substring($kHardStart, $kHardEnd - $kHardStart)
    foreach ($fn in 'Get-SourceChecks', 'Get-BaselineReadersCheck', 'Get-HardeningChecks', 'Get-FlagChecks', 'Get-MiCrtHookCheck', 'Get-ImportChecks', 'Get-PeAslr', 'Get-SmokePrecheck', 'Get-GameProjects', 'Get-ClTlogCounts', 'Get-X64ObjectsCheck') { $kHardText += "`n" + (Get-Command $fn).Definition }
    $kHits = @(& $kUnreadScan $kHardText)
    $kBroken = @(& $kUnreadScan ($kHardText + "`n`$x = Get-Content (Join-Path `$testsDir 'smoke\digest-expected.txt')"))
    if ($kHits.Count -or $kBroken.Count -eq 0) { $failed++; Write-Host "FAILED ${n}: hardening run code names a file KindRunUnread says it never reads: $($kHits -join ', '); scan of an injected read found $($kBroken.Count)" }
    else { Write-Host "OK $n the hardening run code names no file KindRunUnread lists, and the scan finds an injected read" }
    $n++
    $ctText = [IO.File]::ReadAllText((Join-Path $repo 'tests\baseline\compare-tables.ps1'))
    $ctFiles = @([regex]::Matches($ctText, "@\{ File = '([^']+)'") | ForEach-Object { $_.Groups[1].Value } | Where-Object { $_ -notlike '*.md' } | ForEach-Object { "tests\baseline\v143\$_" } | Sort-Object)
    if (($ctFiles -join '|') -cne (@($script:KeyComparedTables | Sort-Object) -join '|')) { $failed++; Write-Host "FAILED ${n}: keys.ps1 compared tables differ from compare-tables.ps1 readers: $($ctFiles -join ', ')" }
    else { Write-Host "OK $n keys.ps1 compared tables equal the compare-tables.ps1 data readers" }
    $n++
    $a = & $kKey
    $b = & $kKey { param($i) $i.Files['tests\smoke.ps1'] = 'changed' }
    if ($a.RunKey -ceq $b.RunKey -or $a.JudgeKey -ceq $b.JudgeKey) { $failed++; Write-Host "FAILED ${n}: verdict script tests\smoke.ps1 must change RunKey and JudgeKey" }
    else { Write-Host "OK $n verdict script tests\smoke.ps1 changes RunKey and JudgeKey" }
    $n++
    foreach ($f in 'tests\common.ps1', 'tests\smoke\cases.ps1') {
        $b = & $kKey ([scriptblock]::Create("param(`$i) `$i.Files['$f'] = 'changed'"))
        if (-not $kTree.ContainsKey($f) -or $a.RunKey -ceq $b.RunKey -or $a.JudgeKey -ceq $b.JudgeKey -or (($a.Header -join '|') -cne ($b.Header -join '|'))) { $failed++; Write-Host "FAILED ${n}: verdict script $f must change RunKey and JudgeKey and no header field" }
        else { Write-Host "OK $n verdict script $f changes RunKey and JudgeKey and no header field" }
        $n++
    }
    $shScan = {
        param([hashtable]$Texts)
        $defs = @{}
        foreach ($name in $Texts.Keys) {
            $errs = $null
            $ast = [Management.Automation.Language.Parser]::ParseInput($Texts[$name], [ref]$null, [ref]$errs)
            if (@($errs).Count) { return @("$name does not parse: $(@($errs)[0].Message)") }
            $found = @($ast.FindAll({ param($x) $x -is [Management.Automation.Language.FunctionDefinitionAst] }, $false) | ForEach-Object { "function $($_.Name)" })
            $found += @($ast.FindAll({ param($x) $x -is [Management.Automation.Language.AssignmentStatementAst] -and $x.Left.Extent.Text -match '^\$script:\w+$' }, $false) | ForEach-Object { 'variable ' + $_.Left.Extent.Text.Substring(8) })
            $defs[$name] = @($found | ForEach-Object { $_.ToLowerInvariant() } | Sort-Object -Unique)
        }
        $hits = @()
        $names = @($defs.Keys | Sort-Object)
        for ($i = 0; $i -lt $names.Count; $i++) {
            for ($j = $i + 1; $j -lt $names.Count; $j++) {
                foreach ($d in $defs[$names[$i]]) { if ($defs[$names[$j]] -contains $d) { $hits += "$d in $($names[$i]) and $($names[$j])" } }
            }
        }
        return $hits
    }
    $shTexts = @{}
    foreach ($f in 'tests\common.ps1', 'tests\smoke.ps1', 'tests\smoke\cases.ps1') { $shTexts[$f] = [IO.File]::ReadAllText((Join-Path $repo $f)) }
    $shHits = @(& $shScan $shTexts)
    $shBad = $shTexts.Clone()
    $shBad['tests\smoke.ps1'] += "`r`nfunction Invoke-T1Exe { }`r`n`$script:StallSeconds = 1`r`n"
    $shBadHits = @(& $shScan $shBad)
    if ($shHits.Count -or $shBadHits.Count -ne 2) { $failed++; Write-Host "FAILED ${n}: common.ps1, smoke.ps1 and smoke\cases.ps1 must define each function and script variable once: $($shHits -join ', '); an injected redefinition found $($shBadHits.Count) of 2" }
    else { Write-Host "OK $n common.ps1, smoke.ps1 and smoke\cases.ps1 define each function and script variable once, and the scan finds an injected redefinition" }
    $n++
    $giOutside = @('tests\smoke.ps1', 'tests\smoke\cases.ps1', 'tests\gate\keys.ps1')
    $giScan = {
        param([hashtable]$Texts)
        $defs = @{}
        $hits = @()
        foreach ($name in $Texts.Keys) {
            $errs = $null
            $ast = [Management.Automation.Language.Parser]::ParseInput($Texts[$name], [ref]$null, [ref]$errs)
            if (@($errs).Count) { return @("$name does not parse: $(@($errs)[0].Message)") }
            $found = @($ast.FindAll({ param($x) $x -is [Management.Automation.Language.FunctionDefinitionAst] }, $false) | ForEach-Object { "function $($_.Name)" })
            $found += @($ast.FindAll({ param($x) $x -is [Management.Automation.Language.AssignmentStatementAst] -and $x.Left -is [Management.Automation.Language.VariableExpressionAst] }, $false) | ForEach-Object { 'variable ' + ($_.Left.VariablePath.UserPath -replace '^script:', '') })
            if ($ast.ParamBlock) { $found += @($ast.ParamBlock.Parameters | ForEach-Object { 'variable ' + $_.Name.VariablePath.UserPath }) }
            $defs[$name] = @($found | ForEach-Object { $_.ToLowerInvariant() } | Sort-Object -Unique)
            if ($giOutside -contains $name) {
                if ($ast.ParamBlock -or $ast.BeginBlock -or $ast.ProcessBlock) { $hits += "$name has a param, begin or process block" }
                foreach ($s in $ast.EndBlock.Statements) {
                    if ($s -is [Management.Automation.Language.FunctionDefinitionAst]) { continue }
                    if ($s -is [Management.Automation.Language.AssignmentStatementAst] -and $s.Left.Extent.Text -match '^\$script:\w+$') { continue }
                    $load = $s.Extent.Text -replace '\. \(Join-Path \$PSScriptRoot ''(?:common\.ps1|vstools\.ps1|smoke\\cases\.ps1)''\)', 'LOAD'
                    if ($load -cmatch '^if \(\$PSScriptRoot\) \{ (?:if \(-not \$script:HarnessCommonLoaded\) \{ LOAD \}; )?LOAD \}$') { continue }
                    $hits += "$name runs top-level code: $($s.Extent.Text)"
                }
            }
        }
        $names = @($defs.Keys | Sort-Object)
        for ($i = 0; $i -lt $names.Count; $i++) {
            for ($j = $i + 1; $j -lt $names.Count; $j++) {
                if ($giOutside -notcontains $names[$i] -and $giOutside -notcontains $names[$j]) { continue }
                foreach ($d in $defs[$names[$i]]) { if ($defs[$names[$j]] -contains $d) { $hits += "$d in $($names[$i]) and $($names[$j])" } }
            }
        }
        return $hits
    }
    $giTexts = @{}
    foreach ($f in 'tests\common.ps1', 'tests\vstools.ps1', 'tests\guard-revert.ps1', 'tests\guardrevert\select.ps1', 'tests\gate\keys.ps1', 'tests\smoke.ps1', 'tests\smoke\cases.ps1') { $giTexts[$f] = [IO.File]::ReadAllText((Join-Path $repo $f)) }
    $giHits = @(& $giScan $giTexts)
    $giBad = $giTexts.Clone()
    $giBad['tests\smoke.ps1'] += "`r`nfunction Run-Case { }`r`n`$script:TestsConfiguration = 'x'`r`n"
    $giBad['tests\smoke\cases.ps1'] += "`r`nSet-StrictMode -Version 2`r`n"
    $giBad['tests\gate\keys.ps1'] += "`r`nfunction Get-GuardT1State { }`r`n"
    $giBadHits = @(& $giScan $giBad)
    if ($giHits.Count -or $giBadHits.Count -ne 4) { $failed++; Write-Host "FAILED ${n}: smoke.ps1, smoke\cases.ps1 and keys.ps1 must not redefine anything a T1 row keys on or run top-level code: $($giHits -join '; '); injected faults found $($giBadHits.Count) of 4: $($giBadHits -join '; ')" }
    else { Write-Host "OK $n smoke.ps1, smoke\cases.ps1 and keys.ps1 redefine nothing in common, vstools, guard-revert or select and run no top-level code, and the scan finds 4 injected faults" }
    $rcTmp = Join-Path $env:TEMP ("openswbf2-selftest-case-" + [guid]::NewGuid().ToString('N'))
    $rcSavedDigest = $env:OPENSWBF2_DIGEST_EXPECTED
    try {
        $env:OPENSWBF2_DIGEST_EXPECTED = $null
        $rcSel = Join-Path $PSScriptRoot 'guardrevert\select.ps1'
        $rcRowsFile = Join-Path $PSScriptRoot 'guardrevert\rows.ps1'
        $rcExtra = @(
            @{ Id = 'X-INI'; Kind = 'Smoke'; Case = 'display-badini'; File = 'src\X\none.cpp'; Anchor = 'a'; Replacement = 'b' },
            @{ Id = 'X-NONE'; Kind = 'Smoke'; Case = 'no-such-case'; File = 'src\X\none.cpp'; Anchor = 'a'; Replacement = 'b' },
            @{ Id = 'X-PNONE'; Kind = 'Perturb'; Config = 'Test'; Missions = @('no_such_mission'); File = 'src\X\none.cpp'; Anchor = 'a'; Replacement = 'b' }
        )
        $rcNew = {
            param([string]$Name)
            $t = Join-Path $rcTmp "$Name\tests"
            New-Item -ItemType Directory -Force -Path (Join-Path $t 'smoke') | Out-Null
            foreach ($f in 'common.ps1', 'vstools.ps1', 'smoke.ps1') { Copy-Item -LiteralPath (Join-Path $PSScriptRoot $f) -Destination $t }
            Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'smoke') -File | Copy-Item -Destination (Join-Path $t 'smoke')
            return $t
        }
        $rcCase = {
            param([string]$Tests)
            $iss = [Management.Automation.Runspaces.InitialSessionState]::CreateDefault()
            $iss.ExecutionPolicy = 'Bypass'
            $rs = [runspacefactory]::CreateRunspace($iss)
            $rs.Open()
            $ps = [powershell]::Create()
            $ps.Runspace = $rs
            try {
                [void]$ps.AddScript({
                    param($t, $sel, $rowsFile, $extra)
                    . (Join-Path $t 'smoke.ps1')
                    . $sel
                    . $rowsFile
                    $rows = @($GuardRevertRows) + @($extra)
                    @{ Rows = $rows; Texts = (Get-GuardCaseTexts $rows 'Release' (Split-Path $t -Parent)); Data = (Get-GuardCaseData (Join-Path $t 'smoke\cases.ps1')) }
                }).AddArgument($Tests).AddArgument($rcSel).AddArgument($rcRowsFile).AddArgument($rcExtra)
                $out = @($ps.Invoke())
                if ($ps.Streams.Error.Count) { throw "case runspace: $($ps.Streams.Error[0])" }
                if ($out.Count -ne 1) { throw "case runspace returned $($out.Count) results" }
                return $out[0]
            }
            finally { $ps.Dispose(); $rs.Dispose() }
        }
        $rcDiff = {
            param($A, $B)
            return @(@($A.Keys) + @($B.Keys) | Sort-Object -Unique | Where-Object { [string]$A[$_] -cne [string]$B[$_] })
        }
        $rcEdit = {
            param([string]$Path, [string]$Old, [string]$New)
            $text = [IO.File]::ReadAllText($Path)
            $at = $text.IndexOf($Old, [StringComparison]::Ordinal)
            if ($at -lt 0 -or $text.IndexOf($Old, $at + 1, [StringComparison]::Ordinal) -ge 0) { throw "edit anchor '$Old' is not unique in $Path" }
            [IO.File]::WriteAllText($Path, $text.Substring(0, $at) + $New + $text.Substring($at + $Old.Length))
        }
        $rcBase = & $rcCase (& $rcNew 'base')
        $rcIds = {
            param([scriptblock]$Pick)
            return @($rcBase.Rows | Where-Object $Pick | ForEach-Object { [string]$_.Id } | Sort-Object)
        }
        $rcCheck = {
            param([string]$Name, $Got, [string[]]$Want, [bool]$DataSame)
            $why = $null
            if ($DataSame -and @($Want).Count -eq 0) { $why = 'no row to prove it on' }
            elseif (((& $rcDiff $rcBase.Texts $Got.Texts) -join ',') -cne (@($Want | Sort-Object) -join ',')) { $why = "changed '$((& $rcDiff $rcBase.Texts $Got.Texts) -join ',')', expected '$(@($Want | Sort-Object) -join ',')'" }
            elseif ($DataSame -ne ($rcBase.Data -ceq $Got.Data)) { $why = "case data same=$($rcBase.Data -ceq $Got.Data), expected $DataSame" }
            Write-SelResult "real cases: $Name" $why
        }
        $why = $null
        $rcSmoke = & $rcIds { $_.Kind -eq 'Smoke' -or $_.Kind -eq 'Perturb' }
        $rcMissing = @($rcSmoke | Where-Object { -not $rcBase.Texts.ContainsKey($_) })
        if (($rcMissing -join ',') -cne 'X-NONE,X-PNONE') { $why = "rows without a case text: '$($rcMissing -join ',')', expected 'X-NONE,X-PNONE'" }
        elseif (-not $rcBase.Data) { $why = 'no case data' }
        Write-SelResult 'real cases: every smoke and perturb row resolves a case text except unknown names' $why
        $t = & $rcNew 'args'
        & $rcEdit (Join-Path $t 'smoke\cases.ps1') '-crashtest resetflap"' '-crashtest resetflap -nowait"'
        & $rcCheck 'one case Args edited changes only the rows naming that case' (& $rcCase $t) (& $rcIds { $_.Kind -eq 'Smoke' -and $_.Case -eq 'crash-resetflap' }) $true
        $t = & $rcNew 'digest'
        $dl = @([IO.File]::ReadAllLines((Join-Path $t 'smoke\digest-expected.txt')) | Where-Object { $_.StartsWith("geo1c_con`t") })
        if ($dl.Count -ne 1) { throw "digest-expected.txt has $($dl.Count) geo1c_con lines" }
        & $rcEdit (Join-Path $t 'smoke\digest-expected.txt') $dl[0] ($dl[0] + 'x')
        & $rcCheck 'one digest-expected line edited changes only the rows naming that case' (& $rcCase $t) (& $rcIds { $_.Kind -eq 'Smoke' -and $_.Case -eq 'geo1c_con' }) $true
        $t = & $rcNew 'errors'
        & $rcEdit (Join-Path $t 'smoke\expected-errors.txt') '[crash-luaoom]' "[crash-luaoom]`r`n^selftest only`$"
        & $rcCheck 'one expected-errors section edited changes only the rows naming that case' (& $rcCase $t) (& $rcIds { $_.Kind -eq 'Smoke' -and $_.Case -eq 'crash-luaoom' }) $true
        $t = & $rcNew 'global'
        & $rcEdit (Join-Path $t 'smoke\expected-errors.txt') '[global]' "[global]`r`n^selftest only`$"
        & $rcCheck 'the expected-errors global section changes every resolved smoke and perturb row' (& $rcCase $t) @($rcSmoke | Where-Object { $rcMissing -notcontains $_ }) $true
        $t = & $rcNew 'ini'
        Add-Content -LiteralPath (Join-Path $t 'smoke\display-bad.ini') -Value 'Selftest=1'
        & $rcCheck 'a display ini a case names changes only that case' (& $rcCase $t) @('X-INI') $true
        $t = & $rcNew 'allow'
        & $rcEdit (Join-Path $t 'smoke\cases.ps1') "'src\lglcd\lgLcd.cpp|Data|[0x104]|x64:" "'src\lglcd\lgLcd.cpp|Data|[0x105]|x64:"
        & $rcCheck 'an allow-list edit changes case data and no case text' (& $rcCase $t) @() $false
        $rcInfo = Get-GuardTestInfo $repo
        $rcMbSrc = 'tests\unit\memory_baseline.cpp'
        $rcMbFiles = @('tests\smoke\missions-all.txt', 'tests\smoke\digest-expected.txt', 'tests\baseline\v143\CAUSES.md', 'tests\baseline\v143\pool-sizes.txt', 'tests\baseline\v143\mem-peaks.txt', 'tests\baseline\v143\lua-bins.txt', 'tests\baseline\v143\cloth-headroom.txt')
        $rcMbTest = 'baseline causes add exactly one digest cause since the milestone base'
        $rcOther = @($rcBase.Rows | Where-Object { (Get-GuardRowKind $_) -eq 'T1' -and @($_.Tests | Where-Object { $rcInfo.Sources[$_] -eq $rcMbSrc }).Count -eq 0 } | Select-Object -First 1)
        $rcRows = @($rcBase.Rows) + @(@{ Id = 'X-MB'; File = 'src\RedEngineFL\Memory\RedMemory.cpp'; Anchor = 'a'; Replacement = 'b'; Tests = @($rcMbTest) })
        $rcHashes = Get-GuardHashes $repo
        $rcIn = @{
            Rows = $rcRows; Proofs = @{}; Hashes = $rcHashes; TestsReads = $null; AsanReads = $null; GameReads = $null
            TestsProjectGameTUs = @(); TestSources = $rcInfo.Sources; TestLiterals = $rcInfo.Literals
            CaseTexts = $rcBase.Texts; CaseData = $rcBase.Data; Toolset = 'x'; Platform = 'Win32'; WithDeps = $true
        }
        $rcRes = @{}
        foreach ($x in @(Select-GuardRows $rcIn)) { $rcRes[$x.Id] = $x }
        $why = $null
        if ($rcInfo.Sources[$rcMbTest] -ne $rcMbSrc) { $why = "test '$rcMbTest' maps to '$($rcInfo.Sources[$rcMbTest])'" }
        elseif ($rcOther.Count -ne 1) { $why = 'no T1 row outside memory_baseline.cpp' }
        else {
            foreach ($f in $rcMbFiles) {
                $h = $rcHashes.Clone()
                $h[$f] = 'changed'
                $in2 = $rcIn.Clone()
                $in2.Hashes = $h
                $in2.Rows = @($rcRows | Where-Object { $_.Id -eq 'X-MB' -or $_.Id -eq $rcOther[0].Id })
                $r2 = @{}
                foreach ($x in @(Select-GuardRows $in2)) { $r2[$x.Id] = $x }
                if ($r2['X-MB'].DepDigest -ceq $rcRes['X-MB'].DepDigest) { $why = "changing $f left the memory_baseline row key alone"; break }
                if ($r2[$rcOther[0].Id].DepDigest -cne $rcRes[$rcOther[0].Id].DepDigest) { $why = "changing $f changed T1 row $($rcOther[0].Id)"; break }
            }
        }
        Write-SelResult 'real T1: each expectation file memory_baseline.cpp reads changes its row key and no other T1 row key' $why
        $rcDepFiles = {
            param($Res)
            $s = New-GuardPathSet
            foreach ($l in @($Res.Deps)) { [void]$s.Add($l.Split("`t")[0]) }
            return , $s
        }
        $rcNever = @('tests\smoke.ps1', 'tests\smoke\cases.ps1', 'tests\run.ps1', 'tests\smoke\expected-errors.txt')
        $rcAlways = @('tests\common.ps1', 'tests\vstools.ps1', 'tests\guard-revert.ps1', 'tests\guardrevert\select.ps1')
        $why = $null
        foreach ($f in $rcAlways) { if ($script:GuardGlobalTriggers -notcontains $f) { $why = "$f is not a global trigger" } }
        foreach ($f in $rcNever) { if ($script:GuardGlobalTriggers -contains $f) { $why = "$f is a global trigger" } }
        $rcKinds = @{}
        foreach ($row in $rcRows) {
            if ($why) { break }
            $kind = Get-GuardRowKind $row
            $res = $rcRes[[string]$row.Id]
            $have = & $rcDepFiles $res
            $need = @([string]$row.File)
            $never = @()
            if ($kind -eq 'T1' -or $kind -eq 'Asan' -or $kind -eq 'AsanCanary') {
                $never = $rcNever
                foreach ($tn in @($row.Tests)) {
                    $src = $rcInfo.Sources[$tn]
                    if (-not $src) { continue }
                    $need += $src
                    foreach ($lit in @($rcInfo.Literals[$src])) {
                        if (-not $lit) { continue }
                        if ($rcHashes.ContainsKey($lit)) { $need += $lit }
                        else { $need += @($rcHashes.Keys | Where-Object { $_.StartsWith($lit.TrimEnd('\') + '\', [StringComparison]::OrdinalIgnoreCase) }) }
                    }
                }
                if ($row.Id -eq 'X-MB') { $need += $rcMbFiles }
            }
            else {
                $need += 'tests\smoke.ps1'
                if (@($res.Deps | Where-Object { $_ -ceq ("casedata`t" + (Get-GuardSha ($rcBase.Data -replace "`r`n", "`n"))) }).Count -ne 1) { $why = "$($row.Id) does not key on the case data" }
                if ($kind -eq 'Smoke' -or $kind -eq 'Perturb') {
                    $need += @($rcHashes.Keys | Where-Object { $_.StartsWith('data\scripts\', [StringComparison]::OrdinalIgnoreCase) })
                    $ct = [string]$rcBase.Texts[[string]$row.Id]
                    if ($ct -and @($res.Deps | Where-Object { $_ -ceq ("case`t" + (Get-GuardSha $ct)) }).Count -ne 1) { $why = "$($row.Id) does not key on its case text" }
                    if ($kind -eq 'Smoke' -and $ct) {
                        $miss = @()
                        if ($ct.IndexOf('"Name":"' + $row.Case + '"', [StringComparison]::Ordinal) -lt 0) { $miss += 'entry' }
                        if ($ct.IndexOf("digest`t" + [string](Get-DigestExpected $row.Case), [StringComparison]::Ordinal) -lt 0) { $miss += 'digest line' }
                        foreach ($a in @(Get-SmokeAllowed $row.Case)) { if ($ct.IndexOf("allowed`t$a", [StringComparison]::Ordinal) -lt 0) { $miss += "allowed $a" } }
                        if ($miss.Count) { $why = "$($row.Id) case text lacks $($miss -join ', ')" }
                    }
                }
            }
            foreach ($f in $need) { if ($f -and $rcHashes.ContainsKey($f) -and -not $have.Contains($f)) { $why = "$($row.Id) ($kind) does not key on $f"; break } }
            foreach ($f in $never) { if ($have.Contains($f)) { $why = "$($row.Id) ($kind) keys on $f"; break } }
            $rcKinds[$kind] = 1 + [int]$rcKinds[$kind]
        }
        if (-not $why) {
            foreach ($k in 'T1', 'Asan', 'AsanCanary', 'Smoke', 'Perturb', 'Hardening', 'Tlog') { if (-not $rcKinds[$k]) { $why = "no $k row checked" } }
        }
        Write-SelResult "real rows: every row keys on each file its kind reads and no T1 row keys on smoke scripts ($(@($rcKinds.Keys | Sort-Object | ForEach-Object { "$_ $($rcKinds[$_])" }) -join ', '))" $why
    }
    catch { $failed++; Write-Host "FAILED: real case key fixtures threw: $_" }
    finally {
        $env:OPENSWBF2_DIGEST_EXPECTED = $rcSavedDigest
        if (Test-Path -LiteralPath $rcTmp) { Remove-Item -LiteralPath $rcTmp -Recurse -Force -ErrorAction SilentlyContinue }
    }
    $n++
    $kUnlisted = @('src\Battlefront2\Source\GameLoop.cpp', 'src\Battlefront2\Source\GameLoop.h', 'BattlefrontII.vcxproj', 'tests\unit\harness_smoke.cpp', 'data\scripts\common\globals.lua')
    foreach ($f in $kUnlisted) {
        $b = & $kKey ([scriptblock]::Create("param(`$i) `$i.Files['$f'] = 'changed'"))
        if (-not $kTree.ContainsKey($f) -or $b.RunKey -ceq $kPlain.RunKey) { $failed++; Write-Host "FAILED ${n}: unlisted tree file $f did not change RunKey" }
        else { Write-Host "OK $n unlisted tree file $f changes RunKey" }
        $n++
    }
    $b = & $kKey { param($i) $i.Files['tests\new-file.txt'] = 'new' }
    if ($b.RunKey -ceq $kPlain.RunKey) { $failed++; Write-Host "FAILED ${n}: a new tree file did not change RunKey" }
    else { Write-Host "OK $n a new tree file changes RunKey" }
    $n++
    $gRepo = Join-Path $kTmp 'repo'
    New-Item -ItemType Directory -Path $gRepo | Out-Null
    $why = $null
    & {
        $ErrorActionPreference = 'Continue'
        & git -C $gRepo init -q 2>&1 | Out-Null
        [IO.File]::WriteAllText((Join-Path $gRepo 'a.txt'), "one`r`n")
        [IO.File]::WriteAllText((Join-Path $gRepo '.gitignore'), "*.ign`r`n")
        & git -C $gRepo add a.txt .gitignore 2>&1 | Out-Null
        & git -C $gRepo -c user.name=selftest -c user.email=selftest@invalid commit -q -m init 2>&1 | Out-Null
    }
    $g0 = Get-TreeFileHashes $gRepo
    $gIn = { param($t) $i = $kBase.Clone(); $i.Files = $t; Get-StepKey $i }
    $gk0 = & $gIn $g0
    [IO.File]::WriteAllText((Join-Path $gRepo 'a.txt'), "two`r`n")
    $g1 = Get-TreeFileHashes $gRepo
    [IO.File]::WriteAllText((Join-Path $gRepo 'b.txt'), "new`r`n")
    $g2 = Get-TreeFileHashes $gRepo
    [IO.File]::WriteAllText((Join-Path $gRepo 'c.ign'), "ignored`r`n")
    $g3 = Get-TreeFileHashes $gRepo
    if ($g0.Count -ne 2 -or $g0['a.txt'] -cne (Get-KeyTextSha "one`r`n")) { $why = "clean tree table: $(@($g0.Keys) -join ', ')" }
    elseif ((& $gIn $g1).RunKey -ceq $gk0.RunKey) { $why = 'an unstaged edit left RunKey equal' }
    elseif (-not $g2.ContainsKey('b.txt') -or (& $gIn $g2).RunKey -ceq (& $gIn $g1).RunKey) { $why = 'a new untracked file left RunKey equal' }
    elseif ($g3.ContainsKey('c.ign') -or (& $gIn $g3).RunKey -cne (& $gIn $g2).RunKey) { $why = 'an ignored file entered the key' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: tree table from a git fixture: $why" }
    else { Write-Host "OK $n tree table hashes working-tree content: an unstaged edit and a new untracked file change RunKey, an ignored file does not" }
    $n++
    $sel = {
        param($s, [string]$prov)
        $i = $kBase.Clone()
        $i.Files = $kBase.Files.Clone()
        $i.Kind = 'guard'
        $i.JudgeKinds = @('guard')
        $i.selection = $s
        if ($prov) { $i.Files['tests\guardrevert\proven.tsv'] = $prov }
        Get-StepKey $i
    }
    $sA = & $sel @('MEM-01', 'LHASH-01')
    $sA2 = & $sel @('LHASH-01', 'MEM-01')
    $sB = & $sel @('MEM-01')
    $sNone = & $sel 'none'
    $sEmpty = & $sel @()
    $sProv = & $sel @('MEM-01', 'LHASH-01') 'changed'
    $sProvSel = & $sel @('MEM-01') 'changed'
    $why = $null
    if (-not $sA.Valid -or $sA.RunKey -cne $sA2.RunKey) { $why = 'the selection is not order-independent' }
    elseif ($sA.RunKey -ceq $sB.RunKey) { $why = 'a different selection left RunKey equal' }
    elseif (-not $sNone.Valid) { $why = "selection 'none' is invalid" }
    elseif ($sEmpty.Valid) { $why = 'an empty selection is valid' }
    elseif ($sProv.RunKey -cne $sA.RunKey) { $why = 'proven.tsv changed RunKey with the selection equal' }
    elseif ($sProvSel.RunKey -ceq $sA.RunKey) { $why = 'proven.tsv with a new selection left RunKey equal' }
    elseif ((& $kKey { param($i) $i.selection = @('MEM-01') }).Header -match '^selection') { $why = 'a non-guard step keys a selection' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: guard selection key: $why" }
    else { Write-Host "OK $n guard step RunKey holds the sorted live selection; empty is invalid; proven.tsv acts only through the selection" }
    $n++
    foreach ($f in @('tests\guardrevert\steps.tsv', 'tests\guardrevert\proven.tsv', 'tests\baseline\gate-times.tsv', 'artifacts\guard-revert\x\proven.partial.tsv') + $script:KeyExcludedDocs) {
        $b = & $kKey ([scriptblock]::Create("param(`$i) `$i.Files['$f'] = 'changed'"))
        $why = $null
        if ($script:KeyExcludedDocs -contains $f -and -not $kTree.ContainsKey($f)) { $why = 'not in the tree table' }
        elseif ($b.RunKey -cne $kPlain.RunKey -or $b.JudgeKey -cne $kPlain.JudgeKey) { $why = 'a key changed' }
        if ($why) { $failed++; Write-Host "FAILED ${n}: excluded $f must change neither key: $why" }
        else { Write-Host "OK $n excluded $f changes neither RunKey nor JudgeKey" }
        $n++
    }
    $why = $null
    foreach ($fld in 'step', 'platform', 'config', 'tier', 'toolset', 'cl', 'ucrt', 'steam', 'overlay', 'gamedata', 'exe', 'dlls', 'vendor') {
        foreach ($bad in @($null, '', @{})) {
            $i = $kBase.Clone(); $i[$fld] = $bad
            if ((Get-StepKey $i).Valid) { $why = "$fld = '$bad' is valid" }
        }
    }
    $i = $kBase.Clone(); $i.Remove('Files')
    if ((Get-StepKey $i).Valid) { $why = 'no file table is valid' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: missing key field: $why" }
    else { Write-Host "OK $n any missing or empty key field or file table makes the key invalid" }
    $n++
    $why = $null
    foreach ($ov in '-', 'unknown', 'ON') { if ((& $kKey ([scriptblock]::Create("param(`$i) `$i.overlay = '$ov'"))).Valid) { $why = "overlay '$ov' is valid" } }
    if ((& $kKey { param($i) $i.vendor['d3dx9_43.dll'] = '-' }).Valid) { $why = 'a vendor DLL hash of - is valid' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: unreadable field: $why" }
    else { Write-Host "OK $n overlay '-' or any value other than on/off, and a vendor hash of '-', make the key invalid" }
    $n++
    $vdf = {
        param([string]$apps)
        "`"UserLocalConfigStore`"`n{`n`t`"Software`"`n`t{`n`t`t`"Valve`"`n`t`t{`n`t`t`t`"Steam`"`n`t`t`t{`n`t`t`t`t`"apps`"`n`t`t`t`t{`n`t`t`t`t`t`"6060`"`n`t`t`t`t`t{`n`t`t`t`t`t`t`"OverlayAppEnable`"`t`t`"0`"`n`t`t`t`t`t}`n`t`t`t`t}`n`t`t`t}`n`t`t}`n`t}`n`t`"apps`"`n`t{`n$apps`t}`n}`n"
    }
    $vdfCases = @(
        @('off', "`t`t`"319630`"`n`t`t{`n`t`t`t`"OverlayAppEnable`"`t`t`"1`"`n`t`t}`n`t`t`"6060`"`n`t`t{`n`t`t`t`"OverlayAppEnable`"`t`t`"0`"`n`t`t}`n"),
        @('on', "`t`t`"6060`"`n`t`t{`n`t`t`t`"OverlayAppEnable`"`t`t`"1`"`n`t`t}`n"),
        @('on', "`t`t`"319630`"`n`t`t{`n`t`t`t`"OverlayAppEnable`"`t`t`"0`"`n`t`t}`n"),
        @('on', "`t`t`"6060`"`n`t`t{`n`t`t`t`"LaunchOptions`"`t`t`"`"`n`t`t}`n"),
        @('-', "`t`t`"6060`"`n`t`t{`n`t`t`t`"OverlayAppEnable`"`t`t`"2`"`n`t`t}`n"),
        @('-', "`t`t`"6060`"`n`t`t{`n`t`t`t`"OverlayAppEnable`"`t`t`"0`"`n")
    )
    $why = $null
    $vi = 0
    foreach ($c in $vdfCases) {
        $vi++
        $vp = Join-Path $kTmp "localconfig$vi.vdf"
        [IO.File]::WriteAllText($vp, (& $vdf $c[1]))
        $got = Get-SteamOverlayFromVdf $vp
        if ($got -cne $c[0]) { $why = "fixture $vi gave '$got', expected '$($c[0])'" }
    }
    if ((Get-SteamOverlayFromVdf (Join-Path $kTmp 'missing.vdf')) -cne '-') { $why = 'a missing file is not -' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: overlay from localconfig.vdf: $why" }
    else { Write-Host "OK $n overlay reads the root-level apps 6060 OverlayAppEnable: 0 off, 1 or absent on, other or unbalanced or missing file -" }
    $n++
    $rec = [pscustomobject]@{ Step = 's1'; RunKey = $kPlain.RunKey; JudgeKey = $kPlain.JudgeKey; Platform = 'Win32'; Config = 'Release'; Tier = 'plan'; Evidence = 'abc'; Result = 'PASS'; Tree = 't'; Commit = 'c'; Date = '2026-11-15T00:00:00Z' }
    $recNoPlat = $rec.PSObject.Copy(); $recNoPlat.Platform = ''
    $recDash = $rec.PSObject.Copy(); $recDash.Evidence = '-'
    $x64Key = & $kKey { param($i) $i.platform = 'x64' }
    $why = $null
    if (-not (Test-StepRecordMatch $rec $kPlain)) { $why = 'a matching Win32 record does not match' }
    elseif (Test-StepRecordUsable $recNoPlat) { $why = 'a record with no platform is usable' }
    elseif (Test-StepRecordUsable $recDash) { $why = 'a record with a - field is usable' }
    elseif (Test-StepRecordMatch $rec $x64Key) { $why = 'a Win32 record matches an x64 key' }
    else {
        $x64Rec = $rec.PSObject.Copy(); $x64Rec.Platform = 'x64'
        if (Test-StepRecordMatch $x64Rec $kPlain) { $why = 'an x64 record matches a Win32 key' }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: step record use: $why" }
    else { Write-Host "OK $n a step record without platform or with a - field is unusable, and a Win32 record never matches an x64 key" }
    $n++
    $stp = Join-Path $kTmp 'steps.tsv'
    Add-StepRecord $stp $rec
    $r2 = $rec.PSObject.Copy(); $r2.Step = 'a0'
    Add-StepRecord $stp $r2
    $r3 = $rec.PSObject.Copy(); $r3.Result = 'PASS2'
    Add-StepRecord $stp $r3
    $r4 = $rec.PSObject.Copy(); $r4.Platform = 'x64'
    Merge-StepRecords $stp @((@($script:StepColumns | ForEach-Object { [string]$r4.$_ }) -join "`t"))
    $raw = [IO.File]::ReadAllText($stp)
    $read = Read-StepRecords $stp
    $rows = @($raw -split "`r`n" | Where-Object { $_ })
    $why = $null
    if ($raw -match "[^`r]`n") { $why = 'not CRLF' }
    elseif ($rows[0] -cne ($script:StepColumns -join "`t")) { $why = "header $($rows[0])" }
    elseif ($rows.Count -ne 4) { $why = "$($rows.Count - 1) records, expected 3" }
    elseif (@($rows[1..3] | ForEach-Object { $_.Split("`t")[0] + '/' + $_.Split("`t")[3] }) -join ' ' -cne 'a0/Win32 s1/Win32 s1/x64') { $why = "order $(@($rows[1..3] | ForEach-Object { $_.Split("`t")[0] + '/' + $_.Split("`t")[3] }) -join ' ')" }
    elseif ($read["s1`tWin32`tRelease`tplan"].Result -cne 'PASS2') { $why = 'the newer record did not replace the old one' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: steps.tsv records: $why" }
    else { Write-Host "OK $n steps.tsv is CRLF, sorted by Step, Platform, Config, Tier, and a newer record replaces the same key" }
    $n++
    $pe = {
        param([bool]$plus)
        $b = New-Object byte[] 0x400
        $b[0] = 0x4D; $b[1] = 0x5A
        [BitConverter]::GetBytes([int]0x40).CopyTo($b, 0x3C)
        [BitConverter]::GetBytes([uint32]0x4550).CopyTo($b, 0x40)
        [BitConverter]::GetBytes([uint16]1).CopyTo($b, 0x46)
        [BitConverter]::GetBytes([uint32]0x6AFA0102).CopyTo($b, 0x48)
        $opt = 0x58
        if ($plus) { $optSize = 240; $magic = 0x20B; $cnt = $opt + 108; $dir = $opt + 112 } else { $optSize = 224; $magic = 0x10B; $cnt = $opt + 92; $dir = $opt + 96 }
        [BitConverter]::GetBytes([uint16]$optSize).CopyTo($b, 0x54)
        [BitConverter]::GetBytes([uint16]$magic).CopyTo($b, $opt)
        [BitConverter]::GetBytes([uint32]0x1234).CopyTo($b, $opt + 64)
        [BitConverter]::GetBytes([uint32]16).CopyTo($b, $cnt)
        [BitConverter]::GetBytes([uint32]0x1000).CopyTo($b, $dir + 48)
        [BitConverter]::GetBytes([uint32]56).CopyTo($b, $dir + 52)
        $sec = $opt + $optSize
        [BitConverter]::GetBytes([uint32]0x1000).CopyTo($b, $sec + 12)
        [BitConverter]::GetBytes([uint32]0x200).CopyTo($b, $sec + 16)
        [BitConverter]::GetBytes([uint32]0x200).CopyTo($b, $sec + 20)
        [BitConverter]::GetBytes([uint32]0x6AFA0102).CopyTo($b, 0x204)
        [BitConverter]::GetBytes([uint32]2).CopyTo($b, 0x20C)
        [BitConverter]::GetBytes([uint32]30).CopyTo($b, 0x210)
        [BitConverter]::GetBytes([uint32]0x240).CopyTo($b, 0x218)
        [BitConverter]::GetBytes([uint32]0x6AFA0102).CopyTo($b, 0x220)
        [BitConverter]::GetBytes([uint32]13).CopyTo($b, 0x228)
        [BitConverter]::GetBytes([uint32]0x53445352).CopyTo($b, 0x240)
        for ($j = 0; $j -lt 20; $j++) { $b[0x244 + $j] = [byte](0x11 + $j) }
        [Text.Encoding]::ASCII.GetBytes('x.pdb').CopyTo($b, 0x258)
        $b[0x300] = 0x90
        return , $b
    }
    $why = $null
    foreach ($plus in $false, $true) {
        $p0 = & $pe $plus
        $h0 = Get-KeyBytesSha (Get-PeNormalisedBytes $p0)
        $edits = [ordered]@{ 'COFF TimeDateStamp' = 0x48; 'CheckSum' = 0x58 + 64; 'debug entry 1 TimeDateStamp' = 0x204; 'debug entry 2 TimeDateStamp' = 0x220; 'RSDS GUID' = 0x244; 'RSDS GUID end' = 0x253; 'RSDS age' = 0x254 }
        foreach ($en in $edits.Keys) {
            $p1 = & $pe $plus
            $p1[$edits[$en]] = [byte]($p1[$edits[$en]] -bxor 0x5A)
            if ((Get-KeyBytesSha (Get-PeNormalisedBytes $p1)) -cne $h0) { $why = "plus=$plus $en changed the hash" }
        }
        foreach ($at in 0x300, 0x258) {
            $p2 = & $pe $plus
            $p2[$at] = [byte]($p2[$at] -bxor 0x01)
            if ((Get-KeyBytesSha (Get-PeNormalisedBytes $p2)) -ceq $h0) { $why = ('plus={0} a byte at 0x{1:X} left the hash equal' -f $plus, $at) }
        }
        if ((Get-KeyBytesSha $p0) -ceq $h0) { $why = "plus=$plus normalising changed nothing" }
    }
    $threw = $false
    try { [void](Get-PeNormalisedBytes ([Text.Encoding]::ASCII.GetBytes('not a pe file at all, just text that is long enough to read 0x3c'))) } catch { $threw = $true }
    if (-not $threw) { $why = 'a non-PE input did not throw' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: PE normalised hash: $why" }
    else { Write-Host "OK $n PE normalised hash ignores COFF and debug timestamps, CheckSum and RSDS GUID and age in PE32 and PE32+, changes on a code or PDB path byte, and throws on a non-PE file" }
    $n++
}
catch { $failed++; Write-Host "FAILED: step key block threw: $_" }
finally {
    Remove-Item -LiteralPath $kTmp -Recurse -Force -ErrorAction SilentlyContinue
}
$rjTmp = Join-Path $env:TEMP ('openswbf2-rejudge-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $rjTmp | Out-Null
$rjDigestCfg = $script:SmokeDigestConfiguration
try {
    $rjDefaults = @{ Configuration = 'Debug'; Platform = $Platform; NoBuild = 'False'; Suite = ''; Filter = ''; Fuzz = 'False'; Seed = '1'; Iterations = '100000'; Smoke = 'Standard'; FactoryLog = 'False'; MiPurge = '-1'; SmokeConfiguration = 'Release'; Movies = 'False'; Hardening = 'False'; HogAddrSpace = 'False'; Jobs = '0'; Asan = 'False'; Clean = 'False'; WorkerBase = '0'; Soak = 'Short' }
    $rjMio = 'OPENSWBF2 MIOPTIONS purge_delay=1000 retry_on_oom=0 arena_reserve=67108864 arena_eager_commit=0 reserve_os_memory=0 page_full_retain=-1'
    $rjDigest = 'OPENSWBF2 SIM DIGEST turns=1500 time=50.000 rand=1/2 snd=49.923 state=0x12345678'
    $rjRun = {
        param([string]$Name, [string]$Smoke, [string]$Missions, [string[]]$Cases, [hashtable]$Logs)
        $d = Join-Path $rjTmp $Name
        New-Item -ItemType Directory -Path (Join-Path $d 'hardening') -Force | Out-Null
        $lines = @()
        foreach ($p in @((Get-Command $run).Parameters.Keys | Where-Object { $_ -ne 'Rejudge' })) {
            if (-not $rjDefaults.ContainsKey($p)) { throw "fixture has no value for run.ps1 -$p" }
            $v = $rjDefaults[$p]
            if ($p -eq 'Smoke') { $v = $Smoke }
            $lines += "param.$p=$v"
        }
        $lines += @('bound=Smoke', "platform=$Platform", 'configuration=Debug', 'smoke_configuration=Release', "missions=$Missions", 'perturb_missions=', 'wer_localdumps=1')
        $lines | Set-Content -Path (Join-Path $d 'run-params.txt') -Encoding ASCII
        Set-Content -Path (Join-Path $d 'hardening\hardening.txt') -Value '[PASS] hardening:fixture' -Encoding ASCII
        foreach ($c in $Cases) {
            $cd = Join-Path $d "smoke\$c"
            New-Item -ItemType Directory -Path $cd -Force | Out-Null
            $lg = @{ Log = "$rjMio`r`n$rjDigest`r`nOPENSWBF2 RUN COMPLETE`r`n"; Out = ''; Err = ''; OutDone = 1; ErrDone = 1 }
            if ($Logs -and $Logs.ContainsKey($c)) { foreach ($k in @($Logs[$c].Keys)) { $lg[$k] = $Logs[$c][$k] } }
            [IO.File]::WriteAllText((Join-Path $cd 'run.log'), $lg.Log)
            [IO.File]::WriteAllText((Join-Path $cd 'stdout.txt'), $lg.Out)
            [IO.File]::WriteAllText((Join-Path $cd 'stderr.txt'), $lg.Err)
            [IO.File]::WriteAllText((Join-Path $cd 'result.txt'), "class=Pass`r`nexit=0x00000000`r`nelapsed_ms=1000`r`ndumps=0`r`nasan=0`r`nworkdir=w1`r`nstdout_complete=$($lg.OutDone)`r`nstderr_complete=$($lg.ErrDone)`r`nstall=0`r`nsilent_max_s=0.0`r`nturns=1500`r`nsteps=1`r`n")
        }
        return $d
    }
    $rjJudge = {
        param([string]$Dir)
        $o = & { $ErrorActionPreference = 'Continue'; & powershell -NoProfile -ExecutionPolicy Bypass -File $run -Rejudge $Dir 2>&1 | Out-String }
        $code = $LASTEXITCODE
        $file = Join-Path $Dir 'results-rejudge.txt'
        $text = $(if (Test-Path -LiteralPath $file) { [IO.File]::ReadAllText($file) } else { '' })
        return New-Object PSObject -Property @{ Code = $code; Out = $o; Text = $text }
    }
    $rjLive = {
        param([string]$Dir, $Lg)
        $script:SmokeDigestConfiguration = 'Release'
        $sc = @(Add-MiOptionsExpect @(Get-SmokeCases 'Dedicated' @() $false $null $false) -1)[0]
        $cd = Join-Path $Dir 'smoke\dedicated'
        $lt = [IO.File]::ReadAllText((Join-Path $cd 'run.log'))
        if ($Lg.OutDone -eq 1) { $lt += "`r`n" + $Lg.Out }
        if ($Lg.ErrDone -eq 1) { $lt += "`r`n" + $Lg.Err }
        $res = New-Object PSObject -Property @{ Class = 'Pass'; Code = [uint32]0; Dumps = @(); LogText = $lt; CaseDir = $cd; Turns = '1500' }
        $tok = Get-SmokeToken $res.Class
        if (Test-SmokeExpect $sc $res) { $tok = 'PASS' } elseif ($tok -eq 'PASS') { $tok = 'FAIL' }
        return $tok
    }
    $why = $null
    $rjVariants = @(
        @{ Name = 'plain'; Lg = @{ OutDone = 1; ErrDone = 1; Out = ''; Err = '' }; Want = 'PASS' },
        @{ Name = 'no-mioptions'; Lg = @{ Log = "$rjDigest`r`nOPENSWBF2 RUN COMPLETE`r`n"; OutDone = 1; ErrDone = 1; Out = ''; Err = '' }; Want = 'FAIL' },
        @{ Name = 'mioptions-in-stdout'; Lg = @{ Log = "$rjDigest`r`nOPENSWBF2 RUN COMPLETE`r`n"; OutDone = 1; ErrDone = 1; Out = $rjMio; Err = '' }; Want = 'PASS' },
        @{ Name = 'stdout-incomplete'; Lg = @{ Log = "$rjDigest`r`nOPENSWBF2 RUN COMPLETE`r`n"; OutDone = 0; ErrDone = 1; Out = $rjMio; Err = '' }; Want = 'FAIL' }
    )
    foreach ($v in $rjVariants) {
        if ($why) { break }
        $d = & $rjRun "eq-$($v.Name)" 'Dedicated' 'none' @('dedicated') @{ dedicated = $v.Lg }
        $live = & $rjLive $d ($v.Lg + @{})
        $rj = & $rjJudge $d
        $wantCode = $(if ($v.Want -eq 'PASS') { 0 } else { 1 })
        if ($live -ne $v.Want) { $why = "$($v.Name): live verdict $live, expected $($v.Want)" }
        elseif ($rj.Code -ne $wantCode -or $rj.Text -notmatch "(?m)^\[$live\] smoke:dedicated class=Pass exit=0x00000000 \(1000 ms\)\r?$") { $why = "$($v.Name): re-judge exit $($rj.Code) does not give the live verdict [$live]: $($rj.Out)" }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: re-judge equals live: $why" }
    else { Write-Host "OK $n re-judge of an archived case gives the live verdict, with and without the case's stdout and with an incomplete stream" }
    $n++
    $why = $null
    $d = & $rjRun 'flip' 'Dedicated' 'none' @('dedicated') $null
    $exp = Join-Path $rjTmp 'digest-expected.txt'
    $oldExp = $env:OPENSWBF2_DIGEST_EXPECTED
    try {
        $env:OPENSWBF2_DIGEST_EXPECTED = $exp
        [IO.File]::WriteAllText($exp, "dedicated`t$rjDigest`r`n")
        $a = & $rjJudge $d
        [IO.File]::WriteAllText($exp, "dedicated`t$($rjDigest.Replace('0x12345678', '0x12345679'))`r`n")
        $b = & $rjJudge $d
    }
    finally {
        if ($null -eq $oldExp) { Remove-Item Env:\OPENSWBF2_DIGEST_EXPECTED -ErrorAction SilentlyContinue } else { $env:OPENSWBF2_DIGEST_EXPECTED = $oldExp }
    }
    if (@(Get-ChildItem -LiteralPath $d -Recurse -Filter '*.exe').Count -ne 0) { $why = 'the fixture folder holds an exe' }
    elseif ($a.Code -ne 0 -or $a.Text -notmatch '(?m)^\[PASS\] smoke:dedicated ') { $why = "matching expectation gave exit $($a.Code): $($a.Out)" }
    elseif ($b.Code -ne 1 -or $b.Text -notmatch '(?m)^\[FAIL\] smoke:dedicated ' -or $b.Out -notmatch 'digest expected: .*0x12345679') { $why = "changed expectation gave exit $($b.Code): $($b.Out)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: re-judge expectation flip: $why" }
    else { Write-Host "OK $n changing a digest-expected line flips the re-judged verdict to FAIL with no exe in the run folder" }
    $n++
    $why = $null
    $d = & $rjRun 'deleted' 'Dedicated' 'none' @('dedicated') $null
    Remove-Item -LiteralPath (Join-Path $d 'smoke\dedicated') -Recurse -Force
    $c = & $rjJudge $d
    $d2 = & $rjRun 'no-result' 'Dedicated' 'none' @('dedicated') $null
    Remove-Item -LiteralPath (Join-Path $d2 'smoke\dedicated\result.txt')
    $c2 = & $rjJudge $d2
    $d3 = Join-Path $rjTmp 'no-params'
    New-Item -ItemType Directory -Path $d3 | Out-Null
    $c3 = & $rjJudge $d3
    if ($c.Code -ne 1 -or $c.Text -notmatch '(?m)^\[FAIL\] smoke:dedicated - no archived output\r?$') { $why = "deleted case folder gave exit $($c.Code): $($c.Out)" }
    elseif ($c2.Code -ne 1 -or $c2.Text -notmatch '(?m)^\[FAIL\] smoke:dedicated - no archived output\r?$') { $why = "missing result.txt gave exit $($c2.Code): $($c2.Out)" }
    elseif ($c3.Code -ne 2 -or $c3.Out -notmatch 'no run-params.txt') { $why = "folder without run-params.txt gave exit $($c3.Code)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: re-judge missing output: $why" }
    else { Write-Host "OK $n re-judge fails a deleted case folder or a missing result.txt as no archived output and refuses a folder without run-params.txt with exit 2" }
    $n++
    $why = $null
    $d = & $rjRun 'old-result' 'Dedicated' 'none' @('dedicated') $null
    $rf = Join-Path $d 'smoke\dedicated\result.txt'
    [IO.File]::WriteAllText($rf, ([IO.File]::ReadAllText($rf) -replace 'stall=0\r\nsilent_max_s=0\.0\r\nturns=1500\r\nsteps=1\r\n', ''))
    $c = & $rjJudge $d
    $d2 = & $rjRun 'stalled' 'Dedicated' 'none' @('dedicated') $null
    $rf = Join-Path $d2 'smoke\dedicated\result.txt'
    [IO.File]::WriteAllText($rf, ([IO.File]::ReadAllText($rf).Replace('stall=0', 'stall=1')))
    $c2 = & $rjJudge $d2
    if ($c.Code -ne 1 -or $c.Text -notmatch '(?m)^\[FAIL\] smoke:dedicated - no archived output: result.txt lacks stall\r?$') { $why = "result.txt without the stall fields gave exit $($c.Code): $($c.Out)" }
    elseif ($c2.Code -ne 1 -or $c2.Text -notmatch '(?m)^\[FAIL\] smoke:dedicated class=Pass ') { $why = "a stall=1 archive gave exit $($c2.Code): $($c2.Out)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: re-judge stall fields: $why" }
    else { Write-Host "OK $n re-judge refuses a result.txt without the stall fields and fails a stall=1 case that does not expect a stall" }
    $n++
    $why = $null
    $missions = Join-Path $rjTmp 'missions.txt'
    [IO.File]::WriteAllText($missions, "geo1c_con`r`n")
    $script:SmokeDigestConfiguration = 'Release'
    $oldWer = $script:SmokeWerKey
    $script:SmokeWerKey = $true
    try { $allNames = @(Get-SmokeCases 'All' @('geo1c_con') $false $null $false | ForEach-Object { $_.Name }) }
    finally { $script:SmokeWerKey = $oldWer }
    $d = & $rjRun 'added' 'All' $missions $allNames $null
    $e1 = & $rjJudge $d
    [IO.File]::WriteAllText($missions, "geo1c_con`r`nnab2g_c`r`n")
    $e2 = & $rjJudge $d
    $miss1 = @([regex]::Matches($e1.Text, '(?m)^\[FAIL\] smoke:(\S+) - no archived output') | ForEach-Object { $_.Groups[1].Value })
    $miss2 = @([regex]::Matches($e2.Text, '(?m)^\[FAIL\] smoke:(\S+) - no archived output') | ForEach-Object { $_.Groups[1].Value })
    if ($allNames.Count -lt 3 -or $allNames -notcontains 'geo1c_con') { $why = "the All case list has $($allNames.Count) cases" }
    elseif ($miss1.Count -ne 0) { $why = "the full archive reported missing cases: $($miss1 -join ', ')" }
    elseif ($e2.Code -ne 1 -or ($miss2 -join ',') -cne 'nab2g_c') { $why = "the grown case list gave exit $($e2.Code), missing '$($miss2 -join ',')'" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: re-judge added case: $why" }
    else { Write-Host "OK $n re-judge names the one case a grown missions file added as no archived output and exits 1" }
    $n++
    $why = $null
    $rpExe = Join-Path $rjTmp 'BattlefrontII.exe'
    [IO.File]::WriteAllBytes($rpExe, (& $pe $false))
    $rpSha = Get-PeNormalisedSha $rpExe
    $rpOther = & $pe $false
    $rpOther[0x300] = 0x91
    $rpOtherSha = Get-KeyBytesSha (Get-PeNormalisedBytes $rpOther)
    $rpBase = Join-Path $rjTmp 'repin-base'
    New-Item -ItemType Directory -Path $rpBase | Out-Null
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'baseline\v143\mem-peaks.txt') -Destination $rpBase
    $rpCauses = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'baseline\v143\CAUSES.md'))
    [IO.File]::WriteAllText((Join-Path $rpBase 'CAUSES.md'), $rpCauses)
    $mp = [IO.File]::ReadAllLines((Join-Path $rpBase 'mem-peaks.txt'))
    $rpRow = -1
    for ($i = 0; $i -lt $mp.Count; $i++) { if ($mp[$i] -cmatch "^cor1c_c`tturn1500`theap`tApplication`t") { $rpRow = $i; break } }
    $rpKey = 'cor1c_c/turn1500/heap/Application'
    $mpEdit = [string[]]$mp.Clone()
    $f = $mpEdit[$rpRow] -split "`t"
    $f[6] = [string]([long]$f[6] + 1)
    $mpEdit[$rpRow] = $f -join "`t"
    $mpRun = [string[]]$mpEdit.Clone()
    $f = $mpRun[$rpRow + 1] -split "`t"
    $f[6] = [string]([long]$f[6] + 1)
    $mpRun[$rpRow + 1] = $f -join "`t"
    $rpMake = {
        param([string]$Name, [string]$Sha)
        $rd = Join-Path $rjTmp $Name
        New-Item -ItemType Directory -Path (Join-Path $rd 'tables'), (Join-Path $rd 'smoke\dedicated') -Force | Out-Null
        @("platform=$Platform", 'smoke_configuration=Release', "exe.BattlefrontII.exe=$Sha") | Set-Content -Path (Join-Path $rd 'run-params.txt') -Encoding ASCII
        [IO.File]::WriteAllText((Join-Path $rd 'tables\mem-peaks.txt'), ($mpRun -join "`r`n") + "`r`n")
        [IO.File]::WriteAllText((Join-Path $rd 'smoke\dedicated\run.log'), "$rjDigest`r`n")
        return $rd
    }
    $rpScript = Join-Path $PSScriptRoot 'baseline\repin.ps1'
    $rpCall = {
        param([string]$Rd, [string]$Table, [string]$Key, [string]$Expected)
        $a = @('-RunDir', $Rd, '-Table', $Table, '-Key', $Key, '-BaselineDir', $rpBase, '-Exe', $rpExe)
        if ($Expected) { $a += @('-ExpectedFile', $Expected) }
        $o = & { $ErrorActionPreference = 'Continue'; & powershell -NoProfile -ExecutionPolicy Bypass -File $rpScript @a 2>&1 | Out-String }
        return New-Object PSObject -Property @{ Code = $LASTEXITCODE; Out = $o }
    }
    $mpPath = Join-Path $rpBase 'mem-peaks.txt'
    $mpBytes = [Convert]::ToBase64String([IO.File]::ReadAllBytes($mpPath))
    $good = & $rpMake 'repin-good' $rpSha
    $rpBadDir = & $rpMake 'repin-bad' $rpOtherSha
    $r1 = & $rpCall $good 'mem-peaks' $rpKey $null
    $after1 = [Convert]::ToBase64String([IO.File]::ReadAllBytes($mpPath))
    [IO.File]::AppendAllText((Join-Path $rpBase 'CAUSES.md'), "`r`n### C-FIXTURE`r`n`r`n- row: mem-peaks $rpKey`r`n- row: digest-expected dedicated`r`n")
    $r2 = & $rpCall $rpBadDir 'mem-peaks' $rpKey $null
    $after2 = [Convert]::ToBase64String([IO.File]::ReadAllBytes($mpPath))
    $r3 = & $rpCall $good 'mem-peaks' $rpKey $null
    $after3 = [IO.File]::ReadAllText($mpPath)
    $rpExp = Join-Path $rjTmp 'repin-expected.txt'
    [IO.File]::WriteAllText($rpExp, "geo1c_con`tOPENSWBF2 SIM DIGEST old`r`ndedicated`tOPENSWBF2 SIM DIGEST old`r`n")
    $r4 = & $rpCall $good 'digest-expected' 'dedicated' $rpExp
    $after4 = [IO.File]::ReadAllText($rpExp)
    if ($mp[$rpRow + 1] -cmatch "^cor1c_c`tturn1500`theap`tApplication`t") { $why = 'the fixture rows share a key' }
    elseif ($r1.Code -ne 1 -or $r1.Out -notmatch [regex]::Escape("row: mem-peaks $rpKey") -or $after1 -cne $mpBytes) { $why = "no-cause call gave exit $($r1.Code), file changed: $($after1 -cne $mpBytes): $($r1.Out)" }
    elseif ($r2.Code -ne 1 -or $r2.Out -notmatch 'is not the current exe' -or $after2 -cne $mpBytes) { $why = "exe-mismatch call gave exit $($r2.Code), file changed: $($after2 -cne $mpBytes): $($r2.Out)" }
    elseif ($r3.Code -ne 0 -or $after3 -cne (($mpEdit -join "`r`n") + "`r`n")) { $why = "copy call gave exit $($r3.Code) or changed more than the row: $($r3.Out)" }
    elseif ($r4.Code -ne 0 -or $after4 -cne "geo1c_con`tOPENSWBF2 SIM DIGEST old`r`ndedicated`t$rjDigest`r`n") { $why = "digest-expected call gave exit $($r4.Code): $($r4.Out)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: repin: $why" }
    else { Write-Host "OK $n repin refuses a row with no CAUSES.md entry and a run from another exe, leaving the file unchanged, and copies only the named row with CRLF kept" }
    $n++
}
catch { $failed++; Write-Host "FAILED: re-judge and repin block threw: $_" }
finally {
    $script:SmokeDigestConfiguration = $rjDigestCfg
    Remove-Item -LiteralPath $rjTmp -Recurse -Force -ErrorAction SilentlyContinue
}
$gtTmp = Join-Path $env:TEMP ('openswbf2-gate-' + [guid]::NewGuid().ToString('N'))
$gtRepo = Join-Path $gtTmp 'repo'
$gtEv = Join-Path $gtTmp 'evidence'
$gtMarks = Join-Path $gtTmp 'marks'
$gtSteps = Join-Path $gtTmp 'steps.ps1'
$gtGate = Join-Path $gtRepo 'tests\gate.ps1'
$gtRecFile = Join-Path $gtRepo 'tests\guardrevert\steps.tsv'
try {
    New-Item -ItemType Directory -Force -Path (Join-Path $gtRepo 'tests\gate'), (Join-Path $gtRepo 'tests\smoke'), (Join-Path $gtRepo 'src'), $gtMarks | Out-Null
    foreach ($f in 'gate.ps1', 'gate\keys.ps1', 'smoke.ps1', 'common.ps1', 'smoke\cases.ps1', 'vstools.ps1', 'guard-revert.ps1') { Copy-Item -LiteralPath (Join-Path $PSScriptRoot $f) -Destination (Join-Path $gtRepo "tests\$f") }
    $gtVdf = { param([string]$apps) "`"UserLocalConfigStore`"`n{`n`t`"system`"`n`t{`n`t`t`"EnableGameOverlay`"`t`t`"1`"`n`t}`n`t`"apps`"`n`t{`n`t`t`"319630`"`n`t`t{`n`t`t`t`"OverlayAppEnable`"`t`t`"0`"`n`t`t}`n$apps`t}`n}`n" }
    $gtVdfOff = Join-Path $gtTmp 'vdf-off.vdf'
    $gtVdfOn = Join-Path $gtTmp 'vdf-on.vdf'
    $gtVdfAbsent = Join-Path $gtTmp 'vdf-absent.vdf'
    [IO.File]::WriteAllText($gtVdfOff, (& $gtVdf "`t`t`"6060`"`n`t`t{`n`t`t`t`"OverlayAppEnable`"`t`t`"0`"`n`t`t}`n"))
    [IO.File]::WriteAllText($gtVdfOn, (& $gtVdf "`t`t`"6060`"`n`t`t{`n`t`t`t`"OverlayAppEnable`"`t`t`"1`"`n`t`t}`n"))
    [IO.File]::WriteAllText($gtVdfAbsent, (& $gtVdf ''))
    $env:OPENSWBF2_SELFTEST_GATE_VDF = $gtVdfOff
    [IO.File]::WriteAllText((Join-Path $gtRepo '.gitignore'), "artifacts/`n")
    [IO.File]::WriteAllText((Join-Path $gtRepo 'tests\smoke\digest-expected.txt'), "0xA`r`n")
    [IO.File]::WriteAllText((Join-Path $gtRepo 'tests\smoke\missions.txt'), "geo1c_con`r`n")
    [IO.File]::WriteAllText((Join-Path $gtRepo 'src\a.cpp'), "int a;`r`n")
    Set-Content -LiteralPath (Join-Path $gtRepo 'tests\stub.ps1') -Encoding ASCII -Value @(
        'param([string]$Name, [string]$Kind, [string]$Out, [switch]$ListSelected)',
        'if ($ListSelected) { Write-Output ''SELECT SELFTEST-01''; exit 0 }',
        '[IO.File]::WriteAllText((Join-Path $env:OPENSWBF2_SELFTEST_GATE_MARKS $Name), ''x'')',
        'if ($env:OPENSWBF2_SELFTEST_GATE_KILL -ceq $Name) {',
        '    Stop-Process -Id (Get-CimInstance Win32_Process -Filter "ProcessId=$PID").ParentProcessId -Force',
        '    exit 9',
        '}',
        'if ($Kind -eq ''run'') {',
        '    $d = Join-Path (Split-Path $PSScriptRoot -Parent) (''artifacts\tests\'' + $Name + ''-'' + [guid]::NewGuid().ToString(''N''))',
        '    New-Item -ItemType Directory -Force -Path $d | Out-Null',
        '    [IO.File]::WriteAllText((Join-Path $d ''run-params.txt''), "platform=Win32`r`n")',
        '    [IO.File]::WriteAllText((Join-Path $d ''run.log''), "stub $Name`r`n")',
        '    [IO.File]::WriteAllText((Join-Path $d ''result.txt''), "0xA`r`n")',
        '}',
        'if ($Out) { New-Item -ItemType Directory -Force -Path $Out | Out-Null; [IO.File]::WriteAllText((Join-Path $Out ''summary.txt''), "stub $Name`r`n") }',
        'Write-Output ''OK 1''',
        'exit 0'
    )
    $gtStep = { param([string]$name, [string]$kind, [string]$extra) "    @{ Name = '$name'; Tiers = @('plan', 'milestone'); Kind = '$kind'; Config = 'Debug'; Exe = 'powershell'; Args = `$stub + @('-Name', '$name', '-Kind', '$kind'$extra)" }
    Set-Content -LiteralPath $gtSteps -Encoding ASCII -Value @(
        'function Get-ToolchainInfo { New-Object PSObject -Property @{ Cl = ''stub-cl''; ToolsVersion = ''stub-tools''; Ucrt = ''stub-ucrt'' } }',
        'function Get-SteamRunning { ''yes'' }',
        'function Get-SteamOverlayState { ''off'' }',
        'function Find-GameDataRoot { $null }',
        'function Get-GameDataManifestHash { ''stub-gamedata'' }',
        'function Get-GatePreflightInput($Steps, [string[]]$Platforms) {',
        '    $p = @(Get-FirewallRulePaths $root @(''Win32'') @(''Debug''))',
        '    if ($env:OPENSWBF2_SELFTEST_GATE_PFOUT) { [IO.File]::WriteAllText($env:OPENSWBF2_SELFTEST_GATE_PFOUT, (@(Get-GateStepConfigs @($Steps | Where-Object { $_.Kind -ne ''guard'' })) -join '','') + "`t" + (@(Get-GateStepConfigs $Steps) -join '','') + "`t" + (Get-GuardWorkerCount) + "`t" + (@($Platforms) -join '','')) }',
        '    $in = @{ Steam = ''yes''; Overlay = Get-SteamOverlayFromVdf $env:OPENSWBF2_SELFTEST_GATE_VDF; RulePaths = $p; GamePlatforms = @(''Win32''); Platforms = @($Platforms); AllowRules = $p; Wer = @(); FreeRamKB = $script:RamFloorKB; RepoDrive = ''C:''; RepoFree = [long]$script:DiskDropBytes; WorktreeDrive = ''D:''; WorktreeFree = 0; WorktreesMissing = 0; GameData = ''G:\GameData''; Toolchain = @(@{ Platform = ''Win32''; MSBuild = $true; Dumpbin = $true; RuntimeDir = ''S:\SysWOW64''; MissingRuntimes = @() }) }',
        '    switch ($env:OPENSWBF2_SELFTEST_GATE_PF) {',
        '        ''steam'' { $in.Steam = ''no'' }',
        '        ''firewall'' { $in.AllowRules = @($p | Select-Object -Skip 1) }',
        '        ''firewall-empty'' { $in.RulePaths = @() }',
        '        ''firewall-nogame'' { $in.GamePlatforms = @(); $in.RulePaths = @(); $in.AllowRules = @() }',
        '        ''wer'' { $in.Wer = @(''WER LocalDumps key for BattlefrontII.exe is missing'') }',
        '        ''ram'' { $in.FreeRamKB = $script:RamFloorKB - 1 }',
        '        ''disk'' { $in.RepoFree = [long]$script:DiskDropBytes - 1 }',
        '        ''disk-worktree'' { $in.WorktreeDrive = ''C:''; $in.WorktreesMissing = 2; $in.RepoFree = [long]$script:DiskDropBytes + [long]$script:WorktreeBytes }',
        '        ''disk-worktree-ok'' { $in.WorktreeDrive = ''C:''; $in.WorktreesMissing = 2; $in.RepoFree = [long]$script:DiskDropBytes + 2 * [long]$script:WorktreeBytes }',
        '        ''disk-other-drive'' { $in.WorktreesMissing = 1; $in.WorktreeFree = [long]$script:WorktreeBytes - 1 }',
        '        ''disk-gamedata'' { $in.GameData = $null }',
        '        ''toolchain'' { $in.Toolchain[0].MissingRuntimes = @(''msvcr120.dll'') }',
        '        ''toolchain-dumpbin'' { $in.Toolchain[0].Dumpbin = $false }',
        '        ''toolchain-msbuild'' { $in.Toolchain[0].MSBuild = $false }',
        '    }',
        '    return $in',
        '}',
        'function Invoke-GateRejudge([string]$Dir) {',
        '    $got = ([IO.File]::ReadAllText((Join-Path $Dir ''result.txt''))).Trim()',
        '    $want = @([IO.File]::ReadAllLines((Join-Path $root ''tests\smoke\digest-expected.txt'')) | ForEach-Object { $_.Trim() })',
        '    if ($want -ccontains $got) { return 0 }',
        '    return 1',
        '}',
        '$stub = @(''-NoProfile'', ''-ExecutionPolicy'', ''Bypass'', ''-File'', ''tests\stub.ps1'')',
        '$GateSteps = @(',
        ((& $gtStep 'build' 'build' '') + ' },'),
        ((& $gtStep 'run-a' 'run' '') + "; Needs = @('build'); JudgeKinds = @('smoke') },"),
        ((& $gtStep 'run-b' 'run' '') + "; Needs = @('build'); Job = 'jb' },"),
        ((& $gtStep 'st' 'selftest' '') + ' },'),
        ((& $gtStep 'gd' 'guard' ", '-Out', 'artifacts\gate\`$Stamp\gd'") + "; Outputs = @('artifacts\gate\`$Stamp\gd') },"),
        ((& $gtStep 'cs' 'census' ", '-Out', 'artifacts\gate\`$Stamp\cs'") + "; Outputs = @('artifacts\gate\`$Stamp\cs') }"),
        ')'
    )
    $gtGit = {
        param([string[]]$a)
        $ErrorActionPreference = 'Continue'
        $o = & git -C $gtRepo -c user.name=selftest -c user.email=selftest@invalid -c core.autocrlf=false @a 2>&1
        if ($LASTEXITCODE -ne 0) { throw "git $($a -join ' '): $o" }
    }
    $gtRun = {
        param([string]$extra, [string]$kill, [string]$tier = 'plan')
        Get-ChildItem -LiteralPath $gtMarks -File | Remove-Item -Force
        $env:OPENSWBF2_SELFTEST_GATE_MARKS = $gtMarks
        $env:OPENSWBF2_SELFTEST_GATE_KILL = $kill
        try {
            $o = @(& powershell -NoProfile -ExecutionPolicy Bypass -Command "& '$gtGate' -Tier $tier -StepsFile '$gtSteps' -EvidenceDir '$gtEv' $extra; exit `$LASTEXITCODE" 2>&1 | ForEach-Object { "$_" })
            $c = $LASTEXITCODE
        }
        finally {
            Remove-Item Env:\OPENSWBF2_SELFTEST_GATE_KILL -ErrorAction SilentlyContinue
            Remove-Item Env:\OPENSWBF2_SELFTEST_GATE_MARKS -ErrorAction SilentlyContinue
        }
        $m = @(Get-ChildItem -LiteralPath $gtMarks -File | ForEach-Object { $_.Name } | Sort-Object)
        New-Object PSObject -Property @{ Code = $c; Marks = $m; Text = ($o -join "`n") }
    }
    $gtRec = { param([string]$name) (Read-StepRecords $gtRecFile)["$name`tWin32`tDebug`tplan"] }
    $gtSetRec = {
        param([string]$name, [string]$col, [string]$value)
        $lines = [IO.File]::ReadAllLines($gtRecFile)
        $cols = $lines[0].Split("`t")
        $at = [Array]::IndexOf($cols, $col)
        for ($i = 1; $i -lt $lines.Count; $i++) {
            $f = $lines[$i].Split("`t")
            if ($f[0] -ceq $name) { $f[$at] = $value; $lines[$i] = $f -join "`t" }
        }
        [IO.File]::WriteAllText($gtRecFile, ($lines -join "`r`n") + "`r`n")
    }
    $gtRestore = { & $gtGit @('checkout', '--', '.') }
    $gtCarried = { param($r, [string]$name) $r.Text -match "(?m)^\[CARRY\] gate:$name " }
    & $gtGit @('init', '-q')
    & $gtGit @('add', '-A')
    & $gtGit @('commit', '-q', '-m', 'selftest')

    $r0 = & $gtRun '' ''
    $r1 = & $gtRun '' ''
    $why = $null
    if ($r0.Code -ne 0 -or ($r0.Marks -join ',') -cne 'build,cs,gd,run-a,run-b,st' -or $r0.Text -notmatch 'gate: tier plan passed') { $why = "first run exit $($r0.Code), marks $($r0.Marks -join ','): $($r0.Text)" }
    elseif (-not (& $gtRec 'run-a') -or -not (& $gtRec 'run-b')) { $why = 'the clean first run wrote no run step record' }
    elseif ($r1.Code -ne 0) { $why = "second run exit $($r1.Code): $($r1.Text)" }
    elseif (-not (& $gtCarried $r1 'run-a') -or -not (& $gtCarried $r1 'run-b')) { $why = "run steps not carried: $($r1.Text)" }
    elseif ($r1.Marks -contains 'run-a' -or $r1.Marks -contains 'run-b') { $why = "a carried run stub ran: $($r1.Marks -join ',')" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate carries unchanged run steps: $why" }
    else { Write-Host "OK $n gate carries unchanged run steps without starting them" }
    $n++

    $why = $null
    $stRec = & $gtRec 'st'
    if (-not $stRec -or -not (& $gtCarried $r1 'st')) { $why = "selftest-Kind step not carried: $($r1.Text)" }
    elseif ($r1.Marks -contains 'st') { $why = 'the carried selftest stub ran' }
    elseif (-not (Test-Path -LiteralPath (Join-Path $gtEv "$($stRec.Evidence)\stdout.txt")) -or -not (Test-Path -LiteralPath (Join-Path $gtEv "$($stRec.Evidence)\exit.txt"))) { $why = 'selftest evidence lacks stdout.txt or exit.txt' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate carries an unchanged selftest-Kind step: $why" }
    else { Write-Host "OK $n gate carries an unchanged selftest-Kind step on its archived stdout and exit code" }
    $n++

    $why = $null
    $gdRec = & $gtRec 'gd'
    if (-not $gdRec -or -not (& $gtCarried $r1 'gd')) { $why = "guard-Kind step not carried: $($r1.Text)" }
    elseif ($r1.Marks -contains 'gd') { $why = 'the carried guard stub ran' }
    elseif (-not (Test-Path -LiteralPath (Join-Path $gtEv "$($gdRec.Evidence)\out\0\summary.txt"))) { $why = 'guard evidence lacks out\0' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate carries an unchanged guard-Kind step: $why" }
    else { Write-Host "OK $n gate carries an unchanged guard-Kind step with its Outputs folder archived as out\0" }
    $n++

    $exp = Join-Path $gtRepo 'tests\smoke\digest-expected.txt'
    [IO.File]::WriteAllText($exp, "0xB`r`n0xA`r`n")
    $ra = & $gtRun '' ''
    [IO.File]::WriteAllText($exp, "0xB`r`n")
    $rb = & $gtRun '' ''
    & $gtRestore
    $why = $null
    if ($ra.Code -ne 0 -or $ra.Text -notmatch '(?m)^\[REJUDGE\] gate:run-a ') { $why = "accepting expectation: exit $($ra.Code), no REJUDGE line: $($ra.Text)" }
    elseif ($ra.Marks -contains 'run-a') { $why = 'the re-judged run stub ran' }
    elseif ($rb.Code -ne 1 -or $rb.Text -notmatch '(?m)^\[FAIL\] gate:run-a Win32 re-judge') { $why = "rejecting expectation: exit $($rb.Code), no re-judge FAIL line: $($rb.Text)" }
    elseif ($rb.Marks -contains 'run-a') { $why = 'the run stub ran on the rejecting re-judge' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate re-judges a judge-only change: $why" }
    else { Write-Host "OK $n gate re-judges a judge-only change without starting the step, and the verdict follows the new expectation" }
    $n++

    Add-Content -LiteralPath (Join-Path $gtRepo 'tests\smoke\missions.txt') -Value 'nab2g_c'
    $r = & $gtRun '' ''
    & $gtRestore
    $why = $null
    if ($r.Code -ne 0 -or $r.Marks -notcontains 'run-a' -or (& $gtCarried $r 'run-a') -or $r.Text -match '(?m)^\[REJUDGE\] gate:run-a ') { $why = "exit $($r.Code), marks $($r.Marks -join ','): $($r.Text)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate runs a step after a run-input change: $why" }
    else { Write-Host "OK $n gate runs a step after a missions file change" }
    $n++

    Add-Content -LiteralPath (Join-Path $gtRepo 'tests\smoke.ps1') -Value ''
    $r = & $gtRun '' ''
    & $gtRestore
    $why = $null
    if ($r.Code -ne 0 -or $r.Marks -notcontains 'run-a' -or (& $gtCarried $r 'run-a') -or $r.Text -match '(?m)^\[REJUDGE\] gate:run-a ') { $why = "exit $($r.Code), marks $($r.Marks -join ','): $($r.Text)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate runs a step after a script change: $why" }
    else { Write-Host "OK $n gate runs a step after a script hash change" }
    $n++

    $ev = (& $gtRec 'run-a').Evidence
    Remove-Item -LiteralPath (Join-Path $gtEv $ev) -Recurse -Force
    $r = & $gtRun '' ''
    $why = $null
    if ($r.Code -ne 0 -or $r.Marks -notcontains 'run-a' -or (& $gtCarried $r 'run-a') -or $r.Text -notmatch "evidence $ev is missing") { $why = "exit $($r.Code), marks $($r.Marks -join ','): $($r.Text)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate runs a step whose evidence is gone: $why" }
    else { Write-Host "OK $n gate runs a step whose evidence folder was deleted" }
    $n++

    $forged = 'f' * 64
    & $gtSetRec 'st' 'Evidence' $forged
    $r = & $gtRun '' ''
    $why = $null
    if ($r.Code -ne 0 -or $r.Marks -notcontains 'st' -or (& $gtCarried $r 'st') -or $r.Text -notmatch "evidence $forged is missing") { $why = "exit $($r.Code), marks $($r.Marks -join ','): $($r.Text)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate refuses a forged selftest record: $why" }
    else { Write-Host "OK $n gate runs a selftest-Kind step whose record names evidence that does not exist" }
    $n++

    $ev = (& $gtRec 'st').Evidence
    Add-Content -LiteralPath (Join-Path $gtEv "$ev\stdout.txt") -Value 'edited'
    $r = & $gtRun '' ''
    $why = $null
    if ($r.Code -ne 0 -or $r.Marks -notcontains 'st' -or (& $gtCarried $r 'st') -or $r.Text -notmatch "evidence $ev no longer hashes to its name") { $why = "exit $($r.Code), marks $($r.Marks -join ','): $($r.Text)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate refuses edited evidence: $why" }
    else { Write-Host "OK $n gate runs a selftest-Kind step whose evidence no longer matches its hash" }
    $n++

    $fake = Join-Path $gtTmp 'fake'
    New-Item -ItemType Directory -Force -Path $fake | Out-Null
    [IO.File]::WriteAllText((Join-Path $fake 'exit.txt'), "0`r`n")
    [IO.File]::WriteAllText((Join-Path $fake 'stdout.txt'), "OK 1`r`nFAILED 1: forged`r`n")
    $fh = Get-EvidenceHash $fake
    Copy-Item -LiteralPath $fake -Destination (Join-Path $gtEv $fh) -Recurse
    & $gtSetRec 'st' 'Evidence' $fh
    $r = & $gtRun '' ''
    $why = $null
    if ((Get-EvidenceHash (Join-Path $gtEv $fh)) -cne $fh) { $why = 'the forged evidence does not hash to its name, so the case proves nothing' }
    elseif ($r.Code -ne 0 -or $r.Marks -notcontains 'st' -or (& $gtCarried $r 'st') -or $r.Text -notmatch 'stdout.txt has a failure line: FAILED') { $why = "exit $($r.Code), marks $($r.Marks -join ','): $($r.Text)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate refuses evidence with a failure line: $why" }
    else { Write-Host "OK $n gate runs a selftest-Kind step whose intact evidence holds exit 0 and a FAILED line" }
    $n++

    & $gtSetRec 'run-a' 'Platform' 'x64'
    $r = & $gtRun '' ''
    $why = $null
    if ($r.Code -ne 0 -or $r.Marks -notcontains 'run-a' -or (& $gtCarried $r 'run-a') -or -not (& $gtCarried $r 'run-b')) { $why = "exit $($r.Code), marks $($r.Marks -join ','): $($r.Text)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate does not carry across platforms: $why" }
    else { Write-Host "OK $n gate does not carry an x64 record for Win32" }
    $n++

    $srcFile = Join-Path $gtRepo 'src\a.cpp'
    [IO.File]::WriteAllText($srcFile, "int b;`r`n")
    $r = & $gtRun '' ''
    & $gtRestore
    $why = $null
    if ($r.Code -ne 0 -or $r.Marks -notcontains 'gd' -or $r.Marks -notcontains 'cs' -or (& $gtCarried $r 'gd') -or (& $gtCarried $r 'cs')) { $why = "exit $($r.Code), marks $($r.Marks -join ','): $($r.Text)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate runs census and guard steps after a src change: $why" }
    else { Write-Host "OK $n gate runs census-Kind and guard-Kind steps after a src change" }
    $n++

    $recBefore = [IO.File]::ReadAllBytes($gtRecFile)
    [IO.File]::WriteAllText((Join-Path $gtRepo 'extra.txt'), "x`r`n")
    $r = & $gtRun '' ''
    Remove-Item -LiteralPath (Join-Path $gtRepo 'extra.txt') -Force
    $recAfter = [IO.File]::ReadAllBytes($gtRecFile)
    $why = $null
    if ($r.Code -ne 0 -or $r.Marks -notcontains 'run-a' -or $r.Text -notmatch 'RECORDS NOT WRITTEN: tree not clean') { $why = "exit $($r.Code), marks $($r.Marks -join ','): $($r.Text)" }
    elseif ([Convert]::ToBase64String($recBefore) -cne [Convert]::ToBase64String($recAfter)) { $why = 'steps.tsv changed' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate writes no record from a dirty tree: $why" }
    else { Write-Host "OK $n gate writes no record when the tree has an untracked change" }
    $n++

    Remove-Item -LiteralPath $gtRecFile -Force
    $r = & $gtRun '-Step run-a' ''
    $why = $null
    if ($r.Code -ne 0 -or ($r.Marks -join ',') -cne 'build,run-a' -or $r.Text -notmatch '(?m)^gate: tier plan partial' -or $r.Text -match 'tier plan passed') { $why = "exit $($r.Code), marks $($r.Marks -join ','): $($r.Text)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate -Step: $why" }
    else { Write-Host "OK $n gate -Step runs the named step and its build only and prints partial" }
    $n++

    $rj = & $gtRun '-Job jb' ''
    $ru = & $gtRun '-Job nope' ''
    $why = $null
    if ($rj.Code -ne 0 -or $rj.Text -notmatch '(?m)^gate: tier plan partial' -or $rj.Text -match 'tier plan passed') { $why = "-Job jb: exit $($rj.Code): $($rj.Text)" }
    elseif ($ru.Code -ne 2 -or $ru.Marks.Count -ne 0 -or $ru.Text -notmatch 'gate: tier plan has no job nope') { $why = "-Job nope: exit $($ru.Code), marks $($ru.Marks -join ','): $($ru.Text)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate -Job: $why" }
    else { Write-Host "OK $n gate -Job prints partial and refuses a job the tier does not have" }
    $n++

    Remove-Item -LiteralPath $gtRecFile -Force
    $rk = & $gtRun '' 'run-a'
    $rr = & $gtRun '-Resume' ''
    $why = $null
    if ($rk.Code -eq 0 -or ($rk.Marks -join ',') -cne 'build,run-a' -or $rk.Text -match 'tier plan passed') { $why = "killed run: exit $($rk.Code), marks $($rk.Marks -join ','): $($rk.Text)" }
    elseif ($rr.Code -ne 0 -or ($rr.Marks -join ',') -cne 'cs,gd,run-a,run-b,st' -or $rr.Text -notmatch '(?m)^\[RESUMED\] gate:build ') { $why = "resume: exit $($rr.Code), marks $($rr.Marks -join ','): $($rr.Text)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate -Resume after a kill: $why" }
    else { Write-Host "OK $n gate -Resume after a kill runs only the unfinished steps" }
    $n++

    $r = & $gtRun '-Platform x64' ''
    $gtProps = Join-Path $gtRepo 'OpenSWBF2.Platform.props'
    [IO.File]::WriteAllText($gtProps, "<ProjectConfiguration Include=`"Release|Win32`" />`r`n")
    $rw = & $gtRun '-Platform x64' ''
    [IO.File]::WriteAllText($gtProps, "<ProjectConfiguration Include=`"Release|x64`" />`r`n")
    $rx = & $gtRun '-Platform x64 -Step run-a' ''
    Remove-Item -LiteralPath $gtProps -Force
    $why = $null
    if ($r.Code -ne 2 -or $r.Text -notmatch 'gate: x64 steps need an x64 configuration' -or $r.Marks.Count) { $why = "no platform props: exit $($r.Code), marks $($r.Marks -join ','): $($r.Text)" }
    elseif ($rw.Code -ne 2 -or $rw.Text -notmatch 'gate: x64 steps need an x64 configuration' -or $rw.Marks.Count) { $why = "Win32-only platform props: exit $($rw.Code), marks $($rw.Marks -join ','): $($rw.Text)" }
    elseif ($rx.Code -ne 0 -or $rx.Text -match 'need an x64 configuration' -or ($rx.Marks -join ',') -cne 'build,run-a') { $why = "with an x64 configuration: exit $($rx.Code), marks $($rx.Marks -join ','): $($rx.Text)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate x64 refusal: $why" }
    else { Write-Host "OK $n gate refuses -Platform x64 before any step when the platform props has no x64 configuration, and runs it when it has one" }
    $n++

    $gtTimes = Join-Path $gtRepo 'tests\baseline\gate-times.tsv'
    New-Item -ItemType Directory -Force -Path (Split-Path $gtTimes -Parent) | Out-Null
    $gtSeed = @("step`ttier`tplatform`tbefore_s`tafter_s`tnote") + @('build', 'run-a', 'run-b', 'st', 'gd', 'cs' | ForEach-Object { "$_`tplan,milestone`tWin32`t5`t-`tseed" })
    [IO.File]::WriteAllText($gtTimes, ($gtSeed -join "`r`n") + "`r`n")
    $gtSeedText = [IO.File]::ReadAllText($gtTimes)
    $rs = & $gtRun '-Step run-a' '' 'milestone'
    $afterStep = [IO.File]::ReadAllText($gtTimes)
    $rp = & $gtRun '' '' 'plan'
    $afterPlan = [IO.File]::ReadAllText($gtTimes)
    $rm = & $gtRun '' '' 'milestone'
    $afterFull = @{}
    foreach ($l in @([IO.File]::ReadAllLines($gtTimes) | Select-Object -Skip 1)) { $f = $l.Split("`t"); $afterFull[$f[0]] = $f[4] }
    $rm2 = & $gtRun '' '' 'milestone'
    $afterCarry = @{}
    foreach ($l in @([IO.File]::ReadAllLines($gtTimes) | Select-Object -Skip 1)) { $f = $l.Split("`t"); $afterCarry[$f[0]] = $f[4] }
    $why = $null
    if ($rs.Code -ne 0 -or $afterStep -cne $gtSeedText) { $why = "a milestone -Step run changed gate-times.tsv (exit $($rs.Code))" }
    elseif ($rp.Code -ne 0 -or $afterPlan -cne $gtSeedText) { $why = "a full plan run changed gate-times.tsv (exit $($rp.Code))" }
    elseif ($rm.Code -ne 0 -or $rm.Text -notmatch 'gate: after_s written' -or $afterFull['run-a'] -cne 'carried' -or @('build', 'run-b', 'st', 'gd', 'cs' | Where-Object { $afterFull[$_] -notmatch '^\d+$' }).Count) { $why = "full milestone run: exit $($rm.Code), after_s $(($afterFull.Keys | Sort-Object | ForEach-Object { "$_=$($afterFull[$_])" }) -join ' '): $($rm.Text)" }
    elseif ($rm2.Code -ne 0 -or $afterCarry['run-a'] -cne 'carried' -or $afterCarry['run-b'] -cne 'carried' -or $afterCarry['build'] -notmatch '^\d+$') { $why = "second full milestone run: exit $($rm2.Code), after_s $(($afterCarry.Keys | Sort-Object | ForEach-Object { "$_=$($afterCarry[$_])" }) -join ' '): $($rm2.Text)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate-times.tsv writer: $why" }
    else { Write-Host "OK $n gate writes after_s only on a full milestone run (seconds for a run step, carried for a carried one) and never on -Step or plan runs" }
    $n++

    $gtPf = {
        param([string]$mode, [string]$vdf = $gtVdfOff)
        $env:OPENSWBF2_SELFTEST_GATE_PF = $mode
        $env:OPENSWBF2_SELFTEST_GATE_VDF = $vdf
        try { & $gtRun '' '' }
        finally {
            Remove-Item Env:\OPENSWBF2_SELFTEST_GATE_PF -ErrorAction SilentlyContinue
            $env:OPENSWBF2_SELFTEST_GATE_VDF = $gtVdfOff
        }
    }
    $gtPfWhy = {
        param($r, [string]$check)
        $pass = @([regex]::Matches($r.Text, '(?m)^\[PASS\] preflight:(\w+)') | ForEach-Object { $_.Groups[1].Value })
        $fail = @([regex]::Matches($r.Text, '(?m)^\[FAIL\] preflight:(\w+)') | ForEach-Object { $_.Groups[1].Value })
        if (-not $check) {
            if ($r.Code -ne 0 -or ($pass -join ',') -cne 'steam,overlay,firewall,wer,ram,disk,toolchain' -or $fail.Count -or $r.Text -notmatch '(?m)^gate: tier plan passed') { return "exit $($r.Code), pass $($pass -join ','), fail $($fail -join ','), marks $($r.Marks -join ','): $($r.Text)" }
            return $null
        }
        $others = @('steam', 'overlay', 'firewall', 'wer', 'ram', 'disk', 'toolchain' | Where-Object { $_ -cne $check })
        if ($r.Code -ne 2 -or ($fail -join ',') -cne $check -or ($pass -join ',') -cne ($others -join ',') -or $r.Text -notmatch "(?m)^gate: preflight failed: $check; no step ran" -or $r.Marks.Count -or $r.Text -match '(?m)^gate: tier ') { return "exit $($r.Code), pass $($pass -join ','), fail $($fail -join ','), marks $($r.Marks -join ','): $($r.Text)" }
        return $null
    }

    $why = & $gtPfWhy (& $gtPf '') ''
    if (-not $why) { $why = & $gtPfWhy (& $gtPf 'disk-worktree-ok') '' }
    if (-not $why) {
        $rng = & $gtPf 'firewall-nogame'
        $why = & $gtPfWhy $rng ''
        if (-not $why -and $rng.Text -notmatch '(?m)^\[PASS\] preflight:firewall no selected step starts the game exe on Win32, so no rule is needed') { $why = "no-game firewall line missing: $($rng.Text)" }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate preflight all pass: $why" }
    else { Write-Host "OK $n gate preflight prints seven PASS lines at the exact floors and runs the steps, also with missing worktrees that fit and with no rule paths when no selected step starts the game exe" }
    $n++

    $gtPfCases = @(
        @('steam', @(, @('steam', $gtVdfOff)), 'Steam not running'),
        @('overlay', @(@('', $gtVdfOn), @('', $gtVdfAbsent), @('', (Join-Path $gtTmp 'no-such.vdf'))), 'overlay on, absent (Steam default on) or unreadable'),
        @('firewall', @(@('firewall', $gtVdfOff), @('firewall-empty', $gtVdfOff)), 'one worker path without an inbound allow rule, or no rule paths while a step starts the game exe'),
        @('wer', @(, @('wer', $gtVdfOff)), 'WER LocalDumps key missing'),
        @('ram', @(, @('ram', $gtVdfOff)), 'free memory one KB below the floor'),
        @('disk', @(@('disk', $gtVdfOff), @('disk-worktree', $gtVdfOff), @('disk-other-drive', $gtVdfOff), @('disk-gamedata', $gtVdfOff)), 'repo drive one byte short, missing worktrees on the same or another drive short, GameData missing'),
        @('toolchain', @(@('toolchain', $gtVdfOff), @('toolchain-dumpbin', $gtVdfOff), @('toolchain-msbuild', $gtVdfOff)), 'a runtime, dumpbin or MSBuild missing')
    )
    foreach ($pc in $gtPfCases) {
        $why = $null
        foreach ($v in $pc[1]) {
            $why = & $gtPfWhy (& $gtPf $v[0] $v[1]) $pc[0]
            if ($why) { $why = "variant $($v[0]) $([IO.Path]::GetFileName($v[1])): $why"; break }
        }
        if ($why) { $failed++; Write-Host "FAILED ${n}: gate preflight:$($pc[0]) alone: $why" }
        else { Write-Host "OK $n gate preflight:$($pc[0]) failing alone ($($pc[2])) exits 2 naming it before any step" }
        $n++
    }

    $gtPfOut = Join-Path $gtTmp 'pf-inputs.txt'
    $env:OPENSWBF2_SELFTEST_GATE_PFOUT = $gtPfOut
    try { $r = & $gtRun '' '' }
    finally { Remove-Item Env:\OPENSWBF2_SELFTEST_GATE_PFOUT -ErrorAction SilentlyContinue }
    $wantWorkers = [Math]::Max(1, [int][Math]::Floor([Environment]::ProcessorCount / 4))
    $gotPf = $(if (Test-Path -LiteralPath $gtPfOut) { [IO.File]::ReadAllText($gtPfOut) } else { '' })
    $wantPf = "Debug`tDebug,Release,Test,Asan`t$wantWorkers`tWin32"
    $fwCount = @(Get-FirewallRulePaths $gtRepo @('Win32') @('Debug')).Count
    $fwAll = @(Get-FirewallRulePaths $gtRepo).Count
    $why = $null
    if ($r.Code -ne 0 -or $gotPf -cne $wantPf) { $why = "exit $($r.Code), inputs '$gotPf', want '$wantPf'" }
    elseif ($fwCount -ne 1 + 5 * $script:WorkerIndexMax -or $fwAll -ne 8 * $fwCount) { $why = "rule paths $fwCount for one pair and $fwAll for all, want $(1 + 5 * $script:WorkerIndexMax) and $(8 * (1 + 5 * $script:WorkerIndexMax))" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate preflight inputs: $why" }
    else { Write-Host "OK $n gate preflight takes the step configurations (all four with a guard step), guard-revert's default worker count and the bin plus worker rule paths" }
    $n++

    $gtOrderSeed = @("step`ttier`tplatform`tbefore_s`tafter_s`tnote", "build`tplan`tWin32`t999`t-`tseed", "run-a`tplan`tWin32`t50`t-`tseed", "run-b`tplan`tWin32`t10`t-`tseed", "st`tplan`tWin32`t30`t-`tseed", "cs`tplan`tWin32`t100`t5`tseed", "gd`tplan`tWin32`t-`t-`tseed", "run-a`tplan`tx64`t1`t-`tseed")
    [IO.File]::WriteAllText($gtTimes, ($gtOrderSeed -join "`r`n") + "`r`n")
    $r = & $gtRun '' ''
    $lo = @(& powershell -NoProfile -ExecutionPolicy Bypass -Command "& '$gtGate' -Tier plan -StepsFile '$gtSteps' -EvidenceDir '$gtEv' -ListOrder; exit `$LASTEXITCODE" 2>&1 | ForEach-Object { "$_" })
    $loCode = $LASTEXITCODE
    Remove-Item -LiteralPath $gtTimes -Force
    $wantOrder = 'build, gd, cs, run-b, st, run-a'
    $ran = @([regex]::Matches($r.Text, '(?m)^\[(?:PASS|CARRY|REJUDGE)\] gate:(\S+) ') | ForEach-Object { $_.Groups[1].Value }) -join ', '
    $why = $null
    if ($r.Code -ne 0 -or $r.Text -notmatch "(?m)^gate: Win32 order $([regex]::Escape($wantOrder))\r?$" -or $ran -cne $wantOrder) { $why = "exit $($r.Code), steps ran as '$ran', want '$wantOrder': $($r.Text)" }
    elseif ($loCode -ne 0 -or ($lo -join "`n") -notmatch "(?m)^gate: plan Win32 order $([regex]::Escape($wantOrder))$" -or ($lo -join "`n") -match 'preflight') { $why = "-ListOrder exit ${loCode}: $($lo -join ' | ')" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: gate step order: $why" }
    else { Write-Host "OK $n gate runs builds first, then steps with no time in list order, then by after_s or before_s ascending, and -ListOrder prints that order without the preflight" }
    $n++
}
catch { $failed++; Write-Host "FAILED: gate driver block threw: $_" }
finally {
    Remove-Item Env:\OPENSWBF2_SELFTEST_GATE_VDF -ErrorAction SilentlyContinue
    Remove-Item Env:\OPENSWBF2_SELFTEST_GATE_PF -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $gtTmp -Recurse -Force -ErrorAction SilentlyContinue
}
if ($failed -gt 0) { exit 1 }
exit 0
