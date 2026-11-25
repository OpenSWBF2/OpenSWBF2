param([switch]$List, [string[]]$Root)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'hardening.ps1')
if (-not $Root) { $Root = @(Split-Path $PSScriptRoot -Parent) }
$Root = @($Root | ForEach-Object { $_ -split ',' } | Where-Object { $_.Trim() } | ForEach-Object { [IO.Path]::GetFullPath($_.Trim()).TrimEnd('\') } | Sort-Object -Unique)

$existing = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
$filters = @(Get-NetFirewallApplicationFilter | Where-Object { $_.Program -like '*BattlefrontII*.exe' })
foreach ($f in $filters) { [void]$existing.Add([Environment]::ExpandEnvironmentVariables($f.Program)) }

$plan = @()
foreach ($r in $Root) {
    $desired = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    foreach ($p in (Get-FirewallRulePaths $r)) { [void]$desired.Add($p) }
    $workerPrefix = Join-Path $r 'artifacts\smoke-workers\'
    $stalePrefixes = @('artifacts\tests\', 'artifacts\laa-compare\', 'artifacts\spike32\work\') | ForEach-Object { Join-Path $r $_ }
    $add = @($desired | Where-Object { -not $existing.Contains($_) } | Sort-Object)
    $remove = @()
    foreach ($f in $filters) {
        $p = [Environment]::ExpandEnvironmentVariables($f.Program)
        if ($desired.Contains($p)) { continue }
        $stale = $p.StartsWith($workerPrefix, [StringComparison]::OrdinalIgnoreCase)
        if (-not $stale -and -not (Test-Path -LiteralPath $p)) {
            foreach ($s in $stalePrefixes) { if ($p.StartsWith($s, [StringComparison]::OrdinalIgnoreCase)) { $stale = $true } }
        }
        if ($stale) { foreach ($rule in @($f | Get-NetFirewallRule)) { $remove += New-Object PSObject -Property @{ Rule = $rule; Path = $p } } }
    }
    $plan += New-Object PSObject -Property @{ Root = $r; Desired = $desired.Count; Add = $add; Remove = $remove }
}
$claude = @()
foreach ($f in $filters) {
    $p = [Environment]::ExpandEnvironmentVariables($f.Program)
    if ((Test-Path -LiteralPath $p) -or -not (($p -like '*\.claude\*') -or ($p -like '*\temp\claude\*'))) { continue }
    foreach ($rule in @($f | Get-NetFirewallRule)) { $claude += New-Object PSObject -Property @{ Rule = $rule; Path = $p } }
}

if (-not $List) {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    if (-not ([Security.Principal.WindowsPrincipal]$id).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        Write-Host 'firewall.ps1 must run from an elevated PowerShell (or use -List)'
        exit 1
    }
}

$verb = if ($List) { 'would add' } else { 'added' }
$verb2 = if ($List) { 'would remove' } else { 'removed' }
foreach ($e in $plan) {
    foreach ($p in $e.Add) {
        Write-Host "ADD $p"
        if (-not $List) {
            New-NetFirewallRule -DisplayName "OpenSWBF2 test $p" -Direction Inbound -Action Allow -Program $p -RemoteAddress LocalSubnet -Profile Any | Out-Null
        }
    }
    foreach ($x in $e.Remove) {
        Write-Host "REMOVE $($x.Rule.DisplayName) $($x.Path)"
        if (-not $List) { Remove-NetFirewallRule -InputObject $x.Rule }
    }
}
foreach ($x in $claude) {
    Write-Host "REMOVE $($x.Rule.DisplayName) $($x.Path)"
    if (-not $List) { Remove-NetFirewallRule -InputObject $x.Rule }
}
foreach ($e in $plan) {
    Write-Host "root $($e.Root): $($e.Desired) rule paths, missing $($e.Add.Count), stale $($e.Remove.Count); $verb $($e.Add.Count) rules, $verb2 $($e.Remove.Count) rules"
}
$totalAdd = ($plan | ForEach-Object { $_.Add.Count } | Measure-Object -Sum).Sum
$totalRemove = ($plan | ForEach-Object { $_.Remove.Count } | Measure-Object -Sum).Sum + $claude.Count
Write-Host "$verb $totalAdd rules, $verb2 $totalRemove rules"
exit 0
