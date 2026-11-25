param(
    [ValidateSet('Debug','Release','Test','Asan')][string]$Configuration = 'Debug',
    [ValidateSet('Win32','x64')][string]$Platform = 'Win32',
    [switch]$NoBuild,
    [string]$Suite,
    [string]$Filter,
    [switch]$Fuzz,
    [int]$Seed = 1,
    [int]$Iterations = 100000,
    [ValidateSet('None','Standard','Dedicated','Crash','All','Twice','Perturb','Soak','Vendor','Boot','SaveCross')][string]$Smoke = 'Standard',
    [ValidateSet('Debug','Release','Test','Asan')][string]$SmokeConfiguration = 'Release',
    [switch]$Hardening,
    [switch]$NoHardening,
    [ValidateRange(0,64)][int]$Jobs = 0,
    [switch]$Asan,
    [switch]$Clean,
    [ValidateSet('Short','Long')][string]$Soak = 'Short',
    [string]$Case
)
$ErrorActionPreference = 'Stop'
$testsDir = $PSScriptRoot
$root = Split-Path $testsDir -Parent
. (Join-Path $testsDir 'hardening.ps1')
. (Join-Path $testsDir 'gate\keys.ps1')
$paramNames = @($MyInvocation.MyCommand.Parameters.Keys)
$bound = @($PSBoundParameters.Keys)
$rawParams = [ordered]@{}
foreach ($pn in $paramNames) { $rawParams[$pn] = [string](Get-Variable -Name $pn -ValueOnly) }
if ($Jobs -eq 0) { $Jobs = [Math]::Max(1, [Math]::Min(8, [int][Math]::Floor([Environment]::ProcessorCount / 2))) }
$script:SmokePlatform = $Platform
Write-ToolBanner $Platform
if ($Asan) {
    if ($bound -contains 'Configuration' -and $Configuration -ne 'Asan') { Write-Host "-Asan conflicts with -Configuration $Configuration."; exit 2 }
    $Configuration = 'Asan'
}
if ($Configuration -in 'Test', 'Asan' -and $bound -notcontains 'SmokeConfiguration') { $SmokeConfiguration = $Configuration }
$asanT1 = ($Configuration -eq 'Asan')
$asanSmoke = ($SmokeConfiguration -eq 'Asan')
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
$hardenActive = $Hardening -or ($smokeActive -and -not $NoHardening)
$gameRunTags = "*[$Smoke]*"

$outDir = New-StampedDir (Join-Path $root 'artifacts\tests')
$results = @()
$bad = 0

