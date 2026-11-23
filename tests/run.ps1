param(
    [ValidateSet('Debug','Release','Test','Asan')][string]$Configuration = 'Debug',
    [ValidateSet('Win32','x64')][string]$Platform = 'Win32',
    [switch]$NoBuild,
    [string]$Suite,
    [string]$Filter,
    [switch]$Fuzz,
    [int]$Seed = 1,
    [int]$Iterations = 100000,
    [ValidateSet('None','Standard','Dedicated','Crash','All','Twice','Perturb','Soak','Vendor','Boot')][string]$Smoke = 'Standard',
    [switch]$FactoryLog,
    [int]$MiPurge = -1,
    [ValidateSet('Debug','Release','Test','Asan')][string]$SmokeConfiguration = 'Release',
    [switch]$Movies,
    [switch]$Hardening,
    [switch]$HogAddrSpace,
    [ValidateRange(0,64)][int]$Jobs = 0,
    [switch]$Asan,
    [switch]$Clean,
    [int]$WorkerBase = 0,
    [ValidateSet('Short','Long')][string]$Soak = 'Short',
    [string]$Case,
    [string]$Rejudge
)
$ErrorActionPreference = 'Stop'
$testsDir = $PSScriptRoot
$root = Split-Path $testsDir -Parent
. (Join-Path $testsDir 'smoke.ps1')
. (Join-Path $testsDir 'gate\keys.ps1')
$paramNames = @($MyInvocation.MyCommand.Parameters.Keys | Where-Object { $_ -ne 'Rejudge' })
$bound = @($PSBoundParameters.Keys)
if ($Rejudge) {
    if ($bound.Count -gt 1) { Write-Host '-Rejudge takes no other switch; the run''s own come from its run-params.txt.'; exit 2 }
    $outDir = [IO.Path]::GetFullPath($Rejudge).TrimEnd('\')
    $paramFile = Join-Path $outDir 'run-params.txt'
    if (-not (Test-Path -LiteralPath $paramFile -PathType Leaf)) { Write-Host "-Rejudge: no run-params.txt in $outDir"; exit 2 }
    $runParams = Read-KeyValueFile $paramFile
    foreach ($k in @($paramNames | ForEach-Object { "param.$_" }) + @('bound', 'wer_localdumps', 'perturb_missions', 'missions')) {
        if (-not $runParams.Contains($k)) { Write-Host "-Rejudge: run-params.txt lacks $k"; exit 2 }
    }
    $bound = @($runParams['bound'] -split ',' | Where-Object { $_ })
    foreach ($pn in $paramNames) {
        $pv = $runParams["param.$pn"]
        $cur = Get-Variable -Name $pn -ValueOnly
        if ($cur -is [switch]) { Set-Variable -Name $pn -Value ([switch]($pv -ceq 'True')) }
        elseif ($cur -is [int]) { Set-Variable -Name $pn -Value ([int]$pv) }
        else { Set-Variable -Name $pn -Value $pv }
    }
    $script:SmokeWerKey = ($runParams['wer_localdumps'] -ceq '1')
    $script:SmokeSteam = $runParams['steam']
    if ($runParams['perturb_missions']) { $env:OPENSWBF2_PERTURB_MISSIONS = $runParams['perturb_missions'] }
    else { Remove-Item Env:\OPENSWBF2_PERTURB_MISSIONS -ErrorAction SilentlyContinue }
}
else { $script:SmokeWerKey = Test-WerKey }
$rawParams = [ordered]@{}
foreach ($pn in $paramNames) { $rawParams[$pn] = [string](Get-Variable -Name $pn -ValueOnly) }
$extraArg = $(if ($FactoryLog) { ' -factorylog' } else { '' })
if ($MiPurge -ge 0) { $extraArg += " -mipurge $MiPurge" }
if ($Jobs -eq 0) { $Jobs = [Math]::Max(1, [Math]::Min(8, [int][Math]::Floor([Environment]::ProcessorCount / 2))) }
$script:SmokePlatform = $Platform
$script:SmokeHog = $HogAddrSpace.IsPresent
Write-ToolBanner $Platform
if ($Asan) {
    if ($bound -contains 'Configuration' -and $Configuration -ne 'Asan') { Write-Host "-Asan conflicts with -Configuration $Configuration."; exit 2 }
    $Configuration = 'Asan'
}
if ($Configuration -in 'Test', 'Asan' -and $bound -notcontains 'SmokeConfiguration') { $SmokeConfiguration = $Configuration }
$script:SmokeDigestConfiguration = $SmokeConfiguration
$asanT1 = ($Configuration -eq 'Asan')
$asanSmoke = ($SmokeConfiguration -eq 'Asan')
if ($Smoke -eq 'Perturb' -and $SmokeConfiguration -notin 'Test', 'Asan') {
    Write-Host '-Smoke Perturb needs -Configuration Test or Asan (-perturbheap is Test-only).'
    exit 2
}
if ($Clean) {
    foreach ($d in (Join-Path $root "bin\$Platform\$SmokeConfiguration"), (Join-Path $root "bin\Tests\$Platform\$Configuration")) {
        if (Test-Path -LiteralPath $d) {
            Remove-StagedOutput $d
            Write-Host "Removed $d"
        }
    }
    $artifacts = Join-Path $root 'artifacts'
    $artLinks = @(Get-ReparsePoints $artifacts)
    if ($artLinks.Count -gt 0) {
        Remove-Junctions $artifacts
        Write-Host "Removed $($artLinks.Count) junction(s) under $artifacts"
    }
    exit 0
}

$hardenOnly = $Hardening -and $bound -notcontains 'Smoke' -and -not ($Suite -or $Filter -or $Fuzz)
$smokeActive = ($Smoke -ne 'None') -and -not ($Suite -or $Filter -or $Fuzz) -and -not $hardenOnly
$runUnit = ($Suite -or $Filter -or $Fuzz -or $bound -notcontains 'Smoke' -or $Smoke -eq 'None') -and -not $hardenOnly
$hardenActive = $Hardening -or $smokeActive
$asanRemoved = @()
$missionFile = 'none'
if ($smokeActive) {
    $missionFile = Join-Path $testsDir 'smoke\missions.txt'
    if ($Smoke -eq 'All') { $missionFile = Join-Path $testsDir 'smoke\missions-all.txt' }
    if ($Rejudge -and $runParams['missions'] -cne 'none') { $missionFile = $runParams['missions'] }
    $missions = @()
    if (Test-Path $missionFile) { $missions = @(Get-Content $missionFile | Where-Object { $_.Trim() }) }
    $smokeCases = @(Add-MiOptionsExpect @(Add-SerialTieExpect @(Get-SmokeCases $Smoke $missions $Movies.IsPresent $null $HogAddrSpace.IsPresent $Soak)) $MiPurge)
    if ($Case) {
        $smokeCases = @($smokeCases | Where-Object { $_.Name -ceq $Case })
        if ($smokeCases.Count -ne 1) { Write-Host "-Case ${Case}: no such case in -Smoke $Smoke for $SmokeConfiguration on $Platform."; exit 2 }
    }
    $excludedNames = @(Get-SmokeExcluded $Platform $SmokeConfiguration)
    foreach ($e in @(Get-SmokeExcludedEntries $Platform $SmokeConfiguration | Where-Object { @($smokeCases | ForEach-Object { $_.Name }) -ccontains $_.Case })) {
        Write-Host "INFO: $($e.Case) excluded on $Platform $SmokeConfiguration, cause $($e.Cause)"
    }
    $asanRemoved = @($smokeCases | Where-Object { $excludedNames -contains $_.Name } | ForEach-Object { $_.Name })
    $smokeCases = @($smokeCases | Where-Object { $excludedNames -notcontains $_.Name })
    if ($asanRemoved.Count -gt 0) { Write-Host "INFO: excludes: $($asanRemoved -join ', ')" }
    $workerCount = [Math]::Max(1, [Math]::Min($Jobs, $smokeCases.Count))
    if (-not $Rejudge) {
        try { $workDirs = @(Get-WorkerPaths $root $Platform $SmokeConfiguration 'smoke' $WorkerBase $workerCount) }
        catch { Write-Host "-WorkerBase ${WorkerBase}: $($_.Exception.Message)."; exit 2 }
    }
}

if (-not $Rejudge) { $outDir = New-StampedDir (Join-Path $root 'artifacts\tests') }
$resultsName = $(if ($Rejudge) { 'results-rejudge.txt' } else { 'results.txt' })
$results = @()
$bad = 0

if ($runUnit -and -not $Rejudge) {
    Assert-Runtimes $T1Runtimes $Platform

    if (-not $NoBuild) {
        $msbuild = Find-MSBuild
        & $msbuild (Join-Path $testsDir 'OpenSWBF2Tests.vcxproj') "/p:Configuration=$Configuration" "/p:Platform=$Platform" /m /v:minimal /nologo
        if ($LASTEXITCODE -ne 0) { Write-Host 'Build failed.'; exit 2 }
    }

    $exe = $env:OPENSWBF2_TEST_EXE
    if (-not $exe) { $exe = Join-Path $root "bin\Tests\$Platform\$Configuration\OpenSWBF2Tests.exe" }
    if (-not (Test-Path $exe)) { Write-Host "Test exe not found: $exe"; exit 2 }
}

function Get-RunExeHash([string]$path) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { return 'none' }
    try { return (Get-PeNormalisedSha $path) }
    catch { return '-' }
}

