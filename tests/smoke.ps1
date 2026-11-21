if ($PSScriptRoot) { if (-not $script:HarnessCommonLoaded) { . (Join-Path $PSScriptRoot 'common.ps1') }; . (Join-Path $PSScriptRoot 'smoke\cases.ps1') }
$script:SmokeMarker = 'OPENSWBF2 RUN COMPLETE'
$script:WerKeyPath = 'SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\BattlefrontII.exe'
$script:WerBasePath = 'SOFTWARE\Microsoft\Windows\Windows Error Reporting'
$script:SourceExtensions = @('.h', '.hpp', '.inl', '.c', '.cpp')
$script:T1Suites = @('unit', 'characterisation', 'regression', 'known_bug', 'corpus', 'fuzz', 'stress')
$script:T1NotForPlatform = @(
    @{ Platform = 'x64'; Name = 'math corpus'; Reason = 'Win32 toolchain corpus: compares VS2013 and v143 x86 CRT results; x64 sim math is checked by sim math corpus' },
    @{ Platform = 'x64'; Name = 'math corpus x87 pc24'; Reason = 'x87 precision control does not exist on x64' }
)
$script:SmokeAsanCoverage = 'ASan covers stack, globals, CRT heap and the game heaps and pools'
$script:SmokeWerKey = $null

function Get-SmokeClass($skipped, $killed, $code, $logText, $dumps) {
    if ($skipped) { return 'Skipped' }
    if ($killed) { return 'Stall' }
    if ($code -eq 3221226505) { return 'GsFailure' }
    if ($code -eq 1 -or $code -eq 3 -or $code -eq 4 -or $code -eq 5 -or $logText -match 'FATAL:' -or @($dumps).Count -gt 0) { return 'Fatal' }
    if ($code -eq 0 -and $logText -match $script:SmokeMarker) { return 'Pass' }
    if ($code -eq 0) { return 'InitFailure' }
    return 'Unknown'
}

function Get-SmokeToken($class) {
    switch ($class) {
        'Pass' { return 'PASS' }
        'Skipped' { return 'SKIP' }
        'Stall' { return 'STALL' }
        'Fatal' { return 'CRASH' }
        'GsFailure' { return 'CRASH' }
        default { return 'FAIL' }
    }
}

function Split-T1NotForPlatform($cases, [string]$platform, [bool]$checkStale) {
    $names = @($cases | ForEach-Object { $_.Name })
    $skipped = @()
    $stale = @()
    foreach ($r in @($script:T1NotForPlatform | Where-Object { $_.Platform -ceq $platform })) {
        if ($names -ccontains $r.Name) { $skipped += $r }
        elseif ($checkStale) { $stale += $r }
    }
    $skipNames = @($skipped | ForEach-Object { $_.Name })
    $keep = @($cases | Where-Object { $skipNames -cnotcontains $_.Name })
    return @{ Run = $keep; Skipped = $skipped; Stale = $stale }
}

function Test-WerKey {
    $base = $null
    $sub = $null
    try {
        $base = [Microsoft.Win32.RegistryKey]::OpenBaseKey('LocalMachine', 'Registry64')
        $sub = $base.OpenSubKey($script:WerKeyPath)
        return [bool]$sub
    }
    catch { return $false }
    finally {
        if ($sub) { $sub.Close() }
        if ($base) { $base.Close() }
    }
}

function Test-WerSetup {
    $warnings = @()
    if (-not (Test-WerKey)) {
        $warnings += 'WER LocalDumps key for BattlefrontII.exe is missing; import tools\wer-localdumps.reg as administrator'
    }
    $dontShow = $false
    foreach ($hive in 'LocalMachine', 'CurrentUser') {
        $base = $null
        $sub = $null
        try {
            $base = [Microsoft.Win32.RegistryKey]::OpenBaseKey($hive, 'Registry64')
            $sub = $base.OpenSubKey($script:WerBasePath)
            if ($sub -and $sub.GetValue('DontShowUI') -eq 1) { $dontShow = $true }
        }
        catch {}
        finally {
            if ($sub) { $sub.Close() }
            if ($base) { $base.Close() }
        }
    }
    if (-not $dontShow) {
        $warnings += 'WER DontShowUI is not set; a crash can leave a dialog open (see tools\wer-localdumps.reg)'
    }
    return $warnings
}

function Get-WerDumpFolder {
    $base = $null
    $sub = $null
    try {
        $base = [Microsoft.Win32.RegistryKey]::OpenBaseKey('LocalMachine', 'Registry64')
        $sub = $base.OpenSubKey($script:WerKeyPath)
        if ($sub) {
            $folder = $sub.GetValue('DumpFolder')
            if ($folder) { return [string]$folder }
        }
    }
    catch {}
    finally {
        if ($sub) { $sub.Close() }
        if ($base) { $base.Close() }
    }
    return (Join-Path $env:LOCALAPPDATA 'OpenSWBF2\crashes')
}

function Invoke-SmokeCase($name, $exe, $workDir, $argLine, $caseDir, $asanOptions = $null) {
    New-Item -ItemType Directory -Force -Path $caseDir | Out-Null
    $scoreLog = Join-Path $workDir 'finalscore.log'
    Remove-Item -LiteralPath $scoreLog -Force -ErrorAction SilentlyContinue
    $appId = Join-Path $workDir 'steam_appid.txt'
    if (-not (Test-Path -LiteralPath $appId)) { Set-Content -Path $appId -Value '6060' -Encoding ASCII }

    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exe
    $psi.Arguments = $argLine
    $psi.WorkingDirectory = $workDir
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    if ($asanOptions) { $psi.EnvironmentVariables['ASAN_OPTIONS'] = $asanOptions }
    Initialize-OutputCapture
    $start = Get-Date
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $p = [System.Diagnostics.Process]::Start($psi)
    try { $p.PriorityClass = [System.Diagnostics.ProcessPriorityClass]::BelowNormal } catch {}
    $o = New-OutputCapture $p.StandardOutput.BaseStream $p.StandardOutput.CurrentEncoding
    $e = New-OutputCapture $p.StandardError.BaseStream $p.StandardError.CurrentEncoding
    $wait = Wait-SmokeProcess $p
    $sw.Stop()
    $o.Wait()
    $e.Wait()
    $code = [BitConverter]::ToUInt32([BitConverter]::GetBytes([int]$p.ExitCode), 0)
    $caseIni = Join-Path $caseDir 'display.ini'
    if (Test-Path -LiteralPath $caseIni) { [IO.File]::SetAttributes($caseIni, [IO.FileAttributes]::Normal) }

    $logText = ''
    $runLog = Join-Path $caseDir 'run.log'
    if (Test-Path -LiteralPath $runLog) { $logText = [IO.File]::ReadAllText($runLog) }
    $outText = ''
    $errText = ''
    if ($o.Complete) { $outText = $o.Text; $logText += "`r`n" + $outText }
    if ($e.Complete) { $errText = $e.Text; $logText += "`r`n" + $errText }
    [IO.File]::WriteAllText((Join-Path $caseDir 'stdout.txt'), $outText)
    [IO.File]::WriteAllText((Join-Path $caseDir 'stderr.txt'), $errText)
    if (Test-Path -LiteralPath $scoreLog) { Copy-Item -LiteralPath $scoreLog -Destination (Join-Path $caseDir 'finalscore.log') -Force }

    $dumps = @(Get-ChildItem -LiteralPath $caseDir -Filter '*.dmp' -ErrorAction SilentlyContinue)
    $class = Get-SmokeClass $false $wait.Killed $code $logText $dumps
    if ($class -eq 'GsFailure') {
        $werDir = Get-WerDumpFolder
        if (Test-Path -LiteralPath $werDir) {
            $werName = '{0}.{1}.dmp' -f [IO.Path]::GetFileName($exe), $p.Id
            $newest = Get-ChildItem -LiteralPath $werDir -Filter $werName -ErrorAction SilentlyContinue |
                Where-Object { $_.LastWriteTime -ge $start } | Select-Object -First 1
            if ($newest) { Copy-Item -LiteralPath $newest.FullName -Destination $caseDir -Force }
        }
        $dumps = @(Get-ChildItem -LiteralPath $caseDir -Filter '*.dmp' -ErrorAction SilentlyContinue)
    }
    $asan = $null
    if ($asanOptions) {
        $asan = Get-AsanReport $caseDir $code
        if ($asan) { $logText += "`r`n" + $asan }
    }
    $ms = $sw.ElapsedMilliseconds
    $silent = $wait.SilentMaxS.ToString('0.0', [Globalization.CultureInfo]::InvariantCulture)
    $turns = $(if ($null -ne $wait.Turns) { [string]$wait.Turns } else { '-' })
    $steps = $(if ($null -ne $wait.Steps) { [string]$wait.Steps } else { '-' })
    $result = ("class={0}`r`nexit=0x{1:X8}`r`nelapsed_ms={2}`r`ndumps={3}`r`nasan={4}`r`nworkdir={5}`r`nstdout_complete={6}`r`nstderr_complete={7}`r`nstall={8}`r`nsilent_max_s={9}`r`nturns={10}`r`nsteps={11}`r`nsilent_at={12}`r`n" -f $class, $code, $ms, $dumps.Count, [int][bool]$asan, $workDir, [int]$o.Complete, [int]$e.Complete, [int][bool]$wait.Stalled, $silent, $turns, $steps, $(if ($wait.SilentMaxAt) { $wait.SilentMaxAt } else { '-' }))
    Set-Content -Path (Join-Path $caseDir 'result.txt') -Value $result -Encoding ASCII
    return New-Object PSObject -Property @{ Name = $name; Class = $class; Code = $code; Dumps = $dumps; LogText = $logText; Ms = $ms; CaseDir = $caseDir; WorkDir = $workDir; Asan = $asan; Stall = [int][bool]$wait.Stalled; SilentMaxS = $silent; Turns = $turns; Steps = $steps }
}

function Read-KeyValueFile([string]$path) {
    $map = [ordered]@{}
    foreach ($line in [IO.File]::ReadAllLines($path)) {
        $i = $line.IndexOf('=')
        if ($i -lt 1) { continue }
        $map[$line.Substring(0, $i)] = $line.Substring($i + 1)
    }
    return $map
}

function Get-HostFaultCount([datetime]$Since) {
    $filters = @(
        @{ LogName = 'System'; ProviderName = 'Microsoft-Windows-WHEA-Logger' },
        @{ LogName = 'System'; ProviderName = 'Microsoft-Windows-Kernel-Power'; Id = 41 },
        @{ LogName = 'System'; ProviderName = 'EventLog'; Id = 6008 }
    )
    if ($PSBoundParameters.ContainsKey('Since')) { foreach ($f in $filters) { $f.StartTime = $Since } }
    try { return @(Get-WinEvent -FilterHashtable $filters -ErrorAction Stop).Count }
    catch {
        if ([string]$_.FullyQualifiedErrorId -like 'NoMatchingEventsFound,*') { return 0 }
        throw
    }
}

function Add-HostFaultParams([string]$ParamFile, [string]$Stage, [datetime]$Start) {
    $count = {
        param($since)
        try {
            if ($null -eq $since) { return [string](Get-HostFaultCount) }
            return [string](Get-HostFaultCount -Since $since)
        }
        catch { return 'unavailable' }
    }
    $lines = @()
    if ($Stage -ceq 'start') { $lines += "whea_before=$(& $count $null)" }
    elseif ($Stage -ceq 'end') {
        $lines += "whea_after=$(& $count $null)"
        $lines += "whea_delta=$(& $count $Start)"
    }
    else { throw "unknown host fault stage '$Stage'" }
    $lines | Add-Content -Path $ParamFile -Encoding ASCII
}

function Get-HostFaultNote($RunParams, [int]$Bad) {
    if ($Bad -le 0 -or $null -eq $RunParams) { return $null }
    $d = [string]$RunParams['whea_delta']
    if ($d -cnotmatch '^\d+$' -or [long]$d -le 0) { return $null }
    return "Suspected hardware: $d host fault event(s) (WHEA-Logger, Kernel-Power 41, EventLog 6008) in this run window, whea_before=$($RunParams['whea_before']) whea_after=$($RunParams['whea_after']); the failures above stay FAIL"
}

function Read-SmokeCaseArchive([string]$caseDir, [bool]$asan) {
    $resultFile = Join-Path $caseDir 'result.txt'
    $runLog = Join-Path $caseDir 'run.log'
    $res = New-Object PSObject -Property @{ Name = (Split-Path $caseDir -Leaf); Class = $null; Code = $null; Dumps = @(); LogText = $null; Ms = $null; CaseDir = $caseDir; WorkDir = $null; Asan = $null; Stall = $null; SilentMaxS = $null; Turns = $null; Steps = $null; Problem = $null; SkipReason = $null }
    if (-not (Test-Path -LiteralPath $resultFile -PathType Leaf) -or -not (Test-Path -LiteralPath $runLog -PathType Leaf)) {
        $res.Problem = 'no archived output'
        return $res
    }
    $f = Read-KeyValueFile $resultFile
    foreach ($k in 'class', 'exit', 'elapsed_ms', 'dumps', 'asan', 'workdir', 'stdout_complete', 'stderr_complete', 'stall', 'silent_max_s', 'turns', 'steps') {
        if (-not $f.Contains($k)) {
            $res.Problem = "no archived output: result.txt lacks $k"
            return $res
        }
    }
    if ($f['exit'] -cnotmatch '^0x([0-9A-F]{8})$') {
        $res.Problem = "no archived output: result.txt exit '$($f['exit'])'"
        return $res
    }
    $res.Code = [Convert]::ToUInt32($Matches[1], 16)
    $logText = [IO.File]::ReadAllText($runLog)
    foreach ($s in 'stdout', 'stderr') {
        if ($f["${s}_complete"] -ceq '1') {
            $p = Join-Path $caseDir "$s.txt"
            if (-not (Test-Path -LiteralPath $p -PathType Leaf)) {
                $res.Problem = "no archived output: $s.txt"
                return $res
            }
            $logText += "`r`n" + [IO.File]::ReadAllText($p)
        }
    }
    $res.Dumps = @(Get-ChildItem -LiteralPath $caseDir -Filter '*.dmp' -File -ErrorAction SilentlyContinue)
    if ($res.Dumps.Count -ne [int]$f['dumps']) {
        $res.Problem = "archived dumps=$($f['dumps']) but $($res.Dumps.Count) .dmp files"
        return $res
    }
    if ($asan) {
        $res.Asan = Get-AsanReport $caseDir $res.Code
        if ($res.Asan) { $logText += "`r`n" + $res.Asan }
    }
    if ([int][bool]$res.Asan -ne [int]$f['asan']) {
        $res.Problem = "archived asan=$($f['asan']) but the asan report is $(if ($res.Asan) { 'present' } else { 'absent' })"
        return $res
    }
    $res.Class = $f['class']
    $res.Ms = [long]$f['elapsed_ms']
    $res.WorkDir = $f['workdir']
    $res.Stall = [int]$f['stall']
    $res.SilentMaxS = $f['silent_max_s']
    $res.Turns = $f['turns']
    $res.Steps = $f['steps']
    if ($res.Class -ceq 'Skipped') { $res.SkipReason = $f['skip'] }
    $res.LogText = $logText
    return $res
}

function Get-FirewallRulePaths([string]$Root, [string[]]$Platforms = $script:WorkerPlatforms, [string[]]$Configurations = $script:WorkerConfigurations) {
    $paths = New-Object System.Collections.Generic.List[string]
    foreach ($p in $Platforms) {
        foreach ($c in $Configurations) {
            $paths.Add((Join-Path $Root "bin\$p\$c\BattlefrontII.exe"))
            foreach ($k in $script:WorkerKinds) {
                for ($w = 1; $w -le $script:WorkerIndexMax; $w++) {
                    $d = Get-WorkerPath $Root $p $c $k $w
                    $paths.Add((Join-Path $d 'BattlefrontII.exe'))
                    if ($k -eq 'laa') { $paths.Add((Join-Path $d 'BattlefrontII-nolaa.exe')) }
                }
            }
        }
    }
    return $paths.ToArray()
}

function Get-InboundAllowPrograms {
    $ids = New-Object 'System.Collections.Generic.HashSet[string]'
    foreach ($r in @(Get-NetFirewallRule -Direction Inbound -Action Allow -Enabled True)) { [void]$ids.Add($r.InstanceID) }
    $set = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    foreach ($f in @(Get-NetFirewallApplicationFilter | Where-Object { $_.Program -like '*BattlefrontII*.exe' })) {
        if ($ids.Contains($f.InstanceID)) { [void]$set.Add([Environment]::ExpandEnvironmentVariables($f.Program)) }
    }
    return ,$set
}

$script:SmokeWorkerFiles = @{
    Win32 = @('BattlefrontII.exe', 'binkw32.dll', 'd3dx9_43.dll', 'Galaxy.dll', 'GalaxyPeer.dll', 'steam_api.dll', 'steam_appid.txt')
    x64 = @('BattlefrontII.exe', 'd3dx9_43.dll', 'steam_api64.dll', 'steam_appid.txt')
}

function Get-SmokeWorkerFiles([ValidateSet('Win32', 'x64')][string]$platform = $script:SmokePlatform) {
    return @($script:SmokeWorkerFiles[$platform])
}

function New-SmokeWorkerDir($srcDir, $dir) {
    Remove-HarnessTree $dir
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    foreach ($f in @((Get-SmokeWorkerFiles) + (Get-AsanRuntimeName))) {
        $s = Join-Path $srcDir $f
        if (Test-Path -LiteralPath $s) { Copy-Item -LiteralPath $s -Destination $dir -Force }
    }
    & cmd /c mklink /J "$(Join-Path $dir 'data')" "$(Join-Path $srcDir 'data')" | Out-Null
}

