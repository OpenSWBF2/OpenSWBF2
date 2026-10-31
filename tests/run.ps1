param(
    [ValidateSet('Debug','Release','Test','Asan')][string]$Configuration = 'Debug',
    [switch]$NoBuild,
    [string]$Suite,
    [string]$Filter,
    [switch]$Fuzz,
    [int]$Seed = 1,
    [int]$Iterations = 100000,
    [int]$Timeout = 10,
    [ValidateSet('None','Standard','Dedicated','Crash','All','Twice')][string]$Smoke = 'Standard',
    [int]$SmokeTimeout = 120,
    [ValidateSet('Debug','Release','Test','Asan')][string]$SmokeConfiguration = 'Release',
    [switch]$Movies,
    [switch]$Hardening,
    [switch]$HogAddrSpace,
    [ValidateRange(0,64)][int]$Jobs = 0,
    [switch]$Asan,
    [switch]$Clean
)
$ErrorActionPreference = 'Stop'
if ($Jobs -eq 0) { $Jobs = [Math]::Max(1, [Math]::Min(8, [int][Math]::Floor([Environment]::ProcessorCount / 2))) }
$testsDir = $PSScriptRoot
$root = Split-Path $testsDir -Parent
. (Join-Path $testsDir 'smoke.ps1')
Write-ToolBanner
if ($Asan) {
    if ($PSBoundParameters.ContainsKey('Configuration') -and $Configuration -ne 'Asan') { Write-Host "-Asan conflicts with -Configuration $Configuration."; exit 2 }
    $Configuration = 'Asan'
}
if ($Configuration -in 'Test', 'Asan' -and -not $PSBoundParameters.ContainsKey('SmokeConfiguration')) { $SmokeConfiguration = $Configuration }
$script:SmokeDigestConfiguration = $SmokeConfiguration
$asanT1 = ($Configuration -eq 'Asan')
$asanSmoke = ($SmokeConfiguration -eq 'Asan')
if ($Clean) {
    foreach ($d in (Join-Path $root "bin\Win32\$SmokeConfiguration"), (Join-Path $root "bin\Tests\$Configuration")) {
        if (Test-Path -LiteralPath $d) {
            Remove-StagedOutput $d
            Write-Host "Removed $d"
        }
    }
    exit 0
}

$hardenOnly = $Hardening -and -not $PSBoundParameters.ContainsKey('Smoke') -and -not ($Suite -or $Filter -or $Fuzz)
$smokeActive = ($Smoke -ne 'None') -and -not ($Suite -or $Filter -or $Fuzz) -and -not $hardenOnly
$runUnit = ($Suite -or $Filter -or $Fuzz -or -not $PSBoundParameters.ContainsKey('Smoke') -or $Smoke -eq 'None') -and -not $hardenOnly
$hardenActive = $Hardening -or $smokeActive

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$outDir = Join-Path $root "artifacts\tests\$stamp"
$results = @()
$bad = 0
$asanRemoved = @()

if ($runUnit) {
    Assert-Runtimes $T1Runtimes

    if (-not $NoBuild) {
        $msbuild = Find-MSBuild
        & $msbuild (Join-Path $testsDir 'OpenSWBF2Tests.vcxproj') "/p:Configuration=$Configuration" '/p:Platform=Win32' /m /v:minimal /nologo
        if ($LASTEXITCODE -ne 0) { Write-Host 'Build failed.'; exit 2 }
    }

    $exe = $env:OPENSWBF2_TEST_EXE
    if (-not $exe) { $exe = Join-Path $root "bin\Tests\$Configuration\OpenSWBF2Tests.exe" }
    if (-not (Test-Path $exe)) { Write-Host "Test exe not found: $exe"; exit 2 }
}

New-Item -ItemType Directory -Force -Path $outDir | Out-Null
Set-Content -Path (Join-Path $root 'artifacts\.gitignore') -Value '*' -Encoding ASCII