if (-not $Rejudge) {
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
    Set-Content -Path (Join-Path $root 'artifacts\.gitignore') -Value '*' -Encoding ASCII
    $testsExePath = $env:OPENSWBF2_TEST_EXE
    if (-not $testsExePath) { $testsExePath = Join-Path $root "bin\Tests\$Platform\$Configuration\OpenSWBF2Tests.exe" }
    $paramLines = @(foreach ($pn in $paramNames) { "param.$pn=$($rawParams[$pn])" })
    $paramLines += "bound=$($bound -join ',')"
    $paramLines += "platform=$Platform"
    $paramLines += "configuration=$Configuration"
    $paramLines += "smoke_configuration=$SmokeConfiguration"
    $paramLines += "missions=$missionFile"
    $paramLines += "perturb_missions=$env:OPENSWBF2_PERTURB_MISSIONS"
    $paramLines += "wer_localdumps=$([int][bool]$script:SmokeWerKey)"
    $script:SmokeSteam = Get-SteamRunning
    $paramLines += "steam=$script:SmokeSteam"
    $paramLines += "tests_exe=$testsExePath"
    $paramLines += "exe.OpenSWBF2Tests.exe=$(Get-RunExeHash $testsExePath)"
    $paramLines | Set-Content -Path (Join-Path $outDir 'run-params.txt') -Encoding ASCII
    $hostFaultStart = Get-Date
    Add-HostFaultParams (Join-Path $outDir 'run-params.txt') 'start' $hostFaultStart
}

