param(
    [Parameter(Mandatory)][string]$Tables,
    [string]$Baseline
)

$ErrorActionPreference = 'Stop'
if (-not $Baseline) { $Baseline = Join-Path $PSScriptRoot 'v143' }

function Read-Rows([string]$dir, [string]$name) {
    $path = Join-Path $dir $name
    if (-not (Test-Path -LiteralPath $path)) { throw "$name not found in $dir" }
    $rows = @()
    foreach ($line in [IO.File]::ReadAllLines($path)) {
        if (-not $line -or $line.StartsWith('#') -or $line.StartsWith("mission`t")) { continue }
        $rows += ,@($line -split "`t")
    }
    return ,$rows
}

function Get-SlackBound([string]$heap) {
    switch ($heap) {
        'Application' { return 16 }
        'Lua' { return 3880 }
        'Runtime' { return 496 }
        default { return 0 }
    }
}

$fail = 0

$clothBase = @{}
foreach ($r in (Read-Rows $Baseline 'cloth-headroom.txt')) { $clothBase[$r[0]] = $r }
$clothNew = @{}
foreach ($r in (Read-Rows $Tables 'cloth-headroom.txt')) { $clothNew[$r[0]] = $r }
$slack = @{}
foreach ($r in (Read-Rows $Baseline 'cloth-slack.txt')) { $slack[$r[0]] = [long]$r[1] }
$bad = @()
foreach ($m in $clothBase.Keys) { if (-not $clothNew.ContainsKey($m)) { $bad += "$m missing from tables" } }
foreach ($m in $clothNew.Keys) { if (-not $clothBase.ContainsKey($m)) { $bad += "$m not in baseline" } }
foreach ($m in ($clothBase.Keys | Sort-Object)) {
    if (-not $clothNew.ContainsKey($m)) { continue }
    $b = $clothBase[$m]
    $t = $clothNew[$m]
    if ($t[2] -cne $b[2] -or $t[3] -cne $b[3] -or $t[4] -cne $b[4]) { $bad += "$m enabled/low/high $($t[2])/$($t[3])/$($t[4]) baseline $($b[2])/$($b[3])/$($b[4])"; continue }
    if (-not $slack.ContainsKey($m)) { $bad += "$m has no slack row"; continue }
    if ($b[1] -eq '-' -or $t[1] -eq '-') {
        if ($b[1] -cne $t[1]) { $bad += "$m free $($t[1]) baseline $($b[1])" }
        continue
    }
    $delta = [long]$t[1] - [long]$b[1]
    if ($delta -ne $slack[$m]) { $bad += "$m free $($t[1]) baseline $($b[1]) delta $delta slack $($slack[$m])" }
}
if ($bad.Count) {
    $fail++
    Write-Host "[FAIL] backend:cloth missions=$($clothBase.Count)"
    foreach ($l in $bad) { Write-Host "    $l" }
}
else { Write-Host "[PASS] backend:cloth missions=$($clothBase.Count)" }

$peakBase = @{}
foreach ($r in (Read-Rows $Baseline 'mem-peaks.txt')) { $peakBase[($r[0..3] -join "`t")] = $r }
$peakNew = @{}
foreach ($r in (Read-Rows $Tables 'mem-peaks.txt')) { $peakNew[($r[0..3] -join "`t")] = $r }
$bad = @()
foreach ($k in $peakBase.Keys) { if (-not $peakNew.ContainsKey($k)) { $bad += "$k missing from tables" } }
foreach ($k in $peakNew.Keys) { if (-not $peakBase.ContainsKey($k)) { $bad += "$k not in baseline" } }
foreach ($k in ($peakBase.Keys | Sort-Object)) {
    if (-not $peakNew.ContainsKey($k)) { continue }
    $b = $peakBase[$k]
    $t = $peakNew[$k]
    if ($b[2] -ceq 'heap') {
        $bound = Get-SlackBound $b[3]
        $why = $null
        if ($t[4] -cne $b[4] -or $t[5] -cne $b[5] -or $t[8] -cne $b[8] -or $t[9] -cne $b[9]) { $why = 'cap/size/allocs/failed differ' }
        else {
            $dPeak = [long]$b[6] - [long]$t[6]
            $dUsed = [long]$b[7] - [long]$t[7]
            if ($dPeak -lt 0 -or $dUsed -lt 0 -or $dUsed -gt $bound) { $why = "peak gap $dPeak used gap $dUsed used bound $bound" }
        }
        if ($why) { $bad += "$k $why tables '$($t[4..9] -join ' ')' baseline '$($b[4..9] -join ' ')'" }
    }
    elseif (($t -join "`t") -cne ($b -join "`t")) { $bad += "$k tables '$($t[4..9] -join ' ')' baseline '$($b[4..9] -join ' ')'" }
}
if ($bad.Count) {
    $fail++
    Write-Host "[FAIL] backend:mem-peaks rows=$($peakBase.Count)"
    foreach ($l in $bad) { Write-Host "    $l" }
}
else { Write-Host "[PASS] backend:mem-peaks rows=$($peakBase.Count)" }

$digBase = @{}
foreach ($r in (Read-Rows $Baseline 'smoke-all-digests.txt')) { $digBase[$r[0]] = $r -join "`t" }
$digNew = @{}
foreach ($r in (Read-Rows $Tables 'smoke-all-digests.txt')) { $digNew[$r[0]] = $r -join "`t" }
$bad = @()
$new = @()
foreach ($k in ($digBase.Keys | Sort-Object)) {
    if ($k -ceq 'dedicated') { continue }
    if (-not $digNew.ContainsKey($k)) { $bad += "$k missing from tables" }
    elseif ($digNew[$k] -cne $digBase[$k]) { $bad += "tables '$($digNew[$k])' baseline '$($digBase[$k])'" }
}
foreach ($k in ($digNew.Keys | Sort-Object)) { if (-not $digBase.ContainsKey($k)) { $new += $k } }
if ($bad.Count) {
    $fail++
    Write-Host "[FAIL] backend:digests rows=$($digBase.Count)"
    foreach ($l in $bad) { Write-Host "    $l" }
}
else { Write-Host "[PASS] backend:digests rows=$($digBase.Count)" }
foreach ($k in $new) { Write-Host "    new case without a baseline row: $k" }

if ($fail) { exit 1 }
exit 0
