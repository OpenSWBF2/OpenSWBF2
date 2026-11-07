if ($PSScriptRoot) { . (Join-Path $PSScriptRoot 'vstools.ps1') }
$script:SmokeStandardMission = 'geo1c_con'
$script:SmokeMarker = 'OPENSWBF2 RUN COMPLETE'
$script:WerKeyPath = 'SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\BattlefrontII.exe'
$script:WerBasePath = 'SOFTWARE\Microsoft\Windows\Windows Error Reporting'
$script:SmokeAsanTimeoutScale = 15
$script:SourceExtensions = @('.h', '.hpp', '.inl', '.c', '.cpp')
$script:T1Suites = @('unit', 'characterisation', 'regression', 'known_bug', 'corpus', 'fuzz', 'stress')
$script:SmokeAsanExcluded = @('crash-nullderef', 'crash-divzero', 'crash-stackoverflow', 'crash-debugbreak', 'crash-abort', 'crash-invalidparam', 'crash-fastfail', 'crash-check', 'crash-hang', 'crash-arenaoom', 'crash-debugheapoom', 'crash-luapanic', 'crash-resetfatal', 'fatal-init')
$script:SmokeAsanCoverage = 'ASan covers stack, globals, CRT heap and the game heaps and pools'

function Get-AsanOptions([string]$caseDir) {
    if ($caseDir.Contains("'")) { throw "ASan log_path cannot be quoted for a path that contains an apostrophe: $caseDir" }
    return "exitcode=66:log_path='$caseDir\asan'"
}

function Get-AsanReport([string]$caseDir, $code) {
    $files = @(Get-ChildItem -LiteralPath $caseDir -Filter 'asan.*' -File -ErrorAction SilentlyContinue)
    if ($files.Count -gt 0) {
        $text = 'ASan report:'
        foreach ($f in $files) { $text += "`r`n" + ((Get-Content -LiteralPath $f.FullName -TotalCount 60) -join "`r`n") }
        return $text
    }
    if ($code -eq 66) { return 'ASan exit 66 without an asan.* report: the ASan runtime failed before main (check ASAN_OPTIONS)' }
    return $null
}

function Get-AsanCanaryState($code, $report, [string]$regex) {
    if ($code -ne 0 -and $null -ne $report -and $report -match $regex) { return 'REPORTED' }
    if ($code -eq 0 -and $null -eq $report) { return 'CLEAN' }
    return 'OTHER'
}

function Find-GameDataRoot {
    $candidate = $null
    if ($env:SWBF2_GAMEDATA) {
        $candidate = $env:SWBF2_GAMEDATA
    }
    else {
        foreach ($view in 'Registry64', 'Registry32') {
            $base = $null
            $sub = $null
            try {
                $base = [Microsoft.Win32.RegistryKey]::OpenBaseKey('LocalMachine', $view)
                $sub = $base.OpenSubKey('SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 6060')
                if ($sub) {
                    $loc = $sub.GetValue('InstallLocation')
                    if ($loc) { $candidate = "$loc\GameData"; break }
                }
            }
            catch {}
            finally {
                if ($sub) { $sub.Close() }
                if ($base) { $base.Close() }
            }
        }
    }
    if ($candidate -and (Test-Path -LiteralPath (Join-Path $candidate 'data\_lvl_pc') -PathType Container)) { return $candidate }
    return $null
}

function Get-SmokePrecheck($gameDataRoot, $exeDir, $asan = $false) {
    if (-not $gameDataRoot) { return 'no retail GameData found (set SWBF2_GAMEDATA or install Steam App 6060)' }
    if (-not (Test-Path -LiteralPath (Join-Path $exeDir 'BattlefrontII.exe'))) { return "BattlefrontII.exe not found in $exeDir" }
    $dlls = @('binkw32.dll', 'Galaxy.dll', 'GalaxyPeer.dll', 'steam_api.dll')
    if ($asan) { $dlls += 'clang_rt.asan_dynamic-i386.dll' }
    foreach ($dll in $dlls) {
        if (-not (Test-Path -LiteralPath (Join-Path $exeDir $dll))) { return "$dll missing beside the exe in $exeDir" }
    }
    $data = Get-Item -LiteralPath (Join-Path $exeDir 'data') -Force -ErrorAction SilentlyContinue
    if (-not $data -or ($data.Attributes -band [IO.FileAttributes]::ReparsePoint) -or -not (Test-Path -LiteralPath (Join-Path $exeDir 'data\_lvl_pc\shell.lvl'))) { return "staged data missing in $exeDir (build without SWBF2_NOSTAGE)" }
    return $null
}