function Clear-SmokeWorkerDir($dir, $keep = @()) {
    $std = Get-SmokeWorkerFiles
    foreach ($e in (New-Object System.IO.DirectoryInfo $dir).GetFileSystemInfos()) {
        if ($e.Name -eq 'data' -or $e.Name -eq (Get-AsanRuntimeName) -or $std -contains $e.Name -or @($keep) -contains $e.Name) { continue }
        if (($e.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "worker folder holds a reparse point: $($e.FullName)" }
        if (($e.Attributes -band [IO.FileAttributes]::Directory) -ne 0) {
            $links = @(Get-ReparsePoints $e.FullName)
            if ($links.Count -gt 0) { throw "worker folder holds a reparse point: $($links -join ', ')" }
            [IO.Directory]::Delete($e.FullName, $true)
        }
        else {
            $e.Attributes = [IO.FileAttributes]::Normal
            $e.Delete()
        }
    }
}

function Assert-SmokeWorkerName([string]$name) {
    if (-not $name -or $name -match '[\\/:]' -or $name.Contains('..') -or $name -ieq 'data' -or $name -ieq 'BattlefrontII.exe') { throw "REFUSING worker file name '$name'" }
}

function Get-SmokeCaseSwap($case) {
    $swap = $case.Swap
    if ($null -eq $swap) { return @() }
    if ($swap -isnot [Collections.IDictionary]) { throw "case $($case.Name): Swap must map a target name to a source path" }
    return @($swap.Keys | ForEach-Object { New-Object PSObject -Property @{ Target = [string]$_; Source = [string]$swap[$_] } })
}

function Get-SmokeCaseSkip($case) {
    foreach ($p in @(@($case.NeedsFile) + @(Get-SmokeCaseSwap $case | ForEach-Object { $_.Source }))) {
        if ($null -eq $p) { continue }
        if ($p -notmatch '^(?:[A-Za-z]:\\|\\\\[^\\])') { throw "case $($case.Name): needed file '$p' is not an absolute path" }
        if (-not [IO.File]::Exists($p)) {
            if ($case.SkipReason) { return [string]$case.SkipReason }
            return "$p not found"
        }
    }
    return $null
}

function Sync-SmokeWorkerFiles($srcDir, $dir, $case) {
    $full = [IO.Path]::GetFullPath($dir).TrimEnd('\')
    if ($full -notmatch '\\artifacts\\smoke-workers\\[^\\]+\\[^\\]+\\[^\\]+\\w\d+$') { throw "REFUSING to sync ${full}: not a worker folder under artifacts\smoke-workers" }
    if (([IO.File]::GetAttributes($full) -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "REFUSING to sync ${full}: it is a reparse point" }
    $omit = @($case.Omit | Where-Object { $null -ne $_ })
    $swap = @(Get-SmokeCaseSwap $case)
    foreach ($o in $omit) { Assert-SmokeWorkerName $o }
    foreach ($s in $swap) {
        Assert-SmokeWorkerName $s.Target
        if ($s.Source -notmatch '^(?:[A-Za-z]:\\|\\\\[^\\])' -or -not [IO.File]::Exists($s.Source)) { throw "case $($case.Name): swap source '$($s.Source)' not found" }
        if ([IO.Path]::GetFullPath($s.Source).StartsWith($full + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "REFUSING swap source inside the worker: $($s.Source)" }
    }
    foreach ($f in @((Get-SmokeWorkerFiles) + (Get-AsanRuntimeName))) {
        if ($f -ieq 'BattlefrontII.exe') { continue }
        $s = Join-Path $srcDir $f
        if (-not [IO.File]::Exists($s)) { continue }
        $d = Join-Path $full $f
        if ([IO.File]::Exists($d)) {
            [IO.File]::SetAttributes($d, [IO.FileAttributes]::Normal)
            [IO.File]::Delete($d)
        }
        [IO.File]::Copy($s, $d)
    }
    foreach ($o in $omit) {
        $d = Join-Path $full $o
        if ([IO.File]::Exists($d)) {
            [IO.File]::SetAttributes($d, [IO.FileAttributes]::Normal)
            [IO.File]::Delete($d)
        }
    }
    foreach ($s in $swap) {
        $d = Join-Path $full $s.Target
        if ([IO.File]::Exists($d)) {
            [IO.File]::SetAttributes($d, [IO.FileAttributes]::Normal)
            [IO.File]::Delete($d)
        }
        [IO.File]::Copy($s.Source, $d)
    }
}

function Get-SmokeSkipLine($name, [string]$reason) {
    return "[SKIP] smoke:$name class=Skipped - $reason"
}

function Write-SmokeSkip($name, $caseDir, [string]$reason) {
    New-Item -ItemType Directory -Force -Path $caseDir | Out-Null
    [IO.File]::WriteAllText((Join-Path $caseDir 'run.log'), '')
    $result = "class=Skipped`r`nexit=0x00000000`r`nelapsed_ms=0`r`ndumps=0`r`nasan=0`r`nworkdir=-`r`nstdout_complete=0`r`nstderr_complete=0`r`nstall=0`r`nsilent_max_s=0.0`r`nturns=-`r`nsteps=-`r`nsilent_at=-`r`nskip=$reason`r`n"
    Set-Content -Path (Join-Path $caseDir 'result.txt') -Value $result -Encoding ASCII
    return New-Object PSObject -Property @{ Name = $name; Class = 'Skipped'; Code = [uint32]0; Dumps = @(); LogText = ''; Ms = 0; CaseDir = $caseDir; WorkDir = $null; Asan = $null; Stall = 0; SilentMaxS = '0.0'; Turns = '-'; Steps = '-'; SkipReason = $reason }
}

function Get-HarnessDataLink([string]$dir) {
    $link = Join-Path $dir 'data'
    $item = Get-Item -LiteralPath $link -Force -ErrorAction SilentlyContinue
    if (-not $item -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) -eq 0) { return $null }
    if ($item.LinkType -ne 'Junction') { return $null }
    $target = [string]@($item.Target)[0]
    if (-not $target -or (Split-Path $target -Leaf) -ne 'data') { return $null }
    return $link
}

function Remove-HarnessTree([string]$dir) {
    $ErrorActionPreference = 'Continue'
    if (-not $dir) { throw 'Remove-HarnessTree needs a folder' }
    $dir = [IO.Path]::GetFullPath($dir).TrimEnd('\')
    try { $attr = [IO.File]::GetAttributes($dir) }
    catch [IO.FileNotFoundException], [IO.DirectoryNotFoundException] { return }
    if (($attr -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "REFUSING to remove ${dir}: it is a reparse point" }
    if (($attr -band [IO.FileAttributes]::Directory) -eq 0) { throw "REFUSING to remove ${dir}: not a folder" }
    $gitPath = Join-Path $dir '.git'
    if ([IO.Directory]::Exists($gitPath)) { throw "REFUSING to remove ${dir}: it holds a .git folder" }
    $isWorktree = [IO.File]::Exists($gitPath)
    $harnessLink = $null
    if (-not $isWorktree) { $harnessLink = Get-HarnessDataLink $dir }
    try { $found = @(Get-ReparsePoints $dir) }
    catch { throw "REFUSING to remove ${dir}: reparse point scan failed: $($_.Exception.Message)" }
    $other = @($found | Where-Object { $_ -ne $harnessLink })
    if ($other.Count -gt 0) { throw "REFUSING to remove ${dir}: reparse points: $($other -join ', ')" }
    if ($harnessLink) {
        & cmd /c rmdir "$harnessLink" | Out-Null
        if ([IO.Directory]::Exists($harnessLink)) { throw "worker data junction not removed: $harnessLink" }
        try { $left = @(Get-ReparsePoints $dir) }
        catch { throw "REFUSING to remove ${dir}: reparse point scan failed: $($_.Exception.Message)" }
        if ($left.Count -gt 0) { throw "REFUSING to remove ${dir}: reparse points: $($left -join ', ')" }
    }
    if ($isWorktree) {
        $common = "$(& git -C $dir rev-parse --path-format=absolute --git-common-dir 2>&1)".Trim()
        if ($LASTEXITCODE -ne 0) { throw "git rev-parse failed in ${dir}: $common" }
        $main = Split-Path ([IO.Path]::GetFullPath($common)) -Parent
        $out = & git -C $dir reset --hard -q 2>&1
        if ($LASTEXITCODE -ne 0) { throw "git reset --hard failed in ${dir}: $out" }
        $out = & git -C $dir clean -fdq 2>&1
        if ($LASTEXITCODE -ne 0) { throw "git clean -fdq failed in ${dir}: $out" }
        $out = & git -C $main worktree remove $dir 2>&1
        if ($LASTEXITCODE -ne 0) { throw "git worktree remove failed for ${dir}: $out" }
    }
    else {
        Remove-Item -LiteralPath $dir -Recurse -Force
    }
    if ([IO.Directory]::Exists($dir)) { throw "Cleanup failed: $dir is still in place" }
}


function Remove-Junctions([string]$dir) {
    if (-not (Test-Path -LiteralPath $dir)) { return }
    foreach ($rp in @(Get-ReparsePoints $dir)) { & cmd /c rmdir "$rp" | Out-Null }
    $left = @(Get-ReparsePoints $dir)
    if ($left.Count -gt 0) { throw "junctions not removed under ${dir}: $($left -join ', ')" }
}

function Remove-StagedOutput([string]$dir) {
    Remove-Junctions $dir
    if (Test-Path -LiteralPath $dir) { Remove-Item -LiteralPath $dir -Recurse -Force }
}

function Start-SmokeBatch($items, $workDirs, $srcDir) {
    if (-not $srcDir) { throw 'Start-SmokeBatch needs the source folder' }
    Initialize-OutputCapture
    $common = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'common.ps1'))
    $src = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'smoke.ps1'))
    $queue =New-Object 'System.Collections.Concurrent.ConcurrentQueue[string]'
    foreach ($d in $workDirs) { $queue.Enqueue([string]$d) }
    $pool = [runspacefactory]::CreateRunspacePool(1, @($workDirs).Count)
    $pool.Open()
    $worker = {
        param($common, $src, $queue, $item, $platform, $keep, $srcDir)
        . ([scriptblock]::Create($common))
        . ([scriptblock]::Create($src))
        $script:SmokePlatform = $platform
        $skip = Get-SmokeCaseSkip $item
        if ($skip) { return (Write-SmokeSkip $item.Name $item.CaseDir $skip) }
        $dir = $null
        while (-not $queue.TryDequeue([ref]$dir)) { Start-Sleep -Milliseconds 50 }
        try {
            Clear-SmokeWorkerDir $dir $keep
            Sync-SmokeWorkerFiles $srcDir $dir $item
            Invoke-SmokeCase $item.Name (Join-Path $dir $item.ExeName) $dir $item.ArgLine $item.CaseDir $item.AsanOptions
        }
        finally { $queue.Enqueue($dir) }
    }
    $keep = [string[]]@($items | ForEach-Object { $_.ExeName } | Sort-Object -Unique)
    $handles = @()
    foreach ($item in $items) {
        $ps = [powershell]::Create()
        $ps.RunspacePool = $pool
        $ps.AddScript($worker).AddArgument($common).AddArgument($src).AddArgument($queue).AddArgument($item).AddArgument($script:SmokePlatform).AddArgument($keep).AddArgument([string]$srcDir) | Out-Null
        $handles += New-Object PSObject -Property @{ Ps = $ps; Async = $ps.BeginInvoke() }
    }
    return New-Object PSObject -Property @{ Pool = $pool; Handles = $handles }
}

function Receive-SmokeCase($batch, $index) {
    $h = $batch.Handles[$index]
    try {
        $out = @($h.Ps.EndInvoke($h.Async))
        if ($h.Ps.Streams.Error.Count -gt 0) { throw ("smoke worker error: " + (($h.Ps.Streams.Error | ForEach-Object { $_.ToString() }) -join '; ')) }
        if ($out.Count -ne 1) { throw "smoke worker returned $($out.Count) results" }
        return $out[0]
    }
    finally { $h.Ps.Dispose() }
}

function Stop-SmokeBatch($batch) {
    foreach ($h in $batch.Handles) { try { $h.Ps.Stop() } catch {}; try { $h.Ps.Dispose() } catch {} }
    $batch.Pool.Close()
    $batch.Pool.Dispose()
}

function Get-SmokeAllowed($caseName) {
    $allowed = @()
    $file = Join-Path $PSScriptRoot 'smoke\expected-errors.txt'
    if (-not (Test-Path -LiteralPath $file)) { return $allowed }
    $section = ''
    foreach ($line in (Get-Content -LiteralPath $file)) {
        $t = $line.Trim()
        if (-not $t -or $t.StartsWith('#')) { continue }
        if ($t -cmatch '^\[(.+)\]$') { $section = $Matches[1]; continue }
        if ($section -ceq 'global' -or $section -ceq $caseName) { $allowed += $t }
    }
    return $allowed
}

function Get-DigestExpected($caseName) {
    $file = $env:OPENSWBF2_DIGEST_EXPECTED
    if (-not $file) { $file = Join-Path $PSScriptRoot 'smoke\digest-expected.txt' }
    if (-not (Test-Path -LiteralPath $file)) { return $null }
    foreach ($line in [IO.File]::ReadAllLines($file)) {
        $t = $line.Trim()
        if (-not $t -or $t.StartsWith('#')) { continue }
        $i = $t.IndexOf("`t")
        if ($i -lt 0) { continue }
        if ($t.Substring(0, $i) -ceq $caseName) { return $t.Substring($i + 1) }
    }
    return $null
}

function Get-SmokeUnexpected($caseName, $caseDir) {
    $runLog = Join-Path $caseDir 'run.log'
    if (-not (Test-Path -LiteralPath $runLog)) { return @() }
    $allowed = @(Get-SmokeAllowed $caseName)
    $bad = @()
    foreach ($line in [IO.File]::ReadAllLines($runLog)) {
        $hit = ($line -cmatch '^\[t=\d+\.\d{3}\] \[f=\d+\] \w+ (Error|Bug|Fatal) ') -or
            $line.Contains('ensure failed: ') -or ($line -cmatch '^Message Severity: [345]$')
        if (-not $hit) { continue }
        $covered = $false
        foreach ($rx in $allowed) { if ($line -cmatch $rx) { $covered = $true; break } }
        if (-not $covered) { $bad += $line }
    }
    return $bad
}

function Test-SameRunAs($caseDir, $refDir) {
    if (-not (Test-Path -LiteralPath (Join-Path $refDir 'run.log'))) { return "reference log missing: $refDir" }
    $digest = Get-DigestKey $caseDir
    $refDigest = Get-DigestKey $refDir
    if (-not $digest -or $digest -cne $refDigest) { return "digest differs: '$digest' reference '$refDigest'" }
    $cloth = @(Select-String -LiteralPath (Join-Path $caseDir 'run.log') -Pattern 'OPENSWBF2 CLOTH free=.*' -CaseSensitive)
    $refCloth = @(Select-String -LiteralPath (Join-Path $refDir 'run.log') -Pattern 'OPENSWBF2 CLOTH free=.*' -CaseSensitive)
    if ($cloth.Count -lt 1 -or $refCloth.Count -lt 1) { return "CLOTH line missing: $($cloth.Count) reference $($refCloth.Count)" }
    $line = $cloth[-1].Matches[0].Value
    $refLine = $refCloth[-1].Matches[0].Value
    if ($line -cne $refLine) { return "CLOTH differs: '$line' reference '$refLine'" }
    return $null
}

function Get-MinidumpException([string]$path, [ref]$why) {
    $why.Value = $null
    try {
        $b = [IO.File]::ReadAllBytes($path)
        $u32 = { param([long]$o) if ($o -lt 0 -or $o + 4 -gt $b.Length) { throw "truncated at offset $o" }; [BitConverter]::ToUInt32($b, [int]$o) }
        $u64 = { param([long]$o) if ($o -lt 0 -or $o + 8 -gt $b.Length) { throw "truncated at offset $o" }; [BitConverter]::ToUInt64($b, [int]$o) }
        if ($b.Length -lt 32 -or [Text.Encoding]::ASCII.GetString($b, 0, 4) -cne 'MDMP') { throw 'no MDMP signature' }
        $count = & $u32 8
        $dir = [long](& $u32 12)
        $exc = $null
        $mods = $null
        for ($i = 0; $i -lt $count; $i++) {
            $e = $dir + 12 * $i
            $type = & $u32 $e
            if ($type -eq 6 -and $null -eq $exc) { $exc = [long](& $u32 ($e + 8)) }
            elseif ($type -eq 4 -and $null -eq $mods) { $mods = [long](& $u32 ($e + 8)) }
        }
        if ($null -eq $exc) { throw 'no ExceptionStream' }
        if ($null -eq $mods) { throw 'no ModuleListStream' }
        $res = New-Object PSObject -Property @{ Code = (& $u32 ($exc + 8)); Address = (& $u64 ($exc + 24)); Module = $null; Base = $null; Size = $null }
        $n = & $u32 $mods
        for ($i = 0; $i -lt $n; $i++) {
            $m = $mods + 4 + 108 * $i
            $base = & $u64 $m
            $size = & $u32 ($m + 8)
            if ($res.Address -lt $base -or $res.Address - $base -ge $size) { continue }
            $nameRva = [long](& $u32 ($m + 20))
            $len = [long](& $u32 $nameRva)
            if ($nameRva + 4 + $len -gt $b.Length) { throw "truncated module name at offset $nameRva" }
            $res.Module = [IO.Path]::GetFileName([Text.Encoding]::Unicode.GetString($b, [int]$nameRva + 4, [int]$len))
            $res.Base = $base
            $res.Size = $size
            break
        }
        return $res
    }
    catch {
        $why.Value = $_.Exception.Message
        return $null
    }
}

function Get-DumpFatalMismatch($res) {
    $d = @($res.Dumps)
    if ($d.Count -ne 1) { return "DumpMatchesFatal mismatch: expected one dump, found $($d.Count)" }
    $f = [regex]::Match([string]$res.LogText, 'FATAL: exception 0x([0-9A-F]{8}) at 0x([0-9A-F]{1,16}) \(([^()\r\n]+)\+0x([0-9A-F]{1,16})\)')
    if (-not $f.Success) { return 'DumpMatchesFatal mismatch: no FATAL exception line' }
    $why = $null
    $x = Get-MinidumpException $d[0].FullName ([ref]$why)
    if (-not $x) { return "DumpMatchesFatal mismatch: dump $($d[0].Name): $why" }
    $fCode = [Convert]::ToUInt32($f.Groups[1].Value, 16)
    $fAddr = [Convert]::ToUInt64($f.Groups[2].Value, 16)
    $fMod = $f.Groups[3].Value
    $fOff = [Convert]::ToUInt64($f.Groups[4].Value, 16)
    $bad = @()
    if ($x.Code -ne $fCode) { $bad += ('code dump 0x{0:X8} FATAL 0x{1:X8}' -f $x.Code, $fCode) }
    if ($x.Address -ne $fAddr) { $bad += ('address dump 0x{0:X} FATAL 0x{1:X}' -f $x.Address, $fAddr) }
    $xMod = $(if ($x.Module) { $x.Module } else { 'none' })
    if ($xMod -ine $fMod) { $bad += "module dump $xMod FATAL $fMod" }
    if ($null -eq $x.Base) { $bad += ('offset dump none FATAL 0x{0:X}' -f $fOff) }
    elseif ($x.Address - $x.Base -ne $fOff) { $bad += ('offset dump 0x{0:X} FATAL 0x{1:X}' -f ($x.Address - $x.Base), $fOff) }
    if ($bad.Count) { return 'DumpMatchesFatal mismatch: ' + ($bad -join '; ') }
    return $null
}

function Get-HeapCapMismatchX64($res) {
    $ct = Join-Path $PSScriptRoot 'baseline\compare-tables.ps1'
    $dir = Join-Path $PSScriptRoot 'baseline\v143'
    $log = Join-Path $res.CaseDir 'run.log'
    $out = @(& { $ErrorActionPreference = 'Continue'; & powershell -NoProfile -ExecutionPolicy Bypass -File $ct -Tables $dir -Baseline $dir -MemPeaksX64 -RunLog $log 2>&1 | ForEach-Object { "$_" } })
    if ($LASTEXITCODE -eq 0) { return $null }
    $lines = @($out | Where-Object { $_ -cmatch '^\s+\S' } | ForEach-Object { $_.Trim() })
    if ($lines.Count -eq 0) { $lines = @("compare-tables exited $LASTEXITCODE`: $($out -join ' ')") }
    return 'HeapCapsX64 mismatch: ' + ($lines -join '; ')
}

function Test-SmokeExpect($case, $res) {
    $x = $case.Expect
    if ($res.Class -ne $x.Class) { return $false }
    if ([int]$res.Stall -ne [int]($x.ContainsKey('Stall') -and $x.Stall -eq 1)) { return $false }
    if ($x.ContainsKey('Code') -and $res.Code -ne [uint32]$x.Code) { return $false }
    $n = @($res.Dumps).Count
    if ($x.ContainsKey('MinDumps') -and $n -lt $x.MinDumps) { return $false }
    if ($x.ContainsKey('MaxDumps') -and $n -gt $x.MaxDumps) { return $false }
    if ($x.ContainsKey('DumpMatchesFatal') -and $x.DumpMatchesFatal) {
        $dmf = Get-DumpFatalMismatch $res
        if ($dmf) {
            Write-Host "    $dmf"
            $res.LogText = [string]$res.LogText + "`r`n" + $dmf
            return $false
        }
    }
    if ($x.ContainsKey('HeapCapsX64') -and $x.HeapCapsX64) {
        $hcm = Get-HeapCapMismatchX64 $res
        if ($hcm) {
            Write-Host "    $hcm"
            $res.LogText = [string]$res.LogText + "`r`n" + $hcm
            return $false
        }
    }
    $logged = [regex]::Matches([string]$res.LogText, 'OPENSWBF2 SOAK turns=(\d+)')
    if ($logged.Count -eq 0) { $logged = [regex]::Matches([string]$res.LogText, 'OPENSWBF2 SIM DIGEST turns=(\d+)') }
    if ($logged.Count -gt 0) {
        $want = [long]$logged[$logged.Count - 1].Groups[1].Value
        $have = 0L
        if (-not [long]::TryParse([string]$res.Turns, [ref]$have) -or $have -lt $want) {
            Write-Host "    progress turns $($res.Turns) below the logged turns=$want"
            return $false
        }
    }
    if ($x.ContainsKey('LogRx')) {
        foreach ($rx in @($x.LogRx)) { if ($res.LogText -cnotmatch $rx) { return $false } }
    }
    if ($x.ContainsKey('SameDigests')) {
        $d = @(Get-DigestLines $res.CaseDir)
        if (-not (Test-SameDigests $d $x.SameDigests)) {
            Write-Host "    expected $($x.SameDigests) equal SIM DIGEST lines, got $($d.Count)"
            foreach ($l in $d) { Write-Host "    $l" }
            return $false
        }
    }
    if ($x.ContainsKey('SameRunAs')) {
        $why = Test-SameRunAs $res.CaseDir (Join-Path (Split-Path -Parent $res.CaseDir) $x.SameRunAs)
        if ($why) {
            Write-Host "    $why"
            return $false
        }
    }
    if ($x.ContainsKey('CommitFlat')) {
        $why = Test-CommitFlat $res.LogText ([long]$x.CommitFlat) ([int]$x.CommitCycles) ([int]$x.CommitSettled)
        if ($why) {
            Write-Host "    commit-flat: $why"
            foreach ($m in [regex]::Matches([string]$res.LogText, 'OPENSWBF2 COMMIT(PARTS)? state=\w+ [^\r\n]*')) { Write-Host "    $($m.Value)" }
            return $false
        }
    }
    if ($x.ContainsKey('NoIni') -and (Test-Path -LiteralPath (Join-Path $res.CaseDir 'display.ini'))) { return $false }
    if ($x.ContainsKey('IniRx')) {
        $ini = Join-Path $res.CaseDir 'display.ini'
        if (-not (Test-Path -LiteralPath $ini)) { return $false }
        $iniText = [IO.File]::ReadAllText($ini)
        foreach ($rx in @($x.IniRx)) { if ($iniText -cnotmatch $rx) { return $false } }
    }
    if (-not $script:SmokeDigestConfiguration -or $script:SmokeDigestConfiguration -in 'Release', 'Test') {
        $want = Get-DigestExpected $case.Name
        if ($want) {
            $got = Get-DigestKey $res.CaseDir
            if ($want -cne $got) {
                Write-Host "    digest expected: $want"
                Write-Host "    digest actual: $got"
                return $false
            }
        }
    }
    elseif ($script:SmokeDigestConfiguration -eq 'Asan') {
        $want = Get-DigestExpected $case.Name
        if ($want) {
            $got = Get-DigestKey $res.CaseDir
            if ($want -cne $got) {
                Write-Host "    digest finding (Asan, not gated): expected $want"
                Write-Host "    digest finding (Asan, not gated): actual $got"
            }
        }
    }
    $unexpected = @(Get-SmokeUnexpected $case.Name $res.CaseDir)
    if ($x.ContainsKey('ErrorRx')) {
        if ($unexpected.Count -lt 1) {
            Write-Host "    expected an Error line, none seen"
            return $false
        }
        $ok = $true
        foreach ($u in $unexpected) {
            $matched = $false
            foreach ($rx in @($x.ErrorRx)) { if ($u -cmatch $rx) { $matched = $true; break } }
            if ($matched) { Write-Host "    expected error line: $u" }
            else {
                Write-Host "    unexpected log line: $u"
                $ok = $false
            }
        }
        return $ok
    }
    if ($unexpected.Count -gt 0) {
        foreach ($u in $unexpected) { Write-Host "    unexpected log line: $u" }
        return $false
    }
    return $true
}


function Get-DigestLines($caseDir) {
    $f = Join-Path $caseDir 'run.log'
    if (-not (Test-Path -LiteralPath $f)) { return @() }
    return @(Select-String -LiteralPath $f -Pattern 'OPENSWBF2 SIM DIGEST .*' -CaseSensitive | ForEach-Object { $_.Matches[0].Value })
}

function Test-SameDigests($lines, [int]$count) {
    $d = @($lines)
    if ($d.Count -ne $count) { return $false }
    foreach ($l in $d) { if ($l -cne $d[0]) { return $false } }
    return $true
}

function Test-CommitFlat([string]$logText, [long]$bound, [int]$cycles, [int]$settled) {
    if ($cycles -lt 1 -or $settled -lt 1) { return 'CommitCycles and CommitSettled are required' }
    if ($settled -ge $cycles) { return 'CommitSettled must be below CommitCycles' }
    $rows = @([regex]::Matches($logText, 'OPENSWBF2 COMMIT state=(\w+) bytes=(\d+)') | ForEach-Object { ,@($_.Groups[1].Value, [long]$_.Groups[2].Value) })
    $parts = @([regex]::Matches($logText, 'OPENSWBF2 COMMITPARTS state=(\w+) private=(\d+) gamecrt=(\d+) heaps=(\d+) threads=(\d+) threadcount=\d+ arena=(\d+)') | ForEach-Object { ,@($_.Groups[1].Value, [long]$_.Groups[3].Value, [long]$_.Groups[4].Value, [long]$_.Groups[6].Value, [long]$_.Groups[5].Value, [long]$_.Groups[2].Value) })
    $states = @($rows | ForEach-Object { $_[0] }) -join ','
    $want = @(0..(2 * $cycles - 1) | ForEach-Object { if ($_ % 2) { 'shell' } else { 'mission' } }) -join ','
    if ($states -cne $want) { return "expected $(2 * $cycles) states starting with mission and alternating, got '$states'" }
    $partStates = @($parts | ForEach-Object { $_[0] }) -join ','
    if ($partStates -cne $want) { return "expected a COMMITPARTS row after each COMMIT row, got '$partStates'" }
    foreach ($s in 'mission', 'shell') {
        $arena = @()
        $rest = @()
        for ($k = 0; $k -lt $rows.Count; $k++) {
            if ($rows[$k][0] -cne $s) { continue }
            $arena += $parts[$k][3]
            $rest += ($parts[$k][5] - $parts[$k][2] - $parts[$k][3] - $parts[$k][4] + $parts[$k][1])
        }
        foreach ($c in @(@('arena', $arena), @('rest', $rest))) {
            $v = $c[1]
            $peak = ($v[0..($settled - 1)] | Measure-Object -Maximum).Maximum
            for ($i = $settled; $i -lt $v.Count; $i++) {
                $d = $v[$i] - $peak
                if ($d -gt $bound) { return "$s $($i + 1) $($c[0]) = $($v[$i]) is $d above the $s $($c[0]) peak $peak of cycles 1-$settled, bound $bound" }
            }
        }
    }
    return $null
}

function Get-PerturbProofMissing($case, [string]$logText) {
    $miss = @()
    foreach ($rx in @($case.Expect.LogRx)) { if ($logText -cnotmatch $rx) { $miss += $rx } }
    return $miss
}

function Add-SerialTieExpect($cases) {
    if ($script:SmokeDigestConfiguration -notin 'Test', 'Asan') { return $cases }
    foreach ($c in $cases) {
        if ($c.Expect.Class -ne 'Pass') { continue }
        $x = $c.Expect.Clone()
        $l = @()
        if ($x.ContainsKey('LogRx')) { $l = @($x.LogRx) }
        $l = $l + @('(?s)\A(?!.*OPENSWBF2 COLLISION serialties=[1-9])', '(?m)OPENSWBF2 COLLISION serialties=0\r?$')
        $x.LogRx = $l
        $c.Expect = $x
    }
    return $cases
}

function Add-MiOptionsExpect($cases, [int]$miPurge) {
    $purge = if ($miPurge -ge 0) { $miPurge } else { 1000 }
    foreach ($c in $cases) {
        if ($c.Expect.Class -ne 'Pass') { continue }
        $x = $c.Expect.Clone()
        $l = @()
        if ($x.ContainsKey('LogRx')) { $l = @($x.LogRx) }
        $l = $l + @("OPENSWBF2 MIOPTIONS purge_delay=$purge retry_on_oom=0 arena_reserve=67108864 arena_eager_commit=0 reserve_os_memory=0 page_full_retain=-1")
        $x.LogRx = $l
        $c.Expect = $x
    }
    return $cases
}

function Get-Turn1500Runtime($caseDir) {
    $f = Join-Path $caseDir 'run.log'
    if (-not (Test-Path -LiteralPath $f)) { return @() }
    $out = @()
    $after = $false
    foreach ($line in [IO.File]::ReadAllLines($f)) {
        if ($line -cmatch 'OPENSWBF2 MEMREPORT source=') {
            $after = $line -cmatch 'OPENSWBF2 MEMREPORT source=turn1500$'
            continue
        }
        if ($after -and $line -cmatch 'OPENSWBF2 MEMREPORT heap=Runtime cap=\d+ (peak=\d+ used=\d+) ') {
            $out += $Matches[1]
            $after = $false
        }
    }
    return $out
}

function Get-DigestKey($caseDir) {
    $f = Join-Path $caseDir 'run.log'
    if (-not (Test-Path -LiteralPath $f)) { return $null }
    $m = Select-String -LiteralPath $f -Pattern 'OPENSWBF2 SIM DIGEST .*' | Select-Object -Last 1
    if (-not $m) { return $null }
    return $m.Matches[0].Value
}

function Get-DigestTrace($runLog) {
    $map = @{}
    if (-not (Test-Path -LiteralPath $runLog)) { return $map }
    foreach ($m in (Select-String -LiteralPath $runLog -Pattern 'OPENSWBF2 DIGEST TRACE turn=(\d+) (.*)' -CaseSensitive)) {
        $map[[int]$m.Matches[0].Groups[1].Value] = $m.Matches[0].Groups[2].Value
    }
    return $map
}

function Compare-DigestTrace($logA, $logB, $maxTurn = 0) {
    $a = Get-DigestTrace $logA
    $b = Get-DigestTrace $logB
    if ($a.Count -eq 0 -or $b.Count -eq 0) { return -1 }
    $last = [Math]::Max($a.Count, $b.Count)
    if ($maxTurn -gt 0) { $last = $maxTurn }
    for ($t = 1; $t -le $last; $t++) {
        if (-not $a.ContainsKey($t) -or -not $b.ContainsKey($t)) { return -1 }
        if (($a[$t] -split ' ')[0] -cne ($b[$t] -split ' ')[0]) { return $t }
    }
    return 0
}

function Get-DigestTraceSegments($runLog) {
    if (-not (Test-Path -LiteralPath $runLog)) { return @() }
    $segs = New-Object System.Collections.ArrayList
    $map = @{}
    [void]$segs.Add($map)
    foreach ($line in [IO.File]::ReadAllLines($runLog)) {
        if ($line -cmatch 'OPENSWBF2 DIGEST TRACE turn=(\d+) (state=0x[0-9A-F]{8} rand=\d+/\d+)$') { $map[[int]$Matches[1]] = $Matches[2] }
        elseif ($line -cmatch 'OPENSWBF2 REPEAT to_mission') {
            $map = @{}
            [void]$segs.Add($map)
        }
    }
    return $segs.ToArray()
}

function Compare-DigestTraceFull($a, $b, $maxTurn = 0) {
    if ($null -eq $a -or $null -eq $b -or $a.Count -eq 0 -or $b.Count -eq 0) { return -1 }
    $last = [Math]::Max($a.Count, $b.Count)
    if ($maxTurn -gt 0) { $last = $maxTurn }
    for ($t = 1; $t -le $last; $t++) {
        if (-not $a.ContainsKey($t) -or -not $b.ContainsKey($t)) { return -1 }
        if ($a[$t] -cne $b[$t]) { return $t }
    }
    return 0
}

function Get-SimDigestLines($log) {
    if (-not (Test-Path -LiteralPath $log)) { return @() }
    return @(Select-String -LiteralPath $log -Pattern 'OPENSWBF2 SIM DIGEST .*' -CaseSensitive | ForEach-Object { $_.Matches[0].Value })
}

function Get-PerturbGroupVerdict([int]$red, [int]$lines, [int]$want) {
    return ($red -eq 0 -and $lines -eq $want)
}

function Get-DigestTraceEnd($map) {
    if ($null -eq $map -or $map.Count -eq 0) { return 0 }
    return [int]($map.Keys | Measure-Object -Maximum).Maximum
}

function Get-PerturbBase($baseLog) {
    $segs = @(Get-DigestTraceSegments $baseLog)
    $trace = $null
    if ($segs.Count -eq 1) { $trace = $segs[0] }
    return @{ Segments = $segs.Count; Trace = $trace; End = (Get-DigestTraceEnd $trace); Digests = @(Get-SimDigestLines $baseLog) }
}

function Test-PerturbBaseFull($base, $simTurn = 1500) {
    return ($base.Segments -eq 1 -and $base.End -ge $simTurn -and (Compare-DigestTraceFull $base.Trace $base.Trace $base.End) -eq 0)
}

function Get-PerturbVerdict($base, $runLog, $runClass, $okClass, $repeat = $false, $simTurn = 1500) {
    $segs = @(Get-DigestTraceSegments $runLog)
    $want = 1
    if ($repeat) { $want = 2 }
    $t = -1
    if ($base.End -gt 0 -and $segs.Count -eq $want) {
        for ($i = 0; $i -lt $want; $i++) {
            $segEnd = Get-DigestTraceEnd $segs[$i]
            $tail = $false
            if ($i -lt $want - 1) { $limit = $segEnd }
            else {
                $limit = $base.End
                $tail = $segEnd -gt $base.End
            }
            if ($limit -lt $simTurn) {
                $t = -1
                break
            }
            $t = Compare-DigestTraceFull $base.Trace $segs[$i] $limit
            if ($t -eq 0 -and $tail) { $t = $base.End + 1 }
            if ($t -ne 0) { break }
        }
    }
    if ($runClass -ne $okClass) {
        $turn = 0
        if ($t -gt 0) { $turn = $t }
        return @{ Result = 'crash'; Turn = $turn; Class = [string]$runClass }
    }
    if ($t -gt 0) { return @{ Result = 'split'; Turn = $t; Class = [string]$runClass } }
    if ($t -lt 0) { return @{ Result = 'crash'; Turn = 0; Class = 'incomplete' } }
    $da = @($base.Digests)
    $db = @(Get-SimDigestLines $runLog)
    $dWant = $db.Count -ge 1
    if ($repeat) { $dWant = $db.Count -eq 2 }
    if ($da.Count -lt 1 -or -not $dWant) { return @{ Result = 'crash'; Turn = 0; Class = 'incomplete' } }
    foreach ($d in $db) { if ($d -cne $da[-1]) { return @{ Result = 'split'; Turn = $simTurn; Class = [string]$runClass } } }
    return @{ Result = 'equal'; Turn = 0; Class = [string]$runClass }
}

function Get-PerturbResults($perturbRuns, $simTurn = 1500) {
    $msgs = New-Object System.Collections.ArrayList
    $verdicts = New-Object System.Collections.ArrayList
    $rows = @()
    $red = 0
    $lines = 0
    $want = 0
    $missions = @($perturbRuns | ForEach-Object { $_.Case.Expect.Perturb.Mission } | Select-Object -Unique)
    foreach ($pm in $missions) {
        $mine = @($perturbRuns | Where-Object { $_.Case.Expect.Perturb.Mission -eq $pm })
        $others = @($mine | Where-Object { $_.Case.Expect.Perturb.Mode -ne 'base' })
        $want += $others.Count
        $base = @($mine | Where-Object { $_.Case.Expect.Perturb.Mode -eq 'base' })
        $parsed = $null
        if ($base.Count -eq 1) { $parsed = Get-PerturbBase $base[0].Log }
        if ($base.Count -ne 1 -or -not $base[0].Proven -or $base[0].Class -ne $base[0].Case.Expect.Class -or -not (Test-PerturbBaseFull $parsed $simTurn)) {
            $bc = 'missing'
            if ($base.Count -eq 1) { $bc = $base[0].Class }
            [void]$msgs.Add(@{ Line = "[FAIL] smoke:perturb base $pm class=$bc"; Tok = 'FAIL' })
            continue
        }
        [void]$msgs.Add(@{ Line = "[INFO] perturb:$pm base last turn=$($parsed.End)"; Tok = 'INFO' })
        $rows += "$pm`tbase`t0`tbase"
        foreach ($r in $others) {
            $p = $r.Case.Expect.Perturb
            if (-not $r.Proven) {
                $rows += "$pm`t$($p.Mode)`t$($p.Seed)`tunproven"
                continue
            }
            $v = Get-PerturbVerdict $parsed $r.Log $r.Class $r.Case.Expect.Class ($p.Mode -eq 'repeat') $simTurn
            $txt = $v.Result
            if ($v.Result -eq 'split') { $txt = "split turn=$($v.Turn)" }
            elseif ($v.Result -eq 'crash') {
                $txt = "crash class=$($v.Class)"
                if ($v.Turn -gt 0) { [void]$msgs.Add(@{ Line = "    perturb:$pm mode=$($p.Mode) seed=$($p.Seed) split turn=$($v.Turn) before the crash"; Tok = '' }) }
            }
            if ($v.Result -ne 'equal') { $red++ }
            $lines++
            [void]$verdicts.Add(@{ Mission = $pm; Mode = $p.Mode; Seed = $p.Seed; Result = $v.Result; Turn = $v.Turn; Text = $txt })
            [void]$msgs.Add(@{ Line = "[INFO] perturb:$pm mode=$($p.Mode) seed=$($p.Seed) $txt"; Tok = 'INFO' })
            $rows += "$pm`t$($p.Mode)`t$($p.Seed)`t$txt"
        }
    }
    $ok = Get-PerturbGroupVerdict $red $lines $want
    if ($ok) { [void]$msgs.Add(@{ Line = "[PASS] smoke:perturb serial equal runs=$lines/$want"; Tok = 'PASS' }) }
    else { [void]$msgs.Add(@{ Line = "[FAIL] smoke:perturb serial red runs=$red/$want - $lines of $want result lines, none may be missing and every run must equal its base"; Tok = 'FAIL' }) }
    return @{ Ok = $ok; Rows = $rows; Messages = $msgs.ToArray(); Verdicts = $verdicts.ToArray(); Red = $red; Lines = $lines; Want = $want }
}

function Get-FinalScoreKey($caseDir) {
    $f = Join-Path $caseDir 'finalscore.log'
    if (-not (Test-Path -LiteralPath $f)) { return $null }
    $line = Get-Content -LiteralPath $f -TotalCount 1
    if (-not $line) { return $null }
    $i = $line.IndexOf("`t")
    if ($i -lt 0) { return $line }
    return $line.Substring($i + 1)
}

function Compare-LaaRuns($base, $laa, $minRuns = 5) {
    $failures = New-Object System.Collections.ArrayList
    $base = @($base)
    $laa = @($laa)
    if ($base.Count -lt $minRuns) { [void]$failures.Add("only $($base.Count) non-LAA runs, need $minRuns") }
    if ($laa.Count -lt $minRuns) { [void]$failures.Add("only $($laa.Count) LAA runs, need $minRuns") }
    $keys = @(($base + $laa) | ForEach-Object { "$($_.Key) [f=$($_.Frames)]" } | Select-Object -Unique)
    if ($keys.Count -gt 1) { [void]$failures.Add("runs differ: $($keys -join ' | ')") }
    return New-Object PSObject -Property @{ Ok = ($failures.Count -eq 0); Failures = $failures; Keys = $keys }
}

function Get-HardeningChecks($hdr, $cfg, $clTotal, $clGsMinus, $clSdl) {
    $relocSize = 0
    if ($hdr -match '(?m)^\s+[0-9A-F]+ \[\s*([0-9A-F]+)\] RVA \[size\] of Base Relocation Directory') { $relocSize = [Convert]::ToInt32($Matches[1], 16) }
    $seh = 0
    if ($cfg -match '(?m)^\s+([0-9A-F]+) Safe Exception Handler Count') { $seh = [Convert]::ToInt32($Matches[1], 16) }
    $cookie = 0
    if ($cfg -match '(?m)^\s+([0-9A-F]+) Security Cookie') { $cookie = [Convert]::ToInt64($Matches[1], 16) }
    $hasReloc =$hdr -match '(?m)^\s+\.reloc name\s*$'
    $stripped = $hdr -match '(?m)^\s+Relocations stripped\s*$'
    $checks = @()
    $dyn = $hdr -match '(?m)^\s+Dynamic base\s*$'
    $checks += New-Object PSObject -Property @{ Name = 'dynamicbase'; Ok = $dyn; Detail = 'Dynamic base flag missing' }
    $nx = $hdr -match '(?m)^\s+NX compatible\s*$'
    $checks += New-Object PSObject -Property @{ Name = 'nxcompat'; Ok = $nx; Detail = 'NX compatible flag missing' }
    $laa = $hdr -match '(?m)^\s+Application can handle large \(>2GB\) addresses\s*$'
    $checks += New-Object PSObject -Property @{ Name = 'laa'; Ok = $laa; Detail = 'Application can handle large (>2GB) addresses missing from dumpbin /headers' }
    $checks += New-Object PSObject -Property @{
        Name = 'reloc'
        Ok = ($hasReloc -and $relocSize -gt 0 -and -not $stripped)
        Detail = ('.reloc={0} reloc-dir-size={1} stripped={2}' -f $hasReloc, $relocSize, $stripped)
    }
    $checks += New-Object PSObject -Property @{ Name = 'safeseh'; Ok = ($seh -gt 0); Detail = "Safe Exception Handler Count=$seh" }
    $checks += New-Object PSObject -Property @{
        Name = 'gs'
        Ok = ($clTotal -gt 0 -and $clGsMinus -eq 0 -and $cookie -ne 0)
        Detail = ('cl entries={0} /GS-={1} security-cookie=0x{2:X8}' -f $clTotal, $clGsMinus, $cookie)
    }
    $checks += New-Object PSObject -Property @{
        Name = 'sdl'
        Ok = ($clTotal -gt 0 -and $clSdl -eq $clTotal)
        Detail = ('cl entries={0} /sdl={1}' -f $clTotal, $clSdl)
    }
    return $checks
}


function Get-ImportChecks($imp, [ValidateSet('Win32', 'x64')][string]$platform) {
    $text = [string]$imp
    $hdrAt = $text.IndexOf('Section contains the following delay load imports:')
    $static = $text
    $delay = ''
    if ($hdrAt -ge 0) {
        $static = $text.Substring(0, $hdrAt)
        $delay = $text.Substring($hdrAt)
    }
    $rows = @()
    foreach ($n in 'SetProcessDpiAwarenessContext', 'SetProcessDpiAwareness', 'SetProcessDPIAware', 'GetDpiForWindow', 'AdjustWindowRectExForDpi') {
        $rows += New-Object PSObject -Property @{ Name = "import-$n"; Ok = ($static -cnotmatch "(?m)\s$n\s*$"); Detail = "$n statically imported (breaks Win7 load)" }
    }
    $rows += New-Object PSObject -Property @{ Name = 'import-shcore'; Ok = ($static -notmatch '(?mi)^\s+shcore\.dll\s*$'); Detail = 'shcore.dll in import table' }
    $dllRx = '(?mi)^\s+(\S+\.dll)\s*$'
    $dlls = @([regex]::Matches($static, $dllRx) | ForEach-Object { $_.Groups[1].Value.ToLowerInvariant() } | Sort-Object -Unique)
    $allow = @($script:ImportStaticAllow[$platform] + $script:AsanRuntimeNames[$platform])
    $extra = @($dlls | Where-Object { $allow -notcontains $_ })
    $detail = 'imports outside the allow list: ' + ($extra -join ' ')
    if ($dlls -notcontains 'kernel32.dll') { $detail = 'kernel32.dll not found in the dumpbin /imports output (' + $dlls.Count + ' DLLs parsed)' }
    $rows += New-Object PSObject -Property @{ Name = 'import-modules'; Ok = ($extra.Count -eq 0 -and $dlls -contains 'kernel32.dll'); Detail = $detail }
    $delayDlls = @([regex]::Matches($delay, $dllRx) | ForEach-Object { $_.Groups[1].Value.ToLowerInvariant() } | Sort-Object -Unique)
    $delayExtra = @($delayDlls | Where-Object { @($script:ImportDelayAllow[$platform]) -notcontains $_ })
    $rows += New-Object PSObject -Property @{ Name = 'import-delay-modules'; Ok = ($delayExtra.Count -eq 0); Detail = 'delay imports outside the allow list: ' + ($delayExtra -join ' ') }
    if ($platform -eq 'x64') {
        $rows += Get-D3dxImportsCheck $static
        $rows += Get-CrtFloatImportsCheck $static
    }
    return $rows
}

$script:CrtFloatImports = @('sinf', 'cosf', 'tanf', 'asinf', 'acosf', 'atanf', 'atan2f', 'expf', 'logf', 'log10f', 'powf', 'fmodf')

function Get-CrtFloatImportsCheck([string]$static) {
    $names = @([regex]::Matches($static, '(?m)^[ \t]+[0-9A-Fa-f]+[ \t]+(\S+)[ \t]*\r?$') | ForEach-Object { $_.Groups[1].Value })
    $found = @($names | Where-Object { $script:CrtFloatImports -ccontains $_ } | Sort-Object -Unique)
    $detail = @()
    if ($static -notmatch '(?mi)^[ \t]+api-ms-win-crt-math-l1-1-0\.dll[ \t]*\r?$') { $detail += 'api-ms-win-crt-math-l1-1-0.dll not in the import table' }
    if ($found.Count) { $detail += 'float CRT imports: ' + ($found -join ' ') }
    return New-Object PSObject -Property @{ Name = 'crt-float-imports'; Ok = ($detail.Count -eq 0); Detail = ($detail -join '; ') }
}

$script:D3dxImportsAllow = @('D3DXCheckTextureRequirements', 'D3DXLoadSurfaceFromSurface')

function Get-D3dxImportsCheck([string]$static) {
    $names = @()
    $m = [regex]::Match($static, '(?msi)^[ \t]+d3dx9_43\.dll[ \t]*\r?$(.*?)(?=^[ \t]+\S+\.dll[ \t]*\r?$|^[ \t]+Summary[ \t]*\r?$|\z)')
    if ($m.Success) {
        foreach ($e in [regex]::Matches($m.Groups[1].Value, '(?m)^[ \t]+(?:[0-9A-Fa-f]+[ \t]+(\S+)|(Ordinal[ \t]+\d+))[ \t]*\r?$')) {
            if ($e.Groups[1].Success) { $names += $e.Groups[1].Value } else { $names += ($e.Groups[2].Value -replace '\s+', ' ') }
        }
    }
    $extra = @($names | Where-Object { $script:D3dxImportsAllow -cnotcontains $_ } | Sort-Object -Unique)
    $missing = @($script:D3dxImportsAllow | Where-Object { $names -cnotcontains $_ })
    $detail = @()
    if (-not $m.Success) { $detail += 'd3dx9_43.dll not in the import table' }
    if ($extra.Count) { $detail += 'extra: ' + ($extra -join ' ') }
    if ($missing.Count) { $detail += 'missing: ' + ($missing -join ' ') }
    return New-Object PSObject -Property @{ Name = 'd3dx-imports'; Ok = ($detail.Count -eq 0); Detail = ($detail -join '; ') }
}

function Get-UnicowsCheck([string]$root, [ValidateSet('Win32', 'x64')][string]$platform, [string]$exeDir, $imp, [string]$vcxText = $null, [string[]]$workerFiles = $null, [string]$srcDir = $null) {
    $hits = New-Object System.Collections.Generic.List[string]
    if ($exeDir -and (Test-Path -LiteralPath (Join-Path $exeDir 'unicows.dll'))) { $hits.Add("staged in $exeDir") }
    if (-not $vcxText) { $vcxText = [IO.File]::ReadAllText((Join-Path $root 'BattlefrontII.vcxproj')) }
    $xml = New-Object xml
    $xml.LoadXml($vcxText)
    $ns = New-Object Xml.XmlNamespaceManager $xml.NameTable
    $ns.AddNamespace('m', 'http://schemas.microsoft.com/developer/msbuild/2003')
    foreach ($n in @($xml.SelectNodes('//m:ItemGroup/m:GameDataFile | //m:ItemGroup/m:VendorFile', $ns))) {
        if ($n.GetAttribute('Include') -match '(?i)unicows') { $hits.Add("vcxproj $($n.LocalName) $($n.GetAttribute('Include'))") }
    }
    foreach ($n in @($xml.SelectNodes('//m:Copy', $ns))) {
        if ($n.GetAttribute('SourceFiles') -match '(?i)unicows') { $hits.Add("vcxproj Copy $($n.GetAttribute('SourceFiles'))") }
    }
    if ($null -eq $workerFiles) { $workerFiles = Get-SmokeWorkerFiles $platform }
    if (@($workerFiles | Where-Object { $_ -match '(?i)unicows' }).Count) { $hits.Add("$platform worker list") }
    $text = [string]$imp
    $hdrAt = $text.IndexOf('Section contains the following delay load imports:')
    $static = $text
    $delay = ''
    if ($hdrAt -ge 0) {
        $static = $text.Substring(0, $hdrAt)
        $delay = $text.Substring($hdrAt)
    }
    if ($static -match '(?mi)^\s+unicows\.dll\s*$') { $hits.Add('static import') }
    if ($delay -match '(?mi)^\s+unicows\.dll\s*$') { $hits.Add('delay import') }
    if (-not $srcDir) { $srcDir = Join-Path $root 'src' }
    foreach ($f in @(Get-ChildItem -LiteralPath $srcDir -Recurse -File -ErrorAction SilentlyContinue)) {
        if ($f.Name -match '(?i)unicows' -or (Select-String -LiteralPath $f.FullName -Pattern 'unicows' -SimpleMatch -Quiet)) { $hits.Add("named in $($f.FullName)") }
    }
    return New-Object PSObject -Property @{ Name = 'unicows-staged'; Ok = ($hits.Count -eq 0); Detail = ($hits -join ', ') }
}

$script:D3dxPinNames = @('D3dxNupkgSha256', 'D3dxWin32Sha256', 'D3dxX64Sha256')

function Get-VendorPins([string]$root, [string]$pinsText = $null) {
    if (-not $pinsText) { $pinsText = [IO.File]::ReadAllText((Join-Path $root 'lib\vendor-pins.props')) }
    $xml = New-Object xml
    $xml.LoadXml($pinsText)
    $pins = @{}
    foreach ($g in @($xml.DocumentElement.ChildNodes | Where-Object { $_.LocalName -eq 'PropertyGroup' })) {
        foreach ($p in @($g.ChildNodes | Where-Object { $_.NodeType -eq 'Element' })) { $pins[$p.LocalName] = $p.InnerText.Trim() }
    }
    return $pins
}

function Get-D3dxPinsCheck([string]$root, [string]$pinsText = $null, [string]$vcxText = $null) {
    $hits = New-Object System.Collections.Generic.List[string]
    $pins = Get-VendorPins $root $pinsText
    foreach ($k in $script:D3dxPinNames) {
        if ([string]$pins[$k] -cnotmatch '^[0-9a-f]{64}$') { $hits.Add("vendor-pins.props $k is not 64 hex digits") }
    }
    if (-not $vcxText) { $vcxText = [IO.File]::ReadAllText((Join-Path $root 'BattlefrontII.vcxproj')) }
    $xml = New-Object xml
    $xml.LoadXml($vcxText)
    $ns = New-Object Xml.XmlNamespaceManager $xml.NameTable
    $ns.AddNamespace('m', 'http://schemas.microsoft.com/developer/msbuild/2003')
    if (-not @($xml.SelectNodes('/m:Project/m:Import', $ns) | Where-Object { $_.GetAttribute('Project') -eq 'lib\vendor-pins.props' -and -not $_.GetAttribute('Condition') }).Count) { $hits.Add('vcxproj does not import lib\vendor-pins.props') }
    $verifies = @($xml.SelectNodes("//m:Target[@Name='StageVendor']/m:VerifyFileHash", $ns))
    $want = @(
        @('$(D3dxNupkg)', '$(D3dxNupkgSha256)', ''),
        @('$(D3dxDll)', '$(D3dxWin32Sha256)', "'`$(Platform)'=='Win32'"),
        @('$(D3dxDll)', '$(D3dxX64Sha256)', "'`$(Platform)'=='x64'")
    )
    foreach ($w in $want) {
        $hit = @($verifies | Where-Object { $_.GetAttribute('File') -eq $w[0] -and $_.GetAttribute('Hash') -eq $w[1] -and $_.GetAttribute('Algorithm') -eq 'SHA256' -and $_.GetAttribute('Condition') -eq $w[2] })
        if (-not $hit.Count) { $hits.Add("StageVendor has no VerifyFileHash of $($w[0]) against $($w[1])") }
    }
    $unzips = @($xml.SelectNodes("//m:Target[@Name='StageVendor']/m:Unzip", $ns))
    if (-not $unzips.Count -or @($unzips | Where-Object { $_.GetAttribute('SourceFiles') -ne '$(D3dxNupkg)' -or $_.GetAttribute('Condition') -ne "'`$(D3dxDllHash)' != '`$(D3dxDllSha256)'" }).Count) { $hits.Add('StageVendor Unzip of $(D3dxNupkg) is not conditioned on the cached DLL hash') }
    return New-Object PSObject -Property @{ Name = 'd3dx9-pins'; Ok = ($hits.Count -eq 0); Detail = ($hits -join ', ') }
}

function Get-D3dxStagedCheck([string]$root, [ValidateSet('Win32', 'x64')][string]$platform, [string]$exeDir, [string]$pinsText = $null) {
    $name = 'vendor:d3dx9-staged'
    if (-not (Test-Path -LiteralPath (Join-Path $exeDir 'data'))) { return New-Object PSObject -Property @{ Name = $name; Status = 'SKIP'; Detail = 'not staged' } }
    $pin = (Get-VendorPins $root $pinsText)[$(if ($platform -eq 'x64') { 'D3dxX64Sha256' } else { 'D3dxWin32Sha256' })]
    $dll = Join-Path $exeDir 'd3dx9_43.dll'
    if (-not (Test-Path -LiteralPath $dll -PathType Leaf)) { return New-Object PSObject -Property @{ Name = $name; Status = 'FAIL'; Detail = "$dll missing" } }
    $hasher = [Security.Cryptography.SHA256]::Create()
    try { $sha = ([BitConverter]::ToString($hasher.ComputeHash([IO.File]::ReadAllBytes($dll))) -replace '-', '').ToLowerInvariant() }
    finally { $hasher.Dispose() }
    if ($sha -cne $pin) { return New-Object PSObject -Property @{ Name = $name; Status = 'FAIL'; Detail = "$dll is $sha, pin is $pin" } }
    return New-Object PSObject -Property @{ Name = $name; Status = 'PASS'; Detail = '' }
}

function Get-D3dxStagedLine($c) {
    if ($c.Detail) { return "[$($c.Status)] $($c.Name) - $($c.Detail)" }
    return "[$($c.Status)] $($c.Name)"
}

$script:SteamApi64Source = '$(SWBF2_VENDOR_X64)\steam_api64.dll'

function Get-SteamApi64PinsCheck([string]$root, [string]$pinsText = $null, [string]$vcxText = $null) {
    $hits = New-Object System.Collections.Generic.List[string]
    if ([string](Get-VendorPins $root $pinsText)['SteamApi64Sha256'] -cnotmatch '^[0-9a-f]{64}$') { $hits.Add('vendor-pins.props SteamApi64Sha256 is not 64 hex digits') }
    if (-not $vcxText) { $vcxText = [IO.File]::ReadAllText((Join-Path $root 'BattlefrontII.vcxproj')) }
    $xml = New-Object xml
    $xml.LoadXml($vcxText)
    $ns = New-Object Xml.XmlNamespaceManager $xml.NameTable
    $ns.AddNamespace('m', 'http://schemas.microsoft.com/developer/msbuild/2003')
    $steps = @($xml.SelectNodes("//m:Target[@Name='StageVendor']/*", $ns))
    $copies = 0
    for ($i = 0; $i -lt $steps.Count; $i++) {
        $s = $steps[$i]
        if ($s.LocalName -ne 'Copy' -or $s.GetAttribute('SourceFiles') -ne $script:SteamApi64Source) { continue }
        $copies++
        $verified = $false
        for ($j = 0; $j -lt $i; $j++) {
            $v = $steps[$j]
            if ($v.LocalName -eq 'VerifyFileHash' -and $v.GetAttribute('File') -eq $script:SteamApi64Source -and $v.GetAttribute('Hash') -eq '$(SteamApi64Sha256)' -and $v.GetAttribute('Algorithm') -eq 'SHA256' -and $v.GetAttribute('Condition') -eq $s.GetAttribute('Condition')) { $verified = $true }
        }
        if (-not $verified) { $hits.Add("StageVendor copies $($script:SteamApi64Source) without a VerifyFileHash against `$(SteamApi64Sha256) before it") }
    }
    if ($copies -eq 0) { $hits.Add("StageVendor has no Copy of $($script:SteamApi64Source)") }
    return New-Object PSObject -Property @{ Name = 'steam-api64-pins'; Ok = ($hits.Count -eq 0); Detail = ($hits -join ', ') }
}

function Get-SteamApi64StagedCheck([string]$root, [string]$exeDir, [string]$imp, [string]$exports = $null, [string]$pinsText = $null, [string]$defText = $null) {
    $name = 'steam-api64-staged'
    $dll = Join-Path $exeDir 'steam_api64.dll'
    if (-not (Test-Path -LiteralPath $dll -PathType Leaf)) { return New-Object PSObject -Property @{ Name = $name; Status = 'SKIP'; Detail = 'not staged' } }
    $pin = [string](Get-VendorPins $root $pinsText)['SteamApi64Sha256']
    $hasher = [Security.Cryptography.SHA256]::Create()
    try { $sha = ([BitConverter]::ToString($hasher.ComputeHash([IO.File]::ReadAllBytes($dll))) -replace '-', '').ToLowerInvariant() }
    finally { $hasher.Dispose() }
    if ($sha -cne $pin) { return New-Object PSObject -Property @{ Name = $name; Status = 'FAIL'; Detail = "$dll is $sha, pin is $pin" } }
    if (-not $exports) {
        $exports = (& (Find-Dumpbin 'x64') /exports $dll) -join "`n"
        if ($LASTEXITCODE -ne 0) { return New-Object PSObject -Property @{ Name = $name; Status = 'FAIL'; Detail = "dumpbin /exports exit $LASTEXITCODE" } }
    }
    $have = @([regex]::Matches($exports, '(?m)^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]{8}\s+(\S+)') | ForEach-Object { $_.Groups[1].Value })
    $text = [string]$imp
    $hdrAt = $text.IndexOf('Section contains the following delay load imports:')
    $block = [regex]::Match($(if ($hdrAt -ge 0) { $text.Substring($hdrAt) } else { '' }), '(?ms)^ {4}steam_api64\.dll\s*$(.*?)(?=^ {4}\S|\z)')
    if (-not $block.Success) { return New-Object PSObject -Property @{ Name = $name; Status = 'FAIL'; Detail = 'the exe has no steam_api64.dll delay import' } }
    $imported = @([regex]::Matches($block.Groups[1].Value, '(?m)^\s+[0-9A-F]{8,16}\s+[0-9A-F]+\s+(\S+)\s*$') | ForEach-Object { $_.Groups[1].Value })
    if (-not $defText) { $defText = [IO.File]::ReadAllText((Join-Path $root 'lib\x64\steam_api64.def')) }
    $defAt = $defText.IndexOf('EXPORTS')
    $defNames = @($(if ($defAt -ge 0) { [regex]::Matches($defText.Substring($defAt + 7), '(?m)^\s+(\w+)') | ForEach-Object { $_.Groups[1].Value } }))
    $hits = New-Object System.Collections.Generic.List[string]
    if (-not $imported.Count) { $hits.Add('no function parsed from the steam_api64.dll delay import') }
    if (-not $defNames.Count) { $hits.Add('no name parsed from lib\x64\steam_api64.def') }
    foreach ($f in $imported) { if ($have -cnotcontains $f) { $hits.Add("$f imported by the exe is not exported") } }
    foreach ($f in $defNames) { if ($have -cnotcontains $f) { $hits.Add("$f in steam_api64.def is not exported") } }
    return New-Object PSObject -Property @{ Name = $name; Status = $(if ($hits.Count) { 'FAIL' } else { 'PASS' }); Detail = ($hits -join ', ') }
}

function Test-MsBuildCondition([string]$cond, [string]$platform, [string]$config) {
    if (-not $cond.Trim()) { return $true }
    $ok = $true
    foreach ($part in ($cond.Trim() -split '\s+and\s+')) {
        $m = [regex]::Match($part, "^'\`$\((Platform|Configuration)\)'\s*==\s*'([^'`$()]*)'$")
        if (-not $m.Success) { throw "unsupported MSBuild condition: $cond" }
        $have = $(if ($m.Groups[1].Value -eq 'Platform') { $platform } else { $config })
        if ($have -ne $m.Groups[2].Value) { $ok = $false }
    }
    return $ok
}

function Test-MsBuildItemIncluded($item, [string]$platform, [string]$config) {
    for ($n = $item; $n -is [Xml.XmlElement]; $n = $n.ParentNode) {
        if (-not (Test-MsBuildCondition $n.GetAttribute('Condition') $platform $config)) { return $false }
        if ($n.LocalName -eq 'ItemGroup') { break }
    }
    return $true
}

function Expand-MsBuildItemSpec([string]$dir, [string]$spec) {
    $out = New-Object System.Collections.Generic.List[string]
    foreach ($s in @($spec.Split(';') | Where-Object { $_.Trim() })) {
        $s = $s.Trim()
        if (-not $s.Contains('*')) { $out.Add($s); continue }
        $sDir = Split-Path $s -Parent
        foreach ($f in @(Get-ChildItem -LiteralPath (Join-Path $dir $sDir) -Filter (Split-Path $s -Leaf) -File | Sort-Object Name)) { $out.Add((Join-Path $sDir $f.Name)) }
    }
    return $out.ToArray()
}

function Expand-MsBuildItemInclude([string]$dir, $item) {
    $ex = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    foreach ($e in @(Expand-MsBuildItemSpec $dir $item.GetAttribute('Exclude'))) { [void]$ex.Add([IO.Path]::GetFullPath((Join-Path $dir $e))) }
    return @(Expand-MsBuildItemSpec $dir $item.GetAttribute('Include') | Where-Object { -not $ex.Contains([IO.Path]::GetFullPath((Join-Path $dir $_))) })
}

function Get-GameProjects([string]$root, [string]$config) {
    $root = $root.TrimEnd('\')
    $exe = Join-Path $root 'BattlefrontII.vcxproj'
    $objDir = Join-Path $root "obj\$script:SmokePlatform\$config"
    $list = @(New-Object PSObject -Property @{ Name = 'BattlefrontII'; Vcxproj = $exe; IntDir = $objDir; TlogDir = (Join-Path $objDir 'BattlefrontII.tlog') })
    $xml = New-Object xml
    $xml.Load($exe)
    $ns = New-Object Xml.XmlNamespaceManager $xml.NameTable
    $ns.AddNamespace('m', 'http://schemas.microsoft.com/developer/msbuild/2003')
    foreach ($r in @($xml.SelectNodes('//m:ItemGroup/m:ProjectReference', $ns))) {
        if (-not (Test-MsBuildItemIncluded $r $script:SmokePlatform $config)) { continue }
        $use = $r.SelectSingleNode('m:UseLibraryDependencyInputs', $ns)
        if (-not $use -or $use.InnerText.Trim() -ne 'true') { continue }
        $path = [IO.Path]::GetFullPath((Join-Path $root $r.GetAttribute('Include')))
        $name = [IO.Path]::GetFileNameWithoutExtension($path)
        $int = Join-Path $objDir "lib\$name"
        $list += New-Object PSObject -Property @{ Name = $name; Vcxproj = $path; IntDir = $int; TlogDir = (Join-Path $int "$name.tlog") }
    }
    return $list
}

function Get-X64ObjectsCheck([string]$root, [string]$config) {
    $projects = @(Get-GameProjects $root $config)
    $linkTlog = Join-Path $projects[0].TlogDir 'link.command.1.tlog'
    $linked = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    $bad = New-Object System.Collections.Generic.List[string]
    if (-not (Test-Path -LiteralPath $linkTlog -PathType Leaf)) { $bad.Add("no link tlog $linkTlog") }
    else {
        foreach ($l in [IO.File]::ReadAllLines($linkTlog, [Text.Encoding]::Unicode)) {
            $l = $l.TrimStart([char]0xFEFF)
            if ($l.StartsWith('^')) { foreach ($p in $l.Substring(1).Split('|')) { [void]$linked.Add($p) } }
        }
    }
    $units = 0
    $masm = 0
    $missing = New-Object System.Collections.Generic.List[string]
    foreach ($p in $projects) {
        $xml = New-Object xml
        $xml.Load($p.Vcxproj)
        $ns = New-Object Xml.XmlNamespaceManager $xml.NameTable
        $ns.AddNamespace('m', 'http://schemas.microsoft.com/developer/msbuild/2003')
        foreach ($item in @($xml.SelectNodes('//m:ItemGroup/m:ClCompile[@Include] | //m:ItemGroup/m:MASM[@Include]', $ns))) {
            if (-not (Test-MsBuildItemIncluded $item $script:SmokePlatform $config)) { continue }
            if ($item.LocalName -eq 'MASM') { $masm++ } else { $units++ }
            $inc = $item.GetAttribute('Include')
            $obj = [IO.Path]::GetFullPath((Join-Path $p.IntDir ((Split-Path $inc -Parent) + '\' + [IO.Path]::GetFileNameWithoutExtension($inc) + '.obj')))
            if (-not (Test-Path -LiteralPath $obj -PathType Leaf)) { $missing.Add("$obj not built") }
            elseif (-not $linked.Contains($obj)) { $missing.Add("$obj not linked") }
        }
    }
    if ($units -eq 0) { $bad.Add('no ClCompile items') }
    $detail = ('units={0} missing={1} masm={2}' -f $units, $missing.Count, $masm)
    if ($missing.Count) { $detail += ' ' + (@($missing | Select-Object -First 3) -join ', ') }
    if ($bad.Count) { $detail += '; ' + ($bad -join '; ') }
    return New-Object PSObject -Property @{ Name = 'x64-objects'; Ok = ($missing.Count -eq 0 -and $bad.Count -eq 0); Detail = $detail }
}

$script:CrtTrigRouteHelper = 'pcCrtTrigX64.obj'

function Get-CrtTrigRouteSymbols([string]$root, [string]$config, [string]$platform) {
    $projects = @(Get-GameProjects $root $config)
    $linkTlog = Join-Path $projects[0].TlogDir 'link.command.1.tlog'
    if (-not (Test-Path -LiteralPath $linkTlog -PathType Leaf)) { throw "no link tlog $linkTlog" }
    $objs = New-Object System.Collections.Generic.List[string]
    foreach ($l in [IO.File]::ReadAllLines($linkTlog, [Text.Encoding]::Unicode)) {
        $l = $l.TrimStart([char]0xFEFF)
        if ($l.StartsWith('^')) { foreach ($p in $l.Substring(1).Split('|')) { if ($p -like '*.obj' -and -not $objs.Contains($p)) { $objs.Add($p) } } }
    }
    if ($objs.Count -eq 0) { throw "no objects in $linkTlog" }
    $db = Find-Dumpbin $platform
    $text = New-Object System.Text.StringBuilder
    for ($k = 0; $k -lt $objs.Count; $k += 100) {
        $batch = $objs.GetRange($k, [Math]::Min(100, $objs.Count - $k)).ToArray()
        $out = & $db /symbols $batch
        if ($LASTEXITCODE -ne 0) { throw "dumpbin /symbols exit $LASTEXITCODE" }
        [void]$text.Append(($out -join "`n")).Append("`n")
    }
    return New-Object PSObject -Property @{ Objects = $objs.ToArray(); Text = $text.ToString() }
}

function Get-CrtTrigRouteCheck([string[]]$objects, [string]$symbols) {
    $dumped = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    $bad = New-Object System.Collections.Generic.List[string]
    foreach ($c in ($symbols -split '(?m)^Dump of file ' | Select-Object -Skip 1)) {
        $file = ($c -split "`r?`n")[0].Trim()
        [void]$dumped.Add([IO.Path]::GetFullPath($file))
        if ([IO.Path]::GetFileName($file) -ieq $script:CrtTrigRouteHelper) { continue }
        $refs = @([regex]::Matches($c, '(?m)^[0-9A-Fa-f]+\s+[0-9A-Fa-f]+\s+UNDEF\s.*\bExternal\s+\|\s+(?:__imp_)?(sin|cos|tan)\s*$') | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
        if ($refs.Count) { $bad.Add("$([IO.Path]::GetFileName($file)) references $($refs -join ',')") }
    }
    $missing = @($objects | Where-Object { -not $dumped.Contains([IO.Path]::GetFullPath($_)) })
    if ($missing.Count) { $bad.Add("no symbols for $($missing.Count) objects: $(@($missing | Select-Object -First 3) -join ', ')") }
    if (@($objects | Where-Object { [IO.Path]::GetFileName($_) -ieq $script:CrtTrigRouteHelper }).Count -ne 1) { $bad.Add("$script:CrtTrigRouteHelper not linked") }
    return New-Object PSObject -Property @{ Name = 'crt-trig-route'; Ok = ($bad.Count -eq 0); Detail = ("objects=$($objects.Count)" + $(if ($bad.Count) { '; ' + ($bad -join '; ') } else { '' })) }
}

function Get-CrtTrigRouteResult([string]$root, [string]$config, [string]$platform) {
    try {
        $s = Get-CrtTrigRouteSymbols $root $config $platform
        return Get-CrtTrigRouteCheck $s.Objects $s.Text
    }
    catch { return New-Object PSObject -Property @{ Name = 'crt-trig-route'; Ok = $false; Detail = $_.Exception.Message } }
}

$script:SizeLiteral = '(?:0[xX][0-9A-Fa-f]+|[1-9]\d*)[uUlL]*'
$script:SizeLiteralArg = '(?:[^,();]|\((?:[^()]|\([^()]*\))*\))+'
$script:SizeFourArg = '(?:4[uUlL]*\s*\*' + $script:SizeLiteralArg + '|(?:[^,();]|\((?:[^()]|\([^()]*\))*\))+?[*+]\s*4[uUlL]*(?=\s*[,)]))'
$script:SizeArg = '(?:' + $script:SizeLiteral + '|' + $script:SizeFourArg + ')'
$script:SizeFourLocalRx = New-Object regex '\b(?<name>[A-Za-z_]\w*)\s*=(?!=)\s*(?:4[uUlL]*\s*\*[^;{}]+|[^;{}=]+?[*+]\s*4[uUlL]*)\s*;'

function New-SizeLiteralRx([string]$size, [string]$alloc) {
    return @(
        (New-Object regex ('\bMemoryPool\s+[\w:]+\s*\(\s*"[^"\r\n]*"\s*,\s*' + $size + '\s*\)')),
        (New-Object regex ('\bFreeList\s+[\w:]+\s*\(\s*' + $size + '\s*,')),
        (New-Object regex ('(?:\.|->)(?:Setup|Create)\s*\(' + $script:SizeLiteralArg + ',\s*' + $size + '\s*,' + $script:SizeLiteralArg + '\)')),
        (New-Object regex ('\bSetupMemoryPool\s*\(' + $script:SizeLiteralArg + ',\s*' + $size + '\s*,')),
        (New-Object regex ('\bUpdateMemoryPoolSize\s*\(\s*' + $size + '\s*\)')),
        (New-Object regex ('\bRed(?:Alloc|Realloc)\w*\s*\((?:' + $script:SizeLiteralArg + ',)*\s*' + $alloc + '\s*[,)]'))
    )
}

$script:SizeLiteralRx = New-SizeLiteralRx $script:SizeArg $script:SizeFourArg

function Get-X64SizeLiteralCheck([string]$root, [string[]]$allow = $script:SizeLiteralAllow) {
    $root = $root.TrimEnd('\')
    $hits = New-Object System.Collections.Generic.List[string]
    $left = New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)
    $verdictRx = New-Object regex '^(.*)\|x64:safe:(.+)$'
    foreach ($a in @($allow)) {
        if (-not $a) { continue }
        $vm = $verdictRx.Match($a)
        if (-not $vm.Success -or -not $vm.Groups[2].Value.Trim()) { $hits.Add('allow without an x64:safe reason ' + $a); continue }
        $left[$vm.Groups[1].Value] = 1 + [int]$left[$vm.Groups[1].Value]
    }
    $src = Join-Path $root 'src'
    $files = @(Get-ChildItem -LiteralPath $src -Recurse -File -ErrorAction SilentlyContinue | Where-Object { $script:SourceExtensions -contains $_.Extension -and $_.FullName -notmatch '\\src\\(LuaFL|ZlibFL|mimalloc)\\' })
    foreach ($f in @($files | Sort-Object FullName)) {
        $text = [IO.File]::ReadAllText($f.FullName)
        $blank = $script:ByteCommentRx.Replace($text, { param($m) if ($m.Value[0] -eq '/') { [regex]::Replace($m.Value, '[^\r\n]', ' ') } else { $m.Value } })
        $rel = $f.FullName.Substring($root.Length + 1)
        $fileRx = $script:SizeLiteralRx
        $locals = @($script:SizeFourLocalRx.Matches($blank) | ForEach-Object { $_.Groups['name'].Value } | Select-Object -Unique)
        if ($locals.Count) {
            $localArg = '\b(?:' + (@($locals | ForEach-Object { [regex]::Escape($_) }) -join '|') + ')\b'
            $fileRx = @($script:SizeLiteralRx) + @(New-SizeLiteralRx $localArg $localArg)
        }
        foreach ($rx in $fileRx) {
            foreach ($m in $rx.Matches($blank)) {
                $key = '{0}|{1}' -f $rel, ([regex]::Replace($m.Value, '\s+', ' '))
                if ($left[$key] -gt 0) { $left[$key]--; continue }
                $hits.Add(('{0}:{1}' -f $rel, $blank.Substring(0, $m.Index).Split("`n").Count))
            }
        }
    }
    foreach ($k in @($left.Keys | Sort-Object)) {
        for ($i = 0; $i -lt $left[$k]; $i++) { $hits.Add('unused allow ' + $k) }
    }
    return New-Object PSObject -Property @{ Name = 'x64-size-literal'; Ok = ($hits.Count -eq 0); Detail = ($hits -join ', ') }
}

function Read-ClTlog($tlogDirs) {
    $flags = @{}
    foreach ($d in @($tlogDirs)) {
        $files = @(Get-ChildItem -LiteralPath $d -Filter 'cl.command*.tlog' -File -ErrorAction SilentlyContinue)
        if ($files.Count -eq 0) { throw "cl command tlog not found in $d" }
        foreach ($f in $files) {
            $keys = @()
            foreach ($l in [IO.File]::ReadAllLines($f.FullName, [Text.Encoding]::Unicode)) {
                $l = $l.TrimStart([char]0xFEFF)
                if ($l.StartsWith('^')) { $keys = @($l.Substring(1).Split('|') | ForEach-Object { $_.ToUpperInvariant() }) }
                elseif ($l -and $keys.Count) { foreach ($k in $keys) { $flags[$k] = $l }; $keys = @() }
            }
        }
    }
    return $flags
}

function Resolve-LayoutPairs($pairs, $flags, [string]$root) {
    $work = New-Object System.Collections.Generic.List[object]
    $problems = New-Object System.Collections.Generic.List[string]
    foreach ($pair in $pairs) {
        $tu = [IO.Path]::GetFullPath($pair[1])
        $rel = $tu.Substring($root.TrimEnd('\').Length + 1)
        $fl = $flags[$tu.ToUpperInvariant()]
        if (-not $fl) { $problems.Add("$($pair[0])@$rel not in any game tlog"); continue }
        $work.Add((New-Object PSObject -Property @{ Class = $pair[0]; Tu = $tu; Rel = $rel; Flags = $fl }))
    }
    return New-Object PSObject -Property @{ Work = $work; Problems = $problems }
}

function Get-ClTlogCounts($objDirs) {
    $missing = New-Object System.Collections.Generic.List[string]
    $files = @()
    foreach ($objDir in @($objDirs)) {
        $found = @(Get-ChildItem -LiteralPath $objDir -Filter 'cl.command*.tlog' -Recurse -ErrorAction SilentlyContinue)
        if ($found.Count -eq 0) { $missing.Add((Split-Path $objDir -Leaf)) }
        $files += $found
    }
    $total = 0
    $gsMinus = 0
    $sdl = 0
    $alignedNewOff = 0
    $inlineOff = 0
    $inlineOn = 0
    $throwingNew = 0
    $gl = 0
    $fpFast = 0
    $fpContract = 0
    $miLines = New-Object System.Collections.Generic.List[string]
    $we = [ordered]@{ '4311' = 0; '4312' = 0; '4302' = 0; '4477' = 0 }
    $weAll = 0
    $weMissing = New-Object System.Collections.Generic.List[string]
    foreach ($f in $files) {
        $isMi = $false
        $unit = ''
        foreach ($l in (Get-Content -LiteralPath $f.FullName)) {
            if ($l -and $l.StartsWith('^')) {
                $unit = $l.Substring(1).Split('|')[0]
                $isMi = @($l.Substring(1).Split('|') | Where-Object { $_.EndsWith('\SRC\MIMALLOC\SRC\STATIC.C', [StringComparison]::OrdinalIgnoreCase) }).Count -gt 0
            }
            elseif ($l) {
                $total++
                if ($isMi) { $miLines.Add($l) }
                $weHits = 0
                foreach ($code in @($we.Keys)) {
                    if ($l -cmatch ('(?:^| )/we"?' + $code + '"?(?= |$)')) { $we[$code]++; $weHits++ }
                }
                if ($weHits -eq $we.Count) { $weAll++ } else { $weMissing.Add($unit) }
                if ($l -match ' /GS-( |$)') { $gsMinus++ }
                if ($l -match ' /sdl( |$)') { $sdl++ }
                if ($l -cmatch ' /Zc:alignedNew-( |$)') { $alignedNewOff++ }
                if ($l -cmatch ' /Zc:inline-( |$)') { $inlineOff++ }
                if ($l -cmatch ' /Zc:inline( |$)') { $inlineOn++ }
                if ($l -cmatch ' /Zc:throwingNew( |$)') { $throwingNew++ }
                if ($l -cmatch ' /GL( |$)') { $gl++ }
                if ($l -cmatch ' /fp:fast( |$)') { $fpFast++ }
                if ($l -cmatch ' /fp:contract( |$)') { $fpContract++ }
            }
        }
    }
    return New-Object PSObject -Property @{
        Total = $total; GsMinus = $gsMinus; Sdl = $sdl
        AlignedNewOff = $alignedNewOff; InlineOff = $inlineOff; InlineOn = $inlineOn; ThrowingNew = $throwingNew
        Gl = $gl; FpFast = $fpFast; FpContract = $fpContract; MiLines = $miLines.ToArray(); Missing = $missing.ToArray()
        We4311 = $we['4311']; We4312 = $we['4312']; We4302 = $we['4302']; We4477 = $we['4477']; WeAll = $weAll; WeMissing = $weMissing.ToArray()
    }
}

$script:Fma3OffFunctions = @('Startup_RedInitHeap', '?PcMathStartup@@YAXXZ')

function Get-Fma3OffDisasm([string]$exe, [string]$mapText, [string]$platform) {
    $syms = @([regex]::Matches($mapText, '(?m)^ [0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{16}) ') | ForEach-Object { New-Object PSObject -Property @{ Name = $_.Groups[1].Value; Va = [Convert]::ToUInt64($_.Groups[2].Value, 16) } })
    $vas = @($syms | ForEach-Object { $_.Va } | Sort-Object -Unique)
    $db = Find-Dumpbin $platform
    $text = @()
    foreach ($name in $script:Fma3OffFunctions) {
        $s = @($syms | Where-Object { $_.Name -ceq $name })
        if ($s.Count -ne 1) { throw "map has $($s.Count) symbols named $name" }
        $end = @($vas | Where-Object { $_ -gt $s[0].Va } | Select-Object -First 1)
        if (-not $end.Count) { throw "map has no symbol after $name" }
        $text += @(& $db /nologo /disasm:nobytes ('/range:0x{0:X},0x{1:X}' -f $s[0].Va, ($end[0] - 1)) $exe)
        if ($LASTEXITCODE -ne 0) { throw "dumpbin /disasm exit $LASTEXITCODE" }
    }
    return ($text -join "`n")
}

function Get-Fma3OffCheck([string]$disasm) {
    $bad = @()
    $blocks = @{}
    $cur = $null
    foreach ($l in ($disasm -split "`r?`n")) {
        if ($l -cmatch '^(\S.*):\s*$') {
            $cur = $Matches[1]
            if (-not $blocks.ContainsKey($cur)) { $blocks[$cur] = New-Object System.Collections.Generic.List[string] }
        }
        elseif ($cur -and $l -cmatch '^\s+[0-9A-F]{8,16}:\s+(\S.*?)\s*$') { $blocks[$cur].Add($Matches[1]) }
    }
    $heap = $blocks['Startup_RedInitHeap']
    if (-not $heap) { $bad += 'Startup_RedInitHeap not found' }
    elseif (-not @($heap | Where-Object { $_ -cmatch '^(call|jmp)\s+\?PcMathStartup@@YAXXZ$' }).Count) { $bad += 'Startup_RedInitHeap does not call PcMathStartup' }
    $ms = $blocks['?PcMathStartup@@YAXXZ']
    if (-not $ms) { $bad += 'PcMathStartup not found' }
    else {
        $zero = $false
        $hit = $null
        foreach ($i in $ms) {
            if ($i -cmatch '^(call|jmp)\s+(qword\s+ptr\s+\[__imp__set_FMA3_enable\]|_set_FMA3_enable)$') { $hit = $zero; break }
            if ($i -cmatch '^(xor\s+ecx,ecx|mov\s+ecx,0)$') { $zero = $true }
            elseif ($i -cmatch '^(call\b|\w+\s+(ecx|rcx|cx|cl|ch)\b)') { $zero = $false }
        }
        if ($null -eq $hit) { $bad += 'PcMathStartup does not call _set_FMA3_enable' }
        elseif (-not $hit) { $bad += 'PcMathStartup does not pass 0 to _set_FMA3_enable' }
    }
    return New-Object PSObject -Property @{ Name = 'fma3-off'; Ok = ($bad.Count -eq 0); Detail = ($bad -join '; ') }
}

function Get-MiCrtHookCheck([string]$mapText) {
    $bad = @()
    if (-not $mapText) { $bad += 'no map' }
    else {
        foreach ($want in @(@('.CRT$XIB', '__mi_crt_callback_init'), @('.CRT$XLB', '__mi_tls_callback_pre'), @('.CRT$XLY', '__mi_tls_callback_post'))) {
            $sec = [regex]::Match($mapText, '(?m)^ ([0-9a-f]{4}):([0-9a-f]{8}) ([0-9a-f]{8})H ' + [regex]::Escape($want[0]) + '\s')
            $sym = [regex]::Match($mapText, '(?m)^ ([0-9a-f]{4}):([0-9a-f]{8})\s+' + [regex]::Escape($want[1]) + '\s+[0-9a-f]{8}\s+(\S+)\s*$')
            if (-not $sec.Success) { $bad += "$($want[0]) missing"; continue }
            if (-not $sym.Success) { $bad += "$($want[1]) missing"; continue }
            $start = [Convert]::ToInt64($sec.Groups[2].Value, 16)
            $end = $start + [Convert]::ToInt64($sec.Groups[3].Value, 16)
            $at = [Convert]::ToInt64($sym.Groups[2].Value, 16)
            if ($sym.Groups[1].Value -ne $sec.Groups[1].Value -or $at -lt $start -or $at -ge $end) { $bad += "$($want[1]) outside $($want[0])" }
            elseif ($sym.Groups[3].Value -notmatch '(^|:)static\.obj$') { $bad += "$($want[1]) from $($sym.Groups[3].Value)" }
        }
    }
    return New-Object PSObject -Property @{ Name = 'mimalloc-crt-hooks'; Ok = ($bad.Count -eq 0); Detail = ($bad -join '; ') }
}


function Get-FlagChecks($game, $tests, [string]$gameConfig, [string]$testsConfig, $luac) {
    $checks = @()
    $gameMissing = @($game.Missing | Where-Object { $_ })
    $missTail = ''
    if ($gameMissing.Count) { $missTail = '; no cl tlog in ' + ($gameMissing -join ' ') }
    $checks += New-Object PSObject -Property @{
        Name = 'flag-alignednew'
        Ok = ($game.Total -gt 0 -and $game.AlignedNewOff -eq $game.Total -and $gameMissing.Count -eq 0)
        Detail = ('cl entries={0} /Zc:alignedNew-={1}' -f $game.Total, $game.AlignedNewOff) + $missTail
    }
    $checks += New-Object PSObject -Property @{
        Name = 'flag-inline'
        Ok = ($game.Total -gt 0 -and $game.InlineOn -eq $game.Total -and $game.InlineOff -eq 0 -and $gameMissing.Count -eq 0)
        Detail = ('cl entries={0} /Zc:inline={1} /Zc:inline-={2}' -f $game.Total, $game.InlineOn, $game.InlineOff) + $missTail
    }
    $bags = @($game)
    if ($tests) {
        $checks += New-Object PSObject -Property @{
            Name = 'flag-alignednew-tests'
            Ok = ($tests.Total -gt 0 -and $tests.AlignedNewOff -eq $tests.Total)
            Detail = ('tests cl entries={0} /Zc:alignedNew-={1}' -f $tests.Total, $tests.AlignedNewOff)
        }
        $bags += $tests
    }
    $sum = @{ ThrowingNew = 0; Gl = 0; FpFast = 0; FpContract = 0; GsMinus = 0 }
    foreach ($b in $bags) { foreach ($k in @($sum.Keys)) { $sum[$k] += [int]$b.$k } }
    $checks += New-Object PSObject -Property @{
        Name = 'flag-banned'
        Ok = (($sum.ThrowingNew + $sum.Gl + $sum.FpFast + $sum.FpContract + $sum.GsMinus) -eq 0)
        Detail = ('/Zc:throwingNew={0} /GL={1} /fp:fast={2} /fp:contract={3} /GS-={4}' -f $sum.ThrowingNew, $sum.Gl, $sum.FpFast, $sum.FpContract, $sum.GsMinus)
    }
    if ($script:SmokePlatform -eq 'x64') {
        $truncOk = ($gameMissing.Count -eq 0)
        $truncParts = @()
        foreach ($tb in @(@('game', $game), @('tests', $tests), @('luac', $luac))) {
            $b = $tb[1]
            if (-not $b -or [int]$b.Total -eq 0) {
                $truncOk = $false
                $truncParts += ('{0} cl entries=0' -f $tb[0])
                continue
            }
            $part = ('{0} cl entries={1} /we4311={2} /we4312={3} /we4302={4} /we4477={5} all four={6}' -f $tb[0], $b.Total, [int]$b.We4311, [int]$b.We4312, [int]$b.We4302, [int]$b.We4477, [int]$b.WeAll)
            if ([int]$b.WeAll -ne [int]$b.Total) {
                $truncOk = $false
                $miss = @($b.WeMissing | Where-Object { $_ })
                $first = $(if ($miss.Count) { $miss[0] } else { 'unknown unit' })
                $part += (' first missing {0}' -f $first)
            }
            $truncParts += $part
        }
        $checks += New-Object PSObject -Property @{
            Name = 'x64-truncation'
            Ok = $truncOk
            Detail = ($truncParts -join '; ') + $missTail
        }
    }
    $miBad = @()
    $miBags = @(, @('game', $game, $gameConfig))
    if ($tests) { $miBags += , @('tests', $tests, $testsConfig) }
    foreach ($mb in $miBags) {
        $lines = @($mb[1].MiLines | Where-Object { $_ })
        if ($lines.Count -ne 1) {
            $miBad += ('{0} static.c command lines={1}' -f $mb[0], $lines.Count)
            continue
        }
        $l = $lines[0]
        foreach ($need in @(@('MI_NO_GETENV=1', 'MI_NO_GETENV=1'), @('MI_USE_RTLGENRANDOM', 'MI_USE_RTLGENRANDOM(?:=1)?'))) {
            if ($l -cnotmatch ('(?:^| )/D "?' + $need[1] + '"?(?= |$)')) { $miBad += ('{0} missing {1}' -f $mb[0], $need[0]) }
        }
        $want = $script:MiDefines[$mb[2]]
        if (-not $want) { $miBad += ('{0} has no expected defines for configuration "{1}"' -f $mb[0], $mb[2]) }
        else {
            $have = (@([regex]::Matches($l, '(?:^| )/D "?(MI_(?:DEBUG|SECURE|TRACK_ASAN)(?:=[^" ]*)?)"?(?= |$)') | ForEach-Object { $_.Groups[1].Value }) | Sort-Object) -join ' '
            $exp = (@($want) | Sort-Object) -join ' '
            if ($have -cne $exp) { $miBad += ('{0} {1} has "{2}" want "{3}"' -f $mb[0], $mb[2], $have, $exp) }
        }
        $asan = $l -cmatch ' /fsanitize=address(?= |$)'
        $track = $l -cmatch '(?:^| )/D "?MI_TRACK_ASAN=1"?(?= |$)'
        if ($asan -and -not $track) { $miBad += ('{0} missing MI_TRACK_ASAN=1 with /fsanitize=address' -f $mb[0]) }
        elseif ($track -and -not $asan) { $miBad += ('{0} MI_TRACK_ASAN=1 without /fsanitize=address' -f $mb[0]) }
    }
    $checks += New-Object PSObject -Property @{
        Name = 'mimalloc-defines'
        Ok = ($miBad.Count -eq 0)
        Detail = ($miBad -join '; ')
    }
    return $checks
}

$script:ByteCommentRx = New-Object regex '"(?:[^"\\\r\n]|\\.)*"|''(?:[^''\\\r\n]|\\.)*''|//[^\r\n]*|/\*[\s\S]*?\*/'
$script:ByteDeclItem = '\s*((?:[*&]\s*(?:(?:const|volatile)\b\s*)*)*)(\w+)\s*((?:\[[^\]]*\]\s*)*)(?:=\s*(?:\{(?:[^{}]|\{(?:[^{}]|\{[^{}]*\})*\})*\}|[^,;{}]*)|\{(?:[^{}]|\{[^{}]*\})*\})?\s*'
$script:ByteDeclRx = New-Object regex ('(?m)(?<=^[ \t]*|[{};:][ \t]*)(?:(?:static|const|volatile|mutable|extern|thread_local|register|constexpr|inline|typedef|__unaligned|alignas\s*\((?:[^()]|\([^()]*\))*\)|__declspec\s*\((?:[^()]|\([^()]*\))*\))\s+)*(?:(?:unsigned|signed)\s+)?(?:char|__int8|BYTE|UCHAR|CHAR|INT8|UINT8|uint8_t|int8_t|std::uint8_t|std::int8_t|std::byte|byte)(?:\s+(?:const|volatile))*\b(?!\s*[(:])(?<list>' + $script:ByteDeclItem + '(?:,' + $script:ByteDeclItem + ')*);')
$script:ByteDeclItemRx = New-Object regex ('\G' + $script:ByteDeclItem + '(?:,|$)')
$script:SpacerNameRx = '(?i)pad|reserv|unmodel|unknown|^_*unk|spacer|filler|tail|unused|dummy|storage|align|^_+\w*\d'
$script:SerialRx = New-Object regex '\b(?:sThreadActivationKey|mCollisionSerial|sCollisionSerialNext)\b'
$script:OffsetQual = '(?:(?:const|volatile)\s*)*'
$script:OffsetByteType = '(?:(?:const|volatile)\s+)*(?:(?:unsigned|signed)\s+)?(?:char|__int8|BYTE|UCHAR|CHAR|INT8|UINT8|uint8_t|int8_t|std::uint8_t|std::int8_t|std::byte|byte)(?:\s+(?:const|volatile))*\s*\*\s*' + $script:OffsetQual
$script:OffsetCall = '(?<=[\w\]])\((?:[^()]|\([^()]*\))*\)'
$script:OffsetOperand = '(?:(?:&\s*)?(?:[\w.]|->|::|\[[^\[\];]*\]|' + $script:OffsetCall + ')+|\((?:[^()]|\([^()]*\))*\))'
$script:OffsetByteCast = '(?:\(\s*' + $script:OffsetByteType + '\)\s*' + $script:OffsetOperand + '|\breinterpret_cast\s*<\s*' + $script:OffsetByteType + '>\s*\((?:[^()]|\([^()]*\))*\))'
$script:OffsetTypeCast = '(?:\(\s*(?:(?:const|volatile|struct|class|unsigned|signed)\s+)*[A-Za-z_][\w:]*(?:\s+(?:int|char|short|long|__int\d+))?(?:\s+(?:const|volatile))*\s*[*&][\s*&]*' + $script:OffsetQual + '\)|\b(?:reinterpret|static|const)_cast\s*<[^<>;()]*[*&]\s*' + $script:OffsetQual + '>)'
$script:OffsetOp = '\s*[+-](?![+=>-]|\s*(?:\(\s*\w+\s*\)\s*)?offsetof\b|\s*(?:\w+\s*(?:->|\.)\s*)+e_lfanew\b)'
$script:OffsetTail = $script:OffsetOp + '[^;()]*(?:\((?:[^()]|\([^()]*\))*\)[^;()]*)*\)'
$script:OffsetRx = @(
    (New-Object regex ($script:OffsetTypeCast + '\s*\(\s*' + $script:OffsetByteCast + $script:OffsetTail)),
    (New-Object regex ($script:OffsetTypeCast + '\s*\(\s*(?!\d)(?:[\w.]|->|::|\[[^\[\];]*\]|' + $script:OffsetCall + ')+' + $script:OffsetTail)),
    (New-Object regex ($script:OffsetByteCast + $script:OffsetOp + '\s*(?:0[xX][0-9A-Fa-f]+|\d+)[uUlL]*\b'))
)

function Get-ByteArrayDecls([string]$text) {
    $blank = $script:ByteCommentRx.Replace($text, { param($m) if ($m.Value[0] -eq '/') { [regex]::Replace($m.Value, '[^\r\n]', ' ') } else { $m.Value } })
    $out = New-Object System.Collections.Generic.List[object]
    foreach ($m in $script:ByteDeclRx.Matches($blank)) {
        $list = $m.Groups['list']
        $pos = 0
        while ($pos -lt $list.Length) {
            $d = $script:ByteDeclItemRx.Match($list.Value, $pos)
            if (-not $d.Success -or $d.Length -eq 0) { break }
            if ($d.Groups[3].Value -and -not $d.Groups[1].Value) {
                $out.Add((New-Object PSObject -Property @{ Name = $d.Groups[2].Value; Size = ($d.Groups[3].Value -replace '\s', ''); Line = $blank.Substring(0, $list.Index + $d.Groups[2].Index).Split("`n").Count }))
            }
            $pos = $d.Index + $d.Length
        }
    }
    return $out
}

function Test-ByteSpacer($decl) {
    return ($decl.Size -match '0[xX][0-9A-Fa-f]' -or $decl.Size -match '\b(?:sizeof|offsetof)\b' -or $decl.Name -match $script:SpacerNameRx)
}

function Get-SourceChecks([string]$root, [string[]]$spacerAllow = $script:SpacerAllow, [string[]]$offsetAllow = $script:OffsetAllow, [string[]]$fastcallAllow = $script:FastcallAllow, [string[]]$serialAllow = $script:SerialAllow, [string[]]$addrOrderAllow = $script:AddrOrderAllow, [string[]]$Only) {
    $want = { param($name) -not $Only -or $Only -contains $name }
    $aoT = 'unsigned\s+int|unsigned\s+long|unsigned|int|long|size_t|uintptr_t|intptr_t|UINT_PTR|DWORD_PTR|DWORD|uint32_t'
    $aoP = '(?:this|(?:\w+(?:\.|->))*m?_?p{1,2}[A-Z]\w*)'
    $aoC = '(?:const\s+)?(?:unsigned\s+|signed\s+)?(?:char|void)\s*\*\s*(?:const\s*)?'
    $defs = @(
        @{ Name = 'new-size-t'; Path = $null; Tests = $true; Rx = @(
            '\boperator\s*(new|delete)\s*(\[\s*\])?\s*\(\s*(?:void\s*\*\s*\w*\s*,\s*)?(?:unsigned(?:\s+int)?|UINT|uint32_t|unsigned\s+__int32)\b') },
        @{ Name = 'win32-callbacks'; Path = $null; Rx = @(
            '\(\s*(?:[A-Z_]*PROC[A-Z]*|L?P?THREAD_START_ROUTINE|PAPCFUNC|PTIMERAPCROUTINE)\s*\)',
            '_cast\s*<\s*(?:[A-Z_]*PROC[A-Z]*|L?P?THREAD_START_ROUTINE|PAPCFUNC|PTIMERAPCROUTINE)\s*>',
            '\b(?:\w+::)*(?!\w*Timer)\w*Proc\s*\(\s*HWND\w*\s*\*?\s*\w*\s*,(?!\s*UINT\b\s*\w*\s*,\s*WPARAM\b\s*\w*\s*,\s*LPARAM\b\s*\w*\s*\))[^,()]*,[^,()]*,[^,()]*\)',
            '(?:CALLBACK|WINAPI|APIENTRY|__stdcall)\s+(?:\w+::)*(?!\w*Timer)\w+\s*\(\s*HWND\w*\s*\*?\s*\w*\s*,\s*(?:UINT|unsigned)\b(?!\s*\w*\s*,\s*WPARAM\b\s*\w*\s*,\s*LPARAM\b\s*\w*\s*\))[^,()]*,[^,()]*,[^,()]*\)',
            '\b(?!LRESULT\b|INT_PTR\b)\w+\s+(?:CALLBACK|WINAPI|APIENTRY|__stdcall)\s+(?:\w+::)*(?!\w*Timer)\w*Proc\s*\(\s*HWND\w*\s*\*?\s*\w*\s*,[^,()]*,[^,()]*,[^,()]*\)',
            '\b(?:\w+::)*\w*Proc\s*\(\s*int\b\s*\w*\s*,(?!\s*WPARAM\b\s*\w*\s*,\s*LPARAM\b\s*\w*\s*\))[^,()]*,[^,()]*\)',
            '\b(?!LRESULT\b)\w+\s+(?:CALLBACK|WINAPI|APIENTRY|__stdcall)\s+(?:\w+::)*\w*Proc\s*\(\s*int\b\s*\w*\s*,[^,()]*,[^,()]*\)') },
        @{ Name = 'galaxy-vtable'; Path = '\\src\\Steam\\'; Rx = @(
            '\(\s*(?:const\s+)?\w+(?:\s+\w+)?\s*(?:const\s*)?\*\s*(?:const\s*)?\*[\s*]*\)',
            '_cast\s*<\s*(?:const\s+)?\w+(?:\s+\w+)?\s*(?:const\s*)?\*\s*(?:const\s*)?\*[\s*]*>',
            '__thiscall\s*\*') },
        @{ Name = 'fastcall-edx'; Path = $null; Rx = @(
            '__fastcall\s*\*?\s*(?:\w+::)*\w*\s*\)?\s*\(\s*[^,()]+?,\s*(?:const\s+)?void\s*\*\s*(?:/\*[^*]*\*/\s*)?(?:\w*(?:[Ee]dx|EDX|[Uu]nused|[Dd]ummy|[Ii]gnored)\w*)?\s*[,)]',
            '__fastcall\s*\*?\s*(?:\w+::)*\w*\s*\)?\s*\(\s*(?:[^,()]+?,\s*)?(?:const\s+)?(?:int|unsigned(?:\s+int)?|void\s*\*)\s*(?:(?:/\*[^*]*\*/\s*)?\w*(?:[Ee]dx|EDX|[Uu]nused|[Dd]ummy|[Ii]gnored)\w*|/\*[^*]*(?:[Ee]dx|EDX|[Uu]nused|[Dd]ummy|[Ii]gnored)[^*]*\*/)\s*[,)]',
            '__fastcall\s*\*?\s*(?:\w+::)*\w*\s*\)?\s*\(\s*int\s+iLocalIndex\s*,',
            '__fastcall\s*\*?\s*(?:\w+::)*\w*\s*\)?\s*\(\s*(?:const\s+)?\w+\s*\*\s*\w*\s*,\s*int\s*,\s*unsigned\b',
            '__fastcall\s+(?:\w+::)*\w+\s*\((?=[^(){};]*\)\s*\{)\s*(?:(?:const\s+)?[\w:]+(?:\s+(?:int|char|short|long))?(?:\s*[*&]+\s*|\s+)(?!(?:int|char|short|long|const)\b)\w+\s*,\s*int|int\s*,\s*(?:const\s+)?[\w:]+(?:\s+(?:int|char|short|long))?(?:\s*[*&]+\s*|\s+)(?!(?:int|char|short|long|const)\b)\w+)\s*[,)]') },
        @{ Name = 'd3dx-decl'; Path = '\\src\\(?!PebbleFL\\pc\\pcD3dxLinkX64\.cpp$)'; Rx = @(
            'extern\s*"C"[^;{}]*?\bD3DX\w*\s*\(',
            '(?:\b_{1,2}stdcall|\bWINAPI|\bAPIENTRY|\bCALLBACK|\bSTDMETHODCALLTYPE|\bSTDAPICALLTYPE|\bNTAPI|\bPASCAL)\b(?:\s|/\*.*?\*/)*(?:(?:const\s+)?[\w:]+(?:\s*\*+\s*|\s+)(?:/\*.*?\*/\s*)*)?D3DX\w*\s*\(') },
        @{ Name = 'gamma-get'; Path = $null; Rx = @('GetDeviceGammaRamp') },
        @{ Name = 'addr-order'; Path = '\\src\\(?:Battlefront2|PebbleFL)\\'; Rx = @(
            "\(\s*(?:$aoT)\s*\)\s*\(?\s*this\b",
            "\(\s*(?:$aoT)\s*\)\s*(?:\(\s*(?:size_t|uintptr_t)\s*\)\s*)?$aoP\s*[<>]=?[^<>=]",
            "[<>]=?\s*\(\s*(?:$aoT)\s*\)\s*(?:\(\s*(?:size_t|uintptr_t)\s*\)\s*)?$aoP\b(?!\s*(?:->|\.|\[|\())",
            "reinterpret_cast\s*<\s*(?:$aoT)\s*>\s*\(\s*$aoP\s*\)\s*[<>]",
            "[<>]=?\s*reinterpret_cast\s*<\s*(?:$aoT)\s*>\s*\(\s*$aoP\s*\)",
            "\(\s*$aoC\)\s*$aoP\s*[<>]=?[^<>=]",
            "[<>]=?\s*\(\s*$aoC\)\s*$aoP\b(?!\s*(?:->|\.|\[|\())",
            "(?:\.|->)(?:Add|Move)\s*\([^;]*\(\s*(?:$aoT)\s*\)\s*\(?\s*$aoP\b(?!\s*(?:->|\.|\[|\())",
            '\bstd::(?:less|greater)(?:_equal)?\s*<[^<>;]*\*\s*>',
            '\bPblPtr(?:OrderKey|After)\b',
            '\b(?:this|m?_?p{1,2}[A-Z]\w*)\s*[<>]=?\s*(?:this|m?_?p{1,2}[A-Z]\w*)\b') },
        @{ Name = 'mi-api-ban'; Path = '\\src\\(?!.*\\RedMemoryMi\.cpp$)'; Rx = @('\bmi_\w+\s*\(') },
        @{ Name = 'crt-heap-ban'; Path = '\\src\\(?!PebbleFL\\Common\\PblCrtHeap\.cpp$)'; Rx = @('(?<![\w.>:~])(?:malloc|calloc|realloc|free|_strdup|strdup|_wcsdup|wcsdup|_msize|_expand|_recalloc|_aligned_malloc|_aligned_offset_malloc|_aligned_realloc|_aligned_free)\s*\(') },
        @{ Name = 'snd-alloc-size-t'; Path = '\\src\\(?:SoundFL\\|Battlefront2\\Source\\GameSoundEngine\.(?:cpp|h)$)'; Rx = @(
            '\b(?:SndAlloc|SndTempAlloc)\s*\(\s*unsigned\b',
            '\(\s*__cdecl\s*\*\s*\w*Alloc\w*\s*\)\s*\(\s*unsigned\b') },
        @{ Name = 'galaxy-getter-cast'; Path = '\\src\\'; Rx = @(
            '\(galaxy::api::I\w+\s*\*\)\s*\(?\s*Get\w*Galaxy',
            'void\*\s+\w+\s*=\s*Get\w*Galaxy',
            'void\s*\*\s*\w*Get\w*Galaxy\w*\s*\(') },
        @{ Name = 'galaxy-factory-seam'; Path = '\\src\\(?!Steam\\gfVendor\.cpp$)'; Rx = @('\bGalaxyFactory\s*::\s*\w+\s*\(') },
        @{ Name = 'steamid-abi'; Path = $null; Note = '(settle the x64 CSteamID hidden-return and by-value convention first)'; Rx = @(
            '(?:->|\.|::)\s*(?:GetSteamID|InitiateGameConnection|BeginAuthSession|EndAuthSession|UserHasLicenseForApp|AdvertiseGame)\s*\(',
            @{ Path = '\\src\\(?!Steam\\steamworks\.h$)'; Rx = '(?<!\b(?:class|struct)\s+)\bCSteamID\b(?!\s*[*&])' }) },
        @{ Name = 'handle-raw'; Path = '\\src\\(?!PebbleFL\\Common\\PblHandle\.(?:h|cpp)$)'; Rx = @(
            '\bPBL_HANDLE_ID\b',
            '\bm(?:Saved)?HandleId\b',
            '(?i)(?:[!=]=\s*(?:\(\s*[\w:]+(?:\s+[\w:]+)*\s*\*?\s*\)\s*)?0xd{8}[ul]*\b|\b0xd{8}[ul]*\s*[!=]=|\*\s*\)\s*0xd{8}[ul]*\b|_cast\s*<[^<>;]*>\s*\(\s*0xd{8}[ul]*\b)',
            '\bGetHandleId\s*\(\s*\)\s*[!=]=(?!\s*\(unsigned int\)\s*debug\w*Id\b)(?!\s*net\w*Trace\w*\b)(?!\s*[1-9]\d*\b)',
            '[!=]=\s*(?:\w+(?:\(\s*\))?(?:\.|->))*(?:\w+::)?GetHandleId\s*\(',
            '(?:&&|\|\||\b(?:if|while)\s*\(|!)\s*(?:\w+(?:\(\s*\))?(?:\.|->))*(?:\w+::)?GetHandleId\s*\(\s*\)\s*(?:\)|&&|\|\||\?)',
            '\bGameObjectHandleId\b') }
    )
    $runDefs = @($defs | Where-Object { & $want $_.Name })
    $doSpacer = & $want 'char-spacer'
    $doOffset = & $want 'offset-hack'
    $doSerial = & $want 'serial-value'
    $root = $root.TrimEnd('\')
    $hits = @{}
    foreach ($d in $defs) { $hits[$d.Name] = New-Object System.Collections.Generic.List[string] }
    $verdictRx = New-Object regex '^(.*)\|x64:(safe|fixed):(.+)$'
    $verdictHits = New-Object System.Collections.Generic.List[string]
    $spacerLeft = @{}
    foreach ($a in @($spacerAllow)) {
        if (-not $a) { continue }
        $vm = $verdictRx.Match($a)
        if ($vm.Success -and $vm.Groups[3].Value.Trim()) { $a = $vm.Groups[1].Value }
        else { $verdictHits.Add('no verdict ' + $a) }
        $spacerLeft[$a] = 1 + [int]$spacerLeft[$a]
    }
    $spacerHits = New-Object System.Collections.Generic.List[string]
    $offsetLeft = New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)
    foreach ($a in @($offsetAllow)) {
        if (-not $a) { continue }
        $vm = $verdictRx.Match($a)
        if ($vm.Success -and $vm.Groups[3].Value.Trim()) { $a = $vm.Groups[1].Value }
        else { $verdictHits.Add('no verdict ' + $a) }
        $offsetLeft[$a] = 1 + [int]$offsetLeft[$a]
    }
    $offsetHits = New-Object System.Collections.Generic.List[string]
    $serialLeft = New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)
    foreach ($a in @($serialAllow)) { if ($a) { $serialLeft[$a] = 1 + [int]$serialLeft[$a] } }
    $serialHits = New-Object System.Collections.Generic.List[string]
    $sigLeft = @{ 'fastcall-edx' = (New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)); 'addr-order' = (New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)) }
    foreach ($a in @($fastcallAllow)) { if ($a) { $sigLeft['fastcall-edx'][$a] = 1 + [int]$sigLeft['fastcall-edx'][$a] } }
    foreach ($a in @($addrOrderAllow)) { if ($a) { $sigLeft['addr-order'][$a] = 1 + [int]$sigLeft['addr-order'][$a] } }
    $src = Join-Path $root 'src'
    $allFiles = @()
    if ($runDefs.Count -or $doSpacer -or $doOffset -or $doSerial) {
        $allFiles = @(Get-ChildItem -LiteralPath $src -Recurse -File | Where-Object { $script:SourceExtensions -contains $_.Extension })
    }
    $srcFiles = @()
    if ($runDefs.Count -or $doSpacer -or $doOffset) {
        $srcFiles = @($allFiles | Where-Object { $_.FullName -notmatch '\\src\\(LuaFL|ZlibFL|mimalloc)\\' })
    }
    $testsDir = Join-Path $root 'tests'
    $testFiles = @()
    if (((& $want 't1-suite') -or @($runDefs | Where-Object { $_.Tests }).Count) -and (Test-Path -LiteralPath $testsDir)) {
        $testFiles = @(Get-ChildItem -LiteralPath $testsDir -Recurse -File | Where-Object { $script:SourceExtensions -contains $_.Extension -and $_.FullName -notmatch '\\tests\\third_party\\' })
    }
    if ($doSerial) {
        foreach ($f in @($allFiles | Sort-Object FullName)) {
            $text = [IO.File]::ReadAllText($f.FullName)
            if (-not $script:SerialRx.IsMatch($text)) { continue }
            $rel = $f.FullName.Substring($root.Length + 1)
            $lines = $text.Split("`n")
            for ($i = 0; $i -lt $lines.Count; $i++) {
                if (-not $script:SerialRx.IsMatch($lines[$i])) { continue }
                $key = '{0}|{1}' -f $rel, ([regex]::Replace($lines[$i].Trim(), '\s+', ' '))
                if ($serialLeft[$key] -gt 0) { $serialLeft[$key]--; continue }
                $serialHits.Add(('{0}:{1}' -f $rel, ($i + 1)))
            }
        }
    }
    foreach ($f in (@($srcFiles) + @($testFiles))) {
        $text = [IO.File]::ReadAllText($f.FullName)
        $rel = $f.FullName.Substring($root.Length + 1)
        $isTest = $rel.StartsWith('tests\', [StringComparison]::OrdinalIgnoreCase)
        if ($doSpacer -and -not $isTest) {
            foreach ($b in (Get-ByteArrayDecls $text)) {
                if (-not (Test-ByteSpacer $b)) { continue }
                $key = '{0}|{1}|{2}' -f $rel, $b.Name, $b.Size
                if ($spacerLeft[$key] -gt 0) { $spacerLeft[$key]--; continue }
                $spacerHits.Add(('{0}:{1}' -f $rel, $b.Line))
            }
        }
        if ($doOffset -and -not $isTest) {
            $blank = $script:ByteCommentRx.Replace($text, { param($m) if ($m.Value[0] -eq '/') { [regex]::Replace($m.Value, '[^\r\n]', ' ') } else { $m.Value } })
            $spans = @()
            foreach ($rx in $script:OffsetRx) {
                foreach ($m in $rx.Matches($blank)) { $spans += New-Object PSObject -Property @{ Start = $m.Index; Length = $m.Length; Text = $m.Value } }
            }
            $end = -1
            foreach ($s in @($spans | Sort-Object Start, @{ Expression = 'Length'; Descending = $true })) {
                if ($s.Start -lt $end) { continue }
                $end = $s.Start + $s.Length
                $key = '{0}|{1}' -f $rel, ([regex]::Replace($s.Text, '\s+', ' '))
                if ($offsetLeft[$key] -gt 0) { $offsetLeft[$key]--; continue }
                $offsetHits.Add(('{0}:{1}' -f $rel, $blank.Substring(0, $s.Start).Split("`n").Count))
            }
        }
        foreach ($d in $runDefs) {
            if ($isTest -and -not $d.Tests) { continue }
            if ($d.Path -and $f.FullName -notmatch $d.Path) { continue }
            $spans = @()
            foreach ($rx in $d.Rx) {
                if ($rx -is [hashtable]) {
                    if ($f.FullName -notmatch $rx.Path) { continue }
                    $rx = $rx.Rx
                }
                foreach ($m in [regex]::Matches($text, $rx)) {
                    $spans += New-Object PSObject -Property @{ Start = $m.Index; End = ($m.Index + [Math]::Max($m.Length, 1)) }
                }
            }
            $end = -1
            $seen = @{}
            $left = $sigLeft[$d.Name]
            foreach ($s in @($spans | Sort-Object Start)) {
                if ($s.Start -lt $end) { $end = [Math]::Max($end, $s.End); continue }
                $end = $s.End
                $line = $text.Substring(0, $s.Start).Split("`n").Count
                $key = '{0}:{1}' -f $rel, $line
                if ($seen[$key]) { continue }
                $seen[$key] = $true
                if ($left) {
                    $sig = '{0}|{1}' -f $rel, ([regex]::Replace($text.Split("`n")[$line - 1].Trim(), '\s+', ' '))
                    if ($left[$sig] -gt 0) { $left[$sig]--; continue }
                }
                $hits[$d.Name].Add($(if ($d.Note) { "$key $($d.Note)" } else { $key }))
            }
        }
    }
    $suiteHits = New-Object System.Collections.Generic.List[string]
    $suiteOk = '\*\s*doctest::(?:skip\(\s*\)|test_suite\(\s*"(?:' + ((@($script:T1Suites) + 'asan_canary') -join '|') + ')"\s*\))'
    $caseRx = New-Object regex '\GTEST_CASE\s*\(\s*"(?:[^"\\]|\\.)*"((?:[^()"]|"(?:[^"\\]|\\.)*"|\((?:[^()"]|"(?:[^"\\]|\\.)*"|\([^()]*\))*\))*)\)'
    if (-not (& $want 't1-suite')) { $testFiles = @() }
    foreach ($f in $testFiles) {
        $text = [IO.File]::ReadAllText($f.FullName)
        $rel = $f.FullName.Substring($root.Length + 1)
        foreach ($m in [regex]::Matches($text, '\b(?:DOCTEST_)?(?:TEST_CASE|TEST_SUITE|SCENARIO)\w*\s*\(')) {
            $c = $caseRx.Match($text, $m.Index)
            if (-not $c.Success -or $c.Groups[1].Value -notmatch $suiteOk) {
                $suiteHits.Add(('{0}:{1}' -f $rel, $text.Substring(0, $m.Index).Split("`n").Count))
            }
        }
    }
    foreach ($n in @($sigLeft.Keys)) {
        foreach ($k in @($sigLeft[$n].Keys | Sort-Object)) {
            for ($i = 0; $i -lt $sigLeft[$n][$k]; $i++) { $hits[$n].Add('unused allow ' + $k) }
        }
    }
    $rows = @($defs | ForEach-Object { New-Object PSObject -Property @{ Name = $_.Name; Ok = ($hits[$_.Name].Count -eq 0); Detail = ($hits[$_.Name] -join ', ') } })
    foreach ($k in @($spacerLeft.Keys | Sort-Object)) {
        for ($i = 0; $i -lt $spacerLeft[$k]; $i++) { $spacerHits.Add('unused allow ' + $k) }
    }
    $rows += New-Object PSObject -Property @{ Name = 'char-spacer'; Ok = ($spacerHits.Count -eq 0); Detail = ($spacerHits -join ', ') }
    foreach ($k in @($offsetLeft.Keys | Sort-Object)) {
        for ($i = 0; $i -lt $offsetLeft[$k]; $i++) { $offsetHits.Add('unused allow ' + $k) }
    }
    $rows += New-Object PSObject -Property @{ Name = 'offset-hack'; Ok = ($offsetHits.Count -eq 0); Detail = ($offsetHits -join ', ') }
    $rows += New-Object PSObject -Property @{ Name = 'allow-x64-verdict'; Ok = ($verdictHits.Count -eq 0); Detail = ($verdictHits -join ', ') }
    foreach ($k in @($serialLeft.Keys | Sort-Object)) {
        for ($i = 0; $i -lt $serialLeft[$k]; $i++) { $serialHits.Add('unused allow ' + $k) }
    }
    $rows += New-Object PSObject -Property @{ Name = 'serial-value'; Ok = ($serialHits.Count -eq 0); Detail = ($serialHits -join ', ') }
    $rows += New-Object PSObject -Property @{ Name = 't1-suite'; Ok = ($suiteHits.Count -eq 0); Detail = ($suiteHits -join ', ') }
    if (& $want 'baseline-readers') { $rows += Get-BaselineReadersCheck $root }
    if (& $want 'host-fault-capture') { $rows += Get-HostFaultCaptureCheck $root }
    if (& $want 'all-checkpoints') { $rows += Get-AllCheckpointsCheck $root }
    return @($rows | Where-Object { & $want $_.Name })
}

function Invoke-HarnessChild([string]$command) {
    $out = @(& { $ErrorActionPreference = 'Continue'; & powershell -NoProfile -ExecutionPolicy Bypass -Command $command 2>&1 | ForEach-Object { "$_" } })
    return New-Object PSObject -Property @{ Code = $LASTEXITCODE; Lines = $out }
}

function Get-HostFaultCaptureCheck([string]$root) {
    $hits = New-Object System.Collections.Generic.List[string]
    $smoke = Join-Path $root 'tests\smoke.ps1'
    $run = Join-Path $root 'tests\run.ps1'
    $tmp = Join-Path $env:TEMP ('openswbf2-hostfault-' + [guid]::NewGuid().ToString('N') + '.txt')
    try {
        $q = { param($s) "'" + $s.Replace("'", "''") + "'" }
        $cmd = ". $(& $q $smoke); function Get-HostFaultCount { param(`$Since) if (`$PSBoundParameters.ContainsKey('Since')) { 2 } else { 5 } }; `$s = Get-Date; Add-HostFaultParams $(& $q $tmp) 'start' `$s; Add-HostFaultParams $(& $q $tmp) 'end' `$s; exit 0"
        $c = Invoke-HarnessChild $cmd
        $got = $(if (Test-Path -LiteralPath $tmp) { @([IO.File]::ReadAllLines($tmp)) -join '|' } else { '' })
        if ($c.Code -ne 0) { $hits.Add("stubbed capture exited $($c.Code): $($c.Lines -join ' ')") }
        elseif ($got -cne 'whea_before=5|whea_after=5|whea_delta=2') { $hits.Add("stubbed capture wrote '$got', expected 'whea_before=5|whea_after=5|whea_delta=2'") }
    }
    finally { Remove-Item -LiteralPath $tmp -Force -ErrorAction SilentlyContinue }
    if (-not (Test-Path -LiteralPath $run)) { $hits.Add('tests\run.ps1 not found') }
    else {
        $errs = $null
        $ast = [Management.Automation.Language.Parser]::ParseFile($run, [ref]$null, [ref]$errs)
        if (@($errs).Count) { $hits.Add("run.ps1 does not parse: $(@($errs)[0].Message)") }
        $calls = @($ast.FindAll({ param($a) $a -is [Management.Automation.Language.CommandAst] }, $true))
        foreach ($stage in 'start', 'end') {
            $hit = @($calls | Where-Object { $_.GetCommandName() -ceq 'Add-HostFaultParams' -and @($_.CommandElements | Where-Object { $_ -is [Management.Automation.Language.StringConstantExpressionAst] -and $_.Value -ceq $stage }).Count })
            if (-not $hit.Count) { $hits.Add("run.ps1 has no Add-HostFaultParams $stage call") }
        }
        if (-not @($calls | Where-Object { $_.GetCommandName() -ceq 'Get-HostFaultNote' }).Count) { $hits.Add('run.ps1 has no Get-HostFaultNote call') }
    }
    return New-Object PSObject -Property @{ Name = 'host-fault-capture'; Ok = ($hits.Count -eq 0); Detail = ($hits -join ', ') }
}

function Get-AllCheckpointsCheck([string]$root) {
    $hits = New-Object System.Collections.Generic.List[string]
    $smoke = Join-Path $root 'tests\smoke.ps1'
    $missions = Join-Path $root 'tests\smoke\missions-all.txt'
    $q = { param($s) "'" + $s.Replace("'", "''") + "'" }
    $cmd = ". $(& $q $smoke); `$script:SmokeWerKey = `$true; `$script:SmokeDigestConfiguration = 'Release'; `$ml = @([IO.File]::ReadAllLines($(& $q $missions)) | Where-Object { `$_.Trim() }); `$names = @(`$ml | ForEach-Object { @(`$_.Trim() -split '\s+')[0] }); foreach (`$p in 'Win32', 'x64') { `$script:SmokePlatform = `$p; foreach (`$hog in `$false, `$true) { foreach (`$c in @(Get-SmokeCases 'All' `$ml `$false `$null `$hog 'Short' | Where-Object { `$names -ccontains `$_.Name })) { `$rx = @(`$c.Expect.LogRx) -join '|'; `$miss = @(); if (-not `$c.Args.Contains(' -digesttrace 1500')) { `$miss += 'switch' }; if (`$c.Expect.Class -ne 'Fatal' -and -not `$rx.Contains('OPENSWBF2 DIGEST TRACE turn=100 ')) { `$miss += 'turn100' }; if (`$c.Expect.Class -ne 'Fatal' -and ((Get-DigestExpected `$c.Name) -or `$rx.Contains('OPENSWBF2 SIM DIGEST turns=1500 ')) -and -not `$rx.Contains('OPENSWBF2 DIGEST TRACE turn=1500 ')) { `$miss += 'turn1500' };`$t = `$(if (`$miss.Count) { 'MISS' } else { 'OK' }); Write-Output ('CASE {0} {1}/{2} {3}' -f `$t, `$p, `$(if (`$hog) { 'hog' } else { 'plain' }), ((@(`$c.Name) + `$miss) -join ' ')) } } }; exit 0"
    $c = Invoke-HarnessChild $cmd
    $rows = @($c.Lines | Where-Object { $_ -cmatch '^CASE ' })
    if ($c.Code -ne 0) { $hits.Add("case build exited $($c.Code): $(@($c.Lines | Select-Object -Last 3) -join ' ')") }
    elseif ($rows.Count -eq 0) { $hits.Add('no All mission case was built') }
    foreach ($r in $rows) { if ($r -cmatch '^CASE MISS (.+)$') { $hits.Add($Matches[1]) } }
    return New-Object PSObject -Property @{ Name = 'all-checkpoints'; Ok = ($hits.Count -eq 0); Detail = ($hits -join ', ') }
}

function Get-BaselineReadersCheck([string]$root) {
    $hits = New-Object System.Collections.Generic.List[string]
    $ct = Join-Path $root 'tests\baseline\compare-tables.ps1'
    $dir = Join-Path $root 'tests\baseline\v143'
    $map = @()
    $params = @()
    if (-not (Test-Path -LiteralPath $ct)) { $hits.Add('tests\baseline\compare-tables.ps1 not found') }
    else {
        $errs = $null
        $ast = [Management.Automation.Language.Parser]::ParseFile($ct, [ref]$null, [ref]$errs)
        if (@($errs).Count) { $hits.Add("compare-tables.ps1 does not parse: $(@($errs)[0].Message)") }
        if ($ast.ParamBlock) { $params = @($ast.ParamBlock.Parameters | Where-Object { $_.StaticType -eq [switch] -or $_.Name.VariablePath.UserPath -ceq 'Tables' } | ForEach-Object { $_.Name.VariablePath.UserPath }) }
        $asg = $ast.Find({ param($n) $n -is [Management.Automation.Language.AssignmentStatementAst] -and $n.Left.Extent.Text -eq '$script:BaselineReaders' }, $true)
        if (-not $asg) { $hits.Add('compare-tables.ps1 has no $script:BaselineReaders map') }
        else {
            try { $map = @($asg.Right.Expression.SafeGetValue()) }
            catch { $hits.Add("the reader map is not a literal: $($_.Exception.Message)") }
        }
    }
    $files = @()
    if (Test-Path -LiteralPath $dir) { $files = @(Get-ChildItem -LiteralPath $dir -Recurse -File | ForEach-Object { $_.FullName.Substring($dir.TrimEnd('\').Length + 1) } | Sort-Object) }
    else { $hits.Add('tests\baseline\v143 not found') }
    $mapped = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    foreach ($e in $map) {
        if ($e -isnot [hashtable] -or -not $e.File -or -not $e.Reader) { $hits.Add("reader map entry without File and Reader: $e"); continue }
        if (-not $mapped.Add($e.File)) { $hits.Add("$($e.File) is in the reader map twice") }
        $r = [string]$e.Reader
        if ($r.StartsWith('t1:')) {
            $src = Join-Path $root $r.Substring(3)
            if (-not (Test-Path -LiteralPath $src) -or [IO.File]::ReadAllText($src).IndexOf("$($e.File)`"", [StringComparison]::Ordinal) -lt 0) { $hits.Add("$($e.File) reader $($r.Substring(3)) does not name it") }
        }
        elseif ($r -ceq 'verify-v143') {
            $cap = Join-Path $root 'tests\baseline\capture-vs2013.ps1'
            if (-not (Test-Path -LiteralPath $cap) -or [IO.File]::ReadAllText($cap).IndexOf("'v143'", [StringComparison]::Ordinal) -lt 0) { $hits.Add("$($e.File) reader verify-v143 is not in capture-vs2013.ps1") }
        }
        elseif ($params -cnotcontains $r) { $hits.Add("$($e.File) reader $r is not a compare-tables.ps1 branch") }
    }
    foreach ($f in $files) { if (-not $mapped.Contains($f)) { $hits.Add("$f has no reader") } }
    foreach ($k in @($mapped | Sort-Object)) { if ($files -cnotcontains $k) { $hits.Add("$k is in the reader map but not in tests\baseline\v143") } }
    return New-Object PSObject -Property @{ Name = 'baseline-readers'; Ok = ($hits.Count -eq 0); Detail = ($hits -join ', ') }
}

function Get-PeAslr([string]$exe) {
    $fs = [IO.File]::OpenRead($exe)
    try {
        $b = New-Object byte[] ([int][Math]::Min(4096, $fs.Length))
        $got = $fs.Read($b, 0, $b.Length)
    }
    finally { $fs.Dispose() }
    if ($got -lt 0x40 -or $b[0] -ne 0x4D -or $b[1] -ne 0x5A) { return 'no MZ header' }
    $e = [BitConverter]::ToInt32($b, 0x3C)
    $opt = $e + 24
    if ($e -lt 0x40 -or $opt + 2 -gt $got) { return ('PE header at 0x{0:X} is outside the first {1} bytes' -f $e, $got) }
    if ([BitConverter]::ToUInt32($b, $e) -ne 0x00004550) { return 'no PE signature' }
    $magic = [BitConverter]::ToUInt16($b, $opt)
    if ($magic -eq 0x10B) { $countAt = $opt + 92; $dirAt = $opt + 96 }
    elseif ($magic -eq 0x20B) { $countAt = $opt + 108; $dirAt = $opt + 112 }
    else { return ('optional header magic 0x{0:X} is not PE32 or PE32+' -f $magic) }
    $relocAt = $dirAt + 5 * 8
    if ($relocAt + 8 -gt $got) { return ('PE header at 0x{0:X} is outside the first {1} bytes' -f $e, $got) }
    if (([BitConverter]::ToUInt16($b, $e + 22) -band 0x0001) -ne 0) { return 'IMAGE_FILE_RELOCS_STRIPPED is set' }
    if (([BitConverter]::ToUInt16($b, $opt + 70) -band 0x0040) -eq 0) { return 'IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE is clear' }
    if ([BitConverter]::ToUInt32($b, $countAt) -lt 6) { return 'no base relocation data directory' }
    if ([BitConverter]::ToUInt32($b, $relocAt) -eq 0 -or [BitConverter]::ToUInt32($b, $relocAt + 4) -eq 0) { return 'base relocation directory is empty' }
    return $null
}
