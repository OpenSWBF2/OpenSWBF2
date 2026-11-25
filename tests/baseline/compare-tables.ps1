param(
    [Parameter(Mandatory)][string]$Tables,
    [string]$Baseline,
    [switch]$LuaBins,
    [switch]$LuaHist,
    [switch]$OffThread,
    [switch]$Hardening,
    [switch]$Collect,
    [switch]$Map,
    [switch]$DebugDigests,
    [switch]$MemPeaksX64,
    [switch]$X87Inventory,
    [switch]$Commit,
    [string]$RunLog,
    [string]$Steam,
    [string]$Hog,
    [int]$Settled,
    [ValidateSet('Win32', 'x64')][string]$Platform = 'Win32'
)

$ErrorActionPreference = 'Stop'
if ($Baseline) {
    $script:BaseX86 = $Baseline
    $script:BaseX64 = $Baseline
}
else {
    $script:BaseX86 = Join-Path $PSScriptRoot 'v143'
    $script:BaseX64 = Join-Path $PSScriptRoot 'v143-x64'
}

$script:BaselineReaders = @(
    @{ File = 'cloth-headroom.txt'; Reader = 'Tables'; Label = 'tables:cloth'; KeyCols = 1 }
    @{ File = 'cloth-headroom.txt'; Folder = 'v143-x64'; Reader = 'Tables'; Label = 'tables:cloth-x64'; KeyCols = 1 }
    @{ File = 'mem-peaks.txt'; Reader = 'Tables'; Label = 'tables:mem-peaks'; KeyCols = 4 }
    @{ File = 'mem-peaks.txt'; Folder = 'v143-x64'; Reader = 'MemPeaksX64'; Label = 'tables:mem-peaks-x64'; Pair = 'mem-peaks.txt'; Bins = 'lua-bins.txt' }
    @{ File = 'commit.txt'; Folder = 'v143-x64'; Reader = 'Commit'; Label = 'tables:commit' }
    @{ File = 'pool-sizes.txt'; Reader = 'Tables'; Label = 'tables:pool-sizes'; KeyCols = 1 }
    @{ File = 'pool-sizes.txt'; Folder = 'v143-x64'; Reader = 'Tables'; Label = 'tables:pool-sizes-x64'; KeyCols = 1 }
    @{ File = 'smoke-all-digests.txt'; Reader = 'Tables'; Label = 'tables:digests'; KeyCols = 1; Skip = @('dedicated') }
    @{ File = 'lua-bins.txt'; Reader = 'LuaBins'; Label = 'tables:lua-bins'; KeyCols = 3 }
    @{ File = 'lua-bins.txt'; Folder = 'v143-x64'; Reader = 'LuaBins'; Label = 'tables:lua-bins-x64'; KeyCols = 3 }
    @{ File = 'lua-hist.txt'; Reader = 'LuaHist'; Label = 'tables:lua-hist'; KeyCols = 2 }
    @{ File = 'lua-hist.txt'; Folder = 'v143-x64'; Reader = 'LuaHist'; Label = 'tables:lua-hist-x64'; KeyCols = 2 }
    @{ File = 'offthread.txt'; Reader = 'OffThread'; Label = 'tables:offthread'; KeyCols = 1; OffThread = $true }
    @{ File = 'offthread.txt'; Folder = 'v143-x64'; Reader = 'OffThread'; Label = 'tables:offthread-x64'; KeyCols = 1; OffThread = $true }
    @{ File = 'hardening.txt'; Reader = 'Hardening'; Label = 'tables:hardening'; KeyCols = 1; KeyPattern = '^\[[A-Z]+\] hardening:(\S+)' }
    @{ File = 'digests.txt'; Reader = 'Collect'; Label = 'collect:digests' }
    @{ File = 'digests-hog.txt'; Reader = 'Collect'; Label = 'collect:digests-hog' }
    @{ File = 'digest-trace-geo1c_con.txt'; Reader = 'Collect'; Label = 'collect:digest-trace-geo1c_con' }
    @{ File = 'digest-trace-hot1g_con.txt'; Reader = 'Collect'; Label = 'collect:digest-trace-hot1g_con' }
    @{ File = 'digest-trace-nab2g_c.txt'; Reader = 'Collect'; Label = 'collect:digest-trace-nab2g_c' }
    @{ File = 'digest-trace-dag1c_ctf.txt'; Reader = 'Collect'; Label = 'collect:digest-trace-dag1c_ctf' }
    @{ File = 'digest-trace-kas2c_uber.txt'; Reader = 'Collect'; Label = 'collect:digest-trace-kas2c_uber' }
    @{ File = 'digest-trace-spa3c_ass.txt'; Reader = 'Collect'; Label = 'collect:digest-trace-spa3c_ass' }
    @{ File = 'factory-geo1c_con.txt'; Reader = 'Collect'; Label = 'collect:factory-geo1c_con' }
    @{ File = 'factory-hot1g_con.txt'; Reader = 'Collect'; Label = 'collect:factory-hot1g_con' }
    @{ File = 'factory-nab2g_c.txt'; Reader = 'Collect'; Label = 'collect:factory-nab2g_c' }
    @{ File = 'factory-spa3c_ass.txt'; Reader = 'Collect'; Label = 'collect:factory-spa3c_ass' }
    @{ File = 'initializers.txt'; Reader = 'Map'; Label = 'map:initializers' }
    @{ File = 'initializers-di.txt'; Reader = 'Map'; Label = 'map:initializers-di' }
    @{ File = 'initializers-discarded.txt'; Reader = 'Map'; Label = 'map:initializers-discarded' }
    @{ File = 'digests-debug.txt'; Reader = 'DebugDigests'; Label = 'debug:digests-debug' }
    @{ File = 'x87-inventory.tsv'; Reader = 'X87Inventory'; Label = 'tables:x87-inventory'; Script = 'x87-inventory.ps1' }
    @{ File = 'CAUSES.md'; Reader = 't1:tests\unit\memory_baseline.cpp' }
    @{ File = 'movie-goldens.txt'; Reader = 't1:tests\unit\memory_baseline.cpp' }
    @{ File = 'movie-frames.txt'; Reader = 't1:tests\unit\movie_retail.cpp' }
    @{ File = 'MANIFEST.md'; Reader = 'verify-v143' }
    @{ File = 'CAUSES.md'; Folder = 'v143-x64'; Reader = 'verify-v143-x64' }
    @{ File = 'MANIFEST.md'; Folder = 'v143-x64'; Reader = 'verify-v143-x64' }
)

