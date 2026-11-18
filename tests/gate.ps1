param(
    [Parameter(Mandatory)][ValidateSet('plan', 'pr', 'branch', 'milestone')][string]$Tier,
    [ValidateSet('Win32', 'x64')][string[]]$Platform = @('Win32'),
    [string]$Job,
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
. (Join-Path $testsDir 'smoke.ps1')
. (Join-Path $testsDir 'gate\keys.ps1')
if ($Step) { $Step = @($Step | ForEach-Object { $_ -split ',' } | Where-Object { $_ }) }
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
$script:Kinds = @('build', 'run', 'compare', 'capture', 'selftest', 'guard', 'census')
$script:OutputKinds = @('guard', 'census', 'capture', 'compare')
$script:FailLines = @{
    selftest = '(?m)^FAILED'
    capture = '(?m)^\[FAIL\] '
    guard = '(?m)^(STATUS CHECK FAILED|PROOF MERGE FAILED|WARNING: main tree status changed)'
}
$script:LedgerColumns = @('Step', 'Platform', 'Config', 'Tier', 'RunKey', 'JudgeKey', 'Decision', 'Result', 'Evidence', 'Wall_s', 'Start', 'End')
$recordsFile = Join-Path $root 'tests\guardrevert\steps.tsv'
$script:PreflightChecks = @('steam', 'overlay', 'firewall', 'wer', 'ram', 'disk', 'toolchain')
$script:RamFloorKB = 5098436
$script:DiskDropBytes = 14.42GB
$script:WorktreeBytes = 994746905

function Invoke-GateGit([string[]]$GitArgs) {
    $ErrorActionPreference = 'Continue'
    $out = & git -C $root @GitArgs 2>&1
    if ($LASTEXITCODE -ne 0) { throw "git $($GitArgs -join ' ') failed: $out" }
    return $out
}

function Get-GateStatus {
    $a = @('status', '--porcelain', '--untracked-files=normal', '--', '.') + @($script:KeyRecordFiles | ForEach-Object { ':!' + $_.Replace('\', '/') })
    return ((Invoke-GateGit $a) -join "`n")
}

function Invoke-GateRejudge([string]$Dir) {
    $ErrorActionPreference = 'Continue'
    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $testsDir 'run.ps1') -Rejudge $Dir 2>&1 | ForEach-Object { Write-Host "$_" }
    return $LASTEXITCODE
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

function Expand-GateStep($S, [string]$Plat) {
    $def = @(
        "kind`t$($S.Kind)", "config`t$($S.Config)", "exe`t$(Expand-GateText $S.Exe $Plat $false)",
        "args`t$(@($S.Args | ForEach-Object { Expand-GateText $_ $Plat $false }) -join [char]0x1f)",
        "outputs`t$(@($S.Outputs | ForEach-Object { Expand-GateText $_ $Plat $false }) -join [char]0x1f)",
        "binaries`t$(@($S.Binaries | ForEach-Object { Expand-GateText $_ $Plat $false }) -join [char]0x1f)",
        "dlldir`t$(if ($S.DllDir) { Expand-GateText $S.DllDir $Plat $false })",
        "judgekinds`t$(@($S.JudgeKinds) -join ',')",
        "compare`t$(@($S.Compare) -join [char]0x1f)"
    ) -join "`n"
    @{
        Name = $S.Name; Kind = $S.Kind; Config = $S.Config; Platform = $Plat
        JudgeKinds = @($S.JudgeKinds | Where-Object { $_ }); Compare = @($S.Compare | Where-Object { $_ })
        Exe = Expand-GateText $S.Exe $Plat $true
        Args = @($S.Args | ForEach-Object { Expand-GateText $_ $Plat $true })
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

function Get-GateGuardSelection($X) {
    $ErrorActionPreference = 'Continue'
    $global:LASTEXITCODE = -1
    $out = @(& $X.Exe @($X.Args) -ListSelected 2>&1 | ForEach-Object { "$_" })
    if ($global:LASTEXITCODE -ne 0) { return '-' }
    $ids = @($out | Where-Object { $_ -match '^SELECT (\S+)' } | ForEach-Object { $_.Split(' ')[1] })
    if ($ids.Count -eq 0) { return 'none' }
    return $ids
}

function Get-GateKey($X, $Tc) {
    $dlls = 'none'
    $vendor = 'none'
    if ($X.DllDir) {
        $dlls = '-'
        $vendor = '-'
        if ([IO.Directory]::Exists($X.DllDir)) {
            $d = [ordered]@{}
            foreach ($f in [IO.Directory]::GetFiles($X.DllDir, '*.dll')) { $d[[IO.Path]::GetFileName($f)] = Get-KeyFileSha $f }
            $dlls = $(if ($d.Count) { $d } else { 'none' })
            $vendor = Get-VendorDllHashes $X.DllDir (Get-SystemDllDir $X.Platform)
        }
    }
    $files = $script:TreeFiles.Clone()
    $files['gate-step-definition'] = $X.Definition
    $in = @{
        step = $X.Name; platform = $X.Platform; config = $X.Config; tier = $Tier
        toolset = $Tc.ToolsVersion; cl = $Tc.Cl; ucrt = $Tc.Ucrt
        steam = Get-SteamRunning; overlay = Get-SteamOverlayState; gamedata = $script:GameDataHash
        exe = Get-GateBinaryHashes $X.Binaries; dlls = $dlls; vendor = $vendor
        Kind = $X.Kind; JudgeKinds = $X.JudgeKinds; Files = $files
    }
    if ($X.Kind -eq 'guard') { $in['selection'] = Get-GateGuardSelection $X }
    return Get-StepKey $in
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
            $c = Invoke-GateRejudge $r.FullName
            if ($c -ne 0) { return "run.ps1 -Rejudge $($r.Name) exited $c" }
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
    if ($X.Kind -eq 'run') {
        foreach ($n in $NewRuns) {
            $src = Join-Path $root "artifacts\tests\$n"
            if (@(Get-ReparsePoints $src).Count) { throw "run folder $src holds a reparse point" }
            New-Item -ItemType Directory -Force -Path (Join-Path $ev 'runs') | Out-Null
            Copy-Item -LiteralPath $src -Destination (Join-Path $ev "runs\$n") -Recurse
        }
    }
    for ($i = 0; $i -lt $X.Outputs.Count; $i++) {
        $src = $X.Outputs[$i]
        if (-not (Test-Path -LiteralPath $src -PathType Container)) { continue }
        if (@(Get-ReparsePoints $src).Count) { throw "output folder $src holds a reparse point" }
        New-Item -ItemType Directory -Force -Path (Join-Path $ev 'out') | Out-Null
        Copy-Item -LiteralPath $src -Destination (Join-Path $ev "out\$i") -Recurse
    }
    $hash = Get-EvidenceHash $ev
    $dest = Join-Path $EvidenceDir $hash
    if ([IO.Directory]::Exists($dest) -and (Get-EvidenceHash $dest) -cne $hash) { Remove-HarnessTree $dest }
    if (-not [IO.Directory]::Exists($dest)) {
        New-Item -ItemType Directory -Force -Path $EvidenceDir | Out-Null
        Copy-Item -LiteralPath $ev -Destination $dest -Recurse
    }
    if ((Get-EvidenceHash $dest) -cne $hash) { throw "evidence $dest does not hash to $hash after the copy" }
    return $hash
}

function Get-GateDecision($X, $Key) {
    if (-not $Key.Valid) { return @{ Action = 'RUN'; Why = 'key invalid: ' + (@($Key.Header | Where-Object { $_.Split("`t")[1] -cmatch '^-$|=-($|[;,])' } | ForEach-Object { $_.Split("`t")[0] }) -join ', ') } }
    $rec = $script:Records["$($X.Name)`t$($X.Platform)`t$($X.Config)`t$Tier"]
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
    if (-not (Test-Path -LiteralPath $script:LedgerFile)) { [IO.File]::WriteAllText($script:LedgerFile, ($script:LedgerColumns -join "`t") + "`r`n") }
    $line = @($X.Name, $X.Platform, $X.Config, $Tier, $Key.RunKey, $Key.JudgeKey, $Decision, $Result, $Evidence, [int]($t1 - $T0).TotalSeconds, $T0.ToString('s'), $t1.ToString('s')) -join "`t"
    [IO.File]::AppendAllText($script:LedgerFile, $line + "`r`n")
}

function Add-GateRecord($X, $Key, [string]$Result, [string]$Evidence) {
    if ($X.Kind -eq 'build') { return }
    $script:NewRecords += Format-StepRecord @{ Step = $X.Name; RunKey = $Key.RunKey; JudgeKey = $Key.JudgeKey; Platform = $X.Platform; Config = $X.Config; Tier = $Tier; Evidence = $Evidence; Result = $Result; Tree = $script:Tree; Commit = $script:Commit; Date = (Get-Date).ToString('s') }
}

function Write-GateRecords {
    if ($script:NewRecords.Count -eq 0) { return }
    if (-not $script:TreeClean) { Write-Host 'RECORDS NOT WRITTEN: tree not clean at start'; return }
    try {
        $after = Get-GateStatus
        $head = "$(Invoke-GateGit @('rev-parse', 'HEAD'))".Trim()
    }
    catch { Write-Host "RECORDS NOT WRITTEN: status check failed: $($_.Exception.Message)"; return }
    if ($after -or $head -cne $script:Commit) { Write-Host 'RECORDS NOT WRITTEN: tree or HEAD changed during the run'; return }
    Merge-StepRecords $recordsFile $script:NewRecords
    Write-Host "gate: $($script:NewRecords.Count) record(s) written to $recordsFile"
}

function Write-GateTimes {
    $file = Join-Path $root 'tests\baseline\gate-times.tsv'
    $cols = @('step', 'tier', 'platform', 'before_s', 'after_s', 'note')
    $lines = New-Object System.Collections.Generic.List[string]
    if (Test-Path -LiteralPath $file -PathType Leaf) { foreach ($l in [IO.File]::ReadAllLines($file)) { if ($l) { $lines.Add($l) } } }
    if ($lines.Count -eq 0) { $lines.Add($cols -join "`t") }
    $seen = @{}
    for ($i = 1; $i -lt $lines.Count; $i++) {
        $f = $lines[$i].Split("`t")
        $k = "$($f[0])`t$($f[2])"
        if ($f.Count -ge 6 -and $script:Times.ContainsKey($k)) {
            $f[4] = $script:Times[$k]
            $lines[$i] = $f -join "`t"
            $seen[$k] = $true
        }
    }
    foreach ($k in $script:TimeOrder) {
        if ($seen.ContainsKey($k)) { continue }
        $p = $k.Split("`t")
        $lines.Add((@($p[0], $Tier, $p[1], '-', $script:Times[$k], 'no reference row') -join "`t"))
    }
    [IO.File]::WriteAllText($file, ($lines -join "`r`n") + "`r`n")
    Write-Host "gate: after_s written to $file"
}

function Set-GateTime($X, [string]$Value) {
    $k = "$($X.Name)`t$($X.Platform)"
    if (-not $script:Times.ContainsKey($k)) { $script:TimeOrder += $k }
    $script:Times[$k] = $Value
}

function Get-GatePlatformSteps($Steps, [string]$Plat) {
    return @($Steps | Where-Object { -not $_.Platform -or @($_.Platform) -contains $Plat })
}

function Test-GateStepStartsGame($S, [string]$Plat) {
    if ($S.Kind -ne 'guard') { return [bool]$S.StartsGame }
    $GuardRevertRows = $null
    . (Join-Path $testsDir 'guardrevert\rows.ps1')
    . (Join-Path $testsDir 'guardrevert\select.ps1')
    return [bool]@(Select-GuardPlatformRows @($GuardRevertRows) $Plat | Where-Object { @('Smoke', 'Perturb') -contains (Get-GuardRowKind $_) }).Count
}

function Get-GateStepConfigs($Steps) {
    $set = @()
    foreach ($s in @($Steps)) {
        if ($s.Kind -eq 'guard') { $set += $script:WorkerConfigurations; continue }
        $set += $s.Config
        foreach ($b in @($s.Binaries)) { if ("$b" -match '^bin\\\$Platform\\([^\\]+)\\BattlefrontII\.exe$') { $set += $Matches[1] } }
    }
    return @($script:WorkerConfigurations | Where-Object { $set -ccontains $_ })
}

function Get-GuardWorkerCount {
    $ast = [Management.Automation.Language.Parser]::ParseFile((Join-Path $testsDir 'guard-revert.ps1'), [ref]$null, [ref]$null)
    $d = @{}
    foreach ($p in $ast.ParamBlock.Parameters) { if ($p.DefaultValue) { $d[$p.Name.VariablePath.UserPath] = $p.DefaultValue.Extent.Text } }
    return [int](& ([scriptblock]::Create("`$BuildJobs = $($d['BuildJobs']); $($d['Workers'])")))
}

function Get-GateDriveFree([string]$Path) {
    $q = Split-Path -Qualifier $Path
    $di = New-Object IO.DriveInfo $q
    if (-not $di.IsReady) { return @{ Drive = $q; Free = 0 } }
    return @{ Drive = $q; Free = $di.AvailableFreeSpace }
}

function Get-GatePreflightInput($Steps, [string[]]$Platforms) {
    $paths = @()
    $tools = @()
    $gamePlats = @()
    $missingTrees = 0
    $wtRoot = "$root-worktrees"
    foreach ($plat in $Platforms) {
        $ps = Get-GatePlatformSteps $Steps $plat
        if ($ps.Count -eq 0) { continue }
        if (@($ps | Where-Object { Test-GateStepStartsGame $_ $plat }).Count) {
            $gamePlats += $plat
            $paths += Get-FirewallRulePaths $root @($plat) (Get-GateStepConfigs $ps)
        }
        if (@($ps | Where-Object { $_.Kind -eq 'guard' }).Count) {
            $n = Get-GuardWorkerCount
            for ($i = 1; $i -le $n; $i++) { if (-not [IO.Directory]::Exists((Join-Path $wtRoot "$plat\w$i"))) { $missingTrees++ } }
        }
        $t = Find-VsTools $plat
        $tools += @{
            Platform = $plat
            MSBuild = [bool]($t -and [IO.File]::Exists($t.MSBuild))
            Dumpbin = [bool]($t -and [IO.File]::Exists((Join-Path $t.Bin 'dumpbin.exe')))
            RuntimeDir = Get-SystemDllDir $plat
            MissingRuntimes = @(Get-MissingRuntimes (Get-SystemDllDir $plat) (Get-PlatformRuntimes $GameRuntimes $plat))
        }
    }
    Add-Type -AssemblyName Microsoft.VisualBasic
    $repo = Get-GateDriveFree $root
    $wt = Get-GateDriveFree $wtRoot
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
        WorktreeDrive = $wt.Drive; WorktreeFree = $wt.Free; WorktreesMissing = $missingTrees
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
    $need = [ordered]@{}
    $free = @{}
    $need[$In.RepoDrive] = [long]$script:DiskDropBytes
    $free[$In.RepoDrive] = [long]$In.RepoFree
    if ([int]$In.WorktreesMissing -gt 0) {
        $need[$In.WorktreeDrive] = [long]$need[$In.WorktreeDrive] + [int]$In.WorktreesMissing * [long]$script:WorktreeBytes
        if (-not $free.ContainsKey($In.WorktreeDrive)) { $free[$In.WorktreeDrive] = [long]$In.WorktreeFree }
    }
    $low = @($need.Keys | Where-Object { $free[$_] -lt $need[$_] } | ForEach-Object { "$_ free $($free[$_]) bytes is below the floor $($need[$_]) bytes" })
    if (-not $In.GameData) { $low += 'retail GameData not found (set SWBF2_GAMEDATA or install Steam App 6060; is its drive attached?)' }
    $res.disk = $(if ($low.Count -eq 0) { @($true, ((@($need.Keys | ForEach-Object { "$_ free $($free[$_]), floor $($need[$_])" }) -join '; ') + "; GameData $($In.GameData)")) } else { @($false, ($low -join '; ')) })
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

function Get-GateTimeTable {
    $file = Join-Path $root 'tests\baseline\gate-times.tsv'
    $t = @{}
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { return $t }
    foreach ($l in @([IO.File]::ReadAllLines($file) | Select-Object -Skip 1)) {
        $f = $l.Split("`t")
        if ($f.Count -lt 5) { continue }
        if ($f[4] -match '^\d+$') { $t["$($f[0])`t$($f[2])"] = [int]$f[4] }
        elseif ($f[3] -match '^\d+$') { $t["$($f[0])`t$($f[2])"] = [int]$f[3] }
    }
    return $t
}

function Get-GateOrder($Steps, [string]$Plat, $Table) {
    $i = 0
    $tagged = @(foreach ($s in (Get-GatePlatformSteps $Steps $Plat)) {
        $k = "$($s.Name)`t$Plat"
        $g = $(if ($s.Kind -eq 'build') { 0 } elseif (-not $Table.ContainsKey($k)) { 1 } else { 2 })
        New-Object PSObject -Property @{ S = $s; I = $i; G = $g; T = $(if ($g -eq 2) { $Table[$k] } else { 0 }) }
        $i++
    })
    return @($tagged | Sort-Object G, T, I | ForEach-Object { $_.S })
}

function Stop-Gate([int]$Code) {
    Write-GateRecords
    exit $Code
}

$GateSteps = $null
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
    $outs = @($s.Outputs | Where-Object { $_ })
    if ($script:OutputKinds -contains $s.Kind -and $outs.Count -eq 0) { $problems += "$($s.Name): Kind $($s.Kind) needs Outputs" }
    if ($s.Kind -in 'selftest', 'build' -and $outs.Count) { $problems += "$($s.Name): Kind $($s.Kind) takes no Outputs" }
    if (@($s.JudgeKinds) -contains 'compare' -and $outs.Count -eq 0) { $problems += "$($s.Name): JudgeKinds compare needs Outputs" }
}
foreach ($s in $all) {
    foreach ($n in @($s.Needs | Where-Object { $_ })) {
        if (-not $names.ContainsKey($n) -or $names[$n].Kind -ne 'build') { $problems += "$($s.Name): Needs '$n' is not a build step" }
    }
}
if ($problems.Count) { $problems | ForEach-Object { Write-Host "gate: $_" }; exit 2 }
if ($Job -and @($all | Where-Object { @($_.Tiers) -contains $Tier -and $_.Job -eq $Job }).Count -eq 0) { Write-Host "gate: tier $Tier has no job $Job"; exit 2 }
$tierSteps = @($all | Where-Object { @($_.Tiers) -contains $Tier -and (-not $Job -or -not $_.Job -or $_.Job -eq $Job) })
if ($tierSteps.Count -eq 0) { Write-Host "gate: tier $Tier has no steps"; exit 2 }
$runSet = $tierSteps
if ($Step) {
    $tierNames = @($tierSteps | ForEach-Object { $_.Name })
    $unknown = @($Step | Where-Object { $tierNames -notcontains $_ })
    if ($unknown.Count) { Write-Host "gate: no step $($unknown -join ', ') in tier $Tier"; exit 2 }
    $want = @($Step)
    foreach ($n in $Step) { $want += @($names[$n].Needs | Where-Object { $_ }) }
    $runSet = @($tierSteps | Where-Object { $want -contains $_.Name })
}
$timeTable = Get-GateTimeTable
if ($ListOrder) {
    foreach ($plat in $Platform) { Write-Host "gate: $Tier $plat order $(@(Get-GateOrder $tierSteps $plat $timeTable | ForEach-Object { $_.Name }) -join ', ')" }
    exit 0
}
$pfFailed = @(Invoke-GatePreflight (Get-GatePreflightInput $tierSteps $Platform))
if ($pfFailed.Count) { Write-Host "gate: preflight failed: $($pfFailed -join ', '); no step ran"; exit 2 }

$gateRoot = Join-Path $root 'artifacts\gate'
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
$script:LedgerFile = Join-Path $script:RunDir 'ledger.tsv'
$ledger = @{}
if ($Resume) { $ledger = Read-KeyedRecords $script:LedgerFile @('Step', 'Platform') }
$script:JudgeCount = 0
$script:NewRecords = @()
$script:Times = @{}
$script:TimeOrder = @()

try {
    $script:TreeClean = -not (Get-GateStatus)
    $script:Tree = "$(Invoke-GateGit @('rev-parse', 'HEAD^{tree}'))".Trim()
    $script:Commit = "$(Invoke-GateGit @('rev-parse', 'HEAD'))".Trim()
    $script:TreeFiles = Get-TreeFileHashes $root
}
catch { Write-Host "gate: $($_.Exception.Message)"; exit 2 }
$script:GameDataHash = Get-GameDataManifestHash (Find-GameDataRoot)
$script:Records = Read-StepRecords $recordsFile
Write-Host "gate: tier $Tier platform $($Platform -join ',') run $($script:RunDir)"
Write-Host "gate: evidence $EvidenceDir"
if (-not $script:TreeClean) { Write-Host 'gate: tree not clean, so no record will be written' }

try {
    foreach ($plat in $Platform) {
        $script:SmokePlatform = $plat
        try { $tc = Get-ToolchainInfo $plat }
        catch { $tc = New-Object PSObject -Property @{ Cl = '-'; ToolsVersion = '-'; Ucrt = '-' } }
        Write-Host "gate: $plat toolset $($tc.ToolsVersion) cl $($tc.Cl) ucrt $($tc.Ucrt)"
        $order = Get-GateOrder $runSet $plat $timeTable
        Write-Host "gate: $plat order $(@($order | ForEach-Object { $_.Name }) -join ', ')"
        foreach ($s in $order) {
            $x = Expand-GateStep $s $plat
            $work = Join-Path $script:RunDir "$plat\$($x.Name)"
            $t0 = Get-Date
            $key = Get-GateKey $x $tc
            $l = $ledger["$($x.Name)`t$plat"]
            if ($l -and $l.Result -ceq 'PASS' -and $key.Valid -and $l.RunKey -ceq $key.RunKey -and $l.JudgeKey -ceq $key.JudgeKey) {
                Write-Host "[RESUMED] gate:$($x.Name) $plat ledger $($l.End)"
                if ($l.Decision -cne 'CARRY') { Add-GateRecord $x $key 'PASS' $l.Evidence }
                Set-GateTime $x $(if ($l.Decision -ceq 'CARRY') { 'carried' } elseif ($l.Decision -ceq 'REJUDGE') { 'rejudged' } else { $l.Wall_s })
                continue
            }
            if (Test-Path -LiteralPath $work) { Remove-HarnessTree $work }
            if ($x.Kind -eq 'build') {
                Write-Host "gate: $($x.Name) $plat build"
                $code = Invoke-GateCommand $x (Join-Path $work 'stdout.txt')
                $post = Get-GateKey $x $tc
                $result = $(if ($code -eq 0) { 'PASS' } else { 'FAIL' })
                Add-GateLedger $x $post 'RUN' $result '-' $t0
                Write-Host "[$result] gate:$($x.Name) $plat exit $code"
                if ($code -ne 0) { Stop-Gate 1 }
                Set-GateTime $x ([string][int]((Get-Date) - $t0).TotalSeconds)
                continue
            }
            $d = Get-GateDecision $x $key
            if ($d.Action -eq 'CARRY') {
                Write-Host "[CARRY] gate:$($x.Name) $plat run=$($key.RunKey) judge=$($key.JudgeKey) evidence=$($d.Record.Evidence) recorded $($d.Record.Date) at $($d.Record.Commit)"
                Add-GateLedger $x $key 'CARRY' 'PASS' $d.Record.Evidence $t0
                Set-GateTime $x 'carried'
                continue
            }
            if ($d.Action -eq 'REJUDGE') {
                Write-Host "gate: $($x.Name) $plat re-judging evidence $($d.Record.Evidence) (JudgeKey $($d.Record.JudgeKey) -> $($key.JudgeKey))"
                $why = Test-StepEvidence $x $d.Record.Evidence
                $result = $(if ($why) { 'FAIL' } else { 'PASS' })
                Add-GateLedger $x $key 'REJUDGE' $result $d.Record.Evidence $t0
                Add-GateRecord $x $key $result $d.Record.Evidence
                if ($why) { Write-Host "[FAIL] gate:$($x.Name) $plat re-judge: $why"; Stop-Gate 1 }
                Write-Host "[REJUDGE] gate:$($x.Name) $plat PASS run=$($key.RunKey) judge=$($key.JudgeKey) evidence=$($d.Record.Evidence)"
                Set-GateTime $x 'rejudged'
                continue
            }
            Write-Host "gate: $($x.Name) $plat run ($($d.Why))"
            $testsArt = Join-Path $root 'artifacts\tests'
            $before = @(Get-ChildItem -LiteralPath $testsArt -Directory -ErrorAction SilentlyContinue | ForEach-Object { $_.Name })
            $code = Invoke-GateCommand $x (Join-Path $work 'evidence\stdout.txt')
            $newRuns = @(Get-ChildItem -LiteralPath $testsArt -Directory -ErrorAction SilentlyContinue | Where-Object { $before -notcontains $_.Name } | ForEach-Object { $_.Name })
            $hash = Save-StepEvidence $x $work $code $newRuns
            $result = $(if ($code -eq 0) { 'PASS' } else { 'FAIL' })
            Add-GateLedger $x $key 'RUN' $result $hash $t0
            Add-GateRecord $x $key $result $hash
            Write-Host "[$result] gate:$($x.Name) $plat exit $code evidence=$hash"
            if ($code -ne 0) { Stop-Gate 1 }
            Set-GateTime $x ([string][int]((Get-Date) - $t0).TotalSeconds)
        }
    }
}
catch {
    Write-Host "gate: ERROR: $($_.Exception.Message)"
    Write-Host $_.InvocationInfo.PositionMessage
    Stop-Gate 1
}
if ($Step -or $Job) { Write-Host "gate: tier $Tier partial" }
else {
    Write-Host "gate: tier $Tier passed"
    if ($Tier -eq 'milestone') { Write-GateTimes }
}
Stop-Gate 0
