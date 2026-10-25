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
    if ($code -eq 1 -or $code -eq 3 -or $logText -match 'FATAL:' -or @($dumps).Count -gt 0) { return 'Fatal' }
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

function Test-SmokeExpect($case, $res) {
    $x = $case.Expect
    if ($res.Class -ne $x.Class) { return $false }
    if ($x.ContainsKey('Code') -and $res.Code -ne [uint32]$x.Code) { return $false }
    $n = @($res.Dumps).Count
    if ($x.ContainsKey('MinDumps') -and $n -lt $x.MinDumps) { return $false }
    if ($x.ContainsKey('MaxDumps') -and $n -gt $x.MaxDumps) { return $false }
    if ($x.ContainsKey('LogRx') -and $res.LogText -notmatch $x.LogRx) { return $false }
    return $true
}

function Get-SmokeCases($which, $missions, $movies, $dedicatedSpec) {
    $standard = $script:SmokeStandardMission
    $render = '-norender'
    if ($movies) { $render = '' }
    $tail = (("$render -nosound -fixedrate 30 -nowait -logfinalscore -DecompCONMinutes 5 -seed 1 -exitongameover -log ""{CASEDIR}\run.log""") -replace '^\s+', '')
    $pass = @{ Class = 'Pass' }
    $cases = @()

    $missionCase = {
        param($m)
        New-Object PSObject -Property @{ Name = $m; Args = "$m $tail"; Expect = $pass; Twice = 0 }
    }

    if ($which -eq 'Standard') { $cases += & $missionCase $standard }
    if ($which -eq 'All') {
        foreach ($line in $missions) {
            $m = ($line.Trim() -split '\s+')[0]
            if ($m) { $cases += & $missionCase $m }
        }
    }
    if ($which -eq 'Dedicated' -or $which -eq 'All') {
        $dm = $standard
        if ($dedicatedSpec) { $dm = $dedicatedSpec }
        $cases += New-Object PSObject -Property @{
            Name = 'dedicated'
            Args = "/autonet dedicated -lan -nopregame -playercount 0 $dm 100 100 $tail"
            Expect = $pass
            Twice = 0
        }
    }
    if ($which -eq 'Crash' -or $which -eq 'All') {
        foreach ($kind in 'nullderef', 'divzero', 'stackoverflow', 'debugbreak', 'abort', 'invalidparam') {
            $cases += New-Object PSObject -Property @{
                Name = "crash-$kind"
                Args = "$standard $tail -crashtest $kind"
                Expect = @{ Class = 'Fatal'; Code = 3; MinDumps = 1; MaxDumps = 1; LogRx = 'FATAL:.*\.dmp' }
                Twice = 0
            }
        }
        $ff = @{ Class = 'GsFailure'; Code = 3221226505 }
        if (Test-WerKey) { $ff.MinDumps = 1 }
        $cases += New-Object PSObject -Property @{ Name = 'crash-fastfail'; Args = "$standard $tail -crashtest fastfail"; Expect = $ff; Twice = 0 }
        $cases += New-Object PSObject -Property @{
            Name = 'fatal-init'
            Args = "no_such_mission $tail"
            Expect = @{ Class = 'Fatal'; Code = 1; MaxDumps = 0; LogRx = 'FATAL: initialization failed' }
            Twice = 0
        }
    }
    if ($which -eq 'Twice') {
        foreach ($i in 1, 2) {
            $cases += New-Object PSObject -Property @{ Name = "twice-$i"; Args = "$standard $tail"; Expect = $pass; Twice = $i }
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