function Get-BaselineDir([string]$name) {
    if ($Platform -ceq 'x64' -and (Test-Path -LiteralPath (Join-Path $script:BaseX64 $name))) { return $script:BaseX64 }
    return $script:BaseX86
}

function Read-Keyed([string]$dir, [string]$name, [int]$keyCols, [string[]]$skip, [bool]$offThread, [string]$keyPattern) {
    $path = Join-Path $dir $name
    if (-not (Test-Path -LiteralPath $path)) { throw "$name not found in $dir" }
    $rows = New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)
    $count = 0
    foreach ($line in [IO.File]::ReadAllLines($path)) {
        if ($offThread) {
            if ($line -ceq '## sound-on') { break }
            if ($line.StartsWith("case`t")) { continue }
            $cols = @($line -split "`t")
            if ($cols.Count -gt 4) { $line = (@($cols[0]) + @($cols[2..($cols.Count - 1)])) -join "`t" }
        }
        if (-not $line -or $line.StartsWith('#') -or $line.StartsWith("mission`t")) { continue }
        $count++
        if ($keyPattern) {
            $km = [regex]::Match($line, $keyPattern)
            if (-not $km.Success) { throw "$name in $dir has a line without a key: $line" }
            $key = $km.Groups[1].Value
        }
        else { $key = (@($line -split "`t")[0..($keyCols - 1)]) -join "`t" }
        if ($skip -ccontains $key) { continue }
        if ($rows.ContainsKey($key)) { $rows[$key] = @($rows[$key]) + @($line) }
        else { $rows[$key] = @($line) }
    }
    if ($count -eq 0) { throw "$name in $dir has no rows" }
    return $rows
}