function Add-SmokeResult($line, $final) {
    Write-Host $line
    $script:results += New-Object PSObject -Property @{ Line = $line; Final = $final; Known = $false }
}

function Start-Exe([string]$argLine, [string]$asanDir) {
    $asanOptions = $null
    if ($asanDir) { $asanOptions = Get-AsanOptions $asanDir }
    return (Invoke-T1Exe $exe $testsDir $argLine $asanOptions)
}

if ($runUnit -and $Rejudge) {
    Write-Host '[INFO] t1 - not re-judged: T1 cases have no judge-only input'
    $results += New-Object PSObject -Property @{ Line = '[INFO] t1 - not re-judged: T1 cases have no judge-only input'; Final = 'INFO'; Known = $false }
}
elseif ($runUnit) {
    $allSuites = $script:T1Suites
    if ($Fuzz) { $suites = @('stress') }
    elseif ($Suite) { $suites = @($Suite) }
    else { $suites = $allSuites | Where-Object { $_ -ne 'stress' } }

    $cases = @()
    foreach ($s in $suites) {
        $listDir = $null
        if ($asanT1) {
            $listDir = Join-Path $outDir "t1\list-$s"
            New-Item -ItemType Directory -Force -Path $listDir | Out-Null
        }
        $h = Start-Exe "-ts=$s --list-test-cases" $listDir
        if ($h.Stalled -or $h.Killed) {
            Write-Host "Listing stalled for suite '$s' (silent $($h.SilentMaxS.ToString('0.0', [Globalization.CultureInfo]::InvariantCulture)) s)."
            exit 2
        }
        if ($h.Omitted -gt 0) {
            Write-Host "Listing output for suite '$s' was cut ($($h.Omitted) bytes omitted)."
            exit 2
        }
        if ($h.Code -ne 0) {
            $msg = "Listing failed for suite '$s' (exit code $($h.Code))."
            if ($asanT1) {
                $rep = Get-AsanReport $listDir $h.Code
                if ($rep) { $msg += "`r`n" + $rep }
            }
            Write-Host $msg
            exit 2
        }
        $lines = ($h.Out -split "`r?`n")
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
    $t1NotFor = Split-T1NotForPlatform $cases $Platform (-not ($Suite -or $Filter -or $Fuzz))
    foreach ($r in $t1NotFor.Stale) {
        Add-SmokeResult "[FAIL] t1-not-for-platform:$($r.Name) - not a test case of $exe" 'FAIL'
        $bad++
    }
    foreach ($r in $t1NotFor.Skipped) { Add-SmokeResult "[N/A] $($r.Name) - $($r.Reason)" 'SKIP' }
    $cases = @($t1NotFor.Run)

    $silentMax = 0.0
    $silentCase = $null
    foreach ($c in $cases) {
        $argLine = '"-tc=' + ($c.Name -replace '"', '\"') + '"'
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
        $h = Start-Exe $argLine $caseAsan
        $text = $h.Text
        $code = $h.Code
        $tok = Get-T1Token $h
        $st = $tok.Token
        $detail = $tok.Detail
        if ($null -eq $silentCase -or [double]$h.SilentMaxS -gt $silentMax) { $silentMax = [double]$h.SilentMaxS; $silentCase = $c.Name }
        $rep = $null
        if ($asanT1) { $rep = Get-AsanReport $caseAsan $code }
        if ($rep) {
            $st = 'FAIL'
            $detail = 'asan'
            $text += "`r`n" + $rep
        }

        $known = ($c.Suite -eq 'known_bug')
        $ms = $h.Ms
        Set-Content -Path (Join-Path $outDir $file) -Value $text -Encoding ASCII

        if ($known -and -not $rep -and ($st -eq 'STALL' -or ($st -eq 'FAIL' -and $code -eq 1 -and -not $detail -and $text -match 'test cases:\s+1 \|'))) {
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
        if ($final -in 'FAIL', 'STALL', 'CRASH', 'KNOWN') { Write-Host "    details: $(Join-Path $outDir $file)" }
        if ($rep) { foreach ($rl in ($rep -split "`r?`n")) { Write-Host "    $rl" } }
        if ($known -and $st -eq 'PASS') {
            $w = "WARNING: known_bug case passed, bug may be fixed: $($c.Name)"
            Write-Host $w
            $results += New-Object PSObject -Property @{ Line = $w; Final = 'WARN'; Known = $true }
        }
    }
    Write-Host ("t1 largest silence {0} s: {1}" -f $silentMax.ToString('0.0', [Globalization.CultureInfo]::InvariantCulture), $silentCase)
}

$hardDir = Join-Path $outDir 'hardening'
if ($hardenActive -and -not $Rejudge) {
    Assert-Runtimes (Get-GameRuntimes $Platform @($(if ($smokeActive) { $Smoke } else { 'None' }))) $Platform
    $exeDir = Join-Path $root "bin\$Platform\$SmokeConfiguration"
    $gameExe = Join-Path $exeDir 'BattlefrontII.exe'
    $gameDataRoot = Find-GameDataRoot
    if (-not $NoBuild) {
        $msbuild = Find-MSBuild
        & $msbuild (Join-Path $root 'BattlefrontII.vcxproj') "/p:Configuration=$SmokeConfiguration" "/p:Platform=$Platform" /m /v:minimal /nologo
        if ($LASTEXITCODE -ne 0) { Write-Host 'Build failed.'; exit 2 }
    }
    $exeLines = @("exe.BattlefrontII.exe=$(Get-RunExeHash $gameExe)")
    foreach ($f in @((Get-SmokeWorkerFiles $Platform) + (Get-AsanRuntimeName)) | Where-Object { $_ -like '*.dll' }) {
        $dp = Join-Path $exeDir $f
        if (Test-Path -LiteralPath $dp -PathType Leaf) { $exeLines += "dll.$f=$(Get-KeyFileSha $dp)" }
    }
    $exeLines | Add-Content -Path (Join-Path $outDir 'run-params.txt') -Encoding ASCII
    $reason = Get-SmokePrecheck $gameDataRoot $exeDir $asanSmoke $Platform

    $vendorHits = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\LuaFL'), (Join-Path $root 'src\ZlibFL'), (Join-Path $root 'src\mimalloc') -Recurse -File -ErrorAction SilentlyContinue |
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

    if ($Platform -eq 'x64') {
        $gameProjects = @(Get-GameProjects $root $SmokeConfiguration)
        $tl = Get-ClTlogCounts @($gameProjects | ForEach-Object { $_.TlogDir })
        $ttDir = Join-Path $root "obj\Tests\$Platform\$Configuration\OpenSWBF2Tests.tlog"
        $tt = $null
        if (Test-Path -LiteralPath $ttDir) { $tt = Get-ClTlogCounts $ttDir }
        else { Add-SmokeResult '[SKIP] hardening:flag-alignednew-tests - tests not built' 'SKIP' }
        $lcDir = Join-Path $root "obj\$Platform\$SmokeConfiguration\LuaCompiler\LuaCompiler.tlog"
        $lc = $null
        if (Test-Path -LiteralPath $lcDir) { $lc = Get-ClTlogCounts $lcDir }
        $tlogChecks = @(Get-HardeningChecks '' '' $tl.Total $tl.GsMinus $tl.Sdl | Where-Object { $_.Name -eq 'sdl' }) + @(Get-FlagChecks $tl $tt $SmokeConfiguration $Configuration $lc) + @(Get-X64ObjectsCheck $root $SmokeConfiguration) + @(Get-CrtTrigRouteResult $root $SmokeConfiguration $Platform) + @(Get-X64SizeLiteralCheck $root)
        foreach ($c in $tlogChecks) {
            if ($c.Ok) { Add-SmokeResult "[PASS] hardening:$($c.Name)" 'PASS' }
            else {
                Add-SmokeResult "[FAIL] hardening:$($c.Name) - $($c.Detail)" 'FAIL'
                $bad++
            }
        }
        $binaryNames = @(@(Get-HardeningChecks '' '' 0 0 0) + @(Get-MiCrtHookCheck '') | Where-Object { $_.Name -ne 'sdl' } | ForEach-Object { $_.Name }) + @('aslr-pe', 'aslr-pe-tests')
        foreach ($name in $binaryNames) { Add-SmokeResult "[SKIP] hardening:$name - Phase 52" 'SKIP' }
        if (-not (Test-Path -LiteralPath $gameExe)) {
            if ($NoBuild -and -not $Hardening) { Add-SmokeResult '[SKIP] hardening:exe - not built (-NoBuild)' 'SKIP' }
            else {
                Add-SmokeResult "[FAIL] hardening:exe - not found: $gameExe" 'FAIL'
                $bad++
            }
        }
        else {
            $imp = (& (Find-Dumpbin $Platform) /imports $gameExe) -join "`n"
            if ($LASTEXITCODE -ne 0) {
                Add-SmokeResult "[FAIL] hardening:dumpbin - exit $LASTEXITCODE" 'FAIL'
                $bad++
            }
            else {
                foreach ($c in @(Get-ImportChecks $imp $Platform) + @(Get-UnicowsCheck $root $Platform $exeDir $imp) + @(Get-D3dxPinsCheck $root) + @(Get-SteamApi64PinsCheck $root)) {
                    if ($c.Ok) { Add-SmokeResult "[PASS] hardening:$($c.Name)" 'PASS' }
                    else {
                        Add-SmokeResult "[FAIL] hardening:$($c.Name) - $($c.Detail)" 'FAIL'
                        $bad++
                    }
                }
                $saCheck = Get-SteamApi64StagedCheck $root $exeDir $imp
                Add-SmokeResult (Get-D3dxStagedLine (New-Object PSObject -Property @{ Status = $saCheck.Status; Name = "hardening:$($saCheck.Name)"; Detail = $saCheck.Detail })) $saCheck.Status
                if ($saCheck.Status -eq 'FAIL') { $bad++ }
            }
            if ($SmokeConfiguration -ne 'Release') { Add-SmokeResult "[SKIP] hardening:fma3-off - judged on the Release exe, smoke configuration is $SmokeConfiguration" 'SKIP' }
            else {
                $fc = $null
                try { $fc = Get-Fma3OffCheck (Get-Fma3OffDisasm $gameExe ([IO.File]::ReadAllText((Join-Path $gameProjects[0].IntDir 'BattlefrontII.map'))) $Platform) }
                catch { $fc = New-Object PSObject -Property @{ Name = 'fma3-off'; Ok = $false; Detail = $_.Exception.Message } }
                if ($fc.Ok) { Add-SmokeResult '[PASS] hardening:fma3-off' 'PASS' }
                else {
                    Add-SmokeResult "[FAIL] hardening:fma3-off - $($fc.Detail)" 'FAIL'
                    $bad++
                }
            }
        }
    }
    elseif (-not (Test-Path -LiteralPath $gameExe)) {
        if ($NoBuild -and -not $Hardening) { Add-SmokeResult '[SKIP] hardening:exe - not built (-NoBuild)' 'SKIP' }
        else {
            Add-SmokeResult "[FAIL] hardening:exe - not found: $gameExe" 'FAIL'
            $bad++
        }
    }
    else {
        $db = Find-Dumpbin $Platform
        $hdr = (& $db /headers $gameExe) -join "`n"
        $dbCode = $LASTEXITCODE
        $cfg = (& $db /loadconfig $gameExe) -join "`n"
        if ($dbCode -eq 0) { $dbCode = $LASTEXITCODE }
        $imp = (& $db /imports $gameExe) -join "`n"
        if ($dbCode -eq 0) { $dbCode = $LASTEXITCODE }
        if ($dbCode -ne 0) {
            Add-SmokeResult "[FAIL] hardening:dumpbin - exit $dbCode" 'FAIL'
            $bad++
        }
        else {
            $gameProjects = @(Get-GameProjects $root $SmokeConfiguration)
            $tl = Get-ClTlogCounts @($gameProjects | ForEach-Object { $_.TlogDir })
            foreach ($c in (Get-HardeningChecks $hdr $cfg $tl.Total $tl.GsMinus $tl.Sdl)) {
                if ($c.Ok) { Add-SmokeResult "[PASS] hardening:$($c.Name)" 'PASS' }
                else {
                    Add-SmokeResult "[FAIL] hardening:$($c.Name) - $($c.Detail)" 'FAIL'
                    $bad++
                }
            }
            $ttDir = Join-Path $root "obj\Tests\$Platform\$Configuration\OpenSWBF2Tests.tlog"
            $tt = $null
            if (Test-Path -LiteralPath $ttDir) { $tt = Get-ClTlogCounts $ttDir }
            else { Add-SmokeResult '[SKIP] hardening:flag-alignednew-tests - tests not built' 'SKIP' }
            $mapPath = Join-Path $gameProjects[0].IntDir 'BattlefrontII.map'
            $mapText = $null
            if (Test-Path -LiteralPath $mapPath) { $mapText = [IO.File]::ReadAllText($mapPath) }
            foreach ($c in @(Get-FlagChecks $tl $tt $SmokeConfiguration $Configuration) + @(Get-MiCrtHookCheck $mapText)) {
                if ($c.Ok) { Add-SmokeResult "[PASS] hardening:$($c.Name)" 'PASS' }
                else {
                    Add-SmokeResult "[FAIL] hardening:$($c.Name) - $($c.Detail)" 'FAIL'
                    $bad++
                }
            }
            foreach ($c in @(Get-ImportChecks $imp $Platform) + @(Get-UnicowsCheck $root $Platform $exeDir $imp) + @(Get-D3dxPinsCheck $root) + @(Get-SteamApi64PinsCheck $root)) {
                if ($c.Ok) { Add-SmokeResult "[PASS] hardening:$($c.Name)" 'PASS' }
                else {
                    Add-SmokeResult "[FAIL] hardening:$($c.Name) - $($c.Detail)" 'FAIL'
                    $bad++
                }
            }
            $testsExe = $env:OPENSWBF2_TEST_EXE
            if (-not $testsExe) { $testsExe = Join-Path $root "bin\Tests\$Platform\$Configuration\OpenSWBF2Tests.exe" }
            $aslrExes = @(, @('aslr-pe', $gameExe))
            if (Test-Path -LiteralPath $testsExe) { $aslrExes += , @('aslr-pe-tests', $testsExe) }
            foreach ($ae in $aslrExes) {
                $why = Get-PeAslr $ae[1]
                if (-not $why) { Add-SmokeResult "[PASS] hardening:$($ae[0])" 'PASS' }
                else {
                    Add-SmokeResult "[FAIL] hardening:$($ae[0]) - $why" 'FAIL'
                    $bad++
                }
            }
            if ($aslrExes.Count -eq 1) { Add-SmokeResult '[SKIP] hardening:aslr-pe-tests - tests not built' 'SKIP' }
        }
    }
    New-Item -ItemType Directory -Force -Path $hardDir | Out-Null
    @($results | Where-Object { $_.Line -match 'hardening:' } | ForEach-Object { $_.Line }) | Set-Content -Path (Join-Path $hardDir 'hardening.txt') -Encoding ASCII
}
elseif ($hardenActive) {
    $hardFile = Join-Path $hardDir 'hardening.txt'
    if (-not (Test-Path -LiteralPath $hardFile -PathType Leaf)) {
        Add-SmokeResult '[FAIL] hardening - no archived output' 'FAIL'
        $bad++
    }
    else {
        foreach ($hl in [IO.File]::ReadAllLines($hardFile)) {
            $ht = $(if ($hl -cmatch '^\[(PASS|FAIL|SKIP)\] ') { $Matches[1] } else { 'FAIL' })
            Add-SmokeResult $hl $ht
            if ($ht -eq 'FAIL') { $bad++ }
        }
    }
}
if ($hardenActive) {
    if ($Platform -ne 'Win32') { Write-Host "[SKIP] tables:hardening - platform is $Platform, owned by Phase 52 HARDX-01" }
    elseif ($Configuration -eq 'Release' -and $SmokeConfiguration -eq 'Release') {
        & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $testsDir 'baseline\compare-tables.ps1') -Platform $Platform -Tables $hardDir -Hardening
        $hardCode = $LASTEXITCODE
        $hardLine = '[PASS] tables:hardening'
        if ($hardCode -ne 0) {
            $hardLine = "[FAIL] tables:hardening - exit $hardCode"
            $bad++
        }
        Add-SmokeResult $hardLine $(if ($hardCode -eq 0) { 'PASS' } else { 'FAIL' })
    }
    else { Write-Host "[SKIP] tables:hardening - configuration is $Configuration, smoke configuration is $SmokeConfiguration" }
}

if ($smokeActive) {
    function Get-SmokeCaseAsan($caseDir) {
        if ($asanSmoke) { return (Get-AsanOptions $caseDir) }
        return $null
    }
    function Invoke-SmokeCaseJudge($sc, $res) {
        $caseDir = $res.CaseDir
        if ($res.Class -ceq 'Skipped') {
            Add-SmokeResult (Get-SmokeSkipLine $sc.Name $res.SkipReason) 'SKIP'
            return
        }
        if ($sc.Expect.Perturb) {
            $pClass = $res.Class
            if ($res.Asan) {
                $pClass = 'Asan'
                foreach ($rl in ($res.Asan -split "`r?`n")) { Write-Host "    $rl" }
            }
            $pMiss = @(Get-PerturbProofMissing $sc $res.LogText)
            if ($pMiss.Count -gt 0) {
                Add-SmokeResult ('[FAIL] smoke:{0} class={1} ({2} ms) - run not proven, log lacks: {3}' -f $sc.Name, $pClass, $res.Ms, ($pMiss -join ' | ')) 'FAIL'
                Write-Host "    details: $caseDir"
                $script:bad++
            }
            else { Add-SmokeResult ('[INFO] smoke:{0} class={1} ({2} ms)' -f $sc.Name, $pClass, $res.Ms) 'INFO' }
            $script:perturbRuns += New-Object PSObject -Property @{ Case = $sc; Class = $pClass; Log = (Join-Path $caseDir 'run.log'); Proven = ($pMiss.Count -eq 0) }
            return
        }
        $ok = Test-SmokeExpect $sc $res
        $tok = Get-SmokeToken $res.Class
        if ($ok) { $tok = 'PASS' }
        elseif ($tok -eq 'PASS') { $tok = 'FAIL' }
        if ($res.Asan) { $tok = 'FAIL' }
        $line = ('[{0}] smoke:{1} class={2} exit=0x{3:X8} ({4} ms)' -f $tok, $sc.Name, $res.Class, $res.Code, $res.Ms)
        $owned = $null
        if (-not $res.Asan) { $owned = Get-SmokeOwnedVerdict $sc $res $ok }
        if ($owned -and $owned.Line) {
            $tok = $owned.Tok
            $line = $owned.Line
        }
        elseif ($owned) { $line += $owned.Note }
        Add-SmokeResult $line $tok
        if ($tok -ne 'PASS' -and $tok -ne 'OWNED') {
            Write-Host "    details: $caseDir"
            $script:bad++
        }
        if ($res.Asan) { foreach ($rl in ($res.Asan -split "`r?`n")) { Write-Host "    $rl" } }
        if ($sc.Twice -gt 2) { $script:scoreKeys[$sc.Twice] = Get-DigestKey $caseDir }
        elseif ($sc.Twice -gt 0) { $script:scoreKeys[$sc.Twice] = Get-FinalScoreKey $caseDir }
    }
    function Invoke-SmokeRunChecks {
        foreach ($pair in @(@(1, 2, 'run-twice'), @(3, 4, 'run-twice-digest'))) {
            if ($script:scoreKeys.ContainsKey($pair[0]) -and $script:scoreKeys.ContainsKey($pair[1])) {
                if ($script:scoreKeys[$pair[0]] -and $script:scoreKeys[$pair[0]] -eq $script:scoreKeys[$pair[1]]) { Add-SmokeResult "[PASS] smoke:$($pair[2]) match" 'PASS' }
                else {
                    Add-SmokeResult "[FAIL] smoke:$($pair[2]) mismatch" 'FAIL'
                    $script:bad++
                }
            }
        }
        $layoutRef = @(Get-Turn1500Runtime (Join-Path $outDir "smoke\$script:SmokeStandardMission"))
        foreach ($ln in 'shell-first', 'repeat-digest') {
            $ld = Join-Path $outDir "smoke\$ln"
            if ($layoutRef.Count -lt 1 -or -not (Test-Path -LiteralPath $ld)) { continue }
            $lg = @(Get-Turn1500Runtime $ld)
            if ($lg.Count -gt 0 -and $lg[-1] -ceq $layoutRef[0]) { Add-SmokeResult "[PASS] smoke:$ln-layout Runtime at turn 1500 matches $($script:SmokeStandardMission) ($($layoutRef[0]))" 'PASS' }
            else {
                Add-SmokeResult "[FAIL] smoke:$ln-layout Runtime at turn 1500 '$(if ($lg.Count) { $lg[-1] })' differs from $($script:SmokeStandardMission) '$($layoutRef[0])'" 'FAIL'
                $script:bad++
            }
        }
        if ($script:perturbRuns.Count -gt 0) {
            $pRes = Get-PerturbResults $script:perturbRuns
            foreach ($pMsg in $pRes.Messages) {
                if ($pMsg.Tok) { Add-SmokeResult $pMsg.Line $pMsg.Tok }
                else { Write-Host $pMsg.Line }
                if ($pMsg.Tok -eq 'FAIL') { $script:bad++ }
            }
            $pRes.Rows | Set-Content -Path (Join-Path $outDir $(if ($Rejudge) { 'perturb-rejudge.tsv' } else { 'perturb.tsv' })) -Encoding ASCII
        }
        $traceLog1 = Join-Path $outDir 'smoke\trace-seed-1\run.log'
        $traceLog2 = Join-Path $outDir 'smoke\trace-seed-2\run.log'
        if ((Test-Path -LiteralPath $traceLog1) -and (Test-Path -LiteralPath $traceLog2)) {
            $t = Compare-DigestTrace $traceLog1 $traceLog2
            if ($t -gt 0) { Add-SmokeResult "[PASS] smoke:digest-trace split at turn $t" 'PASS' }
            elseif ($t -lt 0) {
                Add-SmokeResult '[FAIL] smoke:digest-trace incomplete trace' 'FAIL'
                $script:bad++
            }
            else {
                Add-SmokeResult '[FAIL] smoke:digest-trace no split' 'FAIL'
                $script:bad++
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
                $script:bad++
            }
        }
    }
    $script:scoreKeys = @{}
    $script:perturbRuns = @()
    if ($Rejudge) {
        foreach ($sc in $smokeCases) {
            $res = Read-SmokeCaseArchive (Join-Path $outDir "smoke\$($sc.Name)") $asanSmoke
            if ($res.Problem) {
                Add-SmokeResult "[FAIL] smoke:$($sc.Name) - $($res.Problem)" 'FAIL'
                $bad++
                continue
            }
            Invoke-SmokeCaseJudge $sc $res
        }
        Invoke-SmokeRunChecks
    }
    elseif ($reason) {
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
        $batch = $null
        Write-Host "INFO: smoke jobs=$workerCount workers=$($WorkerBase + 1)..$($WorkerBase + $workerCount)"
        $items = @()
        foreach ($sc in $smokeCases) {
            $caseDir = Join-Path $outDir "smoke\$($sc.Name)"
            $items += New-Object PSObject -Property @{ Name = $sc.Name; ExeName = 'BattlefrontII.exe'; ArgLine = $sc.Args.Replace('{CASEDIR}', $caseDir) + $extraArg; CaseDir = $caseDir; AsanOptions = (Get-SmokeCaseAsan $caseDir); Omit = $sc.Omit; Swap = $sc.Swap; NeedsFile = $sc.NeedsFile; SkipReason = $sc.SkipReason; Then = @(Get-SmokeThenSteps $sc $caseDir $extraArg ([bool]$asanSmoke)) }
        }
        try {
            foreach ($d in $workDirs) { New-SmokeWorkerDir $exeDir $d }
            $batch = Start-SmokeBatch $items $workDirs $exeDir
            $caseIndex = -1
            foreach ($sc in $smokeCases) {
                $caseIndex++
                Invoke-SmokeCaseJudge $sc (Receive-SmokeCase $batch $caseIndex)
            }
        }
        finally {
            if ($batch) { Stop-SmokeBatch $batch }
            foreach ($d in $workDirs) { Remove-HarnessTree $d }
        }
        Invoke-SmokeRunChecks
    }
    if ($Smoke -eq 'Vendor') {
        $vsFile = Join-Path $outDir 'vendor-d3dx9-staged.txt'
        if (-not $Rejudge) { Get-D3dxStagedLine (Get-D3dxStagedCheck $root $Platform (Join-Path $root "bin\$Platform\$SmokeConfiguration")) | Set-Content -LiteralPath $vsFile -Encoding ASCII }
        $vsLine = '[FAIL] vendor:d3dx9-staged - no archived output'
        if (Test-Path -LiteralPath $vsFile -PathType Leaf) { $vsLine = [IO.File]::ReadAllText($vsFile).Trim() }
        $vsTok = $(if ($vsLine -cmatch '^\[(PASS|FAIL|SKIP)\] vendor:d3dx9-staged') { $Matches[1] } else { 'FAIL' })
        Add-SmokeResult $vsLine $vsTok
        if ($vsTok -eq 'FAIL') { $bad++ }
    }
}

function Invoke-TableCapture([string]$dir, [string[]]$captureArgs) {
    if (-not $Rejudge) {
        & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $testsDir 'baseline\capture-vs2013.ps1') @captureArgs | Out-Host
        return $LASTEXITCODE
    }
    if (Test-Path -LiteralPath $dir -PathType Container) { return 0 }
    Write-Host "    no archived output: $dir"
    return 1
}

