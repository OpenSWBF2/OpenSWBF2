if ($PSScriptRoot) { if (-not $script:HarnessCommonLoaded) { . (Join-Path $PSScriptRoot 'common.ps1') }; . (Join-Path $PSScriptRoot 'smoke\cases.ps1') }
$script:SmokeMarker = 'OPENSWBF2 RUN COMPLETE'
$script:WerKeyPath = 'SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\BattlefrontII.exe'
$script:WerBasePath = 'SOFTWARE\Microsoft\Windows\Windows Error Reporting'
$script:SourceExtensions = @('.h', '.hpp', '.inl', '.c', '.cpp')
$script:T1Suites = @('unit', 'characterisation', 'regression', 'known_bug', 'corpus', 'fuzz', 'stress')
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
            $newest = Get-ChildItem -LiteralPath $werDir -Filter '*.dmp' -ErrorAction SilentlyContinue |
                Where-Object { $_.LastWriteTime -ge $start } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
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

function Read-SmokeCaseArchive([string]$caseDir, [bool]$asan) {
    $resultFile = Join-Path $caseDir 'result.txt'
    $runLog = Join-Path $caseDir 'run.log'
    $res = New-Object PSObject -Property @{ Name = (Split-Path $caseDir -Leaf); Class = $null; Code = $null; Dumps = @(); LogText = $null; Ms = $null; CaseDir = $caseDir; WorkDir = $null; Asan = $null; Stall = $null; SilentMaxS = $null; Turns = $null; Steps = $null; Problem = $null }
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

$script:SmokeWorkerFiles = @('BattlefrontII.exe', 'binkw32.dll', 'Galaxy.dll', 'GalaxyPeer.dll', 'steam_api.dll', 'unicows.dll', 'steam_appid.txt')

function New-SmokeWorkerDir($srcDir, $dir) {
    Remove-HarnessTree $dir
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    foreach ($f in @($script:SmokeWorkerFiles + (Get-AsanRuntimeName))) {
        $s = Join-Path $srcDir $f
        if (Test-Path -LiteralPath $s) { Copy-Item -LiteralPath $s -Destination $dir -Force }
    }
    & cmd /c mklink /J "$(Join-Path $dir 'data')" "$(Join-Path $srcDir 'data')" | Out-Null
}

function Clear-SmokeWorkerDir($dir, $keep = @()) {
    foreach ($e in (New-Object System.IO.DirectoryInfo $dir).GetFileSystemInfos()) {
        if ($e.Name -eq 'data' -or $e.Name -eq (Get-AsanRuntimeName) -or $script:SmokeWorkerFiles -contains $e.Name -or @($keep) -contains $e.Name) { continue }
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

function Start-SmokeBatch($items, $workDirs) {
    Initialize-OutputCapture
    $common = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'common.ps1'))
    $src = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'smoke.ps1'))
    $queue =New-Object 'System.Collections.Concurrent.ConcurrentQueue[string]'
    foreach ($d in $workDirs) { $queue.Enqueue([string]$d) }
    $pool = [runspacefactory]::CreateRunspacePool(1, @($workDirs).Count)
    $pool.Open()
    $worker = {
        param($common, $src, $queue, $item, $platform, $keep)
        . ([scriptblock]::Create($common))
        . ([scriptblock]::Create($src))
        $script:SmokePlatform = $platform
        $dir = $null
        while (-not $queue.TryDequeue([ref]$dir)) { Start-Sleep -Milliseconds 50 }
        try {
            Clear-SmokeWorkerDir $dir $keep
            Invoke-SmokeCase $item.Name (Join-Path $dir $item.ExeName) $dir $item.ArgLine $item.CaseDir $item.AsanOptions
        }
        finally { $queue.Enqueue($dir) }
    }
    $keep = [string[]]@($items | ForEach-Object { $_.ExeName } | Sort-Object -Unique)
    $handles = @()
    foreach ($item in $items) {
        $ps = [powershell]::Create()
        $ps.RunspacePool = $pool
        $ps.AddScript($worker).AddArgument($common).AddArgument($src).AddArgument($queue).AddArgument($item).AddArgument($script:SmokePlatform).AddArgument($keep) | Out-Null
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

function Test-SmokeExpect($case, $res) {
    $x = $case.Expect
    if ($res.Class -ne $x.Class) { return $false }
    if ([int]$res.Stall -ne [int]($x.ContainsKey('Stall') -and $x.Stall -eq 1)) { return $false }
    if ($x.ContainsKey('Code') -and $res.Code -ne [uint32]$x.Code) { return $false }
    $n = @($res.Dumps).Count
    if ($x.ContainsKey('MinDumps') -and $n -lt $x.MinDumps) { return $false }
    if ($x.ContainsKey('MaxDumps') -and $n -gt $x.MaxDumps) { return $false }
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


function Get-ImportChecks($imp) {
    $rows = @()
    foreach ($n in 'SetProcessDpiAwarenessContext', 'SetProcessDpiAwareness', 'SetProcessDPIAware', 'GetDpiForWindow', 'AdjustWindowRectExForDpi') {
        $rows += New-Object PSObject -Property @{ Name = "import-$n"; Ok = ($imp -cnotmatch "(?m)\s$n\s*$"); Detail = "$n statically imported (breaks Win7 load)" }
    }
    $rows += New-Object PSObject -Property @{ Name = 'import-shcore'; Ok = ($imp -notmatch '(?mi)^\s+shcore\.dll\s*$'); Detail = 'shcore.dll in import table' }
    $dlls = @([regex]::Matches([string]$imp, '(?mi)^\s+(\S+\.dll)\s*$') | ForEach-Object { $_.Groups[1].Value.ToLowerInvariant() } | Sort-Object -Unique)
    $allow = @($script:ImportModuleAllow + (Get-AsanRuntimeName))
    $extra = @($dlls | Where-Object { $allow -notcontains $_ })
    $detail = 'imports outside the allow list: ' + ($extra -join ' ')
    if ($dlls -notcontains 'kernel32.dll') { $detail = 'kernel32.dll not found in the dumpbin /imports output (' + $dlls.Count + ' DLLs parsed)' }
    $rows += New-Object PSObject -Property @{ Name = 'import-modules'; Ok = ($extra.Count -eq 0 -and $dlls -contains 'kernel32.dll'); Detail = $detail }
    return $rows
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
        $use = $r.SelectSingleNode('m:UseLibraryDependencyInputs', $ns)
        if (-not $use -or $use.InnerText.Trim() -ne 'true') { continue }
        $path = [IO.Path]::GetFullPath((Join-Path $root $r.GetAttribute('Include')))
        $name = [IO.Path]::GetFileNameWithoutExtension($path)
        $int = Join-Path $objDir "lib\$name"
        $list += New-Object PSObject -Property @{ Name = $name; Vcxproj = $path; IntDir = $int; TlogDir = (Join-Path $int "$name.tlog") }
    }
    return $list
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
    foreach ($f in $files) {
        $isMi = $false
        foreach ($l in (Get-Content -LiteralPath $f.FullName)) {
            if ($l -and $l.StartsWith('^')) {
                $isMi = @($l.Substring(1).Split('|') | Where-Object { $_.EndsWith('\SRC\MIMALLOC\SRC\STATIC.C', [StringComparison]::OrdinalIgnoreCase) }).Count -gt 0
            }
            elseif ($l) {
                $total++
                if ($isMi) { $miLines.Add($l) }
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
    }
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


function Get-FlagChecks($game, $tests, [string]$gameConfig, [string]$testsConfig) {
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
$script:OffsetOperand = '(?:(?:&\s*)?(?:[\w.]|->|::|\[[^\[\];]*\])+|\((?:[^()]|\([^()]*\))*\))'
$script:OffsetByteCast = '(?:\(\s*' + $script:OffsetByteType + '\)\s*' + $script:OffsetOperand + '|\breinterpret_cast\s*<\s*' + $script:OffsetByteType + '>\s*\((?:[^()]|\([^()]*\))*\))'
$script:OffsetTypeCast = '(?:\(\s*(?:(?:const|volatile|struct|class|unsigned|signed)\s+)*[A-Za-z_][\w:]*(?:\s+(?:int|char|short|long|__int\d+))?(?:\s+(?:const|volatile))*\s*[*&][\s*&]*' + $script:OffsetQual + '\)|\b(?:reinterpret|static|const)_cast\s*<[^<>;()]*[*&]\s*' + $script:OffsetQual + '>)'
$script:OffsetOp = '\s*[+-](?![+=>-]|\s*(?:\(\s*\w+\s*\)\s*)?offsetof\b|\s*(?:\w+\s*(?:->|\.)\s*)+e_lfanew\b)'
$script:OffsetTail = $script:OffsetOp + '[^;()]*(?:\((?:[^()]|\([^()]*\))*\)[^;()]*)*\)'
$script:OffsetRx = @(
    (New-Object regex ($script:OffsetTypeCast + '\s*\(\s*' + $script:OffsetByteCast + $script:OffsetTail)),
    (New-Object regex ($script:OffsetTypeCast + '\s*\(\s*(?!\d)(?:[\w.]|->|::|\[[^\[\];]*\])+' + $script:OffsetTail)),
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
        @{ Name = 'd3dx-decl'; Path = $null; Rx = @(
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
    $spacerLeft = @{}
    foreach ($a in @($spacerAllow)) { if ($a) { $spacerLeft[$a] = 1 + [int]$spacerLeft[$a] } }
    $spacerHits = New-Object System.Collections.Generic.List[string]
    $offsetLeft = New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)
    foreach ($a in @($offsetAllow)) { if ($a) { $offsetLeft[$a] = 1 + [int]$offsetLeft[$a] } }
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
                $hits[$d.Name].Add($key)
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
    foreach ($k in @($serialLeft.Keys | Sort-Object)) {
        for ($i = 0; $i -lt $serialLeft[$k]; $i++) { $serialHits.Add('unused allow ' + $k) }
    }
    $rows += New-Object PSObject -Property @{ Name = 'serial-value'; Ok = ($serialHits.Count -eq 0); Detail = ($serialHits -join ', ') }
    $rows += New-Object PSObject -Property @{ Name = 't1-suite'; Ok = ($suiteHits.Count -eq 0); Detail = ($suiteHits -join ', ') }
    if (& $want 'baseline-readers') { $rows += Get-BaselineReadersCheck $root }
    return @($rows | Where-Object { & $want $_.Name })
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
    if ($e -lt 0x40 -or $opt + 144 -gt $got) { return ('PE header at 0x{0:X} is outside the first {1} bytes' -f $e, $got) }
    if ([BitConverter]::ToUInt32($b, $e) -ne 0x00004550) { return 'no PE signature' }
    $magic = [BitConverter]::ToUInt16($b, $opt)
    if ($magic -ne 0x10B) { return ('optional header magic 0x{0:X} is not PE32' -f $magic) }
    if (([BitConverter]::ToUInt16($b, $e + 22) -band 0x0001) -ne 0) { return 'IMAGE_FILE_RELOCS_STRIPPED is set' }
    if (([BitConverter]::ToUInt16($b, $opt + 70) -band 0x0040) -eq 0) { return 'IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE is clear' }
    if ([BitConverter]::ToUInt32($b, $opt + 92) -lt 6) { return 'no base relocation data directory' }
    if ([BitConverter]::ToUInt32($b, $opt + 136) -eq 0 -or [BitConverter]::ToUInt32($b, $opt + 140) -eq 0) { return 'base relocation directory is empty' }
    return $null
}