function Compare-Table([string]$label, [string]$name, [int]$keyCols, [string[]]$skip, [bool]$offThread = $false, [string]$keyPattern = '') {
    $base = Read-Keyed (Get-BaselineDir $name) $name $keyCols $skip $offThread $keyPattern
    $new = Read-Keyed $Tables $name $keyCols $skip $offThread $keyPattern
    $bad = @()
    if ($base.Count -eq 0) { $bad += 'no baseline rows to compare' }
    foreach ($k in ($base.Keys | Sort-Object)) {
        if (-not $new.ContainsKey($k)) { $bad += "$k missing from tables"; continue }
        $b = @($base[$k])
        $t = @($new[$k])
        $n = [Math]::Max($b.Count, $t.Count)
        for ($i = 0; $i -lt $n; $i++) {
            $bl = if ($i -lt $b.Count) { $b[$i] } else { '<none>' }
            $tl = if ($i -lt $t.Count) { $t[$i] } else { '<none>' }
            if ($tl -cne $bl) { $bad += "tables '$tl' baseline '$bl'"; break }
        }
    }
    foreach ($k in ($new.Keys | Sort-Object)) { if (-not $base.ContainsKey($k)) { $bad += "$k not in baseline" } }
    if ($bad.Count) {
        Write-Host "[FAIL] $label keys=$($base.Count)"
        foreach ($l in $bad) { Write-Host "    $l" }
        return 1
    }
    Write-Host "[PASS] $label keys=$($base.Count)"
    return 0
}

function Compare-MemPeaksKeys([string]$label, [string]$name, [string]$pair, [string]$bins) {
    $base = Read-Keyed $script:BaseX86 $pair 4 @() $false ''
    $new = Read-Keyed $Tables $name 4 @() $false ''
    $tn = "x64 $name"
    $bn = "x86 $pair"
    $bad = @()
    $luaRx = "^[^`t]+`t[^`t]+`tpool`tLua\d+$"
    $want = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    foreach ($k in (Read-Keyed $script:BaseX64 $bins 3 @() $false '').Keys) {
        $c = @($k -split "`t")
        if ($c[2] -cmatch '^\d+$') { [void]$want.Add("$($c[0])`t$($c[1])`tpool`tLua$($c[2])") }
    }
    if ($want.Count -eq 0) { $bad += "x64 $bins has no bins" }
    foreach ($k in @($base.Keys | Where-Object { $_ -cmatch $luaRx })) { $base.Remove($k) }
    foreach ($k in @($new.Keys | Where-Object { $_ -cmatch $luaRx } | Sort-Object)) {
        if (-not $want.Contains($k)) { $bad += "$k not a bin of x64 $bins" }
        elseif (@($new[$k]).Count -ne 1) { $bad += "$k has $(@($new[$k]).Count) rows in $tn" }
        [void]$want.Remove($k)
        $new.Remove($k)
    }
    foreach ($k in @($want | Sort-Object)) { $bad += "$k missing from $tn" }
    foreach ($k in ($new.Keys | Sort-Object)) {
        foreach ($l in @($new[$k])) {
            $c = @($l -split "`t")
            if ($c.Count -ne 10) { $bad += "$tn row has $($c.Count) columns: $l" }
            elseif ($c[2] -ceq 'heap' -and ($c[4] -cnotmatch '^\d+$' -or $c[6] -cnotmatch '^\d+$')) { $bad += "$tn heap row without a cap and peak: $l" }
        }
    }
    $bm = @($base.Keys | ForEach-Object { @($_ -split "`t")[0] } | Sort-Object -Unique)
    $nm = @($new.Keys | ForEach-Object { @($_ -split "`t")[0] } | Sort-Object -Unique)
    if (($bm -join ' ') -cne ($nm -join ' ')) { $bad += "$tn covers $($nm.Count) missions and $bn $($bm.Count)" }
    foreach ($k in ($base.Keys | Sort-Object)) {
        if (-not $new.ContainsKey($k)) { $bad += "$k missing from $tn" }
        elseif (@($base[$k]).Count -ne @($new[$k]).Count) { $bad += "$k has $(@($new[$k]).Count) rows in $tn and $(@($base[$k]).Count) in $bn" }
    }
    foreach ($k in ($new.Keys | Sort-Object)) {
        if (-not $base.ContainsKey($k)) { $bad += "$k not in $bn" }
    }
    if ($bad.Count) {
        Write-Host "[FAIL] $label keys=$($new.Count)"
        foreach ($l in $bad) { Write-Host "    $l" }
        return 1
    }
    Write-Host "[PASS] $label keys=$($new.Count) missions=$($nm.Count)"
    return 0
}