function Start-Exe([string]$argLine, [string]$asanDir) {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exe
    $psi.Arguments = $argLine
    $psi.WorkingDirectory = $testsDir
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    if ($asanDir) { $psi.EnvironmentVariables['ASAN_OPTIONS'] = Get-AsanOptions $asanDir }
    $p = [System.Diagnostics.Process]::Start($psi)
    $o = $p.StandardOutput.ReadToEndAsync()
    $e = $p.StandardError.ReadToEndAsync()
    return @{ Proc = $p; Out = $o; Err = $e }
}

if ($runUnit) {
    $allSuites = 'unit', 'characterisation', 'regression', 'known_bug', 'corpus', 'fuzz', 'stress'
    if ($Fuzz) { $suites = @('stress') }
    elseif ($Suite) { $suites = @($Suite) }
    else { $suites = $allSuites | Where-Object { $_ -ne 'stress' } }

    $cases = @()
    $listMs = 30000
    if ($env:OPENSWBF2_LIST_TIMEOUT_MS) { $listMs = [int]$env:OPENSWBF2_LIST_TIMEOUT_MS }
    foreach ($s in $suites) {
        $listDir = $null
        if ($asanT1) {
            $listDir = Join-Path $outDir "t1\list-$s"
            New-Item -ItemType Directory -Force -Path $listDir | Out-Null
        }
        $h = Start-Exe "-ts=$s --list-test-cases" $listDir
        if (-not $h.Proc.WaitForExit($listMs)) {
            try { $h.Proc.Kill() } catch {}
            Write-Host "Listing timed out for suite '$s'."
            exit 2
        }
        if ($h.Proc.ExitCode -ne 0) {
            $msg = "Listing failed for suite '$s' (exit code $($h.Proc.ExitCode))."
            if ($asanT1) {
                $rep = Get-AsanReport $listDir $h.Proc.ExitCode
                if ($rep) { $msg += "`r`n" + $rep }
            }
            Write-Host $msg
            exit 2
        }
        $lines = ($h.Out.Result -split "`r?`n")
        $in = $false
        $before = $cases.Count
        foreach ($l in $lines) {
            if ($l -like '=====*') { $in = -not $in; continue }
            if ($in -and $l.Trim() -ne '') { $cases += New-Object PSObject -Property @{ Name = $l.TrimEnd(); Suite = $s } }
        }
        if ($cases.Count -eq $before -and -not $Filter -and $s -ne 'regression' -and $s -ne 'known_bug') {
            Write-Host "Suite '$s' listed 0 cases."
            exit 2
        }
    }
    if ($Filter) { $cases = @($cases | Where-Object { $_.Name -like $Filter }) }
    if ($cases.Count -eq 0) { Write-Host 'No cases run (suite empty or filter matched nothing).'; exit 2 }

    foreach ($c in $cases) {
        $argLine = '"-tc=' + ($c.Name -replace '"', '\"') + '" --timeout ' + $Timeout
        if ($Fuzz) { $argLine += " --fuzz-seed=$Seed --fuzz-count=$Iterations" }
        $safe = ($c.Name -replace '[^A-Za-z0-9_.-]', '_')
        $file = "$safe.txt"
        $n = 2
        while (Test-Path (Join-Path $outDir $file)) { $file = "$safe-$n.txt"; $n++ }
        $caseAsan = $null
        if ($asanT1) {
            $caseAsan = Join-Path $outDir ('t1\' + [IO.Path]::GetFileNameWithoutExtension($file))
            New-Item -ItemType Directory -Force -Path $caseAsan | Out-Null
        }
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        $h = Start-Exe $argLine $caseAsan
        $outerMs = [int64]($Timeout + 10) * 1000
        if ($Fuzz) { $outerMs = [Math]::Max($outerMs, [int64]$Iterations / 5 + 60000) }
        $outer = $h.Proc.WaitForExit([int][Math]::Min($outerMs, [int]::MaxValue))
        if (-not $outer) { try { $h.Proc.Kill() } catch {}; $h.Proc.WaitForExit() }
        $sw.Stop()
        $text = $h.Out.Result + $h.Err.Result
        $code = [BitConverter]::ToUInt32([BitConverter]::GetBytes([int]$h.Proc.ExitCode), 0)
        $detail = ''
        if (-not $outer) { $st = 'TIMEOUT' }
        elseif ($code -eq 0) {
            if ($text -match 'test cases:\s+1 \|\s+1 passed') { $st = 'PASS' }
            else { $st = 'FAIL'; $detail = 'case did not run exactly once' }
        }
        elseif ($code -eq 1) { $st = 'FAIL' }
        elseif ($code -eq 77) { $st = 'SKIP' }
        elseif ($code -eq 124) { $st = 'TIMEOUT' }
        else { $st = 'CRASH'; $detail = ('0x{0:X8}' -f $code) }
        $rep = $null
        if ($asanT1) { $rep = Get-AsanReport $caseAsan $code }
        if ($rep) {
            $st = 'FAIL'
            $detail = 'asan'
            $text += "`r`n" + $rep
        }

        $known = ($c.Suite -eq 'known_bug')
        $ms = $sw.ElapsedMilliseconds
        Set-Content -Path (Join-Path $outDir $file) -Value $text -Encoding ASCII

        if ($known -and -not $rep -and ($st -eq 'TIMEOUT' -or ($st -eq 'FAIL' -and $code -eq 1 -and -not $detail -and $text -match 'test cases:\s+1 \|'))) {
            $under = $st
            $line = "[KNOWN] $($c.Name) ($under) ($ms ms)"
            $final = 'KNOWN'
        }
        else {
            $line = "[$st] $($c.Name) ($ms ms)"
            if ($detail -and $st -ne 'CRASH') { $line += " - $detail" }
            elseif ($st -eq 'CRASH') { $line = "[CRASH] $($c.Name) $detail ($ms ms)" }
            $final = $st
            if ($st -ne 'PASS' -and $st -ne 'SKIP') { $bad++ }
        }
        Write-Host $line
        $results += New-Object PSObject -Property @{ Line = $line; Final = $final; Known = $known }
        if ($final -in 'FAIL', 'TIMEOUT', 'CRASH', 'KNOWN') { Write-Host "    details: $(Join-Path $outDir $file)" }
        if ($rep) { foreach ($rl in ($rep -split "`r?`n")) { Write-Host "    $rl" } }
        if ($known -and $st -eq 'PASS') {
            $w = "WARNING: known_bug case passed, bug may be fixed: $($c.Name)"
            Write-Host $w
            $results += New-Object PSObject -Property @{ Line = $w; Final = 'WARN'; Known = $true }
        }
    }
}

if ($hardenActive) {
    Assert-Runtimes $GameRuntimes
    $exeDir = Join-Path $root "bin\Win32\$SmokeConfiguration"
    $gameExe = Join-Path $exeDir 'BattlefrontII.exe'
    $gameDataRoot = Find-GameDataRoot
    if (-not $NoBuild) {
        $msbuild = Find-MSBuild
        & $msbuild (Join-Path $root 'BattlefrontII.vcxproj') "/p:Configuration=$SmokeConfiguration" '/p:Platform=Win32' /m /v:minimal /nologo
        if ($LASTEXITCODE -ne 0) { Write-Host 'Build failed.'; exit 2 }
    }
    $reason = Get-SmokePrecheck $gameDataRoot $exeDir $asanSmoke

    function Add-SmokeResult($line, $final) {
        Write-Host $line
        $script:results += New-Object PSObject -Property @{ Line = $line; Final = $final; Known = $false }
    }

    $vendorHits = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\LuaFL'), (Join-Path $root 'src\ZlibFL') -Recurse -File -ErrorAction SilentlyContinue |
        Where-Object { Select-String -LiteralPath $_.FullName -Pattern 'RedLog.h' -SimpleMatch -Quiet })
    if ($vendorHits.Count -eq 0) { Add-SmokeResult '[PASS] hardening:redlog-vendor' 'PASS' }
    else {
        foreach ($h in $vendorHits) { Add-SmokeResult "[FAIL] hardening:redlog-vendor - $($h.FullName)" 'FAIL' }
        $bad++
    }
    foreach ($c in (Get-SourceChecks $root)) {
        if ($c.Ok) { Add-SmokeResult "[PASS] hardening:$($c.Name)" 'PASS' }
        else {
            foreach ($h in $c.Detail.Split(',')) { Add-SmokeResult "[FAIL] hardening:$($c.Name) - $($h.Trim())" 'FAIL' }
            $bad++
        }
    }

    $dbCode = 0
    if (-not (Test-Path -LiteralPath $gameExe)) {
        if ($NoBuild -and -not $Hardening) { Add-SmokeResult '[SKIP] hardening:exe - not built (-NoBuild)' 'SKIP' }
        else {
            Add-SmokeResult "[FAIL] hardening:exe - not found: $gameExe" 'FAIL'
            $bad++
        }
    }
    else {
        $db = Find-Dumpbin
        $hdr = (& $db /headers $gameExe) -join "`n"
        $dbCode = $LASTEXITCODE
        $cfg = (& $db /loadconfig $gameExe) -join "`n"
        if ($dbCode -eq 0) { $dbCode = $LASTEXITCODE }
        $imp = (& $db /imports $gameExe) -join "`n"
        if ($dbCode -eq 0) { $dbCode = $LASTEXITCODE }
    }
    if ($dbCode -ne 0) {
        Add-SmokeResult "[FAIL] hardening:dumpbin - exit $dbCode" 'FAIL'
        $bad++
    }
    elseif (Test-Path -LiteralPath $gameExe) {
        $tl = Get-ClTlogCounts (Join-Path $root "obj\Win32\$SmokeConfiguration\BattlefrontII.tlog")
        foreach ($c in (Get-HardeningChecks $hdr $cfg $tl.Total $tl.GsMinus $tl.Sdl)) {
            if ($c.Ok) { Add-SmokeResult "[PASS] hardening:$($c.Name)" 'PASS' }
            else {
                Add-SmokeResult "[FAIL] hardening:$($c.Name) - $($c.Detail)" 'FAIL'
                $bad++
            }
        }
        $ttDir = Join-Path $root "obj\Tests\$Configuration\OpenSWBF2Tests.tlog"
        $tt = $null
        if (Test-Path -LiteralPath $ttDir) { $tt = Get-ClTlogCounts $ttDir }
        else { Add-SmokeResult '[SKIP] hardening:flag-alignednew-tests - tests not built' 'SKIP' }
        foreach ($c in (Get-FlagChecks $tl $tt)) {
            if ($c.Ok) { Add-SmokeResult "[PASS] hardening:$($c.Name)" 'PASS' }
            else {
                Add-SmokeResult "[FAIL] hardening:$($c.Name) - $($c.Detail)" 'FAIL'
                $bad++
            }
        }
        foreach ($c in (Get-ImportChecks $imp)) {
            if ($c.Ok) { Add-SmokeResult "[PASS] hardening:$($c.Name)" 'PASS' }
            else {
                Add-SmokeResult "[FAIL] hardening:$($c.Name) - $($c.Detail)" 'FAIL'
                $bad++
            }
        }
        if ($reason) {
            $tag = 'SKIP'
            if ($gameDataRoot -and -not $NoBuild) {
                $tag = 'FAIL'
                $bad++
            }
            Add-SmokeResult "[$tag] hardening:aslr-base - $reason" $tag
        }
        else {
            $aslrDir = Join-Path $outDir 'hardening\aslr'
            New-Item -ItemType Directory -Force -Path $aslrDir | Out-Null
            $aslrArgs = (Get-SmokeCases 'Standard' @() $false $null)[0].Args.Replace('{CASEDIR}', $aslrDir)
            $base = Get-ImageBase $gameExe $exeDir $aslrArgs
            if ($base -and $base -ne 0x400000) { Add-SmokeResult ('[PASS] hardening:aslr-base (0x{0:X8})' -f $base) 'PASS' }
            else {
                Add-SmokeResult ('[FAIL] hardening:aslr-base - base {0}' -f $(if ($base) { '0x{0:X8}' -f $base } else { 'unreadable' })) 'FAIL'
                $bad++
            }
        }
    }
}