$tablesSkip = $null
$compareSkip = $null
$tableModeSkip = Get-TableModeSkip $Platform $HogAddrSpace.IsPresent
if (-not $smokeActive) { $tablesSkip = 'no smoke run' }
elseif ($Smoke -ne 'All') { $tablesSkip = "smoke group is $Smoke" }
elseif ($SmokeConfiguration -ne 'Release') { $tablesSkip = "smoke configuration is $SmokeConfiguration" }
elseif ($MiPurge -ge 0) { $tablesSkip = '-MiPurge' }
elseif ($Movies) { $tablesSkip = '-Movies' }
elseif ($tableModeSkip) { $compareSkip = $tableModeSkip }
if ($tablesSkip) { Write-Host "[SKIP] tables:compare - $tablesSkip" }
elseif ($compareSkip) {
    $tablesDir = Join-Path $outDir 'tables'
    $tablesCode = Invoke-TableCapture $tablesDir @('-Platform', $Platform, '-Tables', '-SmokeAllDir', $outDir, '-OutDir', $tablesDir)
    if ($tablesCode -eq 0) { Write-Host "[SKIP] tables:compare - $compareSkip" }
    else {
        $tablesLine = "[FAIL] tables:capture exit $tablesCode"
        $bad++
        Write-Host $tablesLine
        $results += New-Object PSObject -Property @{ Line = $tablesLine; Final = 'FAIL'; Known = $false }
    }
}
else {
    $tablesDir = Join-Path $outDir 'tables'
    $tablesCode = Invoke-TableCapture $tablesDir @('-Platform', $Platform, '-Tables', '-SmokeAllDir', $outDir, '-OutDir', $tablesDir)
    if ($tablesCode -eq 0) {
        & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $testsDir 'baseline\compare-tables.ps1') -Platform $Platform -Tables $tablesDir
        $tablesCode = $LASTEXITCODE
    }
    $tablesLine = '[PASS] tables:compare'
    if ($tablesCode -ne 0) {
        $tablesLine = "[FAIL] tables:compare exit $tablesCode"
        $bad++
    }
    Write-Host $tablesLine
    $results += New-Object PSObject -Property @{ Line = $tablesLine; Final = $(if ($tablesCode -eq 0) { 'PASS' } else { 'FAIL' }); Known = $false }
}
if ($smokeActive -and $Smoke -eq 'All' -and $SmokeConfiguration -eq 'Test' -and -not $tableModeSkip -and $MiPurge -lt 0 -and -not $Movies) {
    $luaDir = Join-Path $outDir 'lua-bins'
    $luaCapCode = Invoke-TableCapture $luaDir @('-Platform', $Platform, '-Configuration', 'Test', '-Tables', '-LuaBinsDir', $outDir, '-OutDir', $luaDir)
    $luaCode = $luaCapCode
    if ($luaCode -eq 0) {
        & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $testsDir 'baseline\compare-tables.ps1') -Platform $Platform -Tables $luaDir -LuaBins
        $luaCode = $LASTEXITCODE
    }
    $luaLine = '[PASS] tables:lua-bins'
    if ($luaCode -ne 0) {
        $luaLine = "[FAIL] tables:lua-bins exit $luaCode"
        $bad++
    }
    Write-Host $luaLine
    $results += New-Object PSObject -Property @{ Line = $luaLine; Final = $(if ($luaCode -eq 0) { 'PASS' } else { 'FAIL' }); Known = $false }
    $histCode = $luaCapCode
    if ($histCode -eq 0) {
        & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $testsDir 'baseline\compare-tables.ps1') -Platform $Platform -Tables $luaDir -LuaHist
        $histCode = $LASTEXITCODE
    }
    $histLine = '[PASS] tables:lua-hist'
    if ($histCode -ne 0) {
        $histLine = "[FAIL] tables:lua-hist exit $histCode"
        $bad++
    }
    Write-Host $histLine
    $results += New-Object PSObject -Property @{ Line = $histLine; Final = $(if ($histCode -eq 0) { 'PASS' } else { 'FAIL' }); Known = $false }
    $offDir = Join-Path $outDir 'offthread'
    $offCode = Invoke-TableCapture $offDir @('-Platform', $Platform, '-Configuration', 'Test', '-Tables', '-OffThreadDir', $outDir, '-OutDir', $offDir)
    if ($offCode -eq 0) {
        & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $testsDir 'baseline\compare-tables.ps1') -Platform $Platform -Tables $offDir -OffThread
        $offCode = $LASTEXITCODE
    }
    $offLine = '[PASS] tables:offthread'
    if ($offCode -ne 0) {
        $offLine = "[FAIL] tables:offthread exit $offCode"
        $bad++
    }
    Write-Host $offLine
    $results += New-Object PSObject -Property @{ Line = $offLine; Final = $(if ($offCode -eq 0) { 'PASS' } else { 'FAIL' }); Known = $false }
}