function Read-HeapPeaks([string]$path) {
    if (-not (Test-Path -LiteralPath $path)) { throw "$path not found" }
    $peak = New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)
    $caps = New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)
    foreach ($line in [IO.File]::ReadAllLines($path)) {
        if (-not $line -or $line.StartsWith('#') -or $line.StartsWith("mission`t")) { continue }
        $c = @($line -split "`t")
        if ($c.Count -ne 10 -or $c[2] -cne 'heap') { continue }
        if ($c[4] -cnotmatch '^\d+$' -or $c[6] -cnotmatch '^\d+$') { throw "$path heap row without a cap and peak: $line" }
        $p = [decimal]$c[6]
        if (-not $peak.ContainsKey($c[3]) -or $p -gt $peak[$c[3]]) { $peak[$c[3]] = $p }
        if (-not $caps.ContainsKey($c[3])) { $caps[$c[3]] = @() }
        if ($caps[$c[3]] -cnotcontains $c[4]) { $caps[$c[3]] = @($caps[$c[3]]) + $c[4] }
    }
    return @{ Peak = $peak; Caps = $caps }
}

function Get-HeapCapsX64([string]$x64Path, [string]$x86Path, [ref]$bad) {
    $p64 = Read-HeapPeaks $x64Path
    $p86 = Read-HeapPeaks $x86Path
    $want = New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)
    foreach ($h in ($p86.Caps.Keys | Sort-Object)) {
        $caps = @($p86.Caps[$h])
        if ($caps.Count -ne 1) { $bad.Value += "$h has $($caps.Count) caps in $x86Path`: $($caps -join ', ')"; continue }
        if (-not $p64.Peak.ContainsKey($h)) { $bad.Value += "$h has no heap rows in $x64Path"; continue }
        $cap86 = [decimal]$caps[0]
        $peak64 = $p64.Peak[$h]
        $peak86 = $p86.Peak[$h]
        $w = $cap86
        if ($peak64 -gt 0 -and $peak86 -gt 0) {
            $d = [decimal]::Ceiling([decimal]::Ceiling($peak64 * $cap86 / $peak86) / 65536) * 65536
            if ($d -gt $cap86) { $w = $d }
        }
        $want[$h] = $w
    }
    foreach ($h in ($p64.Peak.Keys | Sort-Object)) { if (-not $p86.Caps.ContainsKey($h)) { $bad.Value += "$h has no heap rows in $x86Path" } }
    return $want
}

function Compare-HeapCapsX64([string]$label, [string]$name, [string]$pair, [string]$runLog) {
    $bad = @()
    $want = Get-HeapCapsX64 (Join-Path $Tables $name) (Join-Path $script:BaseX86 $pair) ([ref]$bad)
    $derived = @($want.Keys | Sort-Object | ForEach-Object { "$_=$($want[$_])" }) -join ' '
    if ($runLog) {
        if (-not (Test-Path -LiteralPath $runLog)) { $bad += "$runLog not found" }
        else {
            $seen = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
            $ms = [regex]::Matches([IO.File]::ReadAllText($runLog), 'OPENSWBF2 MEMREPORT heap=(\S+) cap=(\d+) ')
            if ($ms.Count -eq 0) { $bad += "$runLog has no MEMREPORT heap lines" }
            foreach ($m in $ms) {
                $h = $m.Groups[1].Value
                $c = [decimal]$m.Groups[2].Value
                [void]$seen.Add($h)
                if (-not $want.ContainsKey($h)) { $bad += "$h logged cap $c has no derived cap" }
                elseif ($c -ne $want[$h]) { $bad += "$h logged cap $c derived $($want[$h])" }
            }
            if ($ms.Count) { foreach ($h in ($want.Keys | Sort-Object)) { if (-not $seen.Contains($h)) { $bad += "$h derived $($want[$h]) not logged" } } }
        }
    }
    if ($bad.Count) {
        Write-Host "[FAIL] $label heaps=$($want.Count)"
        foreach ($l in ($bad | Select-Object -Unique)) { Write-Host "    $l" }
        return 1
    }
    Write-Host "[PASS] $label $derived"
    return 0
}

