$script:KeyComparedTables = @('cloth-headroom.txt', 'mem-peaks.txt', 'pool-sizes.txt', 'smoke-all-digests.txt', 'lua-bins.txt', 'offthread.txt', 'hardening.txt', 'digests.txt', 'digests-hog.txt', 'digest-trace-geo1c_con.txt', 'digest-trace-hot1g_con.txt', 'digest-trace-nab2g_c.txt', 'digest-trace-spa3c_ass.txt', 'factory-geo1c_con.txt', 'factory-hot1g_con.txt', 'factory-nab2g_c.txt', 'factory-spa3c_ass.txt', 'initializers.txt', 'initializers-di.txt', 'initializers-discarded.txt', 'digests-debug.txt') | ForEach-Object { "tests\baseline\v143\$_" }

$script:JudgeOnlyFiles = @{
    'smoke' = @(@{ Path = 'tests\smoke\digest-expected.txt'; Reader = 'Test-SmokeExpect' }, @{ Path = 'tests\smoke\expected-errors.txt'; Reader = 'Get-SmokeUnexpected' }) + @($script:KeyComparedTables | ForEach-Object { @{ Path = $_; Reader = 'compare-tables' } })
    'smoke-all' = @(@{ Path = 'tests\smoke\expected-errors.txt'; Reader = 'Get-SmokeUnexpected' }) + @($script:KeyComparedTables | ForEach-Object { @{ Path = $_; Reader = 'compare-tables' } })
    't1' = @()
    'hardening' = @(@{ Path = 'tests\baseline\v143\hardening.txt'; Reader = 'compare-tables' })
    'compare' = @($script:KeyComparedTables | ForEach-Object { @{ Path = $_; Reader = 'compare-tables' } })
}

$script:KindRunUnread = @{
    'hardening' = @('tests\smoke\digest-expected.txt', 'tests\smoke\expected-errors.txt')
}

$script:KeyJudgeScripts = @('tests\smoke.ps1', 'tests\common.ps1', 'tests\smoke\cases.ps1', 'tests\baseline\compare-tables.ps1')

$script:KeyExcludedDocs = @('tests\README.md')

$script:KeyRecordFiles = @('tests\guardrevert\steps.tsv', 'tests\guardrevert\proven.tsv', 'tests\baseline\gate-times.tsv')

$script:KeyRecordLeaves = @('proven.partial.tsv')

$script:StepColumns = @('Step', 'RunKey', 'JudgeKey', 'Platform', 'Config', 'Tier', 'Evidence', 'Result', 'Tree', 'Commit', 'Date')

$script:StepKeyColumns = @('Step', 'Platform', 'Config', 'Tier')

$script:SteamOverlayApp = '6060'

function Get-KeyBytesSha([byte[]]$Bytes) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { $b = $sha.ComputeHash($Bytes) }
    finally { $sha.Dispose() }
    return ([BitConverter]::ToString($b) -replace '-', '').ToLowerInvariant()
}

function Get-KeyTextSha([string]$Text) {
    return Get-KeyBytesSha ([Text.Encoding]::UTF8.GetBytes($Text))
}

