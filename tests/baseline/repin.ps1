param(
    [Parameter(Mandatory)][string]$RunDir,
    [Parameter(Mandatory)][ValidateSet('smoke-all-digests', 'mem-peaks', 'pool-sizes', 'cloth-headroom', 'lua-bins', 'offthread', 'digest-expected')][string]$Table,
    [Parameter(Mandatory)][string[]]$Key,
    [string]$BaselineDir,
    [string]$ExpectedFile,
    [string]$Exe
)
$ErrorActionPreference = 'Stop'
$testsDir = Split-Path $PSScriptRoot -Parent
$root = Split-Path $testsDir -Parent
. (Join-Path $testsDir 'hardening.ps1')
. (Join-Path $testsDir 'gate\keys.ps1')
if (-not $BaselineDir) { $BaselineDir = Join-Path $PSScriptRoot 'v143' }
if (-not $ExpectedFile) { $ExpectedFile = Join-Path $testsDir 'smoke\digest-expected.txt' }
$keys = @($Key | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ } | Sort-Object -Unique)
if ($keys.Count -eq 0) { Write-Host '[FAIL] repin - no -Key given'; exit 2 }

$paramFile = Join-Path $RunDir 'run-params.txt'
if (-not (Test-Path -LiteralPath $paramFile -PathType Leaf)) { Write-Host "[FAIL] repin - no run-params.txt in $RunDir"; exit 2 }
$rp = Read-KeyValueFile $paramFile
if (-not $Exe) {
    if (-not $rp['platform'] -or -not $rp['smoke_configuration']) { Write-Host '[FAIL] repin - run-params.txt names no platform or smoke configuration'; exit 2 }
    $Exe = Join-Path $root "bin\$($rp['platform'])\$($rp['smoke_configuration'])\BattlefrontII.exe"
}
$runSha = [string]$rp['exe.BattlefrontII.exe']
if ($runSha -cnotmatch '^[0-9a-f]{64}$') { Write-Host "[FAIL] repin - run-params.txt has no exe hash ('$runSha')"; exit 1 }
if (-not (Test-Path -LiteralPath $Exe -PathType Leaf)) { Write-Host "[FAIL] repin - exe not found: $Exe"; exit 1 }
$curSha = Get-PeNormalisedSha $Exe
if ($curSha -cne $runSha) { Write-Host "[FAIL] repin - the run's exe $runSha is not the current exe $curSha ($Exe)"; exit 1 }

$causesFile = Join-Path $BaselineDir 'CAUSES.md'
$caused = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
foreach ($line in [IO.File]::ReadAllLines($causesFile)) {
    if ($line -cmatch '^- `(\S+) (\S+)` \| \S+ \| ') { [void]$caused.Add("$($Matches[1]) $($Matches[2])") }
}
$uncaused = @($keys | Where-Object { -not $caused.Contains("$Table $_") })
foreach ($k in $uncaused) { Write-Host "[FAIL] repin - no cause line in $causesFile names '$Table $k'" }
if ($uncaused.Count -gt 0) { exit 1 }

$offThread = ($Table -ceq 'offthread')
if ($Table -ceq 'digest-expected') {
    $pinned = $ExpectedFile
    $keyCols = 1
}
else {
    $pinned = Join-Path $BaselineDir "$Table.txt"
    $srcDir = 'tables'
    if ($Table -ceq 'lua-bins' -or $offThread) { $srcDir = $Table }
    $source = Join-Path $RunDir "$srcDir\$Table.txt"
    $readers = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'compare-tables.ps1'))
    $km = [regex]::Match($readers, "File = '" + [regex]::Escape("$Table.txt") + "';[^}]*?KeyCols = (\d+)")
    if (-not $km.Success) { Write-Host "[FAIL] repin - compare-tables.ps1 has no key columns for $Table.txt"; exit 2 }
    $keyCols = [int]$km.Groups[1].Value
}

function Get-RowKeys([string[]]$lines) {
    $stop = $false
    foreach ($line in $lines) {
        if ($offThread -and $line -ceq '## sound-on') { $stop = $true }
        if ($stop -or -not $line -or $line.StartsWith('#') -or $line.StartsWith("mission`t") -or ($offThread -and $line.StartsWith("case`t"))) { $null }
        else { (@($line -split "`t")[0..($keyCols - 1)]) -join '/' }
    }
}

$want = New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)
$missing = @()
if ($Table -ceq 'digest-expected') {
    foreach ($k in $keys) {
        $d = Get-DigestKey (Join-Path $RunDir "smoke\$k")
        if ($d) { $want[$k] = @("$k`t$d") }
        else { $missing += "$k has no SIM DIGEST line in $(Join-Path $RunDir "smoke\$k")" }
    }
}
else {
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { Write-Host "[FAIL] repin - $source not found"; exit 1 }
    $srcLines = [IO.File]::ReadAllLines($source)
    $srcKeys = @(Get-RowKeys $srcLines)
    for ($i = 0; $i -lt $srcLines.Count; $i++) {
        $k = $srcKeys[$i]
        if (-not $k -or $keys -cnotcontains $k) { continue }
        if (-not $want.ContainsKey($k)) { $want[$k] = New-Object System.Collections.Generic.List[string] }
        $want[$k].Add($srcLines[$i])
    }
    foreach ($k in $keys) { if (-not $want.ContainsKey($k)) { $missing += "$k is not a row of $source" } }
}

$text = [IO.File]::ReadAllText($pinned)
$nl = $(if ($text.Contains("`r`n")) { "`r`n" } else { "`n" })
$lines = $text.Split([string[]]@($nl), [StringSplitOptions]::None)
$lineKeys = @(Get-RowKeys $lines)
foreach ($k in $keys) { if ($lineKeys -cnotcontains $k) { $missing += "$k is not a row of $pinned" } }
foreach ($m in $missing) { Write-Host "[FAIL] repin - $m" }
if ($missing.Count -gt 0) { exit 1 }

$out = New-Object System.Collections.Generic.List[string]
$done = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
for ($i = 0; $i -lt $lines.Count; $i++) {
    $k = $lineKeys[$i]
    if ($k -and $want.ContainsKey($k)) {
        if ($done.Add($k)) { foreach ($l in @($want[$k])) { $out.Add($l) } }
        continue
    }
    $out.Add($lines[$i])
}
[IO.File]::WriteAllText($pinned, ($out -join $nl), (New-Object Text.UTF8Encoding $false))
foreach ($k in $keys) { Write-Host "[PASS] repin $Table $k ($(@($want[$k]).Count) rows from exe $runSha)" }
exit 0