function Read-CommitRows([string]$path, [ref]$bad) {
    $rows = New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { $bad.Value += "$path not found"; return $rows }
    $lines = @([IO.File]::ReadAllLines($path) | Where-Object { $_ })
    if ($lines.Count -eq 0 -or $lines[0] -cnotmatch '^# commit [0-9a-f]{7,40}$') { $bad.Value += "$path does not start with '# commit <sha>'" }
    $body = @($lines | Where-Object { -not $_.StartsWith('#') })
    if ($body.Count -eq 0 -or $body[0] -cne "steam`thog`tstate`tarena`trest") { $bad.Value += "$path header is not steam, hog, state, arena, rest separated by tabs"; return $rows }
    foreach ($l in @($body | Select-Object -Skip 1)) {
        $c = @($l -split "`t")
        if ($c.Count -ne 5 -or $c[0] -cnotmatch '^(yes|no)$' -or $c[1] -cnotmatch '^[01]$' -or $c[2] -cnotmatch '^(mission|shell)$' -or $c[3] -cnotmatch '^\d+$' -or $c[4] -cnotmatch '^\d+$') { $bad.Value += "$path row is not steam, hog, state, arena, rest separated by tabs: $l"; continue }
        $k = "steam=$($c[0]) hog=$($c[1]) state=$($c[2])"
        if ($rows.ContainsKey($k)) { $bad.Value += "$path has two rows for $k"; continue }
        $rows[$k] = @([long]$c[3], [long]$c[4])
    }
    if ($rows.Count -eq 0) { $bad.Value += "$path has no rows" }
    return $rows
}

function Compare-CommitRows([string]$label, [string]$name) {
    $bad = @()
    $rows = Read-CommitRows (Join-Path $Tables $name) ([ref]$bad)
    if ($Steam -cnotmatch '^(yes|no)$') { $bad += "-Steam '$Steam' is not yes or no" }
    if ($Hog -cnotmatch '^[01]$') { $bad += "-Hog '$Hog' is not 0 or 1" }
    if ($Settled -lt 1) { $bad += '-Settled must be 1 or more' }
    if (-not $RunLog -or -not (Test-Path -LiteralPath $RunLog -PathType Leaf)) { $bad += "run log '$RunLog' not found" }
    if ($bad.Count -eq 0) {
        $parts = @([regex]::Matches([IO.File]::ReadAllText($RunLog), 'OPENSWBF2 COMMITPARTS state=(\w+) private=\d+ gamecrt=(\d+) heaps=\d+ threads=\d+ threadcount=\d+ arena=(\d+) crtbusy=\d+ crtcommitted=\d+ crtlarge=\d+ gamevm=(\d+)') | ForEach-Object { ,@($_.Groups[1].Value, [long]$_.Groups[3].Value, ([long]$_.Groups[4].Value + [long]$_.Groups[2].Value)) })
        foreach ($s in 'mission', 'shell') {
            $k = "steam=$Steam hog=$Hog state=$s"
            if (-not $rows.ContainsKey($k)) { $bad += "no commit row for $k"; continue }
            $v = @($parts | Where-Object { $_[0] -ceq $s })
            if ($v.Count -le $Settled) { $bad += "$s has $($v.Count) COMMITPARTS rows, none after cycle $Settled"; continue }
            for ($i = $Settled; $i -lt $v.Count; $i++) {
                if ($v[$i][1] -ne $rows[$k][0]) { $bad += "$s $($i + 1) arena = $($v[$i][1]), the row for $k has $($rows[$k][0])" }
                if ($v[$i][2] -ne $rows[$k][1]) { $bad += "$s $($i + 1) rest = $($v[$i][2]), the row for $k has $($rows[$k][1])" }
            }
        }
    }
    if ($bad.Count) {
        Write-Host "[FAIL] $label steam=$Steam hog=$Hog"
        foreach ($l in $bad) { Write-Host "    $l" }
        return 1
    }
    Write-Host "[PASS] $label steam=$Steam hog=$Hog"
    return 0
}