function Get-KeyFileSha([string]$Path) {
    $sha = [Security.Cryptography.SHA256]::Create()
    $fs = [IO.File]::Open($Path, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
    try { $b = $sha.ComputeHash($fs) }
    finally { $fs.Dispose(); $sha.Dispose() }
    return ([BitConverter]::ToString($b) -replace '-', '').ToLowerInvariant()
}

function Get-PeNormalisedBytes([byte[]]$Bytes) {
    $b = [byte[]]$Bytes.Clone()
    $n = $b.Length
    if ($n -lt 0x40 -or $b[0] -ne 0x4D -or $b[1] -ne 0x5A) { throw 'not a PE file: no MZ header' }
    $e = [BitConverter]::ToInt32($b, 0x3C)
    if ($e -lt 0x40 -or $e + 24 -gt $n -or [BitConverter]::ToUInt32($b, $e) -ne 0x4550) { throw 'not a PE file: no PE signature' }
    $sections = [BitConverter]::ToUInt16($b, $e + 6)
    $optSize = [BitConverter]::ToUInt16($b, $e + 20)
    $opt = $e + 24
    if ($opt + $optSize -gt $n -or $optSize -lt 96) { throw 'not a PE file: optional header truncated' }
    $magic = [BitConverter]::ToUInt16($b, $opt)
    if ($magic -eq 0x10B) { $countAt = $opt + 92; $dirAt = $opt + 96 }
    elseif ($magic -eq 0x20B) { $countAt = $opt + 108; $dirAt = $opt + 112 }
    else { throw ('not a PE file: optional header magic 0x{0:X}' -f $magic) }
    [Array]::Clear($b, $e + 8, 4)
    [Array]::Clear($b, $opt + 64, 4)
    $dirCount = [BitConverter]::ToUInt32($b, $countAt)
    if ($dirCount -le 6 -or $dirAt + 56 -gt $opt + $optSize) { return $b }
    $dbgRva = [BitConverter]::ToUInt32($b, $dirAt + 48)
    $dbgSize = [BitConverter]::ToUInt32($b, $dirAt + 52)
    if ($dbgRva -eq 0 -or $dbgSize -eq 0) { return $b }
    $secAt = $opt + $optSize
    if ($secAt + 40 * $sections -gt $n) { throw 'not a PE file: section table truncated' }
    $dbgOff = -1
    for ($i = 0; $i -lt $sections; $i++) {
        $s = $secAt + 40 * $i
        $va = [BitConverter]::ToUInt32($b, $s + 12)
        $raw = [BitConverter]::ToUInt32($b, $s + 16)
        $ptr = [BitConverter]::ToUInt32($b, $s + 20)
        if ($dbgRva -ge $va -and $dbgRva -lt $va + $raw) { $dbgOff = [long]$ptr + $dbgRva - $va; break }
    }
    if ($dbgOff -lt 0 -or $dbgOff + $dbgSize -gt $n) { throw 'not a PE file: debug directory outside the file' }
    for ($d = [int]$dbgOff; $d + 28 -le $dbgOff + $dbgSize; $d += 28) {
        [Array]::Clear($b, $d + 4, 4)
        $type = [BitConverter]::ToUInt32($b, $d + 12)
        $dataSize = [BitConverter]::ToUInt32($b, $d + 16)
        $dataPtr = [BitConverter]::ToUInt32($b, $d + 24)
        if ($type -eq 2 -and $dataSize -ge 24 -and $dataPtr + 24 -le $n -and [BitConverter]::ToUInt32($b, $dataPtr) -eq 0x53445352) {
            [Array]::Clear($b, $dataPtr + 4, 20)
        }
    }
    return $b
}

function Get-PeNormalisedSha([string]$Path) {
    return Get-KeyBytesSha (Get-PeNormalisedBytes ([IO.File]::ReadAllBytes($Path)))
}

function Get-KeyGitList([string]$Root, [string[]]$GitArgs) {
    $ErrorActionPreference = 'Continue'
    $all = @(& git -C $Root @GitArgs 2>&1)
    $code = $LASTEXITCODE
    $out = @($all | Where-Object { $_ -isnot [Management.Automation.ErrorRecord] })
    if ($code -ne 0) { throw "git $($GitArgs -join ' ') failed: $all" }
    return @(($out -join "`n").Split([char]0) | Where-Object { $_ })
}

function Get-GuardHashes([string]$Root) {
    $h = New-Object System.Collections.Hashtable ([StringComparer]::OrdinalIgnoreCase)
    foreach ($e in (Get-KeyGitList $Root @('ls-files', '-s', '-z'))) {
        $tab = $e.IndexOf("`t")
        $h[$e.Substring($tab + 1).Replace('/', '\')] = $e.Split(' ')[1]
    }
    $live = @()
    foreach ($p in (@(Get-KeyGitList $Root @('diff', '--name-only', '-z', 'HEAD')) + @(Get-KeyGitList $Root @('ls-files', '--others', '--exclude-standard', '-z')))) {
        $k = $p.Replace('/', '\')
        if (Test-Path -LiteralPath (Join-Path $Root $k) -PathType Leaf) { $live += $p }
        else { $h.Remove($k) }
    }
    if ($live.Count) {
        $ErrorActionPreference = 'Continue'
        $out = @($live | & git -C $Root hash-object --stdin-paths)
        if ($LASTEXITCODE -ne 0 -or $out.Count -ne $live.Count) { throw 'git hash-object failed' }
        for ($i = 0; $i -lt $live.Count; $i++) { $h[$live[$i].Replace('/', '\')] = "$($out[$i])".Trim() }
    }
    return $h
}

function Get-TreeFileHashes([string]$Root) {
    $h = New-Object System.Collections.Hashtable ([StringComparer]::OrdinalIgnoreCase)
    foreach ($p in (Get-KeyGitList $Root @('ls-files', '--cached', '--others', '--exclude-standard', '-z'))) {
        $k = $p.Replace('/', '\')
        $full = Join-Path $Root $k
        if ([IO.File]::Exists($full)) { $h[$k] = Get-KeyFileSha $full }
        else { $h[$k] = 'absent' }
    }
    return $h
}

function Get-EvidenceHash([string]$Dir) {
    if (-not $Dir -or -not [IO.Directory]::Exists($Dir)) { return '-' }
    $full = [IO.Path]::GetFullPath($Dir).TrimEnd('\')
    $lines = New-Object System.Collections.Generic.List[string]
    foreach ($f in [IO.Directory]::EnumerateFiles($full, '*', [IO.SearchOption]::AllDirectories)) {
        $lines.Add($f.Substring($full.Length + 1) + "`t" + (Get-KeyFileSha $f))
    }
    if ($lines.Count -eq 0) { return '-' }
    $arr = $lines.ToArray()
    [Array]::Sort($arr, [StringComparer]::Ordinal)
    return Get-KeyTextSha ($arr -join "`n")
}

function Get-GameDataManifestHash([string]$GameDataRoot) {
    if (-not $GameDataRoot -or -not [IO.Directory]::Exists($GameDataRoot)) { return '-' }
    $full = [IO.Path]::GetFullPath($GameDataRoot).TrimEnd('\')
    $lines = New-Object System.Collections.Generic.List[string]
    foreach ($f in [IO.Directory]::EnumerateFiles($full, '*', [IO.SearchOption]::AllDirectories)) {
        $i = New-Object IO.FileInfo $f
        $lines.Add(("{0}`t{1}`t{2}" -f $f.Substring($full.Length + 1).ToLowerInvariant(), $i.Length, $i.LastWriteTimeUtc.Ticks))
    }
    if ($lines.Count -eq 0) { return '-' }
    $arr = $lines.ToArray()
    [Array]::Sort($arr, [StringComparer]::Ordinal)
    return Get-KeyTextSha ($arr -join "`n")
}

function Get-VendorDllHashes([string]$WorkerDir, [string]$SystemDir, [ValidateSet('Win32', 'x64')][string]$Platform) {
    $h = [ordered]@{}
    foreach ($d in @(Get-SmokeWorkerFiles $Platform | Where-Object { $_ -like '*.dll' })) {
        $p = Join-Path $WorkerDir $d
        if (-not ($WorkerDir -and [IO.File]::Exists($p)) -and $d -eq 'd3dx9_43.dll' -and $SystemDir) { $p = Join-Path $SystemDir $d }
        $h[$d] = $(if ([IO.File]::Exists($p)) { Get-KeyFileSha $p } else { '-' })
    }
    return $h
}

function Get-SteamActiveUser {
    $v = Get-ItemProperty -LiteralPath 'HKCU:\Software\Valve\Steam\ActiveProcess' -Name 'ActiveUser' -ErrorAction SilentlyContinue
    if (-not $v) { return 0 }
    return [uint32]$v.ActiveUser
}

function Get-SteamRunning {
    if (-not @(Get-Process -Name 'steam' -ErrorAction SilentlyContinue).Count) { return 'no' }
    if ((Get-SteamActiveUser) -eq 0) { return 'no' }
    return 'yes'
}

function Get-SteamOverlayFromVdf([string]$VdfPath, [string]$AppId = $script:SteamOverlayApp) {
    if (-not $VdfPath -or -not [IO.File]::Exists($VdfPath)) { return '-' }
    $text = [IO.File]::ReadAllText($VdfPath)
    $stack = New-Object System.Collections.Generic.List[string]
    $key = $null
    $value = $null
    foreach ($m in [regex]::Matches($text, '"((?:[^"\\]|\\.)*)"|[{}]')) {
        if ($m.Value -eq '{') {
            if ($null -eq $key) { return '-' }
            $stack.Add($key)
            $key = $null
        }
        elseif ($m.Value -eq '}') {
            if ($stack.Count -eq 0 -or $null -ne $key) { return '-' }
            $stack.RemoveAt($stack.Count - 1)
        }
        elseif ($null -eq $key) { $key = $m.Groups[1].Value }
        else {
            if ($stack.Count -eq 3 -and $stack[1] -ieq 'apps' -and $stack[2] -ceq $AppId -and $key -ieq 'OverlayAppEnable') { $value = $m.Groups[1].Value }
            $key = $null
        }
    }
    if ($stack.Count -ne 0 -or $null -ne $key) { return '-' }
    if ($null -eq $value -or $value -ceq '1') { return 'on' }
    if ($value -ceq '0') { return 'off' }
    return '-'
}

function Get-SteamOverlayState {
    $reg = Get-ItemProperty -LiteralPath 'HKCU:\Software\Valve\Steam' -Name 'SteamPath' -ErrorAction SilentlyContinue
    if (-not $reg -or -not $reg.SteamPath) { return '-' }
    $user = Get-SteamActiveUser
    if ($user -eq 0) { return '-' }
    return Get-SteamOverlayFromVdf (Join-Path ($reg.SteamPath.Replace('/', '\')) "userdata\$user\config\localconfig.vdf")
}

function Get-StepJudgeOnly([string[]]$Kinds) {
    $set = $null
    $judged = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    foreach ($k in @($Kinds | Where-Object { $_ })) {
        $paths = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
        if ($script:JudgeOnlyFiles.ContainsKey($k)) { foreach ($e in $script:JudgeOnlyFiles[$k]) { [void]$paths.Add($e.Path); [void]$judged.Add($e.Path) } }
        if ($script:KindRunUnread.ContainsKey($k)) { foreach ($p in $script:KindRunUnread[$k]) { [void]$paths.Add($p) } }
        if ($null -eq $set) { $set = $paths }
        else { $set.IntersectWith($paths) }
    }
    if ($null -eq $set) { $set = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase) }
    $set.IntersectWith($judged)
    return , $set
}

function Format-KeyField($Value) {
    if ($null -eq $Value) { return '-' }
    if ($Value -is [Collections.IDictionary]) {
        if ($Value.Count -eq 0) { return '-' }
        $parts = @($Value.Keys | ForEach-Object { $v = [string]$Value[$_]; if (-not $v) { $v = '-' }; "$_=$v" })
        [Array]::Sort($parts, [StringComparer]::Ordinal)
        return ($parts -join ';')
    }
    if ($Value -is [Array]) {
        $parts = @($Value | ForEach-Object { [string]$_ } | Where-Object { $_ })
        if ($parts.Count -eq 0) { return '-' }
        [Array]::Sort($parts, [StringComparer]::Ordinal)
        return ($parts -join ',')
    }
    $s = ([string]$Value).Trim()
    if (-not $s) { return '-' }
    return $s
}

function Get-StepKey($In) {
    $valid = $true
    $head = New-Object System.Collections.Generic.List[string]
    $names = @('step', 'platform', 'config', 'tier', 'toolset', 'cl', 'ucrt', 'steam', 'overlay', 'gamedata', 'exe', 'dlls', 'vendor')
    if ($In.Kind -eq 'guard') { $names += 'selection' }
    foreach ($name in $names) {
        $v = Format-KeyField $In[$name]
        if ($name -eq 'overlay' -and $v -cne 'on' -and $v -cne 'off') { $v = '-' }
        if ($v -ceq '-' -or $v -cmatch '(^|[;,])[^;,=]*=-($|[;,])') { $valid = $false }
        $head.Add("$name`t$v")
    }
    $judgeSet = Get-StepJudgeOnly @($In.JudgeKinds)
    $files = $In.Files
    $runLines = New-Object System.Collections.Generic.List[string]
    $judgeLines = New-Object System.Collections.Generic.List[string]
    if ($null -eq $files -or $files.Count -eq 0) {
        $valid = $false
        $head.Add("files`t-")
    }
    else {
        $skip = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
        foreach ($x in (@($script:KeyExcludedDocs) + @($script:KeyRecordFiles))) { [void]$skip.Add($x) }
        $leaves = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
        foreach ($x in $script:KeyRecordLeaves) { [void]$leaves.Add($x) }
        foreach ($p in $files.Keys) {
            if ($skip.Contains($p) -or $leaves.Contains($p.Substring($p.LastIndexOf('\') + 1))) { continue }
            $line = $p.ToLowerInvariant() + "`t" + [string]$files[$p]
            if ($judgeSet.Contains($p)) { $judgeLines.Add($line) }
            else { $runLines.Add($line) }
            if ($script:KeyJudgeScripts -contains $p) { $judgeLines.Add($line) }
        }
    }
    $run = $runLines.ToArray()
    [Array]::Sort($run, [StringComparer]::Ordinal)
    $judge = $judgeLines.ToArray()
    [Array]::Sort($judge, [StringComparer]::Ordinal)
    $runText = (@($head) + @($run)) -join "`n"
    New-Object PSObject -Property ([ordered]@{
        RunKey = Get-KeyTextSha $runText
        JudgeKey = Get-KeyTextSha ($judge -join "`n")
        Valid = $valid
        Platform = Format-KeyField $In['platform']
        Header = @($head)
        RunFiles = @($run | ForEach-Object { $_.Split("`t")[0] })
        JudgeFiles = @($judge | ForEach-Object { $_.Split("`t")[0] })
    })
}

function Read-KeyedRecords([string]$Path, [string[]]$KeyColumns) {
    $records = @{}
    if (-not (Test-Path -LiteralPath $Path)) { return $records }
    $lines = @([IO.File]::ReadAllLines($Path) | Where-Object { $_ })
    if ($lines.Count -lt 2) { return $records }
    $cols = $lines[0].Split("`t")
    foreach ($l in ($lines | Select-Object -Skip 1)) {
        $f = $l.Split("`t")
        $o = [ordered]@{}
        for ($i = 0; $i -lt $cols.Count; $i++) { $o[$cols[$i]] = $(if ($i -lt $f.Count) { $f[$i] } else { '' }) }
        $k = @($KeyColumns | ForEach-Object { [string]$o[$_] }) -join "`t"
        if ($o[$KeyColumns[0]]) { $records[$k] = [pscustomobject]$o }
    }
    return $records
}

function Merge-KeyedRecords([string]$Path, [string[]]$Columns, [string[]]$KeyColumns, [string[]]$NewLines) {
    $byKey = New-Object 'System.Collections.Generic.SortedDictionary[string,string]' ([StringComparer]::Ordinal)
    $old = Read-KeyedRecords $Path $KeyColumns
    foreach ($k in $old.Keys) {
        $p = $old[$k]
        $byKey[$k] = @($Columns | ForEach-Object { [string]$p.$_ }) -join "`t"
    }
    $at = @($KeyColumns | ForEach-Object { [Array]::IndexOf($Columns, $_) })
    foreach ($l in $NewLines) {
        if (-not $l) { continue }
        $f = $l.Split("`t")
        $byKey[(@($at | ForEach-Object { $f[$_] }) -join "`t")] = $l
    }
    New-Item -ItemType Directory -Force -Path (Split-Path $Path -Parent) | Out-Null
    $text = ((@($Columns -join "`t") + @($byKey.Values)) -join "`r`n") + "`r`n"
    [IO.File]::WriteAllText($Path, $text)
}

function Read-StepRecords([string]$Path) {
    return Read-KeyedRecords $Path $script:StepKeyColumns
}

function Merge-StepRecords([string]$Path, [string[]]$Lines) {
    Merge-KeyedRecords $Path $script:StepColumns $script:StepKeyColumns $Lines
}

function Format-StepRecord($Record) {
    return (@($script:StepColumns | ForEach-Object { [string]$Record.$_ }) -join "`t")
}

function Add-StepRecord([string]$Path, $Record) {
    Merge-StepRecords $Path @(Format-StepRecord $Record)
}

function Test-StepRecordUsable($Record) {
    if (-not $Record) { return $false }
    foreach ($c in $script:StepColumns) {
        $v = [string]$Record.$c
        if (-not $v -or $v -ceq '-') { return $false }
    }
    return $true
}

function Test-StepRecordMatch($Record, $Key) {
    if (-not $Key.Valid -or -not (Test-StepRecordUsable $Record)) { return $false }
    return ($Record.Platform -ceq $Key.Platform -and $Record.RunKey -ceq $Key.RunKey)
}

$script:OutcomeResultFields = @('class', 'exit', 'dumps', 'asan', 'stall', 'turns', 'stdout_complete', 'stderr_complete')

$script:OutcomeUndetermined = @('dedicated')

function Get-RunOutcome([string]$Dir) {
    $results = Join-Path $Dir 'results.txt'
    if (-not [IO.File]::Exists($results)) { throw "no results.txt in $Dir" }
    $lines = New-Object 'System.Collections.Generic.SortedDictionary[string,int]' ([StringComparer]::Ordinal)
    foreach ($l in [IO.File]::ReadAllLines($results)) {
        $t = ($l -replace '\s*\(\d+ ms\)', '').TrimEnd()
        if (-not $t) { continue }
        if ($lines.ContainsKey($t)) { $lines[$t]++ }
        else { $lines[$t] = 1 }
    }
    $cases = New-Object 'System.Collections.Generic.SortedDictionary[string,object]' ([StringComparer]::Ordinal)
    $smoke = Join-Path $Dir 'smoke'
    if ([IO.Directory]::Exists($smoke)) {
        foreach ($cd in [IO.Directory]::GetDirectories($smoke)) {
            $f = New-Object 'System.Collections.Generic.SortedDictionary[string,string]' ([StringComparer]::Ordinal)
            $rf = Join-Path $cd 'result.txt'
            $kv = @{}
            if ([IO.File]::Exists($rf)) {
                foreach ($rl in [IO.File]::ReadAllLines($rf)) {
                    $i = $rl.IndexOf('=')
                    if ($i -gt 0) { $kv[$rl.Substring(0, $i)] = $rl.Substring($i + 1) }
                }
            }
            else { $f['result.txt'] = 'missing' }
            foreach ($k in $script:OutcomeResultFields) { $f[$k] = $(if ($kv.ContainsKey($k)) { [string]$kv[$k] } else { '-' }) }
            $sim = New-Object System.Collections.Generic.List[string]
            $soak = New-Object System.Collections.Generic.List[string]
            $done = New-Object System.Collections.Generic.List[string]
            $fatal = New-Object System.Collections.Generic.List[string]
            $trace = New-Object System.Text.StringBuilder
            $traceCount = 0
            $traceLast = '-'
            $log = Join-Path $cd 'run.log'
            if ([IO.File]::Exists($log)) {
                foreach ($ll in [IO.File]::ReadLines($log)) {
                    $j = $ll.IndexOf('FATAL: ')
                    if ($j -ge 0) {
                        $ft = $ll.Substring($j)
                        $b = $ft.IndexOf(' build ')
                        if ($b -ge 0) { $ft = $ft.Substring(0, $b) }
                        $fatal.Add((($ft -replace '0x[0-9A-Fa-f]+', '0x#') -replace '\d+', '#'))
                    }
                    $i = $ll.IndexOf('OPENSWBF2 ')
                    if ($i -lt 0) { continue }
                    $t = $ll.Substring($i)
                    if ($t.StartsWith('OPENSWBF2 SIM DIGEST ')) { $sim.Add($t) }
                    elseif ($t.StartsWith('OPENSWBF2 DIGEST TRACE ')) {
                        [void]$trace.Append($t).Append("`n")
                        $traceCount++
                        $traceLast = $t
                    }
                    elseif ($t.StartsWith('OPENSWBF2 SOAK ')) { $soak.Add($t) }
                    elseif ($t.StartsWith('OPENSWBF2 RUN COMPLETE')) { $done.Add($t) }
                }
            }
            else { $f['run.log'] = 'missing' }
            $f['sim_digest'] = ($sim.ToArray() -join ' | ')
            $f['soak'] = ($soak.ToArray() -join ' | ')
            $f['run_complete'] = ($done.ToArray() -join ' | ')
            $f['fatal'] = ($fatal.ToArray() -join ' | ')
            $f['digest_trace_end'] = "$traceCount $traceLast"
            $f['digest_trace_sha'] = $(if ($traceCount) { Get-KeyTextSha $trace.ToString() } else { '-' })
            $name = [IO.Path]::GetFileName($cd)
            if ($script:OutcomeUndetermined -contains $name) {
                $f['turns'] = 'undetermined'
                $f['sim_digest'] = "$($sim.Count) lines"
                $f['soak'] = "$($soak.Count) lines"
                $f['run_complete'] = "$($done.Count) lines"
                $f['digest_trace_end'] = "$traceCount lines"
                $f['digest_trace_sha'] = 'undetermined'
            }
            $cases[$name] = $f
        }
    }
    return New-Object PSObject -Property @{ Lines = $lines; Cases = $cases }
}

function Compare-RunOutcome([string]$DirA, [string]$DirB) {
    $a = Get-RunOutcome $DirA
    $b = Get-RunOutcome $DirB
    $diff = New-Object System.Collections.Generic.List[string]
    $keys = New-Object 'System.Collections.Generic.SortedSet[string]' ([StringComparer]::Ordinal)
    foreach ($k in $a.Lines.Keys) { [void]$keys.Add($k) }
    foreach ($k in $b.Lines.Keys) { [void]$keys.Add($k) }
    foreach ($k in $keys) {
        $ca = $(if ($a.Lines.ContainsKey($k)) { $a.Lines[$k] } else { 0 })
        $cb = $(if ($b.Lines.ContainsKey($k)) { $b.Lines[$k] } else { 0 })
        if ($ca -ne $cb) { $diff.Add("results.txt: A x$ca B x$cb '$k'") }
    }
    $names = New-Object 'System.Collections.Generic.SortedSet[string]' ([StringComparer]::Ordinal)
    foreach ($k in $a.Cases.Keys) { [void]$names.Add($k) }
    foreach ($k in $b.Cases.Keys) { [void]$names.Add($k) }
    foreach ($name in $names) {
        if (-not $a.Cases.ContainsKey($name)) { $diff.Add("smoke\${name}: case only in B"); continue }
        if (-not $b.Cases.ContainsKey($name)) { $diff.Add("smoke\${name}: case only in A"); continue }
        $fa = $a.Cases[$name]
        $fb = $b.Cases[$name]
        $fields = New-Object 'System.Collections.Generic.SortedSet[string]' ([StringComparer]::Ordinal)
        foreach ($k in $fa.Keys) { [void]$fields.Add($k) }
        foreach ($k in $fb.Keys) { [void]$fields.Add($k) }
        foreach ($k in $fields) {
            $va = $(if ($fa.ContainsKey($k)) { $fa[$k] } else { '(absent)' })
            $vb = $(if ($fb.ContainsKey($k)) { $fb[$k] } else { '(absent)' })
            if ($va -cne $vb) { $diff.Add("smoke\$name ${k}: A '$va' B '$vb'") }
        }
    }
    return New-Object PSObject -Property @{ Equal = ($diff.Count -eq 0); Differences = $diff.ToArray() }
}