if (-not $Rejudge) { Add-HostFaultParams (Join-Path $outDir 'run-params.txt') 'end' $hostFaultStart }
$hostFaultNote = Get-HostFaultNote (Read-KeyValueFile (Join-Path $outDir 'run-params.txt')) $bad
$normal = @($results | Where-Object { -not $_.Known -and $_.Final -ne 'WARN' })
$kn = @($results | Where-Object { $_.Known -and $_.Final -ne 'WARN' })
$summary = @()
$summary += ''
$summary += 'Results'
foreach ($t in 'PASS', 'FAIL', 'SKIP', 'STALL', 'CRASH', 'OWNED') {
    $summary += ('  {0,-8} {1}' -f $t, @($normal | Where-Object { $_.Final -eq $t }).Count)
}
$summary += 'Known bugs'
$summary += ('  {0,-8} {1}' -f 'KNOWN', @($kn | Where-Object { $_.Final -eq 'KNOWN' }).Count)
$summary += ('  {0,-8} {1}' -f 'PASS', @($kn | Where-Object { $_.Final -eq 'PASS' }).Count)
if ($asanT1 -or $asanSmoke) { $summary += $script:SmokeAsanCoverage }
if ($smokeActive -and $asanSmoke) {
    if ($asanRemoved.Count -gt 0) { $summary += "ASan excluded smoke cases: $($asanRemoved -join ', ')" }
    else { $summary += 'ASan excluded smoke cases: none in this group' }
}
if ($hostFaultNote) { $summary += $hostFaultNote }
$summary | ForEach-Object { Write-Host $_ }
(@($results | ForEach-Object { $_.Line }) + $summary) | Set-Content -Path (Join-Path $outDir $resultsName) -Encoding ASCII
Write-Host "Artifacts: $outDir"

if ($bad -gt 0) { exit 1 }
exit 0
