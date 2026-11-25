$script:KeyComparedTables = @(@('cloth-headroom.txt', 'mem-peaks.txt', 'pool-sizes.txt', 'smoke-all-digests.txt', 'lua-bins.txt', 'lua-hist.txt', 'offthread.txt', 'digests.txt', 'digests-hog.txt', 'digest-trace-geo1c_con.txt', 'digest-trace-hot1g_con.txt', 'digest-trace-nab2g_c.txt', 'digest-trace-spa3c_ass.txt', 'digest-trace-dag1c_ctf.txt', 'digest-trace-kas2c_uber.txt', 'factory-geo1c_con.txt', 'factory-hot1g_con.txt', 'factory-nab2g_c.txt', 'factory-spa3c_ass.txt', 'initializers.txt', 'initializers-di.txt', 'initializers-discarded.txt', 'digests-debug.txt', 'x87-inventory.tsv') | ForEach-Object { "tests\baseline\v143\$_" }) + @('tests\baseline\v143-x64\mem-peaks.txt', 'tests\baseline\v143-x64\commit.txt', 'tests\baseline\v143-x64\lua-hist.txt', 'tests\baseline\v143-x64\cloth-headroom.txt', 'tests\baseline\v143-x64\pool-sizes.txt', 'tests\baseline\v143-x64\lua-bins.txt', 'tests\baseline\v143-x64\offthread.txt')

$script:JudgeOnlyFiles = @{
    'smoke' = @()
    'smoke-all' = @()
    't1' = @()
    'compare' = @($script:KeyComparedTables | ForEach-Object { @{ Path = $_; Reader = 'compare-tables' } })
}


$script:KeyJudgeScripts = @('tests\hardening.ps1', 'tests\common.ps1', 'tests\hardening\allow.ps1', 'tests\baseline\compare-tables.ps1')

$script:KeyExcludedDocs = @('tests\README.md')


$script:StepColumns = @('Step', 'RunKey', 'JudgeKey', 'Platform', 'Config', 'Evidence', 'Result', 'Wall_s', 'Tree', 'Commit', 'Date')

$script:StepKeyColumns = @('Step', 'Platform', 'Config')

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

function Test-KeyInput([string]$Path, [string[]]$Inputs) {
    foreach ($i in $Inputs) {
        if ($i.EndsWith('\')) { if ($Path.StartsWith($i, [StringComparison]::OrdinalIgnoreCase)) { return $true } }
        elseif ($i.Contains('*')) { if ($Path -like $i) { return $true } }
        elseif ($Path -ieq $i) { return $true }
    }
    return $false
}

function Select-KeyInputFiles($Files, [string[]]$Inputs) {
    $h = New-Object System.Collections.Hashtable ([StringComparer]::OrdinalIgnoreCase)
    foreach ($p in $Files.Keys) { if (Test-KeyInput $p $Inputs) { $h[$p] = $Files[$p] } }
    return $h
}

function New-GuardPathSet
{
    New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
}

function Read-ClReadTlog([string]$Path, [string]$Root)
{
    if (-not $Path -or -not [IO.File]::Exists($Path))
    {
        return $null
    }
    if (-not $Root.EndsWith('\'))
    {
        $Root += '\'
    }
    $cmp = [StringComparison]::OrdinalIgnoreCase
    $rl = $Root.Length
    $time = [IO.File]::GetLastWriteTimeUtc($Path)
    $text = [IO.File]::ReadAllText($Path)
    $sets = New-Object 'System.Collections.Generic.Dictionary[string,System.Collections.Generic.HashSet[string]]' ([StringComparer]::OrdinalIgnoreCase)
    $cur = New-Object 'System.Collections.Generic.List[System.Collections.Generic.HashSet[string]]'
    foreach ($l in $text.Split([char[]]"`r`n", [StringSplitOptions]::RemoveEmptyEntries))
    {
        if ($l[0] -eq [char]'^')
        {
            $cur.Clear()
            foreach ($s in $l.Substring(1).Split('|'))
            {
                if ($s.StartsWith($Root, $cmp))
                {
                    $rel = $s.Substring($rl)
                    $set = $null
                    if (-not $sets.TryGetValue($rel, [ref]$set))
                    {
                        $set = New-GuardPathSet
                        [void]$set.Add($rel)
                        $sets[$rel] = $set
                    }
                    $cur.Add($set)
                }
            }
        }
        elseif ($cur.Count -gt 0 -and $l.StartsWith($Root, $cmp))
        {
            $rel = $l.Substring($rl)
            foreach ($set in $cur)
            {
                [void]$set.Add($rel)
            }
        }
    }
    $fileStale = New-Object 'System.Collections.Generic.Dictionary[string,bool]' ([StringComparer]::OrdinalIgnoreCase)
    $stale = New-GuardPathSet
    foreach ($kv in $sets.GetEnumerator())
    {
        foreach ($p in $kv.Value)
        {
            $b = $false
            if (-not $fileStale.TryGetValue($p, [ref]$b))
            {
                $full = $Root + $p
                $b = (-not [IO.File]::Exists($full)) -or ([IO.File]::GetLastWriteTimeUtc($full) -gt $time)
                $fileStale[$p] = $b
            }
            if ($b)
            {
                [void]$stale.Add($kv.Key)
                break
            }
        }
    }
    [pscustomobject]@{ Path = $Path; Root = $Root; Time = $time; Sets = $sets; Stale = $stale }
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
    $names = @('step', 'platform', 'config', 'toolset', 'cl', 'ucrt', 'steam', 'overlay', 'gamedata', 'exe', 'dlls', 'vendor')
    if ($In.Header) { $names = @($In.Header) }
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
        foreach ($x in $script:KeyExcludedDocs) { [void]$skip.Add($x) }
        foreach ($p in $files.Keys) {
            if ($skip.Contains($p)) { continue }
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

function Merge-KeyedRecords([string]$Path, [string[]]$Columns, [string[]]$KeyColumns, [string[]]$NewLines, [string[]]$DropKeys = @()) {
    $byKey = New-Object 'System.Collections.Generic.SortedDictionary[string,string]' ([StringComparer]::Ordinal)
    $old = Read-KeyedRecords $Path $KeyColumns
    foreach ($k in $old.Keys) {
        if ($DropKeys -ccontains $k) { continue }
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