function Compare-X87Inventory([string]$label, [string]$name, [string]$script) {
    $global:LASTEXITCODE = -1
    & (Join-Path $PSScriptRoot $script) -Check (Join-Path (Get-BaselineDir $name) $name) -Inventory (Join-Path $Tables $name)
    if ($LASTEXITCODE -ne 0) {
        Write-Host "[FAIL] $label exit $LASTEXITCODE"
        return 1
    }
    Write-Host "[PASS] $label"
    return 0
}

function Compare-Bytes([string]$label, [string]$name) {
    $bd = Get-BaselineDir $name
    $bp = Join-Path $bd $name
    $tp = Join-Path $Tables $name
    if (-not (Test-Path -LiteralPath $bp)) { throw "$name not found in $bd" }
    if (-not (Test-Path -LiteralPath $tp)) {
        Write-Host "[FAIL] $label"
        Write-Host "    $name not found in $Tables"
        return 1
    }
    $b = [IO.File]::ReadAllBytes($bp)
    $t = [IO.File]::ReadAllBytes($tp)
    $bl = [IO.File]::ReadAllLines($bp)
    if ($b.Length -eq 0) {
        Write-Host "[FAIL] $label"
        Write-Host "    $name in $bd is empty"
        return 1
    }
    if ([Convert]::ToBase64String($b) -ceq [Convert]::ToBase64String($t)) {
        Write-Host "[PASS] $label lines=$($bl.Count)"
        return 0
    }
    $tl = [IO.File]::ReadAllLines($tp)
    $n = [Math]::Max($bl.Count, $tl.Count)
    $msg = "bytes differ with equal lines (line endings or final newline): tables $($t.Length) bytes, baseline $($b.Length) bytes"
    for ($i = 0; $i -lt $n; $i++) {
        $x = if ($i -lt $bl.Count) { $bl[$i] } else { '<none>' }
        $y = if ($i -lt $tl.Count) { $tl[$i] } else { '<none>' }
        if ($y -cne $x) { $msg = "line $($i + 1): tables '$y' baseline '$x'"; break }
    }
    Write-Host "[FAIL] $label lines=$($bl.Count)"
    Write-Host "    $msg"
    return 1
}

$modes = @($PSBoundParameters.Keys | Where-Object { $PSBoundParameters[$_] -is [Management.Automation.SwitchParameter] -and $PSBoundParameters[$_].IsPresent })
if ($modes.Count -gt 1) { Write-Host "Pass at most one of -$($modes -join ', -')."; exit 2 }
$mode = 'Tables'
if ($modes.Count -eq 1) { $mode = $modes[0] }

$entries = @($script:BaselineReaders | Where-Object { $_.Reader -ceq $mode -and (-not $_.Folder -or $Platform -ceq 'x64') })
$scoped = @($entries | Where-Object { $_.Folder } | ForEach-Object { $_.File })
$entries = @($entries | Where-Object { $_.Folder -or $scoped -cnotcontains $_.File })
$fail = 0
if ($entries.Count -eq 0) {
    Write-Host "[FAIL] tables:$mode - no reader map entry for -$mode on $Platform"
    $fail++
}
foreach ($e in $entries) {
    if ($e.Pair) {
        $fail += Compare-MemPeaksKeys $e.Label $e.File $e.Pair $e.Bins
        $fail += Compare-HeapCapsX64 'tables:heap-caps-x64' $e.File $e.Pair $RunLog
    }
    elseif ($e.Reader -ceq 'Commit') { $fail += Compare-CommitRows $e.Label $e.File }
    elseif ($e.Script) { $fail += Compare-X87Inventory $e.Label $e.File $e.Script }
    elseif ($e.KeyCols) {
        $skip = @()
        if ($e.Skip) { $skip = @($e.Skip) }
        $fail += Compare-Table $e.Label $e.File $e.KeyCols $skip ([bool]$e.OffThread) ([string]$e.KeyPattern)
    }
    else { $fail += Compare-Bytes $e.Label $e.File }
}

if ($fail) { exit 1 }
exit 0