if ($smokeActive) {
    $missionFile = Join-Path $testsDir 'smoke\missions.txt'
    if ($Smoke -eq 'All') { $missionFile = Join-Path $testsDir 'smoke\missions-all.txt' }
    $missions = @()
    if (Test-Path $missionFile) { $missions = @(Get-Content $missionFile | Where-Object { $_.Trim() }) }
    $smokeCases = @(Get-SmokeCases $Smoke $missions $Movies.IsPresent $null $HogAddrSpace.IsPresent)
    if ($asanSmoke) {
        $asanRemoved = @($smokeCases | Where-Object { $script:SmokeAsanExcluded -contains $_.Name } | ForEach-Object { $_.Name })
        $smokeCases = @($smokeCases | Where-Object { $script:SmokeAsanExcluded -notcontains $_.Name })
        if ($asanRemoved.Count -gt 0) { Write-Host "INFO: ASan excludes: $($asanRemoved -join ', ')" }
    }
    function Get-SmokeCaseTimeout($sc) {
        $t = $SmokeTimeout
        if ($sc.Timeout -gt 0) { $t = $sc.Timeout }
        if ($asanSmoke) { $t *= $script:SmokeAsanTimeoutScale }
        return $t
    }
    function Get-SmokeCaseAsan($caseDir) {
        if ($asanSmoke) { return (Get-AsanOptions $caseDir) }
        return $null
    }
    if ($reason) {
        $final = 'SKIP'
        $tag = 'SKIP'
        if ($gameDataRoot -and -not $NoBuild) {
            $final = 'FAIL'
            $tag = 'FAIL'
        }
        foreach ($sc in $smokeCases) {
            Add-SmokeResult "[$tag] smoke:$($sc.Name) class=Skipped - $reason" $final
        }
    }
    else {
        foreach ($w in (Test-WerSetup)) { Write-Host "WARNING: $w" }
        $gameExe = Join-Path $exeDir 'BattlefrontII.exe'
        $scoreKeys = @{}
        $useParallel = ($Jobs -gt 1) -and ($smokeCases.Count -gt 1) -and ($Smoke -ne 'Twice')
        $batch = $null
        $workDirs = @()
        if ($useParallel) {
            $workerCount = [Math]::Min($Jobs, $smokeCases.Count)
            Write-Host "INFO: smoke jobs=$workerCount"
            for ($w = 1; $w -le $workerCount; $w++) { $workDirs += (Join-Path $outDir "smoke-workers\w$w") }
            $items = @()
            foreach ($sc in $smokeCases) {
                $caseDir = Join-Path $outDir "smoke\$($sc.Name)"
                $items += New-Object PSObject -Property @{ Name = $sc.Name; ExeName = 'BattlefrontII.exe'; ArgLine = $sc.Args.Replace('{CASEDIR}', $caseDir); Timeout = (Get-SmokeCaseTimeout $sc); CaseDir = $caseDir; AsanOptions = (Get-SmokeCaseAsan $caseDir) }
            }
        }
        try {
            if ($useParallel) {
                foreach ($d in $workDirs) { New-SmokeWorkerDir $exeDir $d }
                $batch = Start-SmokeBatch $items $workDirs
            }
            $caseIndex = -1
            foreach ($sc in $smokeCases) {
                $caseIndex++
                $caseDir = Join-Path $outDir "smoke\$($sc.Name)"
                $argLine = $sc.Args.Replace('{CASEDIR}', $caseDir)
                if ($useParallel) { $res = Receive-SmokeCase $batch $caseIndex }
                else { $res = Invoke-SmokeCase $sc.Name $gameExe $exeDir $argLine (Get-SmokeCaseTimeout $sc) $caseDir (Get-SmokeCaseAsan $caseDir) }
                $ok = Test-SmokeExpect $sc $res
                $tok = Get-SmokeToken $res.Class
                if ($ok) { $tok = 'PASS' }
                elseif ($tok -eq 'PASS') { $tok = 'FAIL' }
                if ($res.Asan) { $tok = 'FAIL' }
                $line = ('[{0}] smoke:{1} class={2} exit=0x{3:X8} ({4} ms)' -f $tok, $sc.Name, $res.Class, $res.Code, $res.Ms)
                Add-SmokeResult $line $tok
                if ($tok -ne 'PASS') {
                    Write-Host "    details: $caseDir"
                    $bad++
                }
                if ($res.Asan) { foreach ($rl in ($res.Asan -split "`r?`n")) { Write-Host "    $rl" } }
                if ($sc.Twice -gt 2) { $scoreKeys[$sc.Twice] = Get-DigestKey $caseDir }
                elseif ($sc.Twice -gt 0) { $scoreKeys[$sc.Twice] = Get-FinalScoreKey $caseDir }
            }
        }
        finally {
            if ($batch) { Stop-SmokeBatch $batch }
            foreach ($d in $workDirs) { Remove-SmokeWorkerDir $d }
        }
        foreach ($pair in @(@(1, 2, 'run-twice'), @(3, 4, 'run-twice-digest'))) {
            if ($scoreKeys.ContainsKey($pair[0]) -and $scoreKeys.ContainsKey($pair[1])) {
                if ($scoreKeys[$pair[0]] -and $scoreKeys[$pair[0]] -eq $scoreKeys[$pair[1]]) { Add-SmokeResult "[PASS] smoke:$($pair[2]) match" 'PASS' }
                else {
                    Add-SmokeResult "[FAIL] smoke:$($pair[2]) mismatch" 'FAIL'
                    $bad++
                }
            }
        }
        $traceLog1 = Join-Path $outDir 'smoke\trace-seed-1\run.log'
        $traceLog2 = Join-Path $outDir 'smoke\trace-seed-2\run.log'
        if ((Test-Path -LiteralPath $traceLog1) -and (Test-Path -LiteralPath $traceLog2)) {
            $t = Compare-DigestTrace $traceLog1 $traceLog2
            if ($t -gt 0) { Add-SmokeResult "[PASS] smoke:digest-trace split at turn $t" 'PASS' }
            elseif ($t -lt 0) {
                Add-SmokeResult '[FAIL] smoke:digest-trace incomplete trace' 'FAIL'
                $bad++
            }
            else {
                Add-SmokeResult '[FAIL] smoke:digest-trace no split' 'FAIL'
                $bad++
            }
            $dk = Get-DigestKey (Join-Path $outDir 'smoke\trace-seed-1')
            $dt = $null
            $ds = $null
            if ($dk -and $dk -cmatch 'turns=(\d+) .*state=(0x[0-9A-F]{8})') {
                $dt = [int]$Matches[1]
                $ds = $Matches[2]
            }
            $tr = Get-DigestTrace $traceLog1
            if ($dt -and $tr.ContainsKey($dt) -and $tr[$dt] -cmatch '^state=(0x[0-9A-F]{8}) ' -and $Matches[1] -ceq $ds) {
                Add-SmokeResult "[PASS] smoke:digest-trace matches digest at turn $dt" 'PASS'
            }
            else {
                Add-SmokeResult "[FAIL] smoke:digest-trace mismatch at turn $dt" 'FAIL'
                $bad++
            }
        }
    }
}

