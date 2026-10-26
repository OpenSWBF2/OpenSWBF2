$script:SmokeStandardMission = 'geo1c_con'
$script:SmokeMarker = 'OPENSWBF2 RUN COMPLETE'
$script:WerKeyPath = 'SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\BattlefrontII.exe'
$script:WerBasePath = 'SOFTWARE\Microsoft\Windows\Windows Error Reporting'

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

function Get-SmokePrecheck($gameDataRoot, $exeDir) {
    if (-not $gameDataRoot) { return 'no retail GameData found (set SWBF2_GAMEDATA or install Steam App 6060)' }
    if (-not (Test-Path -LiteralPath (Join-Path $exeDir 'BattlefrontII.exe'))) { return "BattlefrontII.exe not found in $exeDir" }
    foreach ($dll in 'binkw32.dll', 'Galaxy.dll', 'GalaxyPeer.dll', 'steam_api.dll') {
        if (-not (Test-Path -LiteralPath (Join-Path $exeDir $dll))) { return "$dll missing beside the exe in $exeDir" }
    }
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

function Invoke-SmokeCase($name, $exe, $workDir, $argLine, $timeoutSec, $caseDir) {
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
    $ms = $sw.ElapsedMilliseconds
    $result = ("class={0}`r`nexit=0x{1:X8}`r`nelapsed_ms={2}`r`ndumps={3}`r`n" -f $class, $code, $ms, $dumps.Count)
    Set-Content -Path (Join-Path $caseDir 'result.txt') -Value $result -Encoding ASCII
    return New-Object PSObject -Property @{ Name = $name; Class = $class; Code = $code; Dumps = $dumps; LogText = $logText; Ms = $ms; CaseDir = $caseDir }
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
    $unexpected = @(Get-SmokeUnexpected $case.Name $res.CaseDir)
    if ($unexpected.Count -gt 0) {
        foreach ($u in $unexpected) { Write-Host "    unexpected log line: $u" }
        return $false
    }
    return $true
}

function Get-SmokeCases($which, $missions, $movies, $dedicatedSpec) {
    $standard = $script:SmokeStandardMission
    $render = '-norender'
    if ($movies) { $render = '' }
    $tail = (("$render -nosound -fixedrate 30 -nowait -logfinalscore -DecompCONMinutes 5 -seed 1 -exitongameover -nofocuspause -log ""{CASEDIR}\run.log""") -replace '^\s+', '')
    $pass = @{ Class = 'Pass' }
    $cases = @()

    $missionCase = {
        param($m)
        New-Object PSObject -Property @{ Name = $m; Args = "$m $tail"; Expect = $pass; Twice = 0; Timeout = 0 }
    }
    $allCase = {
        param($m, $class)
        $x = $pass
        $to = 0
        if ($class -eq 'TimeOut') { $x = @{ Class = 'TimeOut'; LogRx = 'OPENSWBF2 MISSION RUNNING' } }
        elseif ($class -eq 'Fatal') {
            $x = @{ Class = 'Fatal'; Code = 3; MinDumps = 1; MaxDumps = 1; LogRx = 'FATAL: exception 0xC0000005 at 0x[0-9A-F]+ \(d3dx9_43\.dll\+0x1BA553\)' }
        }
        elseif ($class -ne 'Pass') { throw "unknown expected class '$class' for $m in missions file" }
        New-Object PSObject -Property @{ Name = $m; Args = "$m $tail"; Expect = $x; Twice = 0; Timeout = $to }
    }

    if ($which -eq 'Standard') { $cases += & $missionCase $standard }
    if ($which -eq 'All') {
        foreach ($line in $missions) {
            $tok = @($line.Trim() -split '\s+')
            if ($tok[0]) {
                $class = 'Pass'
                if ($tok.Count -gt 1) { $class = $tok[1] }
                $cases += & $allCase $tok[0] $class
            }
        }
    }
    if ($which -eq 'Dedicated' -or $which -eq 'All') {
        $dm = $standard
        if ($dedicatedSpec) { $dm = $dedicatedSpec }
        $cases += New-Object PSObject -Property @{
            Name = 'dedicated'
            Args = "/autonet dedicated -lan -nopregame -playercount 0 $dm 100 100 $tail"
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
            Expect = @{ Class = 'Pass'; LogRx = 'alloc failure heap \S+ \(\d+\) size \d+ free \d+ load \d+% phys \d+ MB virt \d+ MB' }
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
    }
    if ($which -eq 'Twice') {
        foreach ($i in 1, 2) {
            $cases += New-Object PSObject -Property @{ Name = "twice-$i"; Args = "$standard $tail"; Expect = $pass; Twice = $i; Timeout = 0 }
        }
    }
    return $cases
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

function Find-Dumpbin {
    $cands = @()
    if ($env:VS120COMNTOOLS) { $cands += (Join-Path $env:VS120COMNTOOLS '..\..\VC\bin\dumpbin.exe') }
    $cands += (Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio 12.0\VC\bin\dumpbin.exe')
    foreach ($c in $cands) { if (Test-Path -LiteralPath $c) { return (Resolve-Path -LiteralPath $c).Path } }
    Write-Host 'dumpbin not found (install VS2013 Update 5).'
    exit 2
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

function Get-ClTlogCounts($objDir) {
    $total = 0
    $gsMinus = 0
    $sdl = 0
    foreach ($f in Get-ChildItem -LiteralPath $objDir -Filter 'cl.command*.tlog' -Recurse -ErrorAction SilentlyContinue) {
        foreach ($l in (Get-Content -LiteralPath $f.FullName)) {
            if ($l -and -not $l.StartsWith('^')) {
                $total++
                if ($l -match ' /GS-( |$)') { $gsMinus++ }
                if ($l -match ' /sdl( |$)') { $sdl++ }
            }
        }
    }
    return New-Object PSObject -Property @{ Total = $total; GsMinus = $gsMinus; Sdl = $sdl }
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
