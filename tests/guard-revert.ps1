param(
    [string[]]$Row,
    [string]$OutDir,
    [string]$WorktreeRoot,
    [int]$BuildJobs = 4,
    [int]$Workers = [Math]::Max(1, [int][Math]::Floor([Environment]::ProcessorCount / [Math]::Max(1, $BuildJobs))),
    [int]$WorkerBase = 0,
    [string]$RowsFile,
    [ValidateSet('', 'Debug', 'Release', 'Test')][string]$Configuration = '',
    [ValidateSet('Win32', 'x64')][string]$Platform = 'Win32',
    [switch]$Full,
    [switch]$ListSelected,
    [string]$ProvenFile,
    [string]$Resume,
    [int]$WorkerIndex = 0,
    [ValidateSet('', 'control', 'rows')][string]$WorkerPhase = '',
    [string]$WorkerRun
)
$ErrorActionPreference = 'Stop'
if ($Row) { $Row = @($Row | ForEach-Object { $_ -split ',' }) }
$testsDir = $PSScriptRoot
$root = Split-Path $testsDir -Parent
if (-not $RowsFile) { $RowsFile = Join-Path $testsDir 'guardrevert\rows.ps1' }
if (-not $ProvenFile) { $ProvenFile = Join-Path $testsDir 'guardrevert\proven.tsv' }
$ProvenFile = [IO.Path]::GetFullPath($ProvenFile)
$script:ProofColumns = @('Id', 'RowHash', 'DepDigest', 'TriggerDigest', 'Toolset', 'Tree', 'Commit', 'Result', 'Control', 'Config', 'Platform', 'Date')
$script:LedgerColumns = $script:ProofColumns + @('Outcome')
if ($BuildJobs -lt 1) { Write-Host "-BuildJobs $BuildJobs must be at least 1."; exit 2 }
if ($Workers -lt 1) { Write-Host "-Workers $Workers must be at least 1."; exit 2 }
if (-not $WorktreeRoot) { $WorktreeRoot = "$root-worktrees" }
$WorktreeRoot = [IO.Path]::GetFullPath($WorktreeRoot).TrimEnd('\')
if ($env:TEMP -and $WorktreeRoot.StartsWith([IO.Path]::GetFullPath($env:TEMP), [StringComparison]::OrdinalIgnoreCase)) { Write-Host 'WorktreeRoot must not be under TEMP: MSBuild incremental builds miss header changes there.'; exit 2 }
. (Join-Path $testsDir 'common.ps1')
$script:SmokePlatform = $Platform
$script:GuardSmokeKinds = @('Smoke', 'Perturb', 'Hardening', 'Tlog')
$script:GuardSmokeLoaded = $false
. (Join-Path $testsDir 'guardrevert\select.ps1')
. (Join-Path $testsDir 'gate\keys.ps1')
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
if ($Resume) {
    $OutDir = [IO.Path]::GetFullPath($Resume)
    if (-not (Test-Path -LiteralPath $OutDir -PathType Container)) { Write-Host "Resume folder not found: $OutDir"; exit 2 }
}
if (-not $OutDir) {
    if ($ListSelected) { $OutDir = Join-Path $root "artifacts\guard-revert\$stamp" }
    else { $OutDir = New-StampedDir (Join-Path $root 'artifacts\guard-revert') }
}
$OutDir = [IO.Path]::GetFullPath($OutDir)
$script:TestsConfiguration = 'Debug'
$script:GameConfiguration = 'Release'
if ($Configuration) { $script:TestsConfiguration = $Configuration; $script:GameConfiguration = $Configuration }
$script:DefaultGameConfiguration = $script:GameConfiguration
$script:SmokeDigestConfiguration = $script:GameConfiguration

function Invoke-Git([string]$dir, [string[]]$gitArgs) {
    $ErrorActionPreference = 'Continue'
    $out = & git -C $dir @gitArgs 2>&1
    if ($LASTEXITCODE -ne 0) { throw "git $($gitArgs -join ' ') failed: $out" }
    return $out
}

function Build-Tests([string]$wt, [string]$logPath, [string]$config = $script:TestsConfiguration) {
    $ErrorActionPreference = 'Continue'
    $env:SWBF2_NOSTAGE = 'true'
    $out = & $script:msbuild (Join-Path $wt 'tests\OpenSWBF2Tests.vcxproj') "/p:Configuration=$config" "/p:Platform=$Platform" '/p:SWBF2_NOSTAGE=true' "/m:$BuildJobs" /v:minimal /nologo 2>&1
    $code = $LASTEXITCODE
    $out | Set-Content -Path $logPath -Encoding ASCII
    return ($code -eq 0)
}

function Build-Game([string]$wt, [string]$logPath, [string]$config = $script:GameConfiguration) {
    $ErrorActionPreference = 'Continue'
    $env:SWBF2_NOSTAGE = 'true'
    $out = & $script:msbuild (Join-Path $wt 'BattlefrontII.vcxproj') "/p:Configuration=$config" "/p:Platform=$Platform" '/p:SWBF2_NOSTAGE=true' "/m:$BuildJobs" /v:minimal /nologo 2>&1
    $code = $LASTEXITCODE
    $out | Set-Content -Path $logPath -Encoding ASCII
    return ($code -eq 0)
}

function Run-Smoke([string]$wt, [string]$tag, $case) {
    $caseDir = Join-Path $OutDir "run-$tag\case"
    New-Item -ItemType Directory -Force -Path $caseDir | Out-Null
    $runDir = Get-WorkerPath $root $Platform $script:GameConfiguration 'guard' ($WorkerBase + $WorkerIndex)
    $srcBin = Join-Path $root "bin\$Platform\$script:GameConfiguration"
    New-SmokeWorkerDir $srcBin $runDir
    try {
        Copy-Item (Join-Path $wt "bin\$Platform\$script:GameConfiguration\BattlefrontII.exe") $runDir -Force
        $argLine = $case.Args.Replace('{CASEDIR}', $caseDir)
        $res = Invoke-SmokeCase $case.Name (Join-Path $runDir 'BattlefrontII.exe') $runDir $argLine $caseDir
    }
    finally { Remove-HarnessTree $runDir }
    $evalCase = $case.PSObject.Copy()
    $evalCase.Expect = $case.Expect.Clone()
    $evalCase.Expect.Remove('SameRunAs')
    $ok = Test-SmokeExpect $evalCase $res
    return New-Object PSObject -Property @{ Ok = $ok; Class = $res.Class; Code = $res.Code; Dumps = @($res.Dumps).Count; LogText = $res.LogText }
}

function Run-Perturb([string]$wt, [string]$tag, [string[]]$missions) {
    $saved = $env:OPENSWBF2_PERTURB_MISSIONS
    $env:OPENSWBF2_PERTURB_MISSIONS = $missions -join ','
    try { $cases = @(Add-MiOptionsExpect @(Add-SerialTieExpect @(Get-SmokeCases 'Perturb' @() $false $null)) -1) }
    finally { $env:OPENSWBF2_PERTURB_MISSIONS = $saved }
    $runDir = Get-WorkerPath $root $Platform $script:GameConfiguration 'perturb' ($WorkerBase + $WorkerIndex)
    New-SmokeWorkerDir (Join-Path $root "bin\$Platform\$script:GameConfiguration") $runDir
    $runs = @()
    try {
        Copy-Item (Join-Path $wt "bin\$Platform\$script:GameConfiguration\BattlefrontII.exe") $runDir -Force
        foreach ($c in $cases) {
            $caseDir = Join-Path $OutDir "run-$tag\$($c.Name)"
            $res = Invoke-SmokeCase $c.Name (Join-Path $runDir 'BattlefrontII.exe') $runDir $c.Args.Replace('{CASEDIR}', $caseDir) $caseDir
            $proven = @(Get-PerturbProofMissing $c $res.LogText).Count -eq 0
            $runs += New-Object PSObject -Property @{ Case = $c; Class = $res.Class; Log = (Join-Path $caseDir 'run.log'); Proven = $proven }
        }
    }
    finally { Remove-HarnessTree $runDir }
    $pr = Get-PerturbResults $runs
    $pr.Rows | Set-Content -Path (Join-Path $OutDir "perturb-$tag.tsv") -Encoding ASCII
    $msgs = @($pr.Messages | ForEach-Object { "    [$tag] $($_.Line.Trim())" })
    foreach ($m in $msgs) { Write-Host $m }
    $state = 'GREEN'
    if (-not $pr.Ok) {
        if (@($pr.Verdicts | Where-Object { $_.Result -eq 'split' }).Count -gt 0) { $state = 'RED' }
        else { $state = 'RED-CRASH' }
    }
    $detail = @($pr.Verdicts | ForEach-Object { "$($_.Mission):$($_.Mode)-$($_.Seed)=$($_.Text -replace ' ', ':')" }) -join ' '
    return New-Object PSObject -Property @{ State = $state; Detail = $detail; Rows = $pr.Rows; Messages = $msgs }
}

function Run-Case([string]$exe, [string]$wt, [string]$name, [string]$asanDir) {
    $isProbe = $name -like 'strictness probe*'
    $argLine = '"-tc=' + ($name -replace '"', '\"') + '"' + $(if ($isProbe) { ' -ns' } else { '' })
    $asanOptions = $null
    if ($asanDir) {
        if (Test-Path -LiteralPath $asanDir) { Remove-Item -LiteralPath $asanDir -Recurse -Force }
        New-Item -ItemType Directory -Force -Path $asanDir | Out-Null
        $asanOptions = Get-AsanOptions $asanDir
    }
    $res = Invoke-T1Exe $exe (Join-Path $wt 'tests') $argLine $asanOptions
    $v = Get-GuardT1State $res $isProbe
    $state = $v.State
    $detail = $v.Detail
    $text = $res.Text
    $report = $null
    if ($asanDir) {
        $report = Get-AsanReport $asanDir $v.Code
        if ($report) {
            $state = 'RED'
            $detail = (@($detail, 'asan report') | Where-Object { $_ }) -join ' '
            $text += "`r`n" + $report
        }
    }
    return New-Object PSObject -Property @{ State = $state; Detail = $detail; Code = $v.Code; Text = $text; Report = $report }
}

function Set-Anchor([string]$path, [string]$anchor, [string]$replacement) {
    $latin = [Text.Encoding]::GetEncoding(28591)
    $bytes = [IO.File]::ReadAllBytes($path)
    $text = $latin.GetString($bytes)
    if ($text.Contains("`r`n")) {
        if (-not $anchor.Contains("`r")) { $anchor = $anchor.Replace("`n", "`r`n") }
        if (-not $replacement.Contains("`r")) { $replacement = $replacement.Replace("`n", "`r`n") }
    }
    $count = 0
    $at = 0
    while (($at = $text.IndexOf($anchor, $at, [StringComparison]::Ordinal)) -ge 0) { $count++; $at += $anchor.Length }
    if ($count -ne 1) { throw "anchor matched $count times in $path" }
    $idx = $text.IndexOf($anchor, [StringComparison]::Ordinal)
    $new = $text.Substring(0, $idx) + $replacement + $text.Substring($idx + $anchor.Length)
    [IO.File]::WriteAllBytes($path, $latin.GetBytes($new))
    return $bytes
}

function Get-TestsProjectGameTUs {
    $xml = [IO.File]::ReadAllText((Join-Path $testsDir 'OpenSWBF2Tests.vcxproj'))
    return @([regex]::Matches($xml, '<ClCompile\s+Include="\.\.\\(src\\[^"]+)"') | ForEach-Object { $_.Groups[1].Value })
}

function Read-GuardProofs([string]$path) {
    return Read-KeyedRecords $path @('Id')
}

function Invoke-GuardLocked([scriptblock]$body) {
    try { [void]$script:GuardMutex.WaitOne() }
    catch {
        $ex = $_.Exception
        if ($ex -isnot [Threading.AbandonedMutexException] -and $ex.InnerException -isnot [Threading.AbandonedMutexException]) { throw }
    }
    try { & $body }
    finally { $script:GuardMutex.ReleaseMutex() }
}

function Add-GuardProof($r, [string]$ctl, [string]$result, [bool]$pass = $true) {
    $d = $script:Decisions[$r.Id]
    $dep = $d.DepDigest
    if ($d.Doubt) { $dep = '-' }
    $date = [DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ssZ')
    $line = @($r.Id, $d.RowHash, $dep, $d.TriggerDigest, $script:Toolset, $script:Tree, $script:Commit, $result, $ctl, $script:ProofConfig, $Platform, $date) -join "`t"
    $outcome = 'FAIL'
    if ($pass) { $outcome = 'PASS' }
    Invoke-GuardLocked {
        [IO.File]::AppendAllText($script:LedgerFile, $line + "`t" + $outcome + "`r`n")
        if ($pass -and $script:TreeClean) { [IO.File]::AppendAllText($script:PartialFile, $line + "`r`n") }
    }
}

function Test-GuardResumed($l, $d) {
    if (-not $l -or -not $d -or $d.Doubt -or $l.Outcome -cne 'PASS') { return $false }
    return ($l.RowHash -ceq $d.RowHash -and $l.DepDigest -ceq $d.DepDigest -and $l.TriggerDigest -ceq $d.TriggerDigest -and $l.Toolset -ceq $script:Toolset -and $l.Tree -ceq $script:Tree -and $l.Config -ceq $script:ProofConfig -and $l.Platform -ceq $Platform)
}

function Merge-GuardProofs {
    if (-not $script:PartialFile -or -not (Test-Path -LiteralPath $script:PartialFile)) { return }
    Merge-KeyedRecords $ProvenFile $script:ProofColumns @('Id') ([IO.File]::ReadAllLines($script:PartialFile))
}

function Get-GuardWorktree([int]$index) {
    return Join-Path $WorktreeRoot "$Platform\w$index"
}

function Sync-GuardWorktree([string]$wt, [string]$patch, [string[]]$untracked) {
    if (Test-Path -LiteralPath $wt) {
        if (-not [IO.File]::Exists((Join-Path $wt '.git'))) { throw "$wt exists and is not a git worktree" }
        $links = @(Get-ReparsePoints $wt)
        if ($links.Count) { throw "REFUSING to reuse ${wt}: reparse points: $($links -join ', ')" }
        $common = [IO.Path]::GetFullPath("$(Invoke-Git $wt @('rev-parse', '--path-format=absolute', '--git-common-dir'))".Trim()).TrimEnd('\')
        $mine = [IO.Path]::GetFullPath("$(Invoke-Git $root @('rev-parse', '--path-format=absolute', '--git-common-dir'))".Trim()).TrimEnd('\')
        if ($common -ne $mine) { throw "$wt belongs to $common, not $mine" }
        Invoke-Git $wt @('checkout', '--detach', '--force', '-q', $script:Commit) | Out-Null
        Invoke-Git $wt @('clean', '-fdq') | Out-Null
    }
    else {
        New-Item -ItemType Directory -Force -Path (Split-Path $wt -Parent) | Out-Null
        try { Invoke-Git $root @('worktree', 'add', '--detach', $wt, $script:Commit) | Out-Null }
        catch {
            [IO.File]::WriteAllText((Join-Path $OutDir 'worktree-add-error.txt'), "$(Get-Date -Format o) $wt`r`n$($_.Exception.Message)`r`n")
            throw
        }
    }
    if ((Get-Item $patch).Length -gt 0) {
        Invoke-Git $wt @('apply', '--whitespace=nowarn', $patch) | Out-Null
    }
    foreach ($rel in $untracked) {
        $dst = Join-Path $wt $rel
        New-Item -ItemType Directory -Force -Path (Split-Path $dst -Parent) | Out-Null
        Copy-Item -LiteralPath (Join-Path $root $rel) -Destination $dst -Force
    }
}

function Get-GuardSafeName([string]$s) { return ($s -replace '[^A-Za-z0-9_.-]', '_') }

function Get-GuardControlFile([string]$key) {
    $sha = [Security.Cryptography.SHA256]::Create()
    $hex = -join ($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($key)) | ForEach-Object { $_.ToString('x2') })
    return Join-Path $WorkerRun "control\$hex.clixml"
}

function Get-GuardControl([string]$key) {
    if (-not $script:ControlCache.ContainsKey($key)) {
        $file = Get-GuardControlFile $key
        if (-not (Test-Path -LiteralPath $file)) { throw "no control result for $key" }
        $script:ControlCache[$key] = Import-Clixml -LiteralPath $file
    }
    $c = $script:ControlCache[$key]
    if ($c.Error) { throw "control for $key failed: $($c.Error)" }
    return $c.Value
}

function Get-GuardHardChecks([string[]]$names, [string]$exe) {
    $binNames = @((@(Get-HardeningChecks '' '' 0 0 0) + @(Get-ImportChecks '')) | ForEach-Object { $_.Name })
    $res = @()
    if (@($names | Where-Object { $binNames -contains $_ }).Count) {
        $db = Find-Dumpbin
        $hdr = (& $db /headers $exe) -join "`n"
        $imp = (& $db /imports $exe) -join "`n"
        $res += @(Get-HardeningChecks $hdr '' 0 0 0) + @(Get-ImportChecks $imp)
    }
    $srcNames = @($names | Where-Object { $binNames -notcontains $_ })
    if ($srcNames.Count) { $res += @(Get-SourceChecks $script:Wt -Only $srcNames) }
    return @($res | Where-Object { $names -contains $_.Name })
}

function Get-GuardSmokeCase([string]$config, [string]$name) {
    if (-not $script:SmokeCaseSets.ContainsKey($config)) {
        $set = @{}
        foreach ($w in 'Crash', 'Standard') {
            foreach ($c in (Get-SmokeCases $w @() $false $null)) { $set[$c.Name] = $c }
        }
        $script:SmokeCaseSets[$config] = $set
    }
    if (-not $script:SmokeCaseSets[$config].ContainsKey($name)) { throw "smoke case '$name' is not defined for $config" }
    return $script:SmokeCaseSets[$config][$name]
}

function Set-GuardGameConfig([string]$config) {
    $script:GameConfiguration = $config
    $script:SmokeDigestConfiguration = $config
}

function Get-GuardRowGroup($r) {
    switch ($r.Kind) {
        'Smoke' { if ($r.Config) { return "game|$($r.Config)" } return "game|$script:DefaultGameConfiguration" }
        'Perturb' { return 'game|Test' }
        'Hardening' { return "game|$script:DefaultGameConfiguration" }
        'AsanCanary' { return 'tests|Asan' }
    }
    if ($r.Asan) { return 'tests|Asan' }
    if ($r.Config) { return "tests|$($r.Config)" }
    return "tests|$script:TestsConfiguration"
}

function Get-GuardControlKeys($r) {
    $cfg = (Get-GuardRowGroup $r).Split('|')[1]
    switch ($r.Kind) {
        'Smoke' { return @("smoke|$cfg|$($r.Case)") }
        'Perturb' { return @('perturb|' + (@($r.Missions) -join ',')) }
        'Hardening' { return @("hard|$cfg|$($r.Check)") }
        'Tlog' { return @() }
        'AsanCanary' { return @($r.Tests | ForEach-Object { "asan|$_" }) }
    }
    if ($r.Asan) { return @($r.Tests | ForEach-Object { "asan|$_" }) }
    return @($r.Tests | ForEach-Object { "t1|$cfg|$_" })
}

function Enter-GuardGroup([string]$group) {
    $kind, $cfg = $group.Split('|')
    if ($kind -eq 'game') { Set-GuardGameConfig $cfg }
    else { Set-GuardGameConfig $script:DefaultGameConfiguration }
    if ($script:BuildState[$group] -eq 'control') { return }
    $log = Join-Path $OutDir ("build-control-$kind-$cfg-w$WorkerIndex.log")
    if ($kind -eq 'tests') { $ok = Build-Tests $script:Wt $log $cfg }
    else { $ok = Build-Game $script:Wt $log $cfg }
    if (-not $ok) { throw "$kind $cfg control build failed, see $log" }
    $script:BuildState[$group] = 'control'
}

function Build-GuardReverted([string]$group, [string]$logPath) {
    $kind, $cfg = $group.Split('|')
    $script:BuildState[$group] = 'reverted'
    if ($kind -eq 'tests') { return (Build-Tests $script:Wt $logPath $cfg) }
    return (Build-Game $script:Wt $logPath $cfg)
}

function Invoke-GuardControl([string]$key) {
    $wt = $script:Wt
    $parts = $key.Split('|')
    switch ($parts[0]) {
        't1' {
            $name = $key.Split('|', 3)[2]
            $c = Run-Case (Join-Path $wt "bin\Tests\$Platform\$($parts[1])\OpenSWBF2Tests.exe") $wt $name
            Set-Content -Path (Join-Path $OutDir ('control-' + (Get-GuardSafeName $name) + '.txt')) -Value $c.Text -Encoding ASCII
            return $c
        }
        'asan' {
            $name = $key.Split('|', 2)[1]
            $c = Run-Case (Join-Path $wt "bin\Tests\$Platform\Asan\OpenSWBF2Tests.exe") $wt $name (Join-Path $OutDir ('asan\control-' + (Get-GuardSafeName $name)))
            Set-Content -Path (Join-Path $OutDir ('asan-control-' + (Get-GuardSafeName $name) + '.txt')) -Value $c.Text -Encoding ASCII
            return $c
        }
        'hard' {
            $chk = @(Get-GuardHardChecks @($parts[2]) (Join-Path $wt "bin\$Platform\$($parts[1])\BattlefrontII.exe")) | Select-Object -First 1
            return [bool]$chk.Ok
        }
        'smoke' {
            $name = $key.Split('|', 3)[2]
            return (Run-Smoke $wt "control-$($parts[1])-$name" (Get-GuardSmokeCase $parts[1] $name))
        }
        'perturb' {
            $missions = @($key.Split('|', 2)[1].Split(','))
            return (Run-Perturb $wt ('control-' + ($missions -join '_')) $missions)
        }
    }
    throw "unknown control key $key"
}

function Add-GuardEntry($r, [string]$tests, [string]$ctl, [string]$result, [string]$code, [string]$detail, [bool]$fail, [string[]]$lines) {
    [void]$script:JobEntries.Add((New-Object PSObject -Property @{ Id = $r.Id; Tests = $tests; Control = $ctl; Reverted = $result; Exit = $code; Detail = $detail; Fail = $fail }))
    foreach ($l in $lines) {
        [void]$script:JobLines.Add($l)
        Write-Host $l
    }
}

function Invoke-GuardRevertedRow($r, [scriptblock]$judge) {
    $path = Join-Path $script:Wt $r.File
    $orig = $null
    $out = @{ Result = 'BUILD-FAIL'; Detail = ''; Code = '' }
    try {
        $orig = Set-Anchor $path $r.Anchor $r.Replacement
        if (Build-GuardReverted (Get-GuardRowGroup $r) (Join-Path $OutDir "build-$($r.Id).log")) { & $judge $out }
    }
    catch {
        $out.Result = 'ERROR'
        $out.Detail = $_.Exception.Message
    }
    finally {
        if ($orig) { [IO.File]::WriteAllBytes($path, $orig) }
    }
    return $out
}

function Invoke-GuardT1Row($r) {
    $cfg = (Get-GuardRowGroup $r).Split('|')[1]
    $exe = Join-Path $script:Wt "bin\Tests\$Platform\$cfg\OpenSWBF2Tests.exe"
    $ctl = 'GREEN'
    foreach ($name in $r.Tests) { $s = (Get-GuardControl "t1|$cfg|$name").State; if ($s -ne 'GREEN') { $ctl = $s } }
    $o = Invoke-GuardRevertedRow $r {
        param($out)
        $states = @()
        $codes = @()
        $details = @()
        foreach ($name in $r.Tests) {
            $c = Run-Case $exe $script:Wt $name
            Set-Content -Path (Join-Path $OutDir (("$($r.Id)-" + (Get-GuardSafeName $name)) + '.txt')) -Value $c.Text -Encoding ASCII
            $states += $c.State
            $codes += $c.Code
            if ($c.Detail) { $details += $c.Detail }
        }
        $out.Code = ($codes -join ',')
        $out.Detail = ($details -join ',')
        if ($states -contains 'NO-MATCH') { $out.Result = 'NO-MATCH' }
        elseif ($states -contains 'RED') { $out.Result = 'RED' }
        elseif ($states -contains 'SKIP') { $out.Result = 'SKIP' }
        else { $out.Result = 'STILL-GREEN' }
    }
    $fail = ($o.Result -ne 'RED' -or $ctl -ne 'GREEN')
    if ($fail) { Add-GuardProof $r $ctl $o.Result $false } else { Add-GuardProof $r 'GREEN' 'RED' }
    Add-GuardEntry $r ($r.Tests -join ' | ') $ctl $o.Result $o.Code $o.Detail $fail @('{0,-12} control={1,-8} reverted={2,-11} exit={3} {4}' -f $r.Id, $ctl, $o.Result, $o.Code, $o.Detail)
}

function Invoke-GuardAsanRow($r) {
    $asanExe = Join-Path $script:Wt "bin\Tests\$Platform\Asan\OpenSWBF2Tests.exe"
    $want = 'GREEN'
    if ($r.Kind -eq 'AsanCanary') { $want = 'REPORTED' }
    $asanState = {
        param($c)
        if ($r.Kind -eq 'AsanCanary') { return (Get-AsanCanaryState $c.Code $c.Report $r.Report) }
        return $c.State
    }
    $ctl = 'GREEN'
    foreach ($name in $r.Tests) {
        $s = & $asanState (Get-GuardControl "asan|$name")
        if ($s -ne $want) { $ctl = $s }
    }
    $o = Invoke-GuardRevertedRow $r {
        param($out)
        $states = @()
        $codes = @()
        $details = @()
        foreach ($name in $r.Tests) {
            $c = Run-Case $asanExe $script:Wt $name (Join-Path $OutDir ("asan\$($r.Id)-" + (Get-GuardSafeName $name)))
            Set-Content -Path (Join-Path $OutDir ("asan-$($r.Id)-" + (Get-GuardSafeName $name) + '.txt')) -Value $c.Text -Encoding ASCII
            $states += (& $asanState $c)
            $codes += $c.Code
            if ($c.Detail) { $details += $c.Detail }
        }
        $out.Code = ($codes -join ',')
        $out.Detail = ($details -join ',')
        if ($r.Kind -eq 'AsanCanary') {
            if ($states -contains 'REPORTED') { $out.Result = 'STILL-GREEN' }
            elseif (@($states | Where-Object { $_ -ne 'CLEAN' }).Count) { $out.Result = 'WRONG-RED' }
            else { $out.Result = 'RED' }
        }
        elseif ($states -contains 'NO-MATCH') { $out.Result = 'NO-MATCH' }
        elseif ($states -contains 'RED') { $out.Result = 'RED' }
        elseif ($states -contains 'SKIP') { $out.Result = 'SKIP' }
        else { $out.Result = 'STILL-GREEN' }
    }
    $fail = ($o.Result -ne 'RED' -or $ctl -ne 'GREEN')
    if ($fail) { Add-GuardProof $r $ctl $o.Result $false }
    elseif ($r.Kind -eq 'AsanCanary') { Add-GuardProof $r 'REPORTED' 'CLEAN' }
    else { Add-GuardProof $r 'GREEN' 'RED' }
    Add-GuardEntry $r ($r.Tests -join ' | ') $ctl $o.Result $o.Code $o.Detail $fail @('{0,-12} control={1,-8} reverted={2,-11} exit={3} {4}' -f $r.Id, $ctl, $o.Result, $o.Code, $o.Detail)
}

function Invoke-GuardHardeningRow($r) {
    $cfg = (Get-GuardRowGroup $r).Split('|')[1]
    $ctl = 'GREEN'
    if (-not (Get-GuardControl "hard|$cfg|$($r.Check)")) { $ctl = 'FAIL' }
    $o = Invoke-GuardRevertedRow $r {
        param($out)
        $chk = @(Get-GuardHardChecks @($r.Check) (Join-Path $script:Wt "bin\$Platform\$cfg\BattlefrontII.exe")) | Select-Object -First 1
        $out.Detail = "check=$($r.Check) ok=$($chk.Ok)"
        if ($chk.Ok) { $out.Result = 'STILL-GREEN' } else { $out.Result = 'RED' }
    }
    $fail = ($o.Result -ne 'RED' -or $ctl -ne 'GREEN')
    if ($fail) { Add-GuardProof $r $ctl $o.Result $false } else { Add-GuardProof $r 'GREEN' 'RED' }
    Add-GuardEntry $r "hardening:$($r.Check)" $ctl $o.Result '' $o.Detail $fail @('{0,-12} control={1,-8} reverted={2,-11} {3}' -f $r.Id, $ctl, $o.Result, $o.Detail)
}

function Invoke-GuardSmokeRow($r) {
    $cfg = (Get-GuardRowGroup $r).Split('|')[1]
    Set-GuardGameConfig $cfg
    $ctlRes = Get-GuardControl "smoke|$cfg|$($r.Case)"
    $ctl = 'GREEN'
    if (-not $ctlRes.Ok) { $ctl = 'FAIL-' + $ctlRes.Class }
    $case = Get-GuardSmokeCase $cfg $r.Case
    $o = Invoke-GuardRevertedRow $r {
        param($out)
        $s = Run-Smoke $script:Wt $r.Id $case
        $out.Code = ('0x{0:X8}' -f $s.Code)
        $out.Detail = "class=$($s.Class) dumps=$($s.Dumps)"
        if ($s.Ok) { $out.Result = 'STILL-GREEN' }
        elseif ($r.RedRx -and $s.LogText -cnotmatch $r.RedRx) { $out.Result = 'WRONG-RED' }
        else { $out.Result = 'RED' }
    }
    $fail = ($o.Result -ne 'RED' -or $ctl -ne 'GREEN')
    if ($fail) { Add-GuardProof $r $ctl $o.Result $false } else { Add-GuardProof $r 'GREEN' 'RED' }
    Add-GuardEntry $r "smoke:$($r.Case)" $ctl $o.Result $o.Code $o.Detail $fail @('{0,-12} control={1,-8} reverted={2,-11} exit={3} {4}' -f $r.Id, $ctl, $o.Result, $o.Code, $o.Detail)
}

function Invoke-GuardPerturbRow($r) {
    Set-GuardGameConfig 'Test'
    $key = @($r.Missions) -join ','
    $ctlRes = Get-GuardControl "perturb|$key"
    $ctl = $ctlRes.State
    $o = Invoke-GuardRevertedRow $r {
        param($out)
        $s = Run-Perturb $script:Wt $r.Id @($r.Missions)
        foreach ($m in $s.Messages) { [void]$script:JobLines.Add($m) }
        $out.Detail = $s.Detail
        if ($s.State -eq 'GREEN') { $out.Result = 'STILL-GREEN' }
        else { $out.Result = $s.State }
    }
    $fail = ($o.Result -ne 'RED' -or $ctl -ne 'GREEN')
    if ($fail) { Add-GuardProof $r $ctl $o.Result $false } else { Add-GuardProof $r 'GREEN' 'RED' }
    Add-GuardEntry $r "perturb:$key" $ctl $o.Result '' $o.Detail $fail @(('{0,-12} control={1,-8} reverted={2,-11} {3}' -f $r.Id, $ctl, $o.Result, $o.Detail), "    control: $($ctlRes.Detail)")
}

function Invoke-GuardTlogJob($tlogRows) {
    $wt = $script:Wt
    Set-GuardGameConfig $script:DefaultGameConfiguration
    $script:BuildState["tests|$script:TestsConfiguration"] = 'reverted'
    $script:BuildState["game|$script:DefaultGameConfiguration"] = 'reverted'
    $buildProj = {
        param($project, $logPath)
        if ($project -eq 'tests') { return (Build-Tests $wt $logPath) }
        return (Build-Game $wt $logPath)
    }
    $tlogDirs = @{ game = @(Get-GameProjects $wt $script:GameConfiguration | ForEach-Object { $_.TlogDir }); tests = (Join-Path $wt "obj\Tests\$Platform\$script:TestsConfiguration\OpenSWBF2Tests.tlog") }
    $getFlag = {
        param($name)
        $game = $null
        $tests = $null
        if (@($tlogDirs.game | Where-Object { Test-Path -LiteralPath $_ }).Count) { $game = Get-ClTlogCounts $tlogDirs.game }
        if (Test-Path -LiteralPath $tlogDirs.tests) { $tests = Get-ClTlogCounts $tlogDirs.tests }
        return (Get-FlagChecks $game $tests $script:GameConfiguration $script:TestsConfiguration | Where-Object { $_.Name -eq $name })
    }
    $tlogProjects = @(@($tlogRows | ForEach-Object { $_.Project }) + @($tlogRows | Where-Object { $_.Check -eq 'mimalloc-defines' } | ForEach-Object { 'tests' }) | Select-Object -Unique)
    foreach ($project in $tlogProjects) {
        $log = Join-Path $OutDir "build-tlog-control-$project.log"
        if (-not (& $buildProj $project $log)) { throw "tlog control build failed, see $log" }
    }
    $tlogControl = @{}
    $tlogProven = @()
    foreach ($name in ($tlogRows | ForEach-Object { $_.Check } | Select-Object -Unique)) {
        $tlogControl[$name] = & $getFlag $name
    }
    foreach ($r in $tlogRows) {
        $path = Join-Path $wt $r.File
        $orig = $null
        $result = 'BUILD-FAIL'
        $ctlChk = $tlogControl[$r.Check]
        $detail = "control: $($ctlChk.Detail)"
        $ctl = 'GREEN'
        if (-not $ctlChk.Ok) { $ctl = 'FAIL' }
        try {
            $orig = Set-Anchor $path $r.Anchor $r.Replacement
            foreach ($td in @($tlogDirs[$r.Project])) { Get-ChildItem -LiteralPath $td -Filter 'cl.command*.tlog' -ErrorAction SilentlyContinue | Remove-Item -Force }
            $built = & $buildProj $r.Project (Join-Path $OutDir "build-$($r.Id).log")
            $fresh = (Get-ClTlogCounts $tlogDirs[$r.Project]).Total
            if ($built -or $fresh -gt 0) {
                $chk = & $getFlag $r.Check
                $detail = "reverted: $($chk.Detail)"
                if (-not $built) { $detail = "build failed after $fresh compiles; $detail" }
                if ($chk.Ok) { $result = 'STILL-GREEN' } else { $result = 'RED' }
            }
        }
        catch {
            $result = 'ERROR'
            $detail = $_.Exception.Message
        }
        finally {
            if ($orig) { [IO.File]::WriteAllBytes($path, $orig) }
        }
        $fail = ($result -ne 'RED' -or $ctl -ne 'GREEN')
        if ($fail) { Add-GuardProof $r $ctl $result $false }
        else { $tlogProven += $r }
        Add-GuardEntry $r "tlog:$($r.Check)" $ctl $result '' $detail $fail @('{0,-16} control={1,-8} reverted={2,-11} {3}' -f $r.Id, $ctl, $result, $detail)
    }
    foreach ($project in $tlogProjects) {
        $log = Join-Path $OutDir "build-tlog-restored-$project.log"
        if (-not (& $buildProj $project $log)) { throw "tlog restored build failed, see $log" }
    }
    $restoredOk = $true
    foreach ($name in $tlogControl.Keys) {
        $chk = & $getFlag $name
        $line = ('tlog restored {0}: ok={1} {2}' -f $name, $chk.Ok, $chk.Detail)
        [void]$script:JobLines.Add($line)
        Write-Host $line
        if (-not $chk.Ok) { $restoredOk = $false }
    }
    if (-not $restoredOk) {
        foreach ($e in $script:JobEntries) { $e.Fail = $true }
        [void]$script:JobLines.Add('tlog restored build lost a flag; every tlog row fails')
    }
    foreach ($r in $tlogProven) { Add-GuardProof $r 'GREEN' 'RED' $restoredOk }
}

function Invoke-GuardRowJob($job, $byId) {
    $rows = @($job.Ids | ForEach-Object { $byId[$_] })
    if ($job.Kind -eq 'Tlog') { Invoke-GuardTlogJob $rows; return }
    $r = $rows[0]
    switch ($r.Kind) {
        'Smoke' { Invoke-GuardSmokeRow $r; return }
        'Perturb' { Invoke-GuardPerturbRow $r; return }
        'Hardening' { Invoke-GuardHardeningRow $r; return }
        'AsanCanary' { Invoke-GuardAsanRow $r; return }
    }
    if ($r.Asan) { Invoke-GuardAsanRow $r; return }
    Invoke-GuardT1Row $r
}

function Get-NextGuardJob([string]$phase) {
    $f = Join-Path $WorkerRun "queue-$phase.txt"
    return (Invoke-GuardLocked {
        $next = [int]([IO.File]::ReadAllText($f).Trim())
        [IO.File]::WriteAllText($f, [string]($next + 1))
        $next
    })
}

function New-GuardMutex([string]$run) {
    $sha = [Security.Cryptography.SHA256]::Create()
    $hex = -join ($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($run.ToLowerInvariant())) | ForEach-Object { $_.ToString('x2') })
    return New-Object System.Threading.Mutex($false, "Local\OpenSWBF2-guard-$hex")
}

if ($WorkerIndex -gt 0) {
    try {
        if (-not $WorkerRun -or -not $WorkerPhase) { throw 'a worker needs -WorkerRun and -WorkerPhase' }
        . $RowsFile
        $ctx = Import-Clixml -LiteralPath (Join-Path $WorkerRun 'context.clixml')
        if (@($ctx.RowJobs | Where-Object { $script:GuardSmokeKinds -contains $_.Kind }).Count) {
            . (Join-Path $testsDir 'smoke.ps1')
            $script:GuardSmokeLoaded = $true
        }
        $script:Decisions = @{}
        foreach ($d in @($ctx.Decisions)) { $script:Decisions[$d.Id] = $d }
        $script:Toolset = $ctx.Toolset
        $script:Tree = $ctx.Tree
        $script:Commit = $ctx.Commit
        $script:ProofConfig = $ctx.ProofConfig
        $script:TreeClean = $ctx.TreeClean
        $script:LedgerFile = Join-Path $OutDir 'ledger.tsv'
        $script:PartialFile = Join-Path $OutDir 'proven.partial.tsv'
        $script:GuardMutex = New-GuardMutex $WorkerRun
        $script:Wt = Get-GuardWorktree $WorkerIndex
        $script:ControlCache = @{}
        $script:BuildState = @{}
        $script:SmokeCaseSets = @{}
        $script:msbuild = Find-MSBuild
        $byId = @{}
        foreach ($r in @($GuardRevertRows)) { $byId[$r.Id] = $r }
        if ($WorkerPhase -eq 'control') {
            $jobs = @($ctx.ControlJobs)
            while (($j = Get-NextGuardJob 'control') -lt $jobs.Count) {
                $job = $jobs[$j]
                $err = $null
                try { Enter-GuardGroup $job.Group }
                catch { $err = $_.Exception.Message }
                foreach ($key in @($job.Keys)) {
                    $v = $null
                    $e = $err
                    if (-not $e) {
                        try { $v = Invoke-GuardControl $key }
                        catch { $e = $_.Exception.Message }
                    }
                    if ($e) { Write-Host "ERROR: control $key`: $e" }
                    $file = Get-GuardControlFile $key
                    Export-Clixml -LiteralPath "$file.tmp" -InputObject (New-Object PSObject -Property @{ Key = $key; Value = $v; Error = $e }) -Depth 4
                    Move-Item -LiteralPath "$file.tmp" -Destination $file -Force
                }
            }
        }
        else {
            $jobs = @($ctx.RowJobs)
            while (($j = Get-NextGuardJob 'rows') -lt $jobs.Count) {
                $job = $jobs[$j]
                $script:JobEntries = New-Object System.Collections.ArrayList
                $script:JobLines = New-Object System.Collections.ArrayList
                try { Invoke-GuardRowJob $job $byId }
                catch {
                    $msg = $_.Exception.Message
                    $done = @($script:JobEntries | ForEach-Object { $_.Id })
                    foreach ($id in @($job.Ids | Where-Object { $done -notcontains $_ })) {
                        $r = $byId[$id]
                        Add-GuardProof $r 'ERROR' 'ERROR' $false
                        Add-GuardEntry $r $job.Kind 'ERROR' 'ERROR' '' $msg $true @('{0,-12} control={1,-8} reverted={2,-11} {3}' -f $id, 'ERROR', 'ERROR', $msg)
                    }
                    foreach ($e in $script:JobEntries) { $e.Fail = $true }
                }
                $file = Join-Path $WorkerRun "results\rows-$j.clixml"
                Export-Clixml -LiteralPath "$file.tmp" -InputObject (New-Object PSObject -Property @{ Entries = @($script:JobEntries); Lines = @($script:JobLines) }) -Depth 4
                Move-Item -LiteralPath "$file.tmp" -Destination $file -Force
            }
        }
    }
    catch {
        Write-Host "ERROR: $($_.Exception.Message)"
        Write-Host $_.InvocationInfo.PositionMessage
        if ($_.ScriptStackTrace) { Write-Host $_.ScriptStackTrace }
        exit 1
    }
    exit 0
}

function ConvertTo-GuardArg([string]$s) {
    if ($s -notmatch '[\s"]') { return $s }
    return '"' + ($s -replace '(\\*)"', '$1$1\"' -replace '(\\+)$', '$1$1') + '"'
}

function Invoke-GuardPhase([string]$phase, [int]$jobCount, [int]$count) {
    [IO.File]::WriteAllText((Join-Path $script:RunDir "queue-$phase.txt"), '0')
    $exe = [Diagnostics.Process]::GetCurrentProcess().MainModule.FileName
    $procs = @()
    for ($i = 1; $i -le [Math]::Min($count, $jobCount); $i++) {
        $log = Join-Path $OutDir "worker-$phase-w$i.log"
        $a = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $PSCommandPath, '-WorkerIndex', $i, '-WorkerPhase', $phase, '-WorkerRun', $script:RunDir, '-OutDir', $OutDir, '-WorktreeRoot', $WorktreeRoot, '-WorkerBase', $WorkerBase, '-BuildJobs', $BuildJobs, '-RowsFile', $RowsFile, '-Platform', $Platform, '-ProvenFile', $ProvenFile)
        if ($Configuration) { $a += @('-Configuration', $Configuration) }
        $p = Start-Process -FilePath $exe -ArgumentList (@($a | ForEach-Object { ConvertTo-GuardArg ([string]$_) }) -join ' ') -NoNewWindow -PassThru -RedirectStandardOutput $log -RedirectStandardError "$log.err"
        [void]$p.Handle
        $procs += New-Object PSObject -Property @{ Index = $i; Proc = $p; Log = $log }
    }
    $shown = @{}
    $show = {
        if ($phase -ne 'rows') { return }
        for ($j = 0; $j -lt $jobCount; $j++) {
            if ($shown[$j]) { continue }
            $f = Join-Path $script:RunDir "results\rows-$j.clixml"
            if (-not (Test-Path -LiteralPath $f)) { continue }
            $shown[$j] = $true
            foreach ($l in @((Import-Clixml -LiteralPath $f).Lines)) { Write-Host $l }
        }
    }
    foreach ($w in $procs) {
        while (-not $w.Proc.WaitForExit(2000)) { & $show }
        & $show
    }
    $ok = $true
    foreach ($w in $procs) {
        $code = $w.Proc.ExitCode
        if ($code -ne 0) {
            $ok = $false
            Write-Host "ERROR: $phase worker w$($w.Index) exited $code, see $($w.Log)"
            foreach ($f in @($w.Log, "$($w.Log).err")) {
                if (Test-Path -LiteralPath $f) { Get-Content -LiteralPath $f | Where-Object { $_ -match '^ERROR' } | ForEach-Object { Write-Host "    w$($w.Index): $_" } }
            }
        }
    }
    return $ok
}

try {
    . $RowsFile
    $rows = @($GuardRevertRows)
    $dupIds = @($rows | Group-Object { $_.Id } | Where-Object { $_.Count -gt 1 } | ForEach-Object { $_.Name })
    if ($dupIds.Count) { throw "duplicate row Id: $($dupIds -join ', ')" }
    if ($Row) { $rows = @($rows | Where-Object { $Row -contains $_.Id }) }
    if ($rows.Count -eq 0) { Write-Host 'No rows selected.'; exit 2 }
    if (@($rows | Where-Object { $script:GuardSmokeKinds -contains $_.Kind }).Count) {
        . (Join-Path $testsDir 'smoke.ps1')
        $script:GuardSmokeLoaded = $true
    }
    $vsTools = Find-VsTools $Platform
    $script:Toolset = 'unknown'
    if ($vsTools) { $script:Toolset = $vsTools.ToolsVersion }
    $script:ProofConfig = "$script:TestsConfiguration/$script:GameConfiguration"
    $statusArgs = @('status', '--porcelain', '--untracked-files=normal', '--', '.', ':!tests/guardrevert/proven.tsv')
    $rootPrefix = $root.TrimEnd('\') + '\'
    if ($ProvenFile.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        $statusArgs += ':!' + $ProvenFile.Substring($rootPrefix.Length).Replace('\', '/')
    }
    New-Item -ItemType Directory -Force -Path (Join-Path $root 'artifacts') | Out-Null
    Set-Content -Path (Join-Path $root 'artifacts\.gitignore') -Value '*' -Encoding ASCII
    $before = (Invoke-Git $root $statusArgs) -join "`n"
    $script:TreeClean = -not $before
    $script:Tree = "$(Invoke-Git $root @('rev-parse', 'HEAD^{tree}'))".Trim()
    $script:Commit = "$(Invoke-Git $root @('rev-parse', 'HEAD'))".Trim()
    $script:TreeChanged = $false
    $proofs = Read-GuardProofs $ProvenFile
    $testInfo = Get-GuardTestInfo $root
    $tlogRoot = $root.TrimEnd('\') + '\'
    $caseTexts = @{}
    $caseData = $null
    if ($script:GuardSmokeLoaded) {
        $caseTexts = Get-GuardCaseTexts $rows $script:DefaultGameConfiguration $root
        $caseData = Get-GuardCaseData (Join-Path $testsDir 'smoke\cases.ps1')
    }
    $selIn = @{
        Rows = $rows
        Proofs = $proofs
        Hashes = (Get-GuardHashes $root)
        TestsReads = (Read-ClReadTlog (Join-Path $root "obj\Tests\$Platform\$script:TestsConfiguration\OpenSWBF2Tests.tlog\CL.read.1.tlog") $tlogRoot)
        AsanReads = (Read-ClReadTlog (Join-Path $root "obj\Tests\$Platform\Asan\OpenSWBF2Tests.tlog\CL.read.1.tlog") $tlogRoot)
        GameReads = $(if ($script:GuardSmokeLoaded) { Read-ClReadTlogs @(Get-GameProjects $root $script:GameConfiguration | ForEach-Object { Join-Path $_.TlogDir 'CL.read.1.tlog' }) $tlogRoot } else { $null })
        TestsProjectGameTUs = (Get-TestsProjectGameTUs)
        TestSources = $testInfo.Sources
        TestLiterals = $testInfo.Literals
        CaseTexts = $caseTexts
        CaseData = $caseData
        GlobalTriggers = $script:GuardGlobalTriggers
        Toolset = $script:Toolset
        Platform = $Platform
    }
    $script:Decisions = @{}
    foreach ($d in @(Select-GuardRows $selIn)) {
        $p = $proofs[$d.Id]
        if ($d.Action -eq 'CARRY' -and $p.Config -ne $script:ProofConfig) { $d.Action = 'SELECT'; $d.Reason = "config $($p.Config)" }
        if ($Row) { $d.Action = 'SELECT'; $d.Reason = 'named' }
        elseif ($Full -and $d.Action -eq 'CARRY') { $d.Action = 'SELECT'; $d.Reason = 'full' }
        $script:Decisions[$d.Id] = $d
    }
    $carryLines = @()
    $listLines = @()
    foreach ($r in $rows) {
        $d = $script:Decisions[$r.Id]
        if ($d.Action -eq 'SELECT') { $listLines += "SELECT $($d.Id) $($d.Kind) $($d.Reason)" }
        else {
            $p = $proofs[$d.Id]
            $line = "CARRY $($d.Id) $($d.Kind) $($p.Commit) $($p.Tree) $($p.Date)"
            $carryLines += $line
            $listLines += $line
        }
    }
    $rows = @($rows | Where-Object { $script:Decisions[$_.Id].Action -eq 'SELECT' })
    $totals = "selected $($rows.Count) carried $($carryLines.Count)"
    if ($ListSelected) {
        $listLines | ForEach-Object { Write-Host $_ }
        Write-Host $totals
        exit 0
    }
    if ($rows.Count -eq 0) {
        $carryLines | ForEach-Object { Write-Host $_ }
        Write-Host $totals
        exit 0
    }
    if (-not $script:TreeClean) { Write-Host 'PROOFS NOT RECORDED: tree not clean' }
    $selectedIds = @($rows | ForEach-Object { $_.Id })
    $table = @()
    $script:LedgerFile = Join-Path $OutDir 'ledger.tsv'
    $script:PartialFile = Join-Path $OutDir 'proven.partial.tsv'
    if ($Resume) {
        $ledger = Read-GuardProofs $script:LedgerFile
        $resumedIds = @($rows | Where-Object { Test-GuardResumed $ledger[$_.Id] $script:Decisions[$_.Id] } | ForEach-Object { $_.Id })
        foreach ($id in $resumedIds) {
            $l = $ledger[$id]
            Write-Host "RESUMED $id $($l.Date)"
            $table += New-Object PSObject -Property @{ Id = $id; Tests = 'resumed'; Control = $l.Control; Reverted = $l.Result; Exit = ''; Detail = "ledger $($l.Date)" }
        }
        $partialKeep = @()
        if ($script:TreeClean -and (Test-Path -LiteralPath $script:PartialFile)) {
            foreach ($pl in [IO.File]::ReadAllLines($script:PartialFile)) {
                $f = $pl.Split("`t")
                if ($resumedIds -cnotcontains $f[0]) { continue }
                $o = [ordered]@{ Outcome = 'PASS' }
                for ($i = 0; $i -lt $script:ProofColumns.Count; $i++) { $o[$script:ProofColumns[$i]] = $(if ($i -lt $f.Count) { $f[$i] } else { '' }) }
                if (Test-GuardResumed ([pscustomobject]$o) $script:Decisions[$f[0]]) { $partialKeep += $pl }
            }
        }
        if ($partialKeep.Count) { [IO.File]::WriteAllText($script:PartialFile, (@($partialKeep | ForEach-Object { $_ + "`r`n" }) -join '')) }
        elseif (Test-Path -LiteralPath $script:PartialFile) { Remove-Item -LiteralPath $script:PartialFile -Force }
        $rows = @($rows | Where-Object { $resumedIds -cnotcontains $_.Id })
        $totals += " resumed $($resumedIds.Count)"
    }
    foreach ($r in @($rows | Where-Object { $_.Kind -eq 'Perturb' })) {
        if ($r.Config -ne 'Test') { throw "Perturb row $($r.Id) needs Config = 'Test'" }
        if (@($r.Missions).Count -eq 0) { throw "Perturb row $($r.Id) has no Missions" }
        if (@($r.Missions) -contains 'nab2g_c') { throw "Perturb row $($r.Id) must not run nab2g_c" }
    }
    $gameData = Find-GameDataRoot
    $smokeCases = @{}
    if ($script:GuardSmokeLoaded) {
        foreach ($w in 'Crash', 'Standard') {
            foreach ($c in (Get-SmokeCases $w @() $false $null)) {
                if ($smokeCases.ContainsKey($c.Name)) { throw "smoke case '$($c.Name)' is defined more than once" }
                $smokeCases[$c.Name] = $c
            }
        }
    }

    Assert-Runtimes $GameRuntimes

    $script:msbuild = Find-MSBuild

    $groupKeys = [ordered]@{}
    $rowJobs = @()
    $tlogIds = @()
    foreach ($kind in 'Perturb', 'Tlog', 'Smoke', 'Hardening', 'AsanCanary', '') {
        foreach ($r in @($rows | Where-Object { $(if ($kind) { $_.Kind -eq $kind } else { @('Perturb', 'Tlog', 'Smoke', 'Hardening', 'AsanCanary') -notcontains $_.Kind }) })) {
            if ($r.Kind -eq 'Tlog') { $tlogIds += $r.Id; continue }
            $g = Get-GuardRowGroup $r
            if (-not $groupKeys.Contains($g)) { $groupKeys[$g] = New-Object System.Collections.ArrayList }
            foreach ($k in (Get-GuardControlKeys $r)) { if (-not $groupKeys[$g].Contains($k)) { [void]$groupKeys[$g].Add($k) } }
            $rowJobs += New-Object PSObject -Property @{ Kind = [string]$r.Kind; Ids = @($r.Id) }
        }
        if ($kind -eq 'Tlog' -and $tlogIds.Count) { $rowJobs += New-Object PSObject -Property @{ Kind = 'Tlog'; Ids = $tlogIds } }
    }
    foreach ($g in $groupKeys.Keys) {
        $kind, $cfg = $g.Split('|')
        if ($kind -ne 'game') { continue }
        $pre = Get-SmokePrecheck $gameData (Join-Path $root "bin\$Platform\$cfg")
        if ($pre -and @($groupKeys[$g] | Where-Object { $_ -match '^(smoke|perturb)\|' }).Count) { throw "smoke and Perturb rows need a prior $cfg game build and data: $pre" }
    }
    $workerCount = [Math]::Min($Workers, [Math]::Max(1, $rowJobs.Count))
    $controlJobs = @()
    foreach ($g in $groupKeys.Keys) {
        $keys = @($groupKeys[$g])
        $chunks = [Math]::Min($workerCount, $keys.Count)
        for ($c = 0; $c -lt $chunks; $c++) {
            $part = @(for ($k = $c; $k -lt $keys.Count; $k += $chunks) { $keys[$k] })
            $controlJobs += New-Object PSObject -Property @{ Group = $g; Keys = $part }
        }
    }
    try { $null = Get-WorkerPaths $root $Platform $script:GameConfiguration 'guard' $WorkerBase $workerCount }
    catch { Write-Host "-WorkerBase ${WorkerBase} -Workers ${workerCount}: $($_.Exception.Message)"; exit 2 }

    New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
    if (-not (Test-Path -LiteralPath $script:LedgerFile)) { [IO.File]::WriteAllText($script:LedgerFile, ($script:LedgerColumns -join "`t") + "`r`n") }
    $script:RunDir = Join-Path $OutDir "work-$stamp"
    New-Item -ItemType Directory -Force -Path (Join-Path $script:RunDir 'control'), (Join-Path $script:RunDir 'results') | Out-Null
    $failed = $false
    $cleanupFailed = $false

    try {
        if ($rowJobs.Count -gt 0) {
            $patch = Join-Path $OutDir 'overlay.patch'
            Invoke-Git $root @('diff', 'HEAD', '--binary', "--output=$patch") | Out-Null
            $untracked = @(Invoke-Git $root @('ls-files', '--others', '--exclude-standard') | ForEach-Object { "$_".Trim() } | Where-Object { $_ })
            & git -C $root worktree prune 2>&1 | Out-Null
            for ($i = 1; $i -le $workerCount; $i++) { Sync-GuardWorktree (Get-GuardWorktree $i) $patch $untracked }
            $ctxDecisions = @($rows | ForEach-Object { $script:Decisions[$_.Id] })
            Export-Clixml -LiteralPath (Join-Path $script:RunDir 'context.clixml') -Depth 4 -InputObject (New-Object PSObject -Property @{
                Decisions = $ctxDecisions; Toolset = $script:Toolset; Tree = $script:Tree; Commit = $script:Commit
                ProofConfig = $script:ProofConfig; TreeClean = $script:TreeClean; ControlJobs = $controlJobs; RowJobs = $rowJobs
            })
            Write-Host "workers $workerCount at $(Join-Path $WorktreeRoot $Platform), msbuild /m:$BuildJobs, $($controlJobs.Count) control jobs, $($rowJobs.Count) row jobs"
            if ($controlJobs.Count -gt 0) {
                if (-not (Invoke-GuardPhase 'control' $controlJobs.Count $workerCount)) { $failed = $true }
            }
            if (-not (Invoke-GuardPhase 'rows' $rowJobs.Count $workerCount)) { $failed = $true }
            for ($j = 0; $j -lt $rowJobs.Count; $j++) {
                $f = Join-Path $script:RunDir "results\rows-$j.clixml"
                if (-not (Test-Path -LiteralPath $f)) { continue }
                foreach ($e in @((Import-Clixml -LiteralPath $f).Entries)) {
                    if ($e.Fail) { $failed = $true }
                    $table += $e
                }
            }
        }
    }
    finally {
        $ErrorActionPreference = 'Continue'
        & git -C $root worktree prune 2>&1 | Out-Null
        $script:TreeChanged = $true
        try {
            $after = (Invoke-Git $root $statusArgs) -join "`n"
            $headAfter = "$(Invoke-Git $root @('rev-parse', 'HEAD'))".Trim()
            $script:TreeChanged = ($after -ne $before) -or ($headAfter -ne $script:Commit)
        }
        catch { Write-Host "STATUS CHECK FAILED: $($_.Exception.Message)" }
        if ($script:TreeChanged) { Write-Host 'PROOFS NOT RECORDED: main tree changed during the run' }
        else {
            try { Merge-GuardProofs }
            catch { Write-Host "PROOF MERGE FAILED: $($_.Exception.Message)"; $cleanupFailed = $true }
        }
    }

    $order = @{}
    for ($i = 0; $i -lt $selectedIds.Count; $i++) { $order[$selectedIds[$i]] = $i }
    $table = @($table | Sort-Object { $order[$_.Id] })
    $lines = @()
    $lines += ('{0,-12} {1,-8} {2,-11} {3,-10} {4}' -f 'Id', 'Control', 'Reverted', 'Exit', 'Tests')
    foreach ($t in $table) {
        $lines += ('{0,-12} {1,-8} {2,-11} {3,-10} {4} {5}' -f $t.Id, $t.Control, $t.Reverted, $t.Exit, $t.Tests, $t.Detail)
    }
    Write-Host ''
    $carryLines | ForEach-Object { Write-Host $_ }
    Write-Host $totals
    $lines | ForEach-Object { Write-Host $_ }
    $lines | Set-Content -Path (Join-Path $OutDir 'summary.txt') -Encoding ASCII
    Write-Host "Artifacts: $OutDir"

    $ranIds = @($table | ForEach-Object { $_.Id })
    $missing = @($selectedIds | Where-Object { $ranIds -notcontains $_ })
    if ($table.Count -ne $selectedIds.Count) {
        Write-Host "ERROR: $($table.Count) of $($selectedIds.Count) selected rows ran. Missing: $($missing -join ', ')"
        $failed = $true
    }
}
catch {
    Write-Host "ERROR: $($_.Exception.Message)"
    Write-Host $_.InvocationInfo.PositionMessage
    if ($_.ScriptStackTrace) { Write-Host $_.ScriptStackTrace }
    exit 1
}

if ($script:TreeChanged) { Write-Host 'WARNING: main tree status changed during the run.'; exit 1 }
if ($failed -or $cleanupFailed) { exit 1 }
exit 0