$normal = @($results | Where-Object { -not $_.Known -and $_.Final -ne 'WARN' })
$kn = @($results | Where-Object { $_.Known -and $_.Final -ne 'WARN' })
$summary = @()
$summary += ''
$summary += 'Results'
foreach ($t in 'PASS', 'FAIL', 'SKIP', 'TIMEOUT', 'CRASH') {
    $summary += ('  {0,-8} {1}' -f $t, @($normal | Where-Object { $_.Final -eq $t }).Count)
}
$summary += 'Known bugs'
$summary += ('  {0,-8} {1}' -f 'KNOWN', @($kn | Where-Object { $_.Final -eq 'KNOWN' }).Count)
$summary += ('  {0,-8} {1}' -f 'PASS', @($kn | Where-Object { $_.Final -eq 'PASS' }).Count)
if ($asanT1 -or $asanSmoke) { $summary += $script:SmokeAsanLabel }
if ($smokeActive -and $asanSmoke) {
    if ($asanRemoved.Count -gt 0) { $summary += "ASan excluded smoke cases: $($asanRemoved -join ', ')" }
    else { $summary += 'ASan excluded smoke cases: none in this group' }
}
$summary | ForEach-Object { Write-Host $_ }
(($results | ForEach-Object { $_.Line }) + $summary) | Set-Content -Path (Join-Path $outDir 'results.txt') -Encoding ASCII
Write-Host "Artifacts: $outDir"

if ($bad -gt 0) { exit 1 }
exit 0
