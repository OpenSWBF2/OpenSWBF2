param(
    [Parameter(Mandatory)][string]$Tables,
    [string]$Baseline,
    [switch]$LuaBins
)

$ErrorActionPreference = 'Stop'
if (-not $Baseline) { $Baseline = Join-Path $PSScriptRoot 'v143' }

function Read-Keyed([string]$dir, [string]$name, [int]$keyCols, [string[]]$skip) {
    $path = Join-Path $dir $name
    if (-not (Test-Path -LiteralPath $path)) { throw "$name not found in $dir" }
    $rows = New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)
    $count = 0
    foreach ($line in [IO.File]::ReadAllLines($path)) {
        if (-not $line -or $line.StartsWith('#') -or $line.StartsWith("mission`t")) { continue }
        $count++
        $key = (@($line -split "`t")[0..($keyCols - 1)]) -join "`t"
        if ($skip -ccontains $key) { continue }
        if ($rows.ContainsKey($key)) { $rows[$key] = @($rows[$key]) + @($line) }
        else { $rows[$key] = @($line) }
    }
    if ($count -eq 0) { throw "$name in $dir has no rows" }
    return $rows
}

function Compare-Table([string]$label, [string]$name, [int]$keyCols, [string[]]$skip) {
    $base = Read-Keyed $Baseline $name $keyCols $skip
    $new = Read-Keyed $Tables $name $keyCols $skip
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

$fail = 0
if ($LuaBins) {
    $fail += Compare-Table 'tables:lua-bins' 'lua-bins.txt' 3 @()
}
else {
    $fail += Compare-Table 'tables:cloth' 'cloth-headroom.txt' 1 @()
    $fail += Compare-Table 'tables:mem-peaks' 'mem-peaks.txt' 4 @()
    $fail += Compare-Table 'tables:pool-sizes' 'pool-sizes.txt' 1 @()
    $fail += Compare-Table 'tables:digests' 'smoke-all-digests.txt' 1 @('dedicated')
}

if ($fail) { exit 1 }
exit 0
