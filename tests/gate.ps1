param(
    [Parameter(Mandatory)][ValidateSet('plan', 'phase', 'milestone')][string]$Tier,
    [string[]]$Platform = @('Win32'),
    [string[]]$Step,
    [switch]$Resume,
    [string]$StepsFile,
    [string]$EvidenceDir,
    [switch]$ListOrder
)
$ErrorActionPreference = 'Stop'
$testsDir = $PSScriptRoot
$root = Split-Path $testsDir -Parent
Set-Location -LiteralPath $root
. (Join-Path $testsDir 'hardening.ps1')
. (Join-Path $testsDir 'gate\keys.ps1')
if ($Step) { $Step = @($Step | ForEach-Object { $_ -split ',' } | Where-Object { $_ }) }
$platIn = @($Platform | ForEach-Object { $_ -split ',' } | Where-Object { $_ })
$Platform = @($platIn | ForEach-Object { $p = $_; @('Win32', 'x64') | Where-Object { $_ -ieq $p } })
if ($Platform.Count -eq 0 -or $Platform.Count -ne $platIn.Count) { Write-Host "gate: -Platform takes Win32 and x64, not '$($platIn -join ',')'"; exit 2 }
if (-not $StepsFile) { $StepsFile = Join-Path $testsDir 'gate\tiers.ps1' }
$StepsFile = [IO.Path]::GetFullPath($StepsFile)
if (-not (Test-Path -LiteralPath $StepsFile -PathType Leaf)) { Write-Host "gate: steps file not found: $StepsFile"; exit 2 }
if ($Platform -contains 'x64') {
    $platProps = Join-Path $root 'OpenSWBF2.Platform.props'
    if (-not (Test-Path -LiteralPath $platProps -PathType Leaf) -or -not (Select-String -LiteralPath $platProps -Pattern '<ProjectConfiguration Include="[^"]*\|x64"' -Quiet)) { Write-Host "gate: x64 steps need an x64 configuration in $platProps"; exit 2 }
}
if (-not $EvidenceDir) { $EvidenceDir = $env:OPENSWBF2_EVIDENCE_DIR }
if (-not $EvidenceDir) { $EvidenceDir = Join-Path $env:LOCALAPPDATA 'OpenSWBF2\evidence' }
$EvidenceDir = [IO.Path]::GetFullPath($EvidenceDir).TrimEnd('\')
$rootFull = [IO.Path]::GetFullPath($root).TrimEnd('\')
if ($EvidenceDir.Equals($rootFull, [StringComparison]::OrdinalIgnoreCase) -or $EvidenceDir.StartsWith($rootFull + '\', [StringComparison]::OrdinalIgnoreCase)) {
    Write-Host "gate: evidence root $EvidenceDir is inside the repo"
    exit 2
}
$script:Tiers = @('plan', 'phase', 'milestone')
$script:Kinds = @('build', 'run', 'compare', 'capture', 'script', 'census')
$script:OutputKinds = @('census', 'capture', 'compare')
$script:FailLines = @{
    script = '(?m)^FAILED'
    capture = '(?m)^\[FAIL\] '
}
$script:LedgerColumns = @('Step', 'Platform', 'Config', 'Tier', 'RunKey', 'JudgeKey', 'Decision', 'Result', 'Evidence', 'Wall_s', 'Start', 'End')
$recordsFile = Join-Path $EvidenceDir 'steps.tsv'
$evidenceLedger = Join-Path $EvidenceDir 'ledger.tsv'
$gateRoot = Join-Path $root 'artifacts\gate'
$script:KeepRuns = 3
$script:PreflightChecks = @('steam', 'overlay', 'firewall', 'wer', 'ram', 'disk', 'toolchain')
$script:RamFloorKB = 5098436
$script:DiskDropBytes = 14.42GB
$script:RunDir = $null

function Invoke-GateGit([string[]]$GitArgs) {
    $ErrorActionPreference = 'Continue'
    $out = & git -C $root @GitArgs 2>&1
    if ($LASTEXITCODE -ne 0) { throw "git $($GitArgs -join ' ') failed: $out" }
    return $out
}

function Test-GateRunRecord([string]$Dir) {
    $resultsFile = Join-Path $Dir 'results.txt'
    if (-not (Test-Path -LiteralPath $resultsFile -PathType Leaf)) { return 'no results.txt' }
    $bad = @([IO.File]::ReadAllLines($resultsFile) | Where-Object { $_ -cmatch '^\[(FAIL|STALL|CRASH)\] ' })
    if ($bad.Count) { return "results.txt has a failure line: $($bad[0])" }
    foreach ($rel in 'index.json', 'unit\index.json') {
        $indexFile = Join-Path $Dir $rel
        if (-not (Test-Path -LiteralPath $indexFile -PathType Leaf)) { continue }
        try { $index = [IO.File]::ReadAllText($indexFile) | ConvertFrom-Json }
        catch { return "$rel does not parse" }
        if ($null -eq $index.failed -or $null -eq $index.tests) { return "$rel has no failed count or test list" }
        if ([int]$index.failed -ne 0) { return "$rel counts $($index.failed) failed" }
        $failed = @($index.tests | Where-Object { $_.state -ceq 'Fail' })
        if ($failed.Count) { return "$rel test $($failed[0].fullTestPath) is Fail" }
    }
    return $null
}

function Invoke-GateCompare([string]$Tables, [string[]]$CompareArgs) {
    $ErrorActionPreference = 'Continue'
    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $testsDir 'baseline\compare-tables.ps1') -Tables $Tables @CompareArgs 2>&1 | ForEach-Object { Write-Host "$_" }
    return $LASTEXITCODE
}

function Invoke-GateCommand($S, [string]$OutFile) {
    $ErrorActionPreference = 'Continue'
    New-Item -ItemType Directory -Force -Path (Split-Path $OutFile -Parent) | Out-Null
    $w = New-Object IO.StreamWriter($OutFile, $false, (New-Object Text.UTF8Encoding $false))
    $global:LASTEXITCODE = -1
    try {
        & $S.Exe @($S.Args) 2>&1 | ForEach-Object { $t = "$_"; $w.WriteLine($t); Write-Host $t }
        $code = $global:LASTEXITCODE
    }
    catch {
        $w.WriteLine("gate: command failed to start: $($_.Exception.Message)")
        $code = -1
    }
    finally { $w.Dispose() }
    return [int]$code
}

function Expand-GateText([string]$Text, [string]$Plat, [bool]$WithStamp) {
    $t = $Text.Replace('$Platform', $Plat)
    if ($WithStamp) { $t = $t.Replace('$Stamp', $script:Stamp) }
    return $t
}

function Get-GatePath([string]$Path) {
    if ([IO.Path]::IsPathRooted($Path)) { return [IO.Path]::GetFullPath($Path) }
    return [IO.Path]::GetFullPath((Join-Path $root $Path))
}

function Get-GateStepArgs($S, [string]$Plat) {
    $a = @($S.Args)
    if ($S.PlatformArgs -and $S.PlatformArgs.ContainsKey($Plat)) { $a += @($S.PlatformArgs[$Plat]) }
    return $a
}

function Expand-GateStep($S, [string]$Plat) {
    $src = $null
    if ($S.SourceKey) {
        $src = @{
            Tlog = Get-GatePath (Expand-GateText $S.SourceKey.Tlog $Plat $false)
            Runtime = Get-GatePath (Join-Path (Expand-GateText $S.SourceKey.RuntimeDir $Plat $false) $script:AsanRuntimeNames[$Plat])
        }
    }
    $def = @(
        "kind`t$($S.Kind)", "config`t$($S.Config)", "exe`t$(Expand-GateText $S.Exe $Plat $false)",
        "args`t$(@(Get-GateStepArgs $S $Plat | ForEach-Object { Expand-GateText $_ $Plat $false }) -join [char]0x1f)",
        "outputs`t$(@($S.Outputs | ForEach-Object { Expand-GateText $_ $Plat $false }) -join [char]0x1f)",
        "binaries`t$(@($S.Binaries | ForEach-Object { Expand-GateText $_ $Plat $false }) -join [char]0x1f)",
        "dlldir`t$(if ($S.DllDir) { Expand-GateText $S.DllDir $Plat $false })",
        "judgekinds`t$(@($S.JudgeKinds) -join ',')",
        "compare`t$(@($S.Compare) -join [char]0x1f)",
        "sourcekey`t$(if ($S.SourceKey) { (Expand-GateText $S.SourceKey.Tlog $Plat $false) + [char]0x1f + (Expand-GateText $S.SourceKey.RuntimeDir $Plat $false) })"
    ) -join "`n"
    @{
        Name = $S.Name; Kind = $S.Kind; Config = $S.Config; Platform = $Plat
        JudgeKinds = @($S.JudgeKinds | Where-Object { $_ }); Compare = @($S.Compare | Where-Object { $_ })
        Inputs = @($S.Inputs | Where-Object { $_ }); SourceKey = $src
        Exe = Expand-GateText $S.Exe $Plat $true
        Args = @(Get-GateStepArgs $S $Plat | ForEach-Object { Expand-GateText $_ $Plat $true })
        Outputs = @($S.Outputs | Where-Object { $_ } | ForEach-Object { Get-GatePath (Expand-GateText $_ $Plat $true) })
        Binaries = @($S.Binaries | Where-Object { $_ } | ForEach-Object { Expand-GateText $_ $Plat $true })
        DllDir = $(if ($S.DllDir) { Get-GatePath (Expand-GateText $S.DllDir $Plat $true) } else { $null })
        Definition = Get-KeyTextSha $def
    }
}

function Get-GateBinaryHashes([string[]]$Paths) {
    if (-not $Paths -or $Paths.Count -eq 0) { return 'none' }
    $h = [ordered]@{}
    foreach ($p in $Paths) {
        $full = Get-GatePath $p
        $v = '-'
        if ([IO.File]::Exists($full)) {
            try { $v = Get-PeNormalisedSha $full }
            catch { $v = '-' }
        }
        $h[$p] = $v
    }
    return $h
}

function Get-GateTlogClosure($X) {
    $r = Read-ClReadTlog $X.SourceKey.Tlog $root
    if ($null -eq $r) { return @{ State = '-'; Files = @() } }
    $units = @($r.Sets.Keys | Where-Object { Test-KeyInput $_ $X.Inputs })
    if ($units.Count -eq 0) { return @{ State = '-'; Files = @() } }
    $files = New-GuardPathSet
    $state = 'fresh'
    foreach ($u in $units) {
        if ($r.Stale.Contains($u)) { $state = '-' }
        $files.UnionWith($r.Sets[$u])
    }
    return @{ State = $state; Files = @($files) }
}

function Get-GateKey($X, $Tc) {
    $files = Select-KeyInputFiles $script:TreeFiles $X.Inputs
    $files['gate-step-definition'] = $X.Definition
    if ($X.SourceKey) {
        $tl = Get-GateTlogClosure $X
        foreach ($p in $tl.Files) {
            if ($files.ContainsKey($p)) { continue }
            $full = Join-Path $root $p
            $files[$p] = $(if ([IO.File]::Exists($full)) { Get-KeyFileSha $full } else { 'absent' })
        }
        $rt = $(if ([IO.File]::Exists($X.SourceKey.Runtime)) { Get-KeyFileSha $X.SourceKey.Runtime } else { '-' })
        return Get-StepKey @{
            Header = $script:SourceKeyHeader
            step = $X.Name; platform = $X.Platform; config = $X.Config
            toolset = $Tc.ToolsVersion; cl = $Tc.Cl; gamedata = $script:GameDataHash; asanrt = $rt; tlog = $tl.State
            Kind = $X.Kind; JudgeKinds = $X.JudgeKinds; Files = $files
        }
    }
    $dlls = 'none'
    $vendor = 'none'
    if ($X.DllDir) {
        $dlls = '-'
        $vendor = '-'
        if ([IO.Directory]::Exists($X.DllDir)) {
            $d = [ordered]@{}
            foreach ($f in [IO.Directory]::GetFiles($X.DllDir, '*.dll')) { $d[[IO.Path]::GetFileName($f)] = Get-KeyFileSha $f }
            $dlls = $(if ($d.Count) { $d } else { 'none' })
            $vendor = Get-VendorDllHashes $X.DllDir (Get-SystemDllDir $X.Platform) $X.Platform
        }
    }
    return Get-StepKey @{
        step = $X.Name; platform = $X.Platform; config = $X.Config
        toolset = $Tc.ToolsVersion; cl = $Tc.Cl; ucrt = $Tc.Ucrt
        steam = Get-SteamRunning; overlay = Get-SteamOverlayState; gamedata = $script:GameDataHash
        exe = Get-GateBinaryHashes $X.Binaries; dlls = $dlls; vendor = $vendor
        Kind = $X.Kind; JudgeKinds = $X.JudgeKinds; Files = $files
    }
}

function Test-EvidenceIntact([string]$Hash) {
    if ($Hash -cnotmatch '^[0-9a-f]{64}$') { return "record names no evidence ($Hash)" }
    $dir = Join-Path $EvidenceDir $Hash
    if (-not [IO.Directory]::Exists($dir)) { return "evidence $Hash is missing" }
    if ((Get-EvidenceHash $dir) -cne $Hash) { return "evidence $Hash no longer hashes to its name" }
    return $null
}

function Test-StepEvidence($X, [string]$Hash) {
    $script:JudgeCount++
    $copy = Join-Path $script:RunDir "judge\$($X.Platform)-$($X.Name)-$($script:JudgeCount)"
    New-Item -ItemType Directory -Force -Path (Split-Path $copy -Parent) | Out-Null
    Copy-Item -LiteralPath (Join-Path $EvidenceDir $Hash) -Destination $copy -Recurse
    if ((Get-EvidenceHash $copy) -cne $Hash) { return 'evidence changed while it was copied' }
    $exitFile = Join-Path $copy 'exit.txt'
    if (-not (Test-Path -LiteralPath $exitFile -PathType Leaf)) { return 'no exit.txt' }
    $code = ([IO.File]::ReadAllText($exitFile)).Trim()
    if ($code -cne '0') { return "exit.txt is $code" }
    $stdout = Join-Path $copy 'stdout.txt'
    if (-not (Test-Path -LiteralPath $stdout -PathType Leaf)) { return 'no stdout.txt' }
    if ($script:FailLines.ContainsKey($X.Kind)) {
        $m = [regex]::Match([IO.File]::ReadAllText($stdout), $script:FailLines[$X.Kind])
        if ($m.Success) { return "stdout.txt has a failure line: $($m.Value)" }
    }
    if ($X.Kind -eq 'run') {
        $runs = @(Get-ChildItem -LiteralPath (Join-Path $copy 'runs') -Directory -ErrorAction SilentlyContinue)
        if ($runs.Count -eq 0) { return 'no archived run folder' }
        foreach ($r in $runs) {
            $why = Test-GateRunRecord $r.FullName
            if ($why) { return "run $($r.Name): $why" }
        }
    }
    if ($X.JudgeKinds -contains 'compare') {
        $tables = Join-Path $copy 'out\0'
        if (-not (Test-Path -LiteralPath $tables -PathType Container)) { return 'no archived capture output in out\0' }
        $c = Invoke-GateCompare $tables $X.Compare
        if ($c -ne 0) { return "compare-tables exited $c" }
    }
    return $null
}

function Save-StepEvidence($X, [string]$Work, [int]$Code, [string[]]$NewRuns) {
    $ev = Join-Path $Work 'evidence'
    [IO.File]::WriteAllText((Join-Path $ev 'exit.txt'), "$Code`r`n")
    New-Item -ItemType Directory -Force -Path $EvidenceDir | Out-Null
    $stage = Join-Path $EvidenceDir ('stage-' + [guid]::NewGuid().ToString('N'))
    Copy-Item -LiteralPath $ev -Destination $stage -Recurse
    try {
        if ($X.Kind -eq 'run') {
            $paths = @()
            foreach ($n in $NewRuns) {
                $src = Join-Path $root "artifacts\tests\$n"
                if (@(Get-ReparsePoints $src).Count) { throw "run folder $src holds a reparse point" }
                New-Item -ItemType Directory -Force -Path (Join-Path $stage 'runs') | Out-Null
                Copy-Item -LiteralPath $src -Destination (Join-Path $stage "runs\$n") -Recurse
                $paths += $src
            }
            [IO.File]::WriteAllText((Join-Path $Work 'runs.txt'), (($paths -join "`r`n") + "`r`n"))
        }
        for ($i = 0; $i -lt $X.Outputs.Count; $i++) {
            $src = $X.Outputs[$i]
            if (-not (Test-Path -LiteralPath $src -PathType Container)) { continue }
            if (@(Get-ReparsePoints $src).Count) { throw "output folder $src holds a reparse point" }
            New-Item -ItemType Directory -Force -Path (Join-Path $stage 'out') | Out-Null
            Copy-Item -LiteralPath $src -Destination (Join-Path $stage "out\$i") -Recurse
        }
        $hash = Get-EvidenceHash $stage
        $dest = Join-Path $EvidenceDir $hash
        if ([IO.Directory]::Exists($dest) -and (Get-EvidenceHash $dest) -cne $hash) { Remove-HarnessTree $dest }
        if (-not [IO.Directory]::Exists($dest)) { [IO.Directory]::Move($stage, $dest) }
    }
    finally { if ([IO.Directory]::Exists($stage)) { Remove-HarnessTree $stage } }
    if ((Get-EvidenceHash $dest) -cne $hash) { throw "evidence $dest does not hash to $hash after the copy" }
    return $hash
}

function Get-GateRecord($X) {
    return $script:Records["$($X.Name)`t$($X.Platform)`t$($X.Config)"]
}

function Get-GateDecision($X, $Key) {
    if (-not $Key.Valid) { return @{ Action = 'RUN'; Why = 'key invalid: ' + (@($Key.Header | Where-Object { $_.Split("`t")[1] -cmatch '^-$|=-($|[;,])' } | ForEach-Object { $_.Split("`t")[0] }) -join ', ') } }
    $rec = Get-GateRecord $X
    if (-not (Test-StepRecordMatch $rec $Key)) { return @{ Action = 'RUN'; Why = 'no record with this RunKey' } }
    if ($rec.Result -cne 'PASS') { return @{ Action = 'RUN'; Why = "record result $($rec.Result)" } }
    $why = Test-EvidenceIntact $rec.Evidence
    if ($why) { return @{ Action = 'RUN'; Why = $why } }
    if ($rec.JudgeKey -ceq $Key.JudgeKey) {
        $why = Test-StepEvidence $X $rec.Evidence
        if ($why) { return @{ Action = 'RUN'; Why = "evidence judged FAIL: $why" } }
        return @{ Action = 'CARRY'; Record = $rec }
    }
    if ((Get-StepJudgeOnly @($X.JudgeKinds)).Count -eq 0) { return @{ Action = 'RUN'; Why = 'JudgeKey changed for a step with no judge-only files' } }
    return @{ Action = 'REJUDGE'; Record = $rec }
}

function Add-GateLedger($X, $Key, [string]$Decision, [string]$Result, [string]$Evidence, [datetime]$T0) {
    $t1 = Get-Date
    $line = @($X.Name, $X.Platform, $X.Config, $Tier, $Key.RunKey, $Key.JudgeKey, $Decision, $Result, $Evidence, [int]($t1 - $T0).TotalSeconds, $T0.ToString('s'), $t1.ToString('s')) -join "`t"
    foreach ($f in $script:LedgerFile, $evidenceLedger) {
        if (-not (Test-Path -LiteralPath $f)) { [IO.File]::WriteAllText($f, ($script:LedgerColumns -join "`t") + "`r`n") }
        [IO.File]::AppendAllText($f, $line + "`r`n")
    }
    return [int]($t1 - $T0).TotalSeconds
}

function Add-GateRecord($X, $Key, [string]$Result, [string]$Evidence, [string]$Wall) {
    if ($X.Kind -eq 'build') { return }
    $line = Format-StepRecord @{ Step = $X.Name; RunKey = $Key.RunKey; JudgeKey = $Key.JudgeKey; Platform = $X.Platform; Config = $X.Config; Evidence = $Evidence; Result = $Result; Wall_s = $Wall; Tree = $script:Tree; Commit = $script:Commit; Date = (Get-Date).ToString('s') }
    Merge-StepRecords $recordsFile @($line)
    $script:Records = Read-StepRecords $recordsFile
    $script:Written++
}

function Get-GatePlatformSteps($Steps, [string]$Plat) {
    return @($Steps | Where-Object { -not $_.Platform -or @($_.Platform) -contains $Plat })
}

function Test-GateInTier($S, [string]$T, [string]$Plat) {
    return [bool](@($S.Tiers) | Where-Object { $_ -ceq $T -or $_ -ceq "${T}:$Plat" })
}

function Get-GateRunSteps([string]$Plat) {
    $ps = Get-GatePlatformSteps $all $Plat
    if ($Step) {
        $want = @($Step)
        foreach ($n in $Step) { $want += @($names[$n].Needs | Where-Object { $_ }) }
        return @($ps | Where-Object { $want -contains $_.Name })
    }
    return @($ps | Where-Object { Test-GateInTier $_ $Tier $Plat })
}

function Get-GateStepSmokeGroup($S) {
    $a = @($S.Args)
    $i = [Array]::IndexOf($a, '-Smoke')
    if ($i -ge 0 -and $i + 1 -lt $a.Count) { return [string]$a[$i + 1] }
    return 'Standard'
}

function Get-GateStepConfigs($Steps) {
    $set = @()
    foreach ($s in @($Steps)) {
        $set += $s.Config
        foreach ($b in @($s.Binaries)) { if ("$b" -match '^bin\\\$Platform\\([^\\]+)\\BattlefrontII\.exe$') { $set += $Matches[1] } }
    }
    return @($script:WorkerConfigurations | Where-Object { $set -ccontains $_ })
}

function Get-GateCrossRulePaths([string]$Root, $Steps, [string]$Plat) {
    $out = @()
    foreach ($s in @($Steps)) {
        foreach ($b in @($s.Binaries)) {
            if ("$b" -notmatch '^bin\\(Win32|x64)\\([^\\]+)\\BattlefrontII\.exe$') { continue }
            $bp = $Matches[1]
            $bc = $Matches[2]
            if ($bp -cne $Plat) { $out += Get-FirewallRulePaths $Root @($bp) @($bc) }
        }
    }
    return $out
}

function Get-GateDriveFree([string]$Path) {
    $q = Split-Path -Qualifier $Path
    $di = New-Object IO.DriveInfo $q
    if (-not $di.IsReady) { return @{ Drive = $q; Free = 0 } }
    return @{ Drive = $q; Free = $di.AvailableFreeSpace }
}

function Get-GatePreflightInput([string[]]$Platforms) {
    $paths = @()
    $tools = @()
    $gamePlats = @()
    foreach ($plat in $Platforms) {
        $ps = Get-GateRunSteps $plat
        if ($ps.Count -eq 0) { continue }
        $game = @($ps | Where-Object { $_.StartsGame })
        if ($game.Count) {
            $gamePlats += $plat
            $paths += Get-FirewallRulePaths $root @($plat) (Get-GateStepConfigs $ps)
            $paths += @(Get-GateCrossRulePaths $root $game $plat | Select-Object -Unique | Where-Object { $paths -notcontains $_ })
        }
        $t = Find-VsTools $plat
        $tools += @{
            Platform = $plat
            MSBuild = [bool]($t -and [IO.File]::Exists($t.MSBuild))
            Dumpbin = [bool]($t -and [IO.File]::Exists((Join-Path $t.Bin 'dumpbin.exe')))
            RuntimeDir = Get-SystemDllDir $plat
            MissingRuntimes = @(Get-MissingRuntimes (Get-SystemDllDir $plat) (Get-PlatformRuntimes (Get-GameRuntimes $plat @($game | ForEach-Object { Get-GateStepSmokeGroup $_ })) $plat))
        }
    }
    Add-Type -AssemblyName Microsoft.VisualBasic
    $repo = Get-GateDriveFree $root
    return @{
        Steam = Get-SteamRunning
        Overlay = Get-SteamOverlayState
        RulePaths = $paths
        GamePlatforms = $gamePlats
        Platforms = @($Platforms)
        AllowRules = Get-InboundAllowPrograms
        Wer = @(Test-WerSetup)
        FreeRamKB = [long][Math]::Floor((New-Object Microsoft.VisualBasic.Devices.ComputerInfo).AvailablePhysicalMemory / 1KB)
        RepoDrive = $repo.Drive; RepoFree = $repo.Free
        GameData = Find-GameDataRoot
        Toolchain = $tools
    }
}

function Invoke-GatePreflight($In) {
    $res = [ordered]@{}
    $res.steam = $(if ($In.Steam -ceq 'yes') { @($true, 'Steam running with a logged-in user') } else { @($false, "Steam is not running with a logged-in user (state '$($In.Steam)')") })
    $res.overlay = $(if ($In.Overlay -ceq 'off') { @($true, 'app 6060 overlay off') } else { @($false, "app 6060 overlay is '$($In.Overlay)', not off: set OverlayAppEnable to 0 under apps\6060 in localconfig.vdf with Steam shut down") })
    $allow = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    foreach ($p in @($In.AllowRules)) { [void]$allow.Add($p) }
    $paths = @($In.RulePaths)
    $miss = @($paths | Where-Object { -not $allow.Contains($_) })
    $res.firewall = $(if (@($In.GamePlatforms).Count -eq 0) { @($true, "no selected step starts the game exe on $(@($In.Platforms) -join ', '), so no rule is needed") } elseif ($paths.Count -and $miss.Count -eq 0) { @($true, "$($paths.Count) rule paths have an inbound allow rule") } elseif ($paths.Count -eq 0) { @($false, 'no rule paths for this tier') } else { @($false, "$($miss.Count) of $($paths.Count) rule paths have no enabled inbound allow rule, first $($miss[0]): run tests\firewall.ps1 elevated") })
    $wer = @($In.Wer | Where-Object { $_ })
    $res.wer = $(if ($wer.Count -eq 0) { @($true, 'LocalDumps key and DontShowUI set') } else { @($false, ($wer -join '; ')) })
    $res.ram = $(if ([long]$In.FreeRamKB -ge $script:RamFloorKB) { @($true, "free $($In.FreeRamKB) KB, floor $($script:RamFloorKB) KB") } else { @($false, "free physical memory $($In.FreeRamKB) KB is below the floor $($script:RamFloorKB) KB") })
    $low = @()
    if ([long]$In.RepoFree -lt [long]$script:DiskDropBytes) { $low += "$($In.RepoDrive) free $($In.RepoFree) bytes is below the floor $([long]$script:DiskDropBytes) bytes" }
    if (-not $In.GameData) { $low += 'retail GameData not found (set SWBF2_GAMEDATA or install Steam App 6060; is its drive attached?)' }
    $res.disk = $(if ($low.Count -eq 0) { @($true, "$($In.RepoDrive) free $($In.RepoFree), floor $([long]$script:DiskDropBytes); GameData $($In.GameData)") } else { @($false, ($low -join '; ')) })
    $tc = @()
    foreach ($t in @($In.Toolchain)) {
        if (-not $t.MSBuild) { $tc += "$($t.Platform): MSBuild not found" }
        if (-not $t.Dumpbin) { $tc += "$($t.Platform): dumpbin not found" }
        if (@($t.MissingRuntimes).Count) { $tc += "$($t.Platform): missing from $($t.RuntimeDir): $(@($t.MissingRuntimes) -join ', ')" }
    }
    if (@($In.Toolchain).Count -eq 0) { $tc += 'no platform toolchain checked' }
    $res.toolchain = $(if ($tc.Count -eq 0) { @($true, "MSBuild, dumpbin and runtimes present for $(@($In.Toolchain | ForEach-Object { $_.Platform }) -join ', ')") } else { @($false, ($tc -join '; ')) })
    $failed = @()
    foreach ($c in $script:PreflightChecks) {
        if ($res[$c][0]) { Write-Host "[PASS] preflight:$c $($res[$c][1])" }
        else { Write-Host "[FAIL] preflight:$c $($res[$c][1])"; $failed += $c }
    }
    return $failed
}

function Get-GateOrder($Steps, [string]$Plat) {
    $i = 0
    $tagged = @(foreach ($s in @($Steps)) {
        $rec = $script:Records["$($s.Name)`t$Plat`t$($s.Config)"]
        $w = $(if ($rec -and "$($rec.Wall_s)" -match '^\d+$') { [int]$rec.Wall_s } else { -1 })
        $g = $(if ($s.Kind -eq 'build') { 0 } elseif ($w -lt 0) { 1 } else { 2 })
        New-Object PSObject -Property @{ S = $s; I = $i; G = $g; T = [Math]::Max($w, 0) }
        $i++
    })
    return @($tagged | Sort-Object G, T, I | ForEach-Object { $_.S })
}

function Invoke-GateRetention {
    if (-not $script:RunDir) { return }
    try {
        $dirs = @(Get-ChildItem -LiteralPath $gateRoot -Directory -ErrorAction SilentlyContinue | Sort-Object Name)
        $drop = @($dirs | Select-Object -First ([Math]::Max(0, $dirs.Count - $script:KeepRuns)) | Where-Object { -not $_.FullName.Equals($script:RunDir, [StringComparison]::OrdinalIgnoreCase) })
        foreach ($d in $drop) { Remove-HarnessTree $d.FullName }
        $named = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
        foreach ($r in (Read-StepRecords $recordsFile).Values) { [void]$named.Add([string]$r.Evidence) }
        $gone = @(Get-ChildItem -LiteralPath $EvidenceDir -Directory -ErrorAction SilentlyContinue | Where-Object { $_.Name -cmatch '^[0-9a-f]{64}$' -and -not $named.Contains($_.Name) })
        foreach ($d in $gone) { Remove-HarnessTree $d.FullName }
        Write-Host "gate: retention removed $($drop.Count) run folder(s) under $gateRoot and $($gone.Count) unnamed evidence folder(s)"
    }
    catch { Write-Host "gate: RETENTION FAILED: $($_.Exception.Message)" }
}

function Stop-Gate([int]$Code) {
    if ($script:Written) { Write-Host "gate: $($script:Written) record(s) written to $recordsFile" }
    Invoke-GateRetention
    exit $Code
}

$GateSteps = $null
$script:SourceKeyHeader = @('step', 'platform', 'config', 'toolset', 'cl', 'gamedata', 'asanrt', 'tlog')
. $StepsFile
$all = @($GateSteps)
$problems = @()
$names = @{}
foreach ($s in $all) {
    if (-not $s.Name) { $problems += 'a step has no Name'; continue }
    if ($names.ContainsKey($s.Name)) { $problems += "duplicate step $($s.Name)" }
    $names[$s.Name] = $s
    if ($script:Kinds -notcontains $s.Kind) { $problems += "$($s.Name): Kind '$($s.Kind)'" }
    if (-not $s.Config) { $problems += "$($s.Name): no Config" }
    if (-not $s.Exe) { $problems += "$($s.Name): no Exe" }
    if (-not $s.Tiers) { $problems += "$($s.Name): no Tiers" }
    foreach ($t in @($s.Tiers)) {
        $tp = "$t".Split(':')
        if ($script:Tiers -cnotcontains $tp[0] -or $tp.Count -gt 2 -or ($tp.Count -eq 2 -and $s.Platform -and @($s.Platform) -cnotcontains $tp[1])) { $problems += "$($s.Name): tier '$t'" }
    }
    if ($s.Kind -ne 'build' -and @($s.Inputs | Where-Object { $_ }).Count -eq 0) { $problems += "$($s.Name): no Inputs" }
    if ($s.SourceKey -and (-not $s.SourceKey.Tlog -or -not $s.SourceKey.RuntimeDir)) { $problems += "$($s.Name): SourceKey needs Tlog and RuntimeDir" }
    $outs = @($s.Outputs | Where-Object { $_ })
    if ($script:OutputKinds -contains $s.Kind -and $outs.Count -eq 0) { $problems += "$($s.Name): Kind $($s.Kind) needs Outputs" }
    if ($s.Kind -in 'script', 'build' -and $outs.Count) { $problems += "$($s.Name): Kind $($s.Kind) takes no Outputs" }
    if (@($s.JudgeKinds) -contains 'compare' -and $outs.Count -eq 0) { $problems += "$($s.Name): JudgeKinds compare needs Outputs" }
}
foreach ($s in $all) {
    foreach ($n in @($s.Needs | Where-Object { $_ })) {
        if (-not $names.ContainsKey($n) -or $names[$n].Kind -ne 'build') { $problems += "$($s.Name): Needs '$n' is not a build step" }
    }
}
if ($problems.Count) { $problems | ForEach-Object { Write-Host "gate: $_" }; exit 2 }
if ($Step) {
    $unknown = @($Step | Where-Object { -not $names.ContainsKey($_) })
    if ($unknown.Count) { Write-Host "gate: no step $($unknown -join ', ')"; exit 2 }
    $nowhere = @($Step | Where-Object { $sn = $_; -not @($Platform | Where-Object { @(Get-GatePlatformSteps @($names[$sn]) $_).Count }).Count })
    if ($nowhere.Count) { Write-Host "gate: step $($nowhere -join ', ') is not defined on $($Platform -join ', ')"; exit 2 }
}
elseif (-not @($Platform | Where-Object { @(Get-GateRunSteps $_).Count }).Count) { Write-Host "gate: tier $Tier has no steps on $($Platform -join ', ')"; exit 2 }
$script:Records = Read-StepRecords $recordsFile
if ($ListOrder) {
    foreach ($plat in $Platform) { Write-Host "gate: $Tier $plat order $(@(Get-GateOrder (Get-GateRunSteps $plat) $plat | ForEach-Object { $_.Name }) -join ', ')" }
    exit 0
}
$pfFailed = @(Invoke-GatePreflight (Get-GatePreflightInput $Platform))
if ($pfFailed.Count) { Write-Host "gate: preflight failed: $($pfFailed -join ', '); no step ran"; exit 2 }

if ($Resume) {
    $last = @(Get-ChildItem -LiteralPath $gateRoot -Directory -ErrorAction SilentlyContinue | Sort-Object Name | Select-Object -Last 1)
    if ($last.Count -eq 0 -or -not (Test-Path -LiteralPath (Join-Path $last[0].FullName 'ledger.tsv'))) { Write-Host "gate: -Resume found no ledger under $gateRoot"; exit 2 }
    $script:Stamp = $last[0].Name
}
else {
    $script:Stamp = Split-Path -Leaf (New-StampedDir $gateRoot)
}
$script:RunDir = Join-Path $gateRoot $script:Stamp
New-Item -ItemType Directory -Force -Path $script:RunDir | Out-Null
New-Item -ItemType Directory -Force -Path $EvidenceDir | Out-Null
$script:LedgerFile = Join-Path $script:RunDir 'ledger.tsv'
$ledger = @{}
if ($Resume) { $ledger = Read-KeyedRecords $script:LedgerFile @('Step', 'Platform') }
$script:JudgeCount = 0
$script:Written = 0

try {
    $script:Tree = "$(Invoke-GateGit @('rev-parse', 'HEAD^{tree}'))".Trim()
    $script:Commit = "$(Invoke-GateGit @('rev-parse', 'HEAD'))".Trim()
    $script:TreeFiles = Get-TreeFileHashes $root
}
catch { Write-Host "gate: $($_.Exception.Message)"; exit 2 }
$script:GameDataHash = Get-GameDataManifestHash (Find-GameDataRoot)
Write-Host "gate: tier $Tier platform $($Platform -join ',') run $($script:RunDir)"
Write-Host "gate: evidence $EvidenceDir"

try {
    foreach ($plat in $Platform) {
        $script:SmokePlatform = $plat
        try { $tc = Get-ToolchainInfo $plat }
        catch { $tc = New-Object PSObject -Property @{ Cl = '-'; ToolsVersion = '-'; Ucrt = '-' } }
        Write-Host "gate: $plat toolset $($tc.ToolsVersion) cl $($tc.Cl) ucrt $($tc.Ucrt)"
        $order = Get-GateOrder (Get-GateRunSteps $plat) $plat
        Write-Host "gate: $plat order $(@($order | ForEach-Object { $_.Name }) -join ', ')"
        foreach ($s in $order) {
            $x = Expand-GateStep $s $plat
            $work = Join-Path $script:RunDir "$plat\$($x.Name)"
            $t0 = Get-Date
            $key = Get-GateKey $x $tc
            $l = $ledger["$($x.Name)`t$plat"]
            if ($l -and $l.Result -ceq 'PASS' -and $key.Valid -and $l.RunKey -ceq $key.RunKey -and $l.JudgeKey -ceq $key.JudgeKey) {
                Write-Host "[RESUMED] gate:$($x.Name) $plat ledger $($l.End)"
                if ($l.Decision -cne 'CARRY') { Add-GateRecord $x $key 'PASS' $l.Evidence $l.Wall_s }
                continue
            }
            if (Test-Path -LiteralPath $work) { Remove-HarnessTree $work }
            if ($x.Kind -eq 'build') {
                Write-Host "gate: $($x.Name) $plat build"
                $code = Invoke-GateCommand $x (Join-Path $work 'stdout.txt')
                $post = Get-GateKey $x $tc
                $result = $(if ($code -eq 0) { 'PASS' } else { 'FAIL' })
                [void](Add-GateLedger $x $post 'RUN' $result '-' $t0)
                Write-Host "[$result] gate:$($x.Name) $plat exit $code"
                if ($code -ne 0) { Stop-Gate 1 }
                continue
            }
            $d = Get-GateDecision $x $key
            if ($d.Action -eq 'CARRY') {
                Write-Host "[CARRY] gate:$($x.Name) $plat run=$($key.RunKey) judge=$($key.JudgeKey) evidence=$($d.Record.Evidence) recorded $($d.Record.Date) at $($d.Record.Commit)"
                [void](Add-GateLedger $x $key 'CARRY' 'PASS' $d.Record.Evidence $t0)
                continue
            }
            if ($d.Action -eq 'REJUDGE') {
                Write-Host "gate: $($x.Name) $plat re-judging evidence $($d.Record.Evidence) (JudgeKey $($d.Record.JudgeKey) -> $($key.JudgeKey))"
                $why = Test-StepEvidence $x $d.Record.Evidence
                $result = $(if ($why) { 'FAIL' } else { 'PASS' })
                [void](Add-GateLedger $x $key 'REJUDGE' $result $d.Record.Evidence $t0)
                Add-GateRecord $x $key $result $d.Record.Evidence $d.Record.Wall_s
                if ($why) { Write-Host "[FAIL] gate:$($x.Name) $plat re-judge: $why"; Stop-Gate 1 }
                Write-Host "[REJUDGE] gate:$($x.Name) $plat PASS run=$($key.RunKey) judge=$($key.JudgeKey) evidence=$($d.Record.Evidence)"
                continue
            }
            Write-Host "gate: $($x.Name) $plat run ($($d.Why))"
            $testsArt = Join-Path $root 'artifacts\tests'
            $before = @(Get-ChildItem -LiteralPath $testsArt -Directory -ErrorAction SilentlyContinue | ForEach-Object { $_.Name })
            $code = Invoke-GateCommand $x (Join-Path $work 'evidence\stdout.txt')
            $newRuns = @(Get-ChildItem -LiteralPath $testsArt -Directory -ErrorAction SilentlyContinue | Where-Object { $before -notcontains $_.Name } | ForEach-Object { $_.Name })
            $hash = Save-StepEvidence $x $work $code $newRuns
            $result = $(if ($code -eq 0) { 'PASS' } else { 'FAIL' })
            $wall = Add-GateLedger $x $key 'RUN' $result $hash $t0
            Add-GateRecord $x $key $result $hash ([string]$wall)
            Write-Host "[$result] gate:$($x.Name) $plat exit $code evidence=$hash"
            if ($code -ne 0) { Stop-Gate 1 }
        }
    }
}
catch {
    Write-Host "gate: ERROR: $($_.Exception.Message)"
    Write-Host $_.InvocationInfo.PositionMessage
    Stop-Gate 1
}
if ($Step) { Write-Host "gate: tier $Tier partial" }
else { Write-Host "gate: tier $Tier passed" }
Stop-Gate 0
