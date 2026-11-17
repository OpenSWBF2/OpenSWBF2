param(
    [Parameter(Mandatory)][string]$Tables,
    [string]$Baseline,
    [switch]$LuaBins,
    [switch]$OffThread,
    [switch]$Hardening,
    [switch]$Collect,
    [switch]$Map,
    [switch]$DebugDigests
)

$ErrorActionPreference = 'Stop'
if (-not $Baseline) { $Baseline = Join-Path $PSScriptRoot 'v143' }

$script:BaselineReaders = @(
    @{ File = 'cloth-headroom.txt'; Reader = 'Tables'; Label = 'tables:cloth'; KeyCols = 1 }
    @{ File = 'mem-peaks.txt'; Reader = 'Tables'; Label = 'tables:mem-peaks'; KeyCols = 4 }
    @{ File = 'pool-sizes.txt'; Reader = 'Tables'; Label = 'tables:pool-sizes'; KeyCols = 1 }
    @{ File = 'smoke-all-digests.txt'; Reader = 'Tables'; Label = 'tables:digests'; KeyCols = 1; Skip = @('dedicated') }
    @{ File = 'lua-bins.txt'; Reader = 'LuaBins'; Label = 'tables:lua-bins'; KeyCols = 3 }
    @{ File = 'offthread.txt'; Reader = 'OffThread'; Label = 'tables:offthread'; KeyCols = 1; OffThread = $true }
    @{ File = 'hardening.txt'; Reader = 'Hardening'; Label = 'tables:hardening'; KeyCols = 1; KeyPattern = '^\[[A-Z]+\] hardening:(\S+)' }
    @{ File = 'digests.txt'; Reader = 'Collect'; Label = 'collect:digests' }
    @{ File = 'digests-hog.txt'; Reader = 'Collect'; Label = 'collect:digests-hog' }
    @{ File = 'digest-trace-geo1c_con.txt'; Reader = 'Collect'; Label = 'collect:digest-trace-geo1c_con' }
    @{ File = 'digest-trace-hot1g_con.txt'; Reader = 'Collect'; Label = 'collect:digest-trace-hot1g_con' }
    @{ File = 'digest-trace-nab2g_c.txt'; Reader = 'Collect'; Label = 'collect:digest-trace-nab2g_c' }
    @{ File = 'digest-trace-spa3c_ass.txt'; Reader = 'Collect'; Label = 'collect:digest-trace-spa3c_ass' }
    @{ File = 'factory-geo1c_con.txt'; Reader = 'Collect'; Label = 'collect:factory-geo1c_con' }
    @{ File = 'factory-hot1g_con.txt'; Reader = 'Collect'; Label = 'collect:factory-hot1g_con' }
    @{ File = 'factory-nab2g_c.txt'; Reader = 'Collect'; Label = 'collect:factory-nab2g_c' }
    @{ File = 'factory-spa3c_ass.txt'; Reader = 'Collect'; Label = 'collect:factory-spa3c_ass' }
    @{ File = 'initializers.txt'; Reader = 'Map'; Label = 'map:initializers' }
    @{ File = 'initializers-di.txt'; Reader = 'Map'; Label = 'map:initializers-di' }
    @{ File = 'initializers-discarded.txt'; Reader = 'Map'; Label = 'map:initializers-discarded' }
    @{ File = 'digests-debug.txt'; Reader = 'DebugDigests'; Label = 'debug:digests-debug' }
    @{ File = 'CAUSES.md'; Reader = 't1:tests\unit\memory_baseline.cpp' }
    @{ File = 'MANIFEST.md'; Reader = 'verify-v143' }
)

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
    $base = Read-Keyed $Baseline $name $keyCols $skip $offThread $keyPattern
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

function Compare-Bytes([string]$label, [string]$name) {
    $bp = Join-Path $Baseline $name
    $tp = Join-Path $Tables $name
    if (-not (Test-Path -LiteralPath $bp)) { throw "$name not found in $Baseline" }
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
        Write-Host "    $name in $Baseline is empty"
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

$entries = @($script:BaselineReaders | Where-Object { $_.Reader -ceq $mode })
$fail = 0
if ($entries.Count -eq 0) {
    Write-Host "[FAIL] tables:$mode - no reader map entry for -$mode"
    $fail++
}
foreach ($e in $entries) {
    if ($e.KeyCols) {
        $skip = @()
        if ($e.Skip) { $skip = @($e.Skip) }
        $fail += Compare-Table $e.Label $e.File $e.KeyCols $skip ([bool]$e.OffThread) ([string]$e.KeyPattern)
    }
    else { $fail += Compare-Bytes $e.Label $e.File }
}

if ($fail) { exit 1 }
exit 0
