param([switch]$List)
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $MyInvocation.MyCommand.Path) -Parent
$configs = 'Debug', 'Release', 'Test', 'Asan'

$desired = New-Object System.Collections.Generic.List[string]
foreach ($c in $configs) {
    $desired.Add((Join-Path $root "bin\Win32\$c\BattlefrontII.exe"))
    for ($w = 1; $w -le 16; $w++) { $desired.Add((Join-Path $root "artifacts\smoke-workers\$c\w$w\BattlefrontII.exe")) }
    $desired.Add((Join-Path $root "artifacts\smoke-workers\guard-$c\BattlefrontII.exe"))
}
for ($w = 1; $w -le 16; $w++) {
    foreach ($n in 'BattlefrontII.exe', 'BattlefrontII-nolaa.exe') { $desired.Add((Join-Path $root "artifacts\smoke-workers\laa\w$w\$n")) }
}

$existing = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
$stalePrefixes = @('artifacts\tests\', 'artifacts\guard-revert\', 'artifacts\laa-compare\', 'artifacts\spike32\work\') | ForEach-Object { Join-Path $root $_ }
$remove = @()
foreach ($f in @(Get-NetFirewallApplicationFilter | Where-Object { $_.Program -like '*BattlefrontII*.exe' })) {
    $p = [Environment]::ExpandEnvironmentVariables($f.Program)
    [void]$existing.Add($p)
    if (Test-Path -LiteralPath $p) { continue }
    $stale = ($p -like '*\.claude\*') -or ($p -like '*\temp\claude\*')
    foreach ($s in $stalePrefixes) { if ($p.StartsWith($s, [StringComparison]::OrdinalIgnoreCase)) { $stale = $true } }
    if ($stale) { foreach ($r in @($f | Get-NetFirewallRule)) { $remove += New-Object PSObject -Property @{ Rule = $r; Path = $p } } }
}
$add = @($desired | Where-Object { -not $existing.Contains($_) })

if (-not $List) {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    if (-not ([Security.Principal.WindowsPrincipal]$id).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        Write-Host 'firewall.ps1 must run from an elevated PowerShell (or use -List)'
        exit 1
    }
}

foreach ($p in $add) {
    Write-Host "ADD $p"
    if (-not $List) {
        New-NetFirewallRule -DisplayName "OpenSWBF2 test $($p.Substring($root.Length + 1))" -Direction Inbound -Action Allow -Program $p -RemoteAddress LocalSubnet -Profile Any | Out-Null
    }
}
foreach ($x in $remove) {
    Write-Host "REMOVE $($x.Rule.DisplayName) $($x.Path)"
    if (-not $List) { Remove-NetFirewallRule -InputObject $x.Rule }
}
$verb = if ($List) { 'would add' } else { 'added' }
$verb2 = if ($List) { 'would remove' } else { 'removed' }
Write-Host "$verb $($add.Count) rules, $verb2 $($remove.Count) rules"
exit 0