function Get-SmokeClass($skipped, $timedOut, $code, $logText, $dumps) {
    if ($skipped) { return 'Skipped' }
    if ($timedOut) { return 'TimeOut' }
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
        'TimeOut' { return 'TIMEOUT' }
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

function Invoke-SmokeCase($name, $exe, $workDir, $argLine, $timeoutSec, $caseDir, $asanOptions = $null) {
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
    $start = Get-Date
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $p = [System.Diagnostics.Process]::Start($psi)
    try { $p.PriorityClass = [System.Diagnostics.ProcessPriorityClass]::BelowNormal } catch {}
    $o = $p.StandardOutput.ReadToEndAsync()
    $e = $p.StandardError.ReadToEndAsync()
    $timedOut = -not $p.WaitForExit([int]([int64]$timeoutSec * 1000))
    if ($timedOut) {
        & taskkill /PID $p.Id /T /F 2>&1 | Out-Null
        $p.WaitForExit()
    }
    $sw.Stop()
    $o.Wait(5000) | Out-Null
    $e.Wait(5000) | Out-Null
    $code = [BitConverter]::ToUInt32([BitConverter]::GetBytes([int]$p.ExitCode), 0)
    $caseIni = Join-Path $caseDir 'display.ini'
    if (Test-Path -LiteralPath $caseIni) { [IO.File]::SetAttributes($caseIni, [IO.FileAttributes]::Normal) }

    $logText = ''
    $runLog = Join-Path $caseDir 'run.log'
    if (Test-Path -LiteralPath $runLog) { $logText = [IO.File]::ReadAllText($runLog) }
    if ($o.IsCompleted) { $logText += "`r`n" + $o.Result }
    if ($e.IsCompleted) { $logText += "`r`n" + $e.Result }
    if (Test-Path -LiteralPath $scoreLog) { Copy-Item -LiteralPath $scoreLog -Destination (Join-Path $caseDir 'finalscore.log') -Force }

    $dumps = @(Get-ChildItem -LiteralPath $caseDir -Filter '*.dmp' -ErrorAction SilentlyContinue)
    $class = Get-SmokeClass $false $timedOut $code $logText $dumps
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
    $result = ("class={0}`r`nexit=0x{1:X8}`r`nelapsed_ms={2}`r`ndumps={3}`r`nasan={4}`r`n" -f $class, $code, $ms, $dumps.Count, [int][bool]$asan)
    Set-Content -Path (Join-Path $caseDir 'result.txt') -Value $result -Encoding ASCII
    return New-Object PSObject -Property @{ Name = $name; Class = $class; Code = $code; Dumps = $dumps; LogText = $logText; Ms = $ms; CaseDir = $caseDir; WorkDir = $workDir; Asan = $asan }
}

function Get-SmokeWorkerRoot($root, $name) {
    return Join-Path $root "artifacts\smoke-workers\$name"
}

function New-SmokeWorkerDir($srcDir, $dir) {
    Remove-SmokeWorkerDir $dir
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    foreach ($f in 'BattlefrontII.exe', 'binkw32.dll', 'Galaxy.dll', 'GalaxyPeer.dll', 'steam_api.dll', 'unicows.dll', 'steam_appid.txt', 'clang_rt.asan_dynamic-i386.dll') {
        $s = Join-Path $srcDir $f
        if (Test-Path -LiteralPath $s) { Copy-Item -LiteralPath $s -Destination $dir -Force }
    }
    & cmd /c mklink /J "$(Join-Path $dir 'data')" "$(Join-Path $srcDir 'data')" | Out-Null
}

function Remove-SmokeWorkerDir($dir) {
    if (-not (Test-Path -LiteralPath $dir)) { return }
    $link = Join-Path $dir 'data'
    if (Test-Path -LiteralPath $link) { & cmd /c rmdir "$link" | Out-Null }
    if (Test-Path -LiteralPath $link) { throw "worker data junction not removed: $link" }
    Remove-Item -LiteralPath $dir -Recurse -Force
}

function Get-ReparsePoints([string]$dir) {
    $found = New-Object System.Collections.Generic.List[string]
    $top = New-Object System.IO.DirectoryInfo $dir
    if (-not $top.Exists) { return }
    if (($top.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { return $top.FullName }
    $pending = New-Object System.Collections.Generic.Stack[System.IO.DirectoryInfo]
    $pending.Push($top)
    while ($pending.Count -gt 0) {
        $cur = $pending.Pop()
        foreach ($e in $cur.EnumerateFileSystemInfos()) {
            if (($e.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { $found.Add($e.FullName) }
            elseif (($e.Attributes -band [IO.FileAttributes]::Directory) -ne 0) { $pending.Push((New-Object System.IO.DirectoryInfo $e.FullName)) }
        }
    }
    return $found.ToArray()
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
    $src = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'smoke.ps1'))
    $queue = New-Object 'System.Collections.Concurrent.ConcurrentQueue[string]'
    foreach ($d in $workDirs) { $queue.Enqueue([string]$d) }
    $pool = [runspacefactory]::CreateRunspacePool(1, @($workDirs).Count)
    $pool.Open()
    $worker = {
        param($src, $queue, $item)
        . ([scriptblock]::Create($src))
        $dir = $null
        while (-not $queue.TryDequeue([ref]$dir)) { Start-Sleep -Milliseconds 50 }
        try { Invoke-SmokeCase $item.Name (Join-Path $dir $item.ExeName) $dir $item.ArgLine $item.Timeout $item.CaseDir $item.AsanOptions }
        finally { $queue.Enqueue($dir) }
    }
    $handles = @()
    foreach ($item in $items) {
        $ps = [powershell]::Create()
        $ps.RunspacePool = $pool
        $ps.AddScript($worker).AddArgument($src).AddArgument($queue).AddArgument($item) | Out-Null
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

function Test-SmokeExpect($case, $res) {
    $x = $case.Expect
    if ($res.Class -ne $x.Class) { return $false }
    if ($x.ContainsKey('Code') -and $res.Code -ne [uint32]$x.Code) { return $false }
    $n = @($res.Dumps).Count
    if ($x.ContainsKey('MinDumps') -and $n -lt $x.MinDumps) { return $false }
    if ($x.ContainsKey('MaxDumps') -and $n -gt $x.MaxDumps) { return $false }
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
    if ($x.ContainsKey('NoIni') -and (Test-Path -LiteralPath (Join-Path $res.CaseDir 'display.ini'))) { return $false }
    if ($x.ContainsKey('IniRx')) {
        $ini = Join-Path $res.CaseDir 'display.ini'
        if (-not (Test-Path -LiteralPath $ini)) { return $false }
        $iniText = [IO.File]::ReadAllText($ini)
        foreach ($rx in @($x.IniRx)) { if ($iniText -cnotmatch $rx) { return $false } }
    }
    if (-not $script:SmokeDigestConfiguration -or $script:SmokeDigestConfiguration -in 'Release', 'Test') {
        $want = Get-DigestExpected $case.Name
        if ($want -and $x.ContainsKey('SerialDigest')) {
            $got = Get-DigestKey $res.CaseDir
            if ($got -cnotmatch '^OPENSWBF2 SIM DIGEST turns=') {
                Write-Host "    serial digest line missing"
                return $false
            }
        }
        elseif ($want) {
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

$script:SmokeAddrSpaceRx = 'OPENSWBF2 ADDRSPACE arena=0x[89A-F][0-9A-F]{7} arena_end=0x[0-9A-F]{8} laa=1 steam=1 steam_api=0x(?!0{8})[0-9A-F]{8} Galaxy=0x(?!0{8})[0-9A-F]{8} GalaxyPeer=0x[0-9A-F]{8} binkw32=0x(?!0{8})[0-9A-F]{8}'

function Get-SmokeCases($which, $missions, $movies, $dedicatedSpec, $hog = $false) {
    $standard = $script:SmokeStandardMission
    $render = '-norender'
    if ($movies) { $render = '' }
    $tail = (("$render -nosound -fixedrate 30 -nowait -logfinalscore -DecompCONMinutes 5 -seed 1 -exitongameover -nofocuspause -log ""{CASEDIR}\run.log""") -replace '^\s+', '')
    $htail = $tail
    $pass = @{ Class = 'Pass' }
    $hrx = @()
    if ($hog) {
        $htail = "$tail -hogaddrspace"
        $hrx = @($script:SmokeAddrSpaceRx)
        $pass = @{ Class = 'Pass'; LogRx = $hrx }
    }
    $cases = @()

    $allCase = {
        param($m, $class, $sec)
        $x = $pass
        $to = 0
        if ($sec) {
            if ($sec -notmatch '^\d+$') { throw "bad timeout '$sec' for $m in missions file" }
            $to = [int]$sec
        }
        if ($class -eq 'TimeOut') { $x = @{ Class = 'TimeOut'; LogRx = (@('OPENSWBF2 MISSION RUNNING') + $hrx) } }
        elseif ($class -eq 'Fatal') {
            $x = @{ Class = 'Fatal'; Code = 3; MinDumps = 1; MaxDumps = 1; LogRx = (@('FATAL: exception 0xC0000005 at 0x[0-9A-F]+ \(d3dx9_43\.dll\+0x1BA553\)') + $hrx) }
        }
        elseif ($class -ne 'Pass') { throw "unknown expected class '$class' for $m in missions file" }
        New-Object PSObject -Property @{ Name = $m; Args = "$m $htail -logcmds ""LogMemory Verbose"""; Expect = $x; Twice = 0; Timeout = $to }
    }

    if ($which -eq 'Standard') {
        $memRx = @('OPENSWBF2 MEMREPORT source=exit', 'OPENSWBF2 MEMREPORT heap=Runtime cap=\d+ peak=\d+ used=\d+ allocs=\d+ failed=\d+', 'OPENSWBF2 MEMREPORT heap=Lua ', 'OPENSWBF2 MEMREPORT heap=Application ', 'OPENSWBF2 MEMREPORT heap=Artist ', 'OPENSWBF2 MEMREPORT heap=Gamespy ', 'OPENSWBF2 MEMREPORT heap=Debug ', 'OPENSWBF2 MEMREPORT pool=\S+ count=\d+ size=\d+ peak=\d+ grown=\d+', 'OPENSWBF2 MEMREPORT pool=Lua\d+ ', 'OPENSWBF2 MEMREPORT pool=Weapon ', 'OPENSWBF2 CLOTH free=-?\d+ enabled=[01] low=\d+ high=\d+')
        if ($script:SmokeDigestConfiguration -in 'Test', 'Asan') { $memRx += 'OPENSWBF2 MEMREPORT offthread count=\d+ sources=\d+ dropped=\d+' }
        $cases += New-Object PSObject -Property @{ Name = $standard; Args = "$standard $htail"; Expect = @{ Class = 'Pass'; LogRx = ($memRx + $hrx) }; Twice = 0; Timeout = 0 }
    }
    if ($which -eq 'Standard') {
        $cases += New-Object PSObject -Property @{ Name = 'display-switches'; Args = "$standard $htail -borderless -adapter 0 -refresh 60 -vsync half -fps 120 -nowaitquery -nodpi"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 PACING fps=120 vsync=on interval=0x00000000 waitquery=0 timer=none', 'pacing: fps cap 120 is above 80', 'vsync: requested interval not supported here, using on', 'OPENSWBF2 DPI awareness=off') + $hrx) }; Twice = 0; Timeout = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'pacing-default'; Args = "$standard $htail"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 PACING fps=80 vsync=off interval=0x80000000 waitquery=0 timer=none', 'OPENSWBF2 DPI awareness=(pmv2|pm|system|none) dpi=\d+', 'OPENSWBF2 GAMMA path=ramp reason=norender', '(?s)\A(?!.*OPENSWBF2 GAMMA FAKE)', '(?s)\A(?!.*OPENSWBF2 GAMMA path=pass)', 'OPENSWBF2 CURSOR mode=hw image=skipped') + $hrx) }; Twice = 0; Timeout = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'cursor-sw'; Args = "$standard $htail -swcursor"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 CURSOR mode=sw image=skipped', '(?s)\A(?!.*OPENSWBF2 CURSOR mode=hw)') + $hrx) }; Twice = 0; Timeout = 0 }
        $swIni = Join-Path $PSScriptRoot 'smoke\display-swcursor.ini'
        $cases += New-Object PSObject -Property @{ Name = 'cursor-sw-ini'; Args = "$standard $htail -displayini `"$swIni`""; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 CURSOR mode=sw image=skipped', '(?s)\A(?!.*OPENSWBF2 CURSOR mode=hw)') + $hrx) }; Twice = 0; Timeout = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'window-borderless-headless'; Args = "$standard $htail -borderless -adapter 5"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 DPI awareness=(pmv2|pm|system|none) dpi=\d+') + $hrx) }; Twice = 0; Timeout = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'fps-0'; Args = "$standard $htail -fps 0"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 PACING fps=0 ', 'pacing: fps cap is uncapped') + $hrx) }; Twice = 0; Timeout = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'fps-30'; Args = "$standard $htail -fps 30"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 PACING fps=30 ') + $hrx) }; Twice = 0; Timeout = 0 }
        $rtTail = $htail -replace ' -fixedrate 30', ''
        $cases += New-Object PSObject -Property @{ Name = 'pacing-realtime'; Args = "nab2g_c $rtTail -fps 30"; Expect = @{ Class = 'TimeOut'; LogRx = (@('OPENSWBF2 MISSION RUNNING', 'OPENSWBF2 PACING fps=30 ') + $hrx) }; Timeout = 30; Twice = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'pacing-realtime-uncapped'; Args = "nab2g_c $rtTail -fps 0"; Expect = @{ Class = 'TimeOut'; LogRx = (@('OPENSWBF2 MISSION RUNNING', 'OPENSWBF2 PACING fps=0 ') + $hrx) }; Timeout = 30; Twice = 0 }
        $badIni = Join-Path $PSScriptRoot 'smoke\display-bad.ini'
        $cases += New-Object PSObject -Property @{ Name = 'display-badini'; Args = "$standard $htail -adapter abc -refresh 99999 -displayini `"$badIni`""; Expect = $pass; Twice = 0; Timeout = 0 }
        $waitIni = Join-Path $PSScriptRoot 'smoke\display-waitquery.ini'
        $cases += New-Object PSObject -Property @{ Name = 'pacing-waitquery-ini'; Args = "$standard $htail -displayini `"$waitIni`""; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 PACING fps=80 vsync=off interval=0x80000000 waitquery=1 timer=none') + $hrx) }; Twice = 0; Timeout = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'digest-dag1c_ctf'; Args = "dag1c_ctf $htail"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 MISSION RUNNING', 'OPENSWBF2 SIM DIGEST turns=') + $hrx) }; Twice = 0; Timeout = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'digest-kas2c_uber'; Args = "kas2c_uber $htail"; Expect = $pass; Twice = 0; Timeout = 0 }
        if ($script:SmokeDigestConfiguration -in 'Test', 'Asan') {
            $scanRx = @('OPENSWBF2 HANDLESCAN state=shell live=0 dead=0', 'OPENSWBF2 HANDLESCAN state=mission live=0 dead=0', '(?s)\A(?!.*OPENSWBF2 HANDLESCAN (live |dead |state=\w+ (?!live=0 dead=0\b)))')
            $cases += New-Object PSObject -Property @{ Name = 'shell-first'; Args = "$standard $htail -shellfirst"; Expect = @{ Class = 'Pass'; SameDigests = 1; LogRx = (@('OPENSWBF2 SHELLFIRST to_mission mission=' + $standard) + $scanRx + $hrx) }; Twice = 0; Timeout = 0 }
            $cases += New-Object PSObject -Property @{ Name = 'repeat-digest'; Args = "$standard $htail -repeatmission"; Expect = @{ Class = 'Pass'; SameDigests = 2; LogRx = (@('OPENSWBF2 REPEAT to_shell', 'OPENSWBF2 REPEAT to_mission') + $scanRx + $hrx) }; Twice = 0; Timeout = 300 }
        }
    }
    if ($which -eq 'All') {
        foreach ($line in $missions) {
            $tok = @($line.Trim() -split '\s+')
            if ($tok[0]) {
                $class = 'Pass'
                if ($tok.Count -gt 1) { $class = $tok[1] }
                $sec = $null
                if ($tok.Count -gt 2) { $sec = $tok[2] }
                $cases += & $allCase $tok[0] $class $sec
            }
        }
    }
    if ($which -eq 'Dedicated' -or $which -eq 'All') {
        $dm = $standard
        if ($dedicatedSpec) { $dm = $dedicatedSpec }
        $cases += New-Object PSObject -Property @{
            Name = 'dedicated'
            Args = "/autonet dedicated -lan -nopregame -playercount 0 $dm 100 100 $htail"
            Expect = $pass
            Twice = 0; Timeout = 0
        }
    }
    if ($which -eq 'Crash' -or $which -eq 'All') {
        foreach ($kind in 'nullderef', 'divzero', 'stackoverflow', 'debugbreak', 'abort', 'invalidparam') {
            $cases += New-Object PSObject -Property @{
                Name = "crash-$kind"
                Args = "$standard $tail -crashtest $kind"
                Expect = @{ Class = 'Fatal'; Code = 3; MinDumps = 1; MaxDumps = 1; LogRx = 'FATAL:.*\.dmp' }
                Twice = 0; Timeout = 0
            }
        }
        $ff = @{ Class = 'GsFailure'; Code = 3221226505 }
        if (Test-WerKey) { $ff.MinDumps = 1 }
        $cases += New-Object PSObject -Property @{ Name = 'crash-fastfail'; Args = "$standard $tail -crashtest fastfail"; Expect = $ff; Twice = 0; Timeout = 0 }
        $cases += New-Object PSObject -Property @{
            Name = 'fatal-init'
            Args = "no_such_mission $tail"
            Expect = @{ Class = 'Fatal'; Code = 1; MaxDumps = 0; LogRx = 'FATAL: initialization failed' }
            Twice = 0; Timeout = 0
        }
        foreach ($kind in 'arenaoom', 'debugheapoom') {
            $cases += New-Object PSObject -Property @{
                Name = "crash-$kind"
                Args = "$standard $tail -crashtest $kind"
                Expect = @{ Class = 'Fatal'; Code = 1; MaxDumps = 0; LogRx = ('FATAL: out of memory allocating ' + $(if ($kind -eq 'arenaoom') { 'arena' } else { 'debug heap' }) + ' \(\d+ bytes\) load \d+% phys \d+ MB virt \d+ MB') }
                Twice = 0; Timeout = 0
            }
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-heapfail'
            Args = "$standard $tail -crashtest heapfail"
            Expect = @{ Class = 'Pass'; LogRx = 'alloc failure heap \S+ \(\d+\) size \d+ free \d+ load \d+% phys \d+ MB virt \d+ MB used \d+ cap \d+ peak \d+ largestfree \d+' }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-poolcount'
            Args = "$standard $tail -crashtest poolcount"
            Expect = @{ Class = 'Pass'; LogRx = @('LogMemory Warning RedEngineFL\\Memory\\RedScriptLimits\.h:\d+: SetMemoryPoolSize\(crashtestpool\): script value 2147483647 clamped to \d+', 'LogMemory Warning RedEngineFL\\Memory\\RedMemoryPool\.cpp:\d+: Memory pool "crashtesthuge" refused count 1073741825 size 16') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'pool-full'
            Args = "$standard $tail -crashtest poolfull"
            Expect = @{ Class = 'Pass'; LogRx = @('LogMemory Warning RedEngineFL\\Memory\\RedMemoryPool\.cpp:\d+: Memory pool "crashtestpool" is full; raise count to at least 3', '(?s)\A(?!.*crashtestpool" is full.*crashtestpool" is full)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'pool-full-strict'
            Args = "$standard $tail -crashtest poolfull -poolerrors"
            Expect = @{ Class = 'Pass'; LogRx = @('LogMemory Error RedEngineFL\\Memory\\RedMemoryPool\.cpp:\d+: Memory pool "crashtestpool" is full; raise count to at least 3'); ErrorRx = @('^\[t=\d+\.\d{3}\] \[f=\d+\] LogMemory Error RedEngineFL\\Memory\\RedMemoryPool\.cpp:\d+: Memory pool "[^"]*" is full; raise count to at least \d+$') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-luaoom'
            Args = "$standard $tail -crashtest luaoom"
            Expect = @{ Class = 'Pass'; LogRx = 'Lua out of memory' }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-luapanic'
            Args = "$standard $tail -crashtest luapanic"
            Expect = @{ Class = 'Fatal'; Code = 4; MaxDumps = 0; LogRx = 'FATAL: lua panic' }
            Twice = 0; Timeout = 0
        }
    }
    if ($which -eq 'Crash' -or $which -eq 'All') {
        $cases += New-Object PSObject -Property @{
            Name = 'crash-hang'
            Args = "$standard $tail -crashtest hang -hangtimeout 5"
            Expect = @{ Class = 'Fatal'; Code = 5; MinDumps = 1; MaxDumps = 1; LogRx = 'FATAL: hang' }
            Twice = 0; Timeout = 30
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-check'
            Args = "$standard $tail -crashtest check"
            Expect = @{ Class = 'Fatal'; Code = 3; MinDumps = 1; MaxDumps = 1; LogRx = @('FATAL: check failed', 'ensure summary: 1 failures at 1 sites') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-resetinitlost'
            Args = "$standard $tail -crashtest resetinitlost"
            Expect = @{ Class = 'Pass'; LogRx = 'OPENSWBF2 RESET recovered after 4 attempts' }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-resetrecover'
            Args = "$standard $tail -crashtest resetrecover"
            Expect = @{ Class = 'Pass'; LogRx = 'OPENSWBF2 RESET recovered after 4 attempts' }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-resetflap'
            Args = "$standard $tail -crashtest resetflap"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 RESET recovered after 25 attempts', '(?s)\A(?!.*OPENSWBF2 RESET failed.*OPENSWBF2 RESET failed)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-resetlaterinit'
            Args = "$standard $tail -crashtest resetlaterinit"
            Expect = @{ Class = 'InitFailure'; Code = 0; MaxDumps = 0; LogRx = '(?s)OPENSWBF2 MISSION RUNNING.*OPENSWBF2 CLOSE deferred until loading finishes.*OPENSWBF2 RESET recovered after 2 attempts' }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-resetfatal'
            Args = "$standard $tail -crashtest resetfatal"
            Expect = @{ Class = 'Fatal'; Code = 1; MinDumps = 1; MaxDumps = 1; LogRx = '(?s)\A(?:(?!FATAL:).)*OPENSWBF2 RESET reverting asked=\d+x\d+ windowed=\d used=\d+x\d+ windowed=\d(?:(?!FATAL:).)*FATAL: device reset failed hr=0x8876086C(?:(?!FATAL:).)*\z' }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-applymode'
            Args = "$standard $tail -crashtest applymode"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY applied \d+x\d+ mode=', 'OPENSWBF2 APPLY TARGETS alive=\d+ fresh=(?:[2-9]|[1-9]\d+) stale=0 buffers=3/3 moved=1 ', '(?s)OPENSWBF2 DISPLAY applied (\d+)x(\d+) .*OPENSWBF2 DISPLAY relayout generation=\d+ screen=\1x\2 view=\1x\2 hud=\1x\2 aspect=(\d+\.\d{4}) camera=\3\b', '(?s)\A(?!.*FATAL:)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-applymodesmall'
            Args = "$standard $tail -crashtest applymodesmall"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY applied 320x240 mode=', 'OPENSWBF2 APPLY TARGETS alive=\d+ fresh=(?:[3-9]|[1-9]\d+) stale=0 buffers=3/3 moved=3 ', '(?s)OPENSWBF2 DISPLAY applied (\d+)x(\d+) .*OPENSWBF2 DISPLAY relayout generation=\d+ screen=\1x\2 view=\1x\2 hud=\1x\2 ', '(?s)\A(?!.*FATAL:)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-applyfail'
            Args = "$standard $tail -crashtest applyfail"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY reverted asked=(\d+)x(\d+) mode=\d+ used=(?!\1x\2 )', '(?s)OPENSWBF2 DISPLAY reverted .*OPENSWBF2 RESET recovered after \d+ attempts', '(?s)OPENSWBF2 DISPLAY reverted .*OPENSWBF2 DISPLAY profile restored ', '(?s)\A(?!.*FATAL:)', '(?s)\A(?!.*OPENSWBF2 DISPLAY applied)', '(?s)\A(?!.*OPENSWBF2 DISPLAY relayout)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-applypresentfail'
            Args = "$standard $tail -crashtest applypresentfail"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY reverted asked=(\d+)x(\d+) mode=\d+ used=(?!\1x\2 )', '(?s)\A(?!.*FATAL:)', '(?s)\A(?!.*OPENSWBF2 DISPLAY applied)', '(?s)OPENSWBF2 DISPLAY reverted asked=\d+x\d+ mode=\d+ used=(\d+)x(\d+) .*OPENSWBF2 DISPLAY relayout generation=\d+ screen=\1x\2 view=\1x\2 hud=\1x\2 (?!.*OPENSWBF2 DISPLAY relayout)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-applytargetfail'
            Args = "$standard $tail -crashtest applytargetfail"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY reverted asked=(\d+)x(\d+) mode=\d+ used=(?!\1x\2 )\d+x\d+ mode=\d+ hr=0x8876017C', '(?s)\A(?!.*FATAL:)', '(?s)\A(?!.*OPENSWBF2 DISPLAY applied)', '(?s)OPENSWBF2 DISPLAY reverted asked=\d+x\d+ mode=\d+ used=(\d+)x(\d+) .*OPENSWBF2 DISPLAY relayout generation=\d+ screen=\1x\2 view=\1x\2 hud=\1x\2 (?!.*OPENSWBF2 DISPLAY relayout)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-gammapass'
            Args = "$standard $tail -crashtest gammapass"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 GAMMA path=pass', '(?s)OPENSWBF2 GAMMA FAKE target=(\d+)x(\d+) alive=1 bb=\1x\2 draws=[1-9]\d* ramp_nonid=0.*OPENSWBF2 DISPLAY applied \1x\2 ', '(?s)OPENSWBF2 RESET recovered after \d+ attempts.*OPENSWBF2 GAMMA FAKE target=(\d+)x(\d+) alive=1 bb=\1x\2 draws=[1-9]\d* ramp_nonid=0', '(?s)\A(?!.*ramp_nonid=[1-9])', '(?s)\A(?!.*OPENSWBF2 GAMMA fallback)', '(?s)\A(?!.*FATAL:)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-gammafallback'
            Args = "$standard $tail -crashtest gammafallback"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 GAMMA path=ramp reason=createfail hr=0x8876017C', 'OPENSWBF2 GAMMA FAKE target=\d+x\d+ alive=0 bb=\d+x\d+ draws=0 ramp_nonid=[1-9]', '(?s)\A(?!.*OPENSWBF2 GAMMA path=pass)', '(?s)\A(?!.*draws=[1-9])', '(?s)\A(?!.*FATAL:)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-gammalost'
            Args = "$standard $tail -crashtest gammalost"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 GAMMA path=pass', 'OPENSWBF2 GAMMA FAKE target=(\d+)x(\d+) alive=1 bb=\1x\2 draws=[1-9]\d* ramp_nonid=0', '(?s)\A(?!.*OPENSWBF2 GAMMA path=ramp)', '(?s)\A(?!.*OPENSWBF2 GAMMA fallback)', '(?s)\A(?!.*ramp_nonid=[1-9])', '(?s)\A(?!.*FATAL:)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-shellstretch'
            Args = "$standard $tail -crashtest shellstretch"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY shell stretch linear (\d+)x(\d+)->(\d+)x(\d+)', '(?s)OPENSWBF2 SHELLSTRETCH FAKE chain=\d+x\d+ presents=[1-9]\d* filter=linear.*OPENSWBF2 RESET recovered after \d+ attempts.*OPENSWBF2 SHELLSTRETCH FAKE chain=\d+x\d+ presents=[1-9]\d* filter=linear', '(?s)\A(?!.*shell stretch unavailable)', '(?s)\A(?!.*FATAL:)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-gammadrawfail'
            Args = "$standard $tail -crashtest gammadrawfail"
            Expect = @{ Class = 'Pass'; LogRx = @('(?s)OPENSWBF2 GAMMA path=pass.*OPENSWBF2 GAMMA fallback to ramp reason=drawfail hr=0x8876086C', 'OPENSWBF2 GAMMA FAKE target=\d+x\d+ alive=0 bb=\d+x\d+ draws=0 ramp_nonid=[1-9]', '(?s)\A(?!.*OPENSWBF2 GAMMA fallback.*OPENSWBF2 GAMMA fallback)', '(?s)\A(?!.*draws=[1-9])', '(?s)\A(?!.*FATAL:)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-applyadapter'
            Args = "$standard $tail -crashtest applyadapter -displayini ""{CASEDIR}\display.ini"""
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY adapter=1 invalid', 'OPENSWBF2 DISPLAY adapter=0 pending, running adapter unchanged', '(?s)\A(?!.*OPENSWBF2 DISPLAY adapter=\d+ saved)', '(?s)\A(?!.*FATAL:)'); NoIni = $true }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-adaptercancel'
            Args = "$standard $tail -crashtest adaptercancel -displayini ""{CASEDIR}\display.ini"""
            Expect = @{ Class = 'Pass'; LogRx = @('(?s)OPENSWBF2 DISPLAY adapter=0 pending.*OPENSWBF2 DISPLAY menu cancel restored.*OPENSWBF2 DISPLAY adaptertest save=0', '(?s)\A(?!.*OPENSWBF2 DISPLAY adapter=\d+ saved)', '(?s)\A(?!.*FATAL:)'); NoIni = $true }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-adapteraccept'
            Args = "$standard $tail -crashtest adapteraccept -displayini ""{CASEDIR}\display.ini"""
            Expect = @{ Class = 'Pass'; LogRx = @('(?s)OPENSWBF2 DISPLAY adapter=0 pending, running adapter unchanged.*OPENSWBF2 DISPLAY adapter=0 saved, running adapter unchanged.*OPENSWBF2 DISPLAY menutest save=1', 'OPENSWBF2 DISPLAY confirm timeout, reverting', 'OPENSWBF2 DISPLAY menutest done result=2', '(?s)\A(?!.*FATAL:)'); IniRx = @('(?m)^Adapter=0\r?$', '(?s)\A(?!.*\bVsync=)(?!.*\bAdapter=1)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-menuconfirm'
            Args = "$standard $tail -crashtest menuconfirm"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY confirm pending 15s', '(?s)OPENSWBF2 DISPLAY confirm pending 15s.*OPENSWBF2 DISPLAY menutest plain vsync=\d.*OPENSWBF2 DISPLAY confirm timeout, reverting to ', '(?s)OPENSWBF2 DISPLAY menutest base (\d+x\d+).*OPENSWBF2 DISPLAY confirm timeout, reverting to \1 mode=', '(?s)OPENSWBF2 DISPLAY confirm timeout, reverting to (\d+x\d+) .*OPENSWBF2 DISPLAY applied \1 ', '(?s)OPENSWBF2 DISPLAY menutest base \d+x\d+.*?OPENSWBF2 DISPLAY applied \d+x\d+ mode=\w+ refresh=\S+ vsync=(\w+) .*OPENSWBF2 DISPLAY confirm timeout, reverting to .*?OPENSWBF2 DISPLAY applied \d+x\d+ mode=\w+ refresh=\S+ vsync=\1 ', '(?s)OPENSWBF2 DISPLAY menutest plain vsync=\d.*?OPENSWBF2 DISPLAY applied \d+x\d+ mode=\w+ refresh=\S+ vsync=(\w+) .*OPENSWBF2 DISPLAY confirm timeout, reverting to .*?OPENSWBF2 DISPLAY applied \d+x\d+ mode=\w+ refresh=\S+ vsync=(?!\1 )', 'OPENSWBF2 DISPLAY menutest done result=2', '(?s)OPENSWBF2 DISPLAY menutest base \d+x\d+ fps=(\d+) waitquery=(\d)\b.*OPENSWBF2 DISPLAY confirm timeout, reverting to \d+x\d+ mode=\w+ fps=\1 waitquery=\2\b.*OPENSWBF2 DISPLAY menutest done result=2 fps=\1 waitquery=\2\b','(?s)\A(?!.*OPENSWBF2 DISPLAY confirm kept)', '(?s)\A(?!.*FATAL:)', 'OPENSWBF2 DISPLAY menu options built adapters=\d+', '(?s)\A(?!.*OPENSWBF2 LUA error)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-menukeep'
            Args = "$standard $tail -crashtest menukeep -displayini ""{CASEDIR}\display.ini"""
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY confirm pending 15s', 'OPENSWBF2 DISPLAY confirm kept', 'OPENSWBF2 DISPLAY menutest done result=1', 'OPENSWBF2 DISPLAY menutest save=1', 'OPENSWBF2 DISPLAY menu save deferred until the confirm ends', '(?s)OPENSWBF2 DISPLAY confirm kept.*OPENSWBF2 DISPLAY menu saved keys=0x', 'menutest done result=1 fps=\d+ waitquery=\d keepsave=0', '(?s)\A(?!.*OPENSWBF2 DISPLAY confirm timeout)', '(?s)\A(?!.*FATAL:)'); IniRx = @('(?m)^Vsync=2\r?$', '(?s)\A(?!.*\bFps=)(?!.*\bRefresh=)(?!.*\bNoWaitQuery=)(?!.*\bBorderless=)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-menusavefail'
            Args = "$standard $tail -crashtest menusavefail -displayini ""{CASEDIR}\display.ini"""
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY menutest save=1', '(?s)OPENSWBF2 DISPLAY confirm kept.*OPENSWBF2 DISPLAY menu save failed', 'OPENSWBF2 DISPLAY menutest save=2', 'menutest done result=1 fps=\d+ waitquery=\d keepsave=2', '(?s)\A(?!.*FATAL:)'); IniRx = @('(?s)\A(?!.*\bVsync=)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-menuorder'
            Args = "$standard $tail -crashtest menuorder"
            Expect = @{ Class = 'Pass'; LogRx = @('(?s)OPENSWBF2 DISPLAY menutest order=legacy-menu(?:(?!OPENSWBF2 DISPLAY applied ).)*OPENSWBF2 DISPLAY applied (?:(?!OPENSWBF2 DISPLAY applied ).)*OPENSWBF2 DISPLAY confirm pending 15s', 'OPENSWBF2 DISPLAY confirm timeout, reverting to ', 'OPENSWBF2 DISPLAY menutest done result=2', '(?s)\A(?!.*FATAL:)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-menuorderrev'
            Args = "$standard $tail -crashtest menuorderrev"
            Expect = @{ Class = 'Pass'; LogRx = @('(?s)OPENSWBF2 DISPLAY menutest order=menu-legacy(?:(?!OPENSWBF2 DISPLAY applied ).)*OPENSWBF2 DISPLAY applied (?:(?!OPENSWBF2 DISPLAY applied ).)*OPENSWBF2 DISPLAY confirm pending 15s', 'OPENSWBF2 DISPLAY confirm timeout, reverting to ', 'OPENSWBF2 DISPLAY menutest done result=2', '(?s)\A(?!.*FATAL:)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-menurevertsave'
            Args = "$standard $tail -crashtest menurevertsave -displayini ""{CASEDIR}\display.ini"""
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY menutest save=1', '(?s)OPENSWBF2 DISPLAY confirm pending 15s.*OPENSWBF2 DISPLAY confirm timeout, reverting to .*OPENSWBF2 DISPLAY menu saved keys=0x10\b.*OPENSWBF2 DISPLAY menu cancel restored', '(?s)OPENSWBF2 DISPLAY menutest prior vsync=(\d+) fps=(\d+)\b.*OPENSWBF2 DISPLAY menutest restored vsync=\1 fps=(?!\2\b)\d+', 'OPENSWBF2 DISPLAY menutest done result=2 fps=\d+ waitquery=\d keepsave=0', '(?s)\A(?!.*FATAL:)'); IniRx = @('(?m)^Fps=(?:30|60)\r?$', '(?s)\A(?!.*\bVsync=)(?!.*\bBorderless=)(?!.*\bRefresh=)') }
            Twice = 0; Timeout = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-menuapplyfail'
            Args = "$standard $tail -crashtest menuapplyfail -displayini ""{CASEDIR}\display.ini"""
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY menutest save=1', '(?s)OPENSWBF2 DISPLAY menutest base \d+x\d+ fps=\d+ waitquery=(\d)\b.*OPENSWBF2 DISPLAY menutest prior vsync=\d+ fps=\d+ accept=(\d+)\b.*OPENSWBF2 DISPLAY reverted asked=.*OPENSWBF2 DISPLAY profile restored \d+x\d+ windowed=\d vsync=\d fps=\2 waitquery=\1\b.*OPENSWBF2 DISPLAY menu saved keys=0x10\b.*OPENSWBF2 DISPLAY menutest done result=0 fps=\2 waitquery=\1 keepsave=0', '(?s)\A(?!.*OPENSWBF2 DISPLAY confirm pending)', '(?s)\A(?!.*OPENSWBF2 DISPLAY applied)', '(?s)\A(?!.*FATAL:)'); IniRx = @('(?m)^Fps=(?:30|60)\r?$', '(?s)\A(?!.*\bVsync=)(?!.*\bBorderless=)(?!.*\bRefresh=)(?!.*\bNoWaitQuery=)') }
            Twice = 0; Timeout = 0
        }
    }
    if ($which -eq 'Twice') {
        foreach ($i in 1, 2) {
            $cases += New-Object PSObject -Property @{ Name = "twice-$i"; Args = "$standard $htail"; Expect = $pass; Twice = $i; Timeout = 0 }
        }
        $digestExpect = @{ Class = 'TimeOut'; LogRx = (@('OPENSWBF2 MISSION RUNNING', 'OPENSWBF2 SIM DIGEST turns=') + $hrx) }
        foreach ($i in 1, 2) {
            $cases += New-Object PSObject -Property @{ Name = "twice-digest-$i"; Args = "nab2g_c $htail"; Expect = $digestExpect; Twice = 2 + $i; Timeout = 30 }
        }
        foreach ($m in 'hot1g_con', 'spa3c_ass') {
            $cases += New-Object PSObject -Property @{ Name = "digest-$m"; Args = "$m $htail"; Expect = $pass; Twice = 0; Timeout = 0 }
        }
        $traceExpect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 DIGEST TRACE turn=1500 ') + $hrx) }
        foreach ($i in 1, 2) {
            $ttail = $htail.Replace('-seed 1 ', "-seed $i ")
            $cases += New-Object PSObject -Property @{ Name = "trace-seed-$i"; Args = "hot1g_con $ttail -digesttrace 1600"; Expect = $traceExpect; Twice = 0; Timeout = 0 }
        }
    }
    if ($which -eq 'Perturb') {
        $all = @('geo1c_con', 'nab2g_c', 'hot1g_con', 'spa3c_ass', 'dag1c_ctf', 'kas2c_uber')
        $pm = $all
        if ($env:OPENSWBF2_PERTURB_MISSIONS) {
            $pm = @($env:OPENSWBF2_PERTURB_MISSIONS -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ })
            foreach ($m in $pm) { if ($all -notcontains $m) { throw "OPENSWBF2_PERTURB_MISSIONS: '$m' is not a Perturb mission" } }
        }
        $runs = @(@{ Mode = 'base'; Seed = 0; Extra = '' })
        foreach ($mode in 'pad', 'top', 'fill', 'pool') {
            foreach ($s in 1, 2) { $runs += @{ Mode = $mode; Seed = $s; Extra = " -perturbheap ${s}:$mode" } }
        }
        $runs += @{ Mode = 'hog'; Seed = 0; Extra = ' -hogaddrspace' }
        foreach ($m in $pm) {
            foreach ($r in $runs) {
                $rx = @('(?s)\A(?!.*OPENSWBF2 PERTURB )')
                if ($r.Seed -gt 0) { $rx = @("(?m)OPENSWBF2 PERTURB seed=$($r.Seed) modes=$($r.Mode)\r?$") }
                $class = 'Pass'
                $to = 0
                if ($m -eq 'nab2g_c') {
                    $class = 'TimeOut'
                    $to = 60
                }
                if ($r.Mode -eq 'hog') { $rx += $script:SmokeAddrSpaceRx }
                $name = "perturb-$m-$($r.Mode)"
                if ($r.Seed -gt 0) { $name = "$name-$($r.Seed)" }
                $x = @{ Class = $class; LogRx = $rx; Perturb = @{ Mission = $m; Mode = $r.Mode; Seed = $r.Seed } }
                $cases += New-Object PSObject -Property @{ Name = $name; Args = "$m $tail -digesttrace 1600$($r.Extra)"; Expect = $x; Twice = 0; Timeout = $to }
            }
        }
    }
    return $cases
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

function Get-PerturbProofMissing($case, [string]$logText) {
    $miss = @()
    foreach ($rx in @($case.Expect.LogRx)) { if ($logText -cnotmatch $rx) { $miss += $rx } }
    return $miss
}

function Add-SimOrderExpect($cases, [string]$mode) {
    if (-not $mode) { return $cases }
    foreach ($c in $cases) {
        if ($c.Expect.Class -notin 'Pass', 'TimeOut') { continue }
        $x = $c.Expect.Clone()
        $l = @()
        if ($x.ContainsKey('LogRx')) { $l = @($x.LogRx) }
        $x.LogRx = $l + @("(?m)OPENSWBF2 SIMORDER mode=$mode\r?$", '(?s)\A(?!.*-simorder needs address or serial)')
        if ($mode -eq 'serial') {
            $x.SerialDigest = $true
            if ($script:SmokeDigestConfiguration -in 'Test', 'Asan') {
                $x.LogRx = @($x.LogRx) + @('(?s)\A(?!.*OPENSWBF2 COLLISION serialties=[1-9])')
                if ($x.Class -eq 'Pass') { $x.LogRx = @($x.LogRx) + @('(?m)OPENSWBF2 COLLISION serialties=0\r?$') }
            }
        }
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

function Get-PerturbVerdict($baseLog, $runLog, $runClass, $okClass, $maxTurn = 1500) {
    $t = Compare-DigestTrace $baseLog $runLog $maxTurn
    if ($runClass -ne $okClass) {
        $turn = 0
        if ($t -gt 0) { $turn = $t }
        return @{ Result = 'crash'; Turn = $turn; Class = [string]$runClass }
    }
    if ($t -gt 0) { return @{ Result = 'split'; Turn = $t; Class = [string]$runClass } }
    if ($t -lt 0) { return @{ Result = 'crash'; Turn = 0; Class = 'incomplete' } }
    $a = Get-DigestTrace $baseLog
    $b = Get-DigestTrace $runLog
    $da = $null
    $db = $null
    if (Test-Path -LiteralPath $baseLog) { $da = Select-String -LiteralPath $baseLog -Pattern 'OPENSWBF2 SIM DIGEST .*' -CaseSensitive | Select-Object -Last 1 | ForEach-Object { $_.Matches[0].Value } }
    if (Test-Path -LiteralPath $runLog) { $db = Select-String -LiteralPath $runLog -Pattern 'OPENSWBF2 SIM DIGEST .*' -CaseSensitive | Select-Object -Last 1 | ForEach-Object { $_.Matches[0].Value } }
    if ($a[$maxTurn] -cne $b[$maxTurn] -or $da -cne $db) { return @{ Result = 'split'; Turn = $maxTurn; Class = [string]$runClass } }
    return @{ Result = 'equal'; Turn = 0; Class = [string]$runClass }
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
    return $rows
}

function Get-ClTlogCounts($objDir) {
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
    foreach ($f in Get-ChildItem -LiteralPath $objDir -Filter 'cl.command*.tlog' -Recurse -ErrorAction SilentlyContinue) {
        foreach ($l in (Get-Content -LiteralPath $f.FullName)) {
            if ($l -and -not $l.StartsWith('^')) {
                $total++
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
        Gl = $gl; FpFast = $fpFast; FpContract = $fpContract
    }
}

function Get-FlagChecks($game, $tests) {
    $checks = @()
    $checks += New-Object PSObject -Property @{
        Name = 'flag-alignednew'
        Ok = ($game.Total -gt 0 -and $game.AlignedNewOff -eq $game.Total)
        Detail = ('cl entries={0} /Zc:alignedNew-={1}' -f $game.Total, $game.AlignedNewOff)
    }
    $checks += New-Object PSObject -Property @{
        Name = 'flag-inline'
        Ok = ($game.Total -gt 0 -and $game.InlineOn -eq $game.Total -and $game.InlineOff -eq 0)
        Detail = ('cl entries={0} /Zc:inline={1} /Zc:inline-={2}' -f $game.Total, $game.InlineOn, $game.InlineOff)
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
    return $checks
}

$script:SpacerAllow = @(
    'src\Battlefront2\Source\CommandFlyer.h|_tailPad|[0x7]',
    'src\Battlefront2\Source\CommandHover.h|_tailPad|[0x8]',
    'src\Battlefront2\Source\CommandLine.cpp|szCmdLine|[0x800]',
    'src\Battlefront2\Source\CommandLine.cpp|szArgv0|[0x100]',
    'src\Battlefront2\Source\CommandLine.cpp|acBuf|[0x80]',
    'src\Battlefront2\Source\CollisionMesh.h|buffer|[sizeof(T)*N]',
    'src\Battlefront2\Source\CommandWalker.h|_tailPad|[0xc]',
    'src\Battlefront2\Source\CustomContent.cpp|source|[0x4000]',
    'src\Battlefront2\Source\CustomContent.cpp|unusedPath|[64]',
    'src\Battlefront2\Source\EntityBuildingAnimatedArmed.h|mTailPadding|[0xc]',
    'src\Battlefront2\Source\EntityCarrier.h|_flightFlagPadding|[3]',
    'src\Battlefront2\Source\EntityCarrier.h|_turretPadding|[3]',
    'src\Battlefront2\Source\EntityCarrier.h|_renderPadding|[3]',
    'src\Battlefront2\Source\EntityCarrier.h|_tailPad|[0xc]',
    'src\Battlefront2\Source\EntityPortableTurret.h|_tailPad|[6]',
    'src\Battlefront2\Source\EntityPropAnimated.cpp|acUnused|[256]',
    'src\Battlefront2\Source\FLInputManager.h|mReservedAfterFlip|[3]',
    'src\Battlefront2\Source\FLInputManager.h|mReservedBeforeGuiInputs|[2]',
    'src\Battlefront2\Source\FLInputManager.h|mReservedBeforeProcessDestination|[3]',
    'src\Battlefront2\Source\FLInputManager.h|mReservedBeforeMouse|[2]',
    'src\Battlefront2\Source\FLInputManager.h|mReservedBeforeTextTimer|[2]',
    'src\Battlefront2\Source\FLInputManager.h|mReservedBeforeJoysticks|[4]',
    'src\Battlefront2\Source\FLInputManager.h|mReservedBeforeMissingPort|[3]',
    'src\Battlefront2\Source\gamespyStatsManager.cpp|sStatsMatchData|[0x40]',
    'src\Battlefront2\Source\gamespyStatsManager.cpp|sStatsMatchData|[0x40]',
    'src\Battlefront2\Source\GameVoiceChat.cpp|_gDecoderStorage|[sizeof(Decoder)+4]',
    'src\Battlefront2\Source\GameVoiceChat.cpp|_gNodeStorage|[sizeof(Node)+4]',
    'src\Battlefront2\Source\GameVoiceChat.cpp|_gEncoderStorage|[sizeof(Encoder)+4]',
    'src\Battlefront2\Source\GameVoiceChat.cpp|pad|[3]',
    'src\Battlefront2\Source\HUDEvent.cpp|acName|[0x400]',
    'src\Battlefront2\Source\HUDManager.cpp|acName|[0x400]',
    'src\Battlefront2\Source\LuaCallbacks_LoadSave.cpp|aSerialized|[0x10000]',
    'src\Battlefront2\Source\LuaCallbacks_Net.cpp|padding|[3]',
    'src\Battlefront2\Source\LuaCallbacks_Net.cpp|padding|[3]',
    'src\Battlefront2\Source\LuaCallbacks_Options.cpp|kSha512Padding|[128]',
    'src\Battlefront2\Source\MountedTurret.cpp|mFlyerBaseStorage|[0x564]',
    'src\Battlefront2\Source\MuzzleFlash.cpp|mUnmodeledPrefix|[0x88]',
    'src\Battlefront2\Source\NetAdmin.cpp|remoteResponse|[0x1800]',
    'src\Battlefront2\Source\NetAdmin.cpp|remoteResponse|[0x1800]',
    'src\Battlefront2\Source\NetAdmin.h|pad|[3]',
    'src\Battlefront2\Source\NetAdminMd5.cpp|md5Padding|[64]',
    'src\Battlefront2\Source\NetSetup.cpp|sStatsMatchData|[0x40]',
    'src\Battlefront2\Source\NetSetup.h|mQueryWaitPadding|[3]',
    'src\Battlefront2\Source\NetState.h|mInvisibilityStateStorage|[0x258-0x22c]',
    'src\Battlefront2\Source\OrdnanceBeam.cpp|mWeaponState|[0x70]',
    'src\Battlefront2\Source\ParticleSystem.cpp|pool|[301*sizeof(PblHeap<unsignedint,float>::RECORD)]',
    'src\Battlefront2\Source\PlayerProfile.h|mAlignmentBeforeDifficulty|[2]',
    'src\Battlefront2\Source\PlayerProfile.h|mAlignmentBeforeUnlockState|[3]',
    'src\Battlefront2\Source\PlayerProfile.h|mPadding|[61]',
    'src\Battlefront2\Source\PlayerProfile.h|mTailAlignment|[2]',
    'src\Battlefront2\Source\PlayerProfile.h|mPadding|[63]',
    'src\Battlefront2\Source\PlayerProfile.h|mAlignmentBeforeMustBeZero|[2]',
    'src\Battlefront2\Source\PlayerProfile.h|mPadding|[64]',
    'src\Battlefront2\Source\PlayerProfile.h|mPadding|[32]',
    'src\Battlefront2\Source\PlayerProfile.h|mPadding|[128]',
    'src\Battlefront2\Source\PlayerProfile.h|mAlignmentBeforeGameOptions|[3]',
    'src\Battlefront2\Source\PlayerProfile.h|mAlignmentBeforeAccumPoints|[2]',
    'src\Battlefront2\Source\PlayerProfile.h|mPadding|[96]',
    'src\Battlefront2\Source\PlayerProfile.h|mAlignmentBeforeBonus|[3]',
    'src\Battlefront2\Source\PlayerProfile.h|mAlignmentBeforeCRC|[3]',
    'src\Battlefront2\Source\ScopeDisplay.cpp|mWeaponState|[0x64-sizeof(Thread)]',
    'src\Battlefront2\Source\VisionManager.cpp|heapMem|[(150+1)*sizeof(VisHeap::RECORD)]',
    'src\Battlefront2\Source\WeaponGrenade.h|mUnmodeledTail|[0x120-0x118]',
    'src\Battlefront2\Source\WeaponLauncher.h|mUnmodeledTail|[8]',
    'src\Battlefront2\Source\WeaponRepair.h|mUnmodeledTail|[0x120-0x111]',
    'src\lglcd\lgLcd.cpp|Data|[0x104]',
    'src\RedEngineFL\Application\pc\pcFLRender.h|unknown04|[20]',
    'src\RedEngineFL\Graphics\PC\pcRedAutoSettings.h|mReservedAfterDistortion|[2]',
    'src\RedEngineFL\Graphics\PC\pcRedAutoSettings.h|mReservedAfterHDR|[3]',
    'src\RedEngineFL\Graphics\PC\pcRedAutoSettings.h|mReservedAfterBumpMapping|[3]',
    'src\RedEngineFL\Graphics\PC\StateManager_DX.h|pad_40932|[2]',
    'src\RedEngineFL\Interface\RedBorderElement.h|mOtherPositionAttributes|[0x0c-0x08]',
    'src\RedEngineFL\Interface\RedBorderElement.h|mOtherTextureAttributes|[0x1c-0x14]',
    'src\RedEngineFL\Interface\RedBorderElement.h|mReservedTail|[0x260-0x254]',
    'src\RedEngineFL\movie\pc\pcRedMoviePlayer.h|pad1c|[0xd0-0x1c]',
    'src\SoundFL\source\sndenginebase.h|smVoiceStorage|[0xA800]',
    'src\SoundFL\source\sndenginebase.h|smStreamStorage|[0x1446A68]',
    'src\SoundFL\source\sndglobals.cpp|acMessage|[0x800]',
    'src\SoundFL\source\sndsound.h|smVoiceVirtualStorage|[12800]',
    'src\SoundFL\source\sndsoundlayered.h|smFireForgetLayerStorage|[5888]',
    'src\SoundFL\source\win\sndengine.cpp|gSoftOutputStorage|[2288]',
    'src\SoundFL\source\win\sndengine.h|smSampleRAMBitmap|[0x8000]',
    'src\SoundFL\source\win\sndstreamrenderer.h|mHardwareRendererStorage|[416]'
)
$script:ByteCommentRx = New-Object regex '"(?:[^"\\\r\n]|\\.)*"|''(?:[^''\\\r\n]|\\.)*''|//[^\r\n]*|/\*[\s\S]*?\*/'
$script:ByteDeclItem = '\s*((?:[*&]\s*(?:(?:const|volatile)\b\s*)*)*)(\w+)\s*((?:\[[^\]]*\]\s*)*)(?:=\s*(?:\{(?:[^{}]|\{(?:[^{}]|\{[^{}]*\})*\})*\}|[^,;{}]*)|\{(?:[^{}]|\{[^{}]*\})*\})?\s*'
$script:ByteDeclRx = New-Object regex ('(?m)(?<=^[ \t]*|[{};:][ \t]*)(?:(?:static|const|volatile|mutable|extern|thread_local|register|constexpr|inline|typedef|__unaligned|alignas\s*\((?:[^()]|\([^()]*\))*\)|__declspec\s*\((?:[^()]|\([^()]*\))*\))\s+)*(?:(?:unsigned|signed)\s+)?(?:char|__int8|BYTE|UCHAR|CHAR|INT8|UINT8|uint8_t|int8_t|std::uint8_t|std::int8_t|std::byte|byte)(?:\s+(?:const|volatile))*\b(?!\s*[(:])(?<list>' + $script:ByteDeclItem + '(?:,' + $script:ByteDeclItem + ')*);')
$script:ByteDeclItemRx = New-Object regex ('\G' + $script:ByteDeclItem + '(?:,|$)')
$script:SpacerNameRx = '(?i)pad|reserv|unmodel|unknown|^_*unk|spacer|filler|tail|unused|dummy|storage|align|^_+\w*\d'
$script:OffsetAllow = @(
    'src\Battlefront2\Source\CollisionMesh.cpp|(unsigned short*)(memblock + iBytes - 2 * iIndices)',
    'src\Battlefront2\Source\GameVoiceChatEncoder.cpp|(Snd::StreamPacket*)(mEncoderBuffer + uiAlignedEncoded)',
    'src\Battlefront2\Source\GameVoiceChatEncoder.cpp|(VoiceChat::AudioPacket*)(mEncoderPackets + 1)',
    'src\Battlefront2\Source\HUDManager.cpp|(char*)HUD::gEditHeapMem + 0xFFFFF',
    'src\Battlefront2\Source\NetSetup.cpp|(char*)(fileNames + iCount)',
    'src\Battlefront2\Source\pcMain.cpp|(char*)p + 0x300000',
    'src\Battlefront2\Source\PlayerProfile.cpp|(PlayerProfile::tProfile**)((char*)&mDefaultProfile + 0x10)',
    'src\Battlefront2\Source\VoiceChatNode.cpp|(SendMetrics*)(pPerNode + uiNodes * sizeof(ReceiveMetrics))',
    'src\Battlefront2\Source\VoiceChatNode.cpp|(AudioPacket*)(pBlock + uiNodes * sizeof(ReceiveStream))',
    'src\Battlefront2\Source\VoiceChatNode.cpp|(char*)mBitmap + 1',
    'src\RedEngineFL\Graphics\PC\pcRedSegment.cpp|(PblVector3*)(vertex + normalOffset)',
    'src\RedEngineFL\Graphics\PC\pcRedSegment.cpp|(PblVector3*)(vertex + normalOffset)',
    'src\RedEngineFL\Graphics\PC\pcRedSegment.cpp|(PblVector3*)(vertex + tangentOffset)',
    'src\RedEngineFL\Graphics\PC\pcRedSegment.cpp|(PblVector3*)(vertex + binormalOffset)',
    'src\RedEngineFL\Memory\RedMemory.cpp|(Tag*)((char*)tag + rest)',
    'src\RedEngineFL\Memory\RedMemory.cpp|(Tag*)((char*)tag + need)',
    'src\RedEngineFL\Memory\RedMemory.cpp|(Tag*)((char*)block - hdr)',
    'src\RedEngineFL\Memory\RedMemory.cpp|(Tag*)((char*)pBlock - uTagSize)',
    'src\RedEngineFL\Memory\RedMemoryPool.cpp|(char**)((char*)mPool + i * mSize)',
    'src\RedEngineFL\Memory\RedMemoryPool.cpp|(char**)((char*)mPool + (mCount - 1) * mSize)',
    'src\RedEngineFL\Memory\RedMemoryPool.cpp|(char**)((char*)mFree + i * mSize)',
    'src\RedEngineFL\Memory\RedMemoryPool.cpp|(char**)((char*)mFree + (mGrow - 1) * mSize)',
    'src\RedEngineFL\movie\pc\pcRedMoviePlayer.cpp|(RedBitmapElement**)(pBlock + 16 * uiNumTextures + uiBufferSize)',
    'src\RedEngineFL\movie\pc\pcRedMoviePlayer.cpp|(RedTexture**)(pBlock + uiBufferSize)',
    'src\RedEngineFL\Util\RedLocalizeDB.cpp|(FLLocalizeEntry*)((char*)pEntry + pEntry->mEntryLen)',
    'src\RedEngineFL\Util\RedScreenShot.cpp|(const unsigned int*)((unsigned char*)rect.pBits + rect.Pitch * y)',
    'src\SoundFL\source\sndbufferio.cpp|(void*)(mOffset + (char*)mBuffer)',
    'src\SoundFL\source\sndfreelist.cpp|(Block*)(pEntries + mEntriesPerBlock * mEntrySize)',
    'src\SoundFL\source\sndsamplebankbase.cpp|(Sample*)((char*)pMemory + sizeof(SampleBank))',
    'src\SoundFL\source\sndsoundlayered.cpp|(FireForgetLayer*)(smFireForgetLayerStorage + i * sizeof(FireForgetLayer))',
    'src\SoundFL\source\sndstreambase.cpp|(Segment*)((char*)pMem + sizeof(Info))',
    'src\SoundFL\source\sndstreammixer.cpp|(int*)((char*)mMixBuffer + mMixBufferOffsets[uiInput])',
    'src\SoundFL\source\sndstreamresampler.cpp|(const short*)((const char*)pInput->mBuffer + pInput->mBufferUsed)',
    'src\SoundFL\source\sndstreamresampler.cpp|(short*)((char*)pDst + mBufferSize)',
    'src\SoundFL\source\win\snddsbufferrenderer.cpp|(short*)((unsigned char*)pData + uiWritten)',
    'src\Zephyr\Source\ZephyrAnimBank.cpp|(const ZephyrAnimD*)(pkBlock + 1)',
    'src\Zephyr\Source\ZephyrAnimBank.cpp|(const ZephyrAnimBlockHeader*)(pkAnimD + u16NumAnims)',
    'src\Zephyr\Source\ZephyrAnimBank.cpp|(const ZephyrAnimJointD*)(pkBlock + 1)',
    'src\Zephyr\Source\ZephyrAnimBank.cpp|(const ZephyrAnimBlockHeader*)((const char*)pkJointD + 48 * u32NumTotalJoints)',
    'src\Zephyr\Source\ZephyrAnimBank.cpp|(char*)(pkAnims + u16NumAnims)',
    'src\Zephyr\Source\ZephyrAnimBank.cpp|(const ZephyrAnimJointD*)((const char*)pkJointD + 48 * u16NumJoints)',
    'src\Zephyr\Source\ZephyrSkeletonShared.cpp|(ZephyrJointShared*)((char*)this + kOffsets.m_uJoints)',
    'src\Zephyr\Source\ZephyrSkeletonShared.cpp|(unsigned int*)(i + 1)',
    'src\Zephyr\Source\ZephyrSkinShared.cpp|(ZephyrJointSet*)((char*)this + kOffsets.m_uJointSets)',
    'src\Zephyr\Source\ZephyrSkinShared.cpp|(ZephyrSkeletonShared*)((char*)this + kOffsets.m_uSkeleton)'
)
$script:SerialAllow = @(
    'src\Battlefront2\Source\CollisionObject.cpp|static unsigned int sCollisionSerialNext = 0;',
    'src\Battlefront2\Source\CollisionObject.cpp|mCollisionSerial = sCollisionSerialNext < 0x3FFFFFFF ? ++sCollisionSerialNext : sCollisionSerialNext;',
    'src\Battlefront2\Source\CollisionObject.cpp|sCollisionSerialNext = 0;',
    'src\Battlefront2\Source\CollisionObject.h|unsigned int mCollisionSerial : 30;',
    'src\Battlefront2\Source\CollisionObject.h|return PblSerialAfter(pA->mCollisionSerial, pB->mCollisionSerial);',
    'src\Battlefront2\Source\CollisionObject.h|if (pA->mCollisionSerial == pB->mCollisionSerial)',
    'src\Battlefront2\Source\Thread.cpp|static unsigned int sThreadActivationKey = 0;',
    'src\Battlefront2\Source\Thread.cpp|return (int)(sThreadActivationKey < 0x7FFFFFFF ? sThreadActivationKey++ : sThreadActivationKey);',
    'src\Battlefront2\Source\Thread.cpp|sThreadActivationKey = 0;'
)
$script:SerialRx = New-Object regex '\b(?:sThreadActivationKey|mCollisionSerial|sCollisionSerialNext)\b'
$script:FastcallAllow = @(
    'src\Battlefront2\Source\ActiveRegion.cpp|void* __fastcall InitRawListIterator(void** pCurrent, void*, const void* pSource)',
    'src\Battlefront2\Source\ActiveRegion.h|void* __fastcall InitRawListIterator(void** pCurrent, void* pEdx, const void* pSource);',
    'src\Battlefront2\Source\CameraManager.cpp|typedef const PblVector3& (__fastcall *ObjectVelocityFn)(GameObject* pThis, void*);',
    'src\Battlefront2\Source\CameraManager.cpp|typedef Controllable* (__fastcall *ObjectControllableFn)(GameObject* pThis, void*);',
    'src\Battlefront2\Source\ChaseCamera.cpp|typedef const PblVector3& (__fastcall *GetVelocityFn)(GameObject* pThis, void*);',
    'src\Battlefront2\Source\EntityPathFollower.cpp|int __fastcall ReturnEdxValue(int, int value)',
    'src\Battlefront2\Source\FriendBuddyList.cpp|int __fastcall GetFriendBuddy(int, unsigned int index)',
    'src\Battlefront2\Source\FriendBuddyList.cpp|bool __fastcall IsFriendBuddy(int, int playerId)',
    'src\Battlefront2\Source\GameVoiceChatTransport.cpp|bool __fastcall IsFriendBuddy(int ignored, int iPlayer);',
    'src\Battlefront2\Source\HUDElementMap.cpp|bool __fastcall IsFriendBuddy(int iLocalIndex, int iPlayerId);',
    'src\Battlefront2\Source\LuaCallbacks.cpp|int __fastcall GetScreenTransitionType(const char* pcName, void*)',
    'src\Battlefront2\Source\LuaCallbacks.h|int __fastcall GetScreenTransitionType(const char* pcName, void*);',
    'src\Battlefront2\Source\LuaCallbacks_LoadSave.cpp|void __fastcall GetMissionWorld(char* pcWorld, int)',
    'src\Battlefront2\Source\LuaCallbacks_Net.cpp|bool __fastcall IsFriendBuddy(int ignored, int iPlayer);',
    'src\Battlefront2\Source\NameDisplay.cpp|bool __fastcall IsFriendBuddy(int iLocalIndex, int iPlayerId);',
    'src\Battlefront2\Source\SkyDome.cpp|typedef void(__fastcall* SceneObjectFn)(void* pThis, void*);',
    'src\Battlefront2\Source\SpawnManager.cpp|bool __fastcall IsFriendBuddy(int iLocalIndex, int iPlayerId);',
    'src\Battlefront2\Source\StatusDisplay.cpp|bool __fastcall IsFriendBuddy(int ignored, int iPlayer);'
)
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

function Get-SourceChecks([string]$root, [string[]]$spacerAllow = $script:SpacerAllow, [string[]]$offsetAllow = $script:OffsetAllow, [string[]]$fastcallAllow = $script:FastcallAllow, [string[]]$serialAllow = $script:SerialAllow, [string[]]$Only) {
    $want = { param($name) -not $Only -or $Only -contains $name }
    $defs = @(
        @{ Name = 'new-size-t'; Path = $null; Rx = @(
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
    $sigLeft = @{ 'fastcall-edx' = (New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)) }
    foreach ($a in @($fastcallAllow)) { if ($a) { $sigLeft['fastcall-edx'][$a] = 1 + [int]$sigLeft['fastcall-edx'][$a] } }
    $src = Join-Path $root 'src'
    $allFiles = @()
    if ($runDefs.Count -or $doSpacer -or $doOffset -or $doSerial) {
        $allFiles = @(Get-ChildItem -LiteralPath $src -Recurse -File | Where-Object { $script:SourceExtensions -contains $_.Extension })
    }
    $srcFiles = @()
    if ($runDefs.Count -or $doSpacer -or $doOffset) {
        $srcFiles = @($allFiles | Where-Object { $_.FullName -notmatch '\\src\\(LuaFL|ZlibFL)\\' })
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
    foreach ($f in $srcFiles) {
        $text = [IO.File]::ReadAllText($f.FullName)
        $rel = $f.FullName.Substring($root.Length + 1)
        if ($doSpacer) {
            foreach ($b in (Get-ByteArrayDecls $text)) {
                if (-not (Test-ByteSpacer $b)) { continue }
                $key = '{0}|{1}|{2}' -f $rel, $b.Name, $b.Size
                if ($spacerLeft[$key] -gt 0) { $spacerLeft[$key]--; continue }
                $spacerHits.Add(('{0}:{1}' -f $rel, $b.Line))
            }
        }
        if ($doOffset) {
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
    $testFiles = @()
    if (& $want 't1-suite') {
        $testFiles = @(Get-ChildItem -LiteralPath (Join-Path $root 'tests') -Recurse -File | Where-Object { $script:SourceExtensions -contains $_.Extension -and $_.FullName -notmatch '\\tests\\third_party\\' })
    }
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
    return @($rows | Where-Object { & $want $_.Name })
}

function Get-ImageBase($exe, $workDir, $argLine) {
    $p = Start-Process -FilePath $exe -ArgumentList $argLine -WorkingDirectory $workDir -PassThru -WindowStyle Hidden
    try {
        $base = $null
        for ($i = 0; $i -lt 50 -and -not $base; $i++) {
            Start-Sleep -Milliseconds 200
            try { $base = $p.MainModule.BaseAddress.ToInt64() } catch {}
        }
        return $base
    }
    finally {
        try { if (-not $p.HasExited) { & taskkill /PID $p.Id /T /F 2>&1 | Out-Null } } catch {}
        $p.WaitForExit()
    }
}