if ($runUnit) {
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

Set-Content -Path (Join-Path $root 'artifacts\.gitignore') -Value '*' -Encoding ASCII
$testsExePath = $env:OPENSWBF2_TEST_EXE
if (-not $testsExePath) { $testsExePath = Join-Path $root "bin\Tests\$Platform\$Configuration\OpenSWBF2Tests.exe" }
$paramLines = @(foreach ($pn in $paramNames) { "param.$pn=$($rawParams[$pn])" })
$paramLines += "bound=$($bound -join ',')"
$paramLines += "platform=$Platform"
$paramLines += "configuration=$Configuration"
$paramLines += "smoke_configuration=$SmokeConfiguration"
$paramLines += "wer_localdumps=$([int][bool](Test-WerKey))"
$paramLines += "steam=$(Get-SteamRunning)"
$paramLines += "tests_exe=$testsExePath"
$paramLines += "exe.OpenSWBF2Tests.exe=$(Get-RunExeHash $testsExePath)"
$paramLines | Set-Content -Path (Join-Path $outDir 'run-params.txt') -Encoding ASCII
$hostFaultStart = Get-Date
Add-HostFaultParams (Join-Path $outDir 'run-params.txt') 'start' $hostFaultStart

function Add-SmokeResult($line, $final) {
    Write-Host $line
    $script:results += New-Object PSObject -Property @{ Line = $line; Final = $final }
}

function Invoke-TestRunner([string]$runnerExe, [string[]]$runnerArgs, [string]$reportDir) {
    $lines = New-Object System.Collections.Generic.List[string]
    Push-Location -LiteralPath $testsDir
    try {
        & $runnerExe @runnerArgs | ForEach-Object {
            $l = [string]$_
            Write-Host $l
            if ($l -cmatch '^\[(PASS|FAIL|SKIP|STALL|CRASH|N/A)\] ') { $lines.Add($l) }
        }
        $code = $LASTEXITCODE
    }
    finally { Pop-Location }
    $index = $null
    try { $index = [IO.File]::ReadAllText((Join-Path $reportDir 'index.json')) | ConvertFrom-Json }
    catch { $index = $null }
    return New-Object PSObject -Property @{ Code = $code; Lines = $lines; Index = $index }
}

function Add-RunnerResults($lines) {
    foreach ($l in $lines) {
        $tok = $(if ($l -cmatch '^\[(PASS|FAIL|SKIP|STALL|CRASH)\] ') { $Matches[1] } else { 'SKIP' })
        $script:results += New-Object PSObject -Property @{ Line = $l; Final = $tok }
    }
}

if ($runUnit) {
    $unitDir = Join-Path $outDir 'unit'
    $runnerArgs = @('--runner', "--out=$unitDir", "--platform=$Platform", "--config=$Configuration", "--exclude=$(Join-Path $testsDir 'excluded.tsv')")
    if ($Fuzz) { $runnerArgs += @('--tags=*[StressFilter][OnRequest]*', "--fuzz-seed=$Seed", "--fuzz-count=$Iterations") }
    elseif ($Suite) { $runnerArgs += "--tags=$Suite" }
    if ($Filter) { $runnerArgs += "--name=$Filter" }
    $t1 = Invoke-TestRunner $exe $runnerArgs $unitDir
    if ($t1.Code -ge 2) { Write-Host "The test runner exited $($t1.Code)."; exit 2 }
    if (-not $t1.Index) { Write-Host "The test runner left no readable $(Join-Path $unitDir 'index.json')."; exit 2 }
    Add-RunnerResults $t1.Lines
    if ($t1.Code -ne 0) { $bad++ }
}

$hardDir = Join-Path $outDir 'hardening'
if ($hardenActive -or $smokeActive) {
    Assert-Runtimes (Get-GameRuntimes $Platform @($(if ($smokeActive) { $Smoke } else { 'None' }))) $Platform
    $exeDir = Join-Path $root "bin\$Platform\$SmokeConfiguration"
    $gameExe = Join-Path $exeDir 'BattlefrontII.exe'
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
}
if ($hardenActive) {
    $vendorHits = @(Get-ChildItem -LiteralPath (Join-Path $root 'src\LuaFL'), (Join-Path $root 'src\ZlibFL'), (Join-Path $root 'src\mimalloc'), (Join-Path $root 'src\FFmpegFL') -Recurse -File -ErrorAction SilentlyContinue |
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
                foreach ($c in @(Get-ImportChecks $imp $Platform) + @(Get-UnicowsCheck $root $Platform $exeDir $imp) + @(Get-BinkGoneCheck $root $exeDir $imp) + @(Get-D3dxPinsCheck $root) + @(Get-SteamApi64PinsCheck $root)) {
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
            foreach ($c in @(Get-ImportChecks $imp $Platform) + @(Get-UnicowsCheck $root $Platform $exeDir $imp) + @(Get-BinkGoneCheck $root $exeDir $imp) + @(Get-D3dxPinsCheck $root) + @(Get-SteamApi64PinsCheck $root)) {
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
    foreach ($c in (Get-FFmpegChecks $root $Platform $SmokeConfiguration)) {
        if ($c.Ok) { Add-SmokeResult "[PASS] hardening:$($c.Name)" 'PASS' }
        else {
            Add-SmokeResult "[FAIL] hardening:$($c.Name) - $($c.Detail)" 'FAIL'
            $bad++
        }
    }
    New-Item -ItemType Directory -Force -Path $hardDir | Out-Null
    @($results | Where-Object { $_.Line -match 'hardening:' } | ForEach-Object { $_.Line }) | Set-Content -Path (Join-Path $hardDir 'hardening.txt') -Encoding ASCII
}
if ($smokeActive) {
    if ($Smoke -eq 'Vendor') {
        $vsLine = Get-D3dxStagedLine (Get-D3dxStagedCheck $root $Platform $exeDir)
        [IO.File]::WriteAllText((Join-Path $outDir 'vendor-d3dx9-staged.txt'), $vsLine + "`r`n")
        $vsTok = $(if ($vsLine -cmatch '^\[(PASS|FAIL|SKIP)\] vendor:d3dx9-staged') { $Matches[1] } else { 'FAIL' })
        Add-SmokeResult $vsLine $vsTok
        if ($vsTok -eq 'FAIL') { $bad++ }
    }
    $gameTestsExe = $env:OPENSWBF2_TEST_EXE
    if (-not $gameTestsExe) { $gameTestsExe = Join-Path $root "bin\Tests\$Platform\$Configuration\OpenSWBF2Tests.exe" }
    if (-not $NoBuild) {
        $msbuild = Find-MSBuild
        & $msbuild (Join-Path $testsDir 'OpenSWBF2Tests.vcxproj') "/p:Configuration=$Configuration" "/p:Platform=$Platform" /m /v:minimal /nologo
        if ($LASTEXITCODE -ne 0) { Write-Host 'Build failed.'; exit 2 }
    }
    if (-not (Test-Path -LiteralPath $gameTestsExe -PathType Leaf)) {
        Add-SmokeResult "[FAIL] game runs $gameRunTags - tests exe not found: $gameTestsExe" 'FAIL'
        $bad++
    }
    else {
        foreach ($w in (Test-WerSetup)) { Write-Host "WARNING: $w" }
        $gameArgs = @('--runner', "--out=$outDir", "--platform=$Platform", "--config=$SmokeConfiguration", "--exclude=$(Join-Path $testsDir 'excluded.tsv')", "--tags=$gameRunTags", "--game-dir=$exeDir", "--jobs=$Jobs", "--soak=$Soak")
        if ($Case) { $gameArgs += "--name=$Case" }
        $gr = Invoke-TestRunner $gameTestsExe $gameArgs $outDir
        Add-RunnerResults $gr.Lines
        if ($gr.Code -ge 2) { Add-SmokeResult "[FAIL] game runs $gameRunTags - the test runner exited $($gr.Code)" 'FAIL' }
        elseif (-not $gr.Index) { Add-SmokeResult "[FAIL] game runs $gameRunTags - no readable $(Join-Path $outDir 'index.json')" 'FAIL' }
        if ($gr.Code -ne 0 -or -not $gr.Index) { $bad++ }
    }
}

function Invoke-TableCompare([string]$Label, [string]$Folder, [string[]]$CaptureArgs, [string[]]$CompareArgs) {
    $dir = Join-Path $outDir $Folder
    if (-not (Test-Path -LiteralPath $dir -PathType Container)) {
        & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $testsDir 'baseline\capture-vs2013.ps1') -Platform $Platform -Tables @CaptureArgs $outDir -OutDir $dir | Out-Host
        $code = $LASTEXITCODE
    }
    else { $code = 0 }
    if ($code -eq 0) {
        & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $testsDir 'baseline\compare-tables.ps1') -Platform $Platform -Tables $dir @CompareArgs
        $code = $LASTEXITCODE
    }
    if ($code -eq 0) { Add-SmokeResult "[PASS] tables:$Label" 'PASS' }
    else {
        Add-SmokeResult "[FAIL] tables:$Label exit $code" 'FAIL'
        $script:bad++
    }
}

if (-not $smokeActive) { Write-Host '[SKIP] tables:compare - no smoke run' }
elseif ($Smoke -ne 'All') { Write-Host "[SKIP] tables:compare - smoke group is $Smoke" }
elseif ($SmokeConfiguration -eq 'Release') { Invoke-TableCompare 'compare' 'tables' @('-SmokeAllDir') @() }
elseif ($SmokeConfiguration -eq 'Test') {
    Invoke-TableCompare 'lua-bins' 'lua-bins' @('-Configuration', 'Test', '-LuaBinsDir') @('-LuaBins')
    Invoke-TableCompare 'lua-hist' 'lua-bins' @('-Configuration', 'Test', '-LuaBinsDir') @('-LuaHist')
    Invoke-TableCompare 'offthread' 'offthread' @('-Configuration', 'Test', '-OffThreadDir') @('-OffThread')
}
else { Write-Host "[SKIP] tables:compare - smoke configuration is $SmokeConfiguration" }

Add-HostFaultParams (Join-Path $outDir 'run-params.txt') 'end' $hostFaultStart
$hostFaultNote = Get-HostFaultNote (Read-KeyValueFile (Join-Path $outDir 'run-params.txt')) $bad
$normal = @($results)
$summary = @()
$summary += ''
$summary += 'Results'
foreach ($t in 'PASS', 'FAIL', 'SKIP', 'STALL', 'CRASH') {
    $summary += ('  {0,-8} {1}' -f $t, @($normal | Where-Object { $_.Final -eq $t }).Count)
}
if ($asanT1 -or $asanSmoke) { $summary += $script:SmokeAsanCoverage }
if ($hostFaultNote) { $summary += $hostFaultNote }
$summary | ForEach-Object { Write-Host $_ }
(@($results | ForEach-Object { $_.Line }) + $summary) | Set-Content -Path (Join-Path $outDir 'results.txt') -Encoding ASCII
Write-Host "Artifacts: $outDir"
$named = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
foreach ($rf in @(Get-ChildItem -LiteralPath (Join-Path $root 'artifacts\gate') -Recurse -File -Filter 'runs.txt' -ErrorAction SilentlyContinue)) {
    foreach ($rl in [IO.File]::ReadAllLines($rf.FullName)) { if ($rl.Trim()) { [void]$named.Add([IO.Path]::GetFullPath($rl.Trim()).TrimEnd('\')) } }
}
$runDirs = @(Get-ChildItem -LiteralPath (Join-Path $root 'artifacts\tests') -Directory | Sort-Object CreationTimeUtc, Name)
foreach ($rd in @($runDirs | Select-Object -First ([Math]::Max(0, $runDirs.Count - 3)))) {
    if ($rd.FullName -ne $outDir -and -not $named.Contains($rd.FullName)) { Remove-HarnessTree $rd.FullName }
}

if ($bad -gt 0) { exit 1 }
exit 0
