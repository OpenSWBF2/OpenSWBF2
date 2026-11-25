param(
    [Parameter(Mandatory)][ValidateSet('Win32', 'x64')][string]$Platform,
    [Parameter(Mandatory)][ValidateSet('Running', 'Closed')][string]$SteamState,
    [switch]$HogAddrSpace,
    [Parameter(Mandatory)][string]$OutDir
)
$ErrorActionPreference = 'Stop'
$testsDir = Split-Path $PSScriptRoot -Parent
$root = Split-Path $testsDir -Parent
. (Join-Path $testsDir 'hardening.ps1')
. (Join-Path $testsDir 'gate\keys.ps1')
$steamExe = 'C:\Program Files (x86)\Steam\steam.exe'

$out = [IO.Path]::GetFullPath($OutDir).TrimEnd('\')
$baseDir = [IO.Path]::GetFullPath((Join-Path $testsDir 'baseline')).TrimEnd('\')
if ($out -ieq $baseDir -or $out.StartsWith($baseDir + '\', [StringComparison]::OrdinalIgnoreCase)) { Write-Host "[FAIL] capture-commit - -OutDir $out is inside tests\baseline; capture into a scratch folder"; exit 2 }

$blockers = @()
$blockers += @(Get-Process -ErrorAction SilentlyContinue | Where-Object { $_.Name -like 'BattlefrontII*' } | ForEach-Object { "game process $($_.Name) ($($_.Id))" })
$blockers += @(Get-CimInstance Win32_Process -Filter "Name='powershell.exe' OR Name='pwsh.exe'" -ErrorAction SilentlyContinue | Where-Object { $_.ProcessId -ne $PID -and [string]$_.CommandLine -imatch '[\\/](gate|run)\.ps1\b' } | ForEach-Object { "harness process $($_.ProcessId): $($_.CommandLine)" })
if ($blockers.Count) {
    Write-Host '[FAIL] capture-commit - a commit capture runs alone; refusing beside:'
    foreach ($b in $blockers) { Write-Host "    $b" }
    exit 2
}

$was = Get-SteamRunning
if ($SteamState -ceq 'Running' -and $was -cne 'yes') { Write-Host "[FAIL] capture-commit - -SteamState Running needs Steam running and logged in; Get-SteamRunning is $was"; exit 2 }
$want = $(if ($SteamState -ceq 'Running') { 'yes' } else { 'no' })
if ($HogAddrSpace.IsPresent -ne ($Platform -ceq 'x64')) { Write-Host '[FAIL] capture-commit - Game::Standard::commit-flat runs under -hogaddrspace on x64 only; pass -HogAddrSpace exactly with -Platform x64'; exit 2 }
$hog = [string][int]$HogAddrSpace.IsPresent
$seen = $null
$runDir = $null
$runCode = -1
try {
    if ($SteamState -ceq 'Closed' -and @(Get-Process -Name 'steam' -ErrorAction SilentlyContinue).Count) {
        & $steamExe -shutdown
        $deadline = (Get-Date).AddMinutes(3)
        while (@(Get-Process -Name 'steam' -ErrorAction SilentlyContinue).Count -and (Get-Date) -lt $deadline) { Start-Sleep -Seconds 2 }
        if (@(Get-Process -Name 'steam' -ErrorAction SilentlyContinue).Count) { throw 'Steam did not exit within 3 minutes of -shutdown' }
    }
    $seen = Get-SteamRunning
    $runDir = New-StampedDir (Join-Path $root 'artifacts\tests')
    $testsExe = Join-Path $root "bin\Tests\$Platform\Test\OpenSWBF2Tests.exe"
    Push-Location -LiteralPath $testsDir
    try {
        & $testsExe --runner "--out=$runDir" "--platform=$Platform" '--config=Test' "--exclude=$(Join-Path $testsDir 'excluded.tsv')" '--tags=*[Standard]*' '--name=Game::Standard::commit-flat' "--game-dir=$(Join-Path $root "bin\$Platform\Test")" | ForEach-Object { Write-Host $_ }
        $runCode = $LASTEXITCODE
    }
    finally { Pop-Location }
}
finally {
    if ($was -ceq 'yes' -and (Get-SteamRunning) -cne 'yes') {
        Start-Process -FilePath $steamExe -ArgumentList '-silent'
        $deadline = (Get-Date).AddMinutes(5)
        while ((Get-SteamRunning) -cne 'yes' -and (Get-Date) -lt $deadline) { Start-Sleep -Seconds 3 }
        Write-Host "capture-commit: Steam restarted, Get-SteamRunning is $(Get-SteamRunning)"
    }
}

if (-not $runDir) { Write-Host "[FAIL] capture-commit - the test runner did not start"; exit 1 }
if ($seen -cne $want) { Write-Host "[FAIL] capture-commit - Steam was steam=$seen at the run, -SteamState $SteamState needs steam=$want"; exit 1 }
$settled = 8
$logs = @(Get-ChildItem -LiteralPath $runDir -Recurse -File -Filter 'run.log' | Where-Object { $_.Directory.Name -ceq 'commit-flat' })
if ($logs.Count -ne 1) { Write-Host "[FAIL] capture-commit - $($logs.Count) commit-flat run logs under $runDir"; exit 1 }
$log = $logs[0].FullName
$parts = @([regex]::Matches([IO.File]::ReadAllText($log), 'Commit parts state=(\w+) private=\d+ gamecrt=(\d+) heaps=\d+ threads=\d+ threadcount=\d+ arena=(\d+) crtbusy=\d+ crtcommitted=\d+ crtlarge=\d+ gamevm=(\d+)') | ForEach-Object { ,@($_.Groups[1].Value, [long]$_.Groups[3].Value, ([long]$_.Groups[4].Value + [long]$_.Groups[2].Value)) })
$rows = @()
$bad = @()
foreach ($s in 'mission', 'shell') {
    $v = @($parts | Where-Object { $_[0] -ceq $s } | Select-Object -Skip $settled)
    if ($v.Count -eq 0) { $bad += "$s has no COMMITPARTS rows after cycle $settled"; continue }
    $arena = @($v | ForEach-Object { $_[1] } | Sort-Object -Unique)
    $rest = @($v | ForEach-Object { $_[2] } | Sort-Object -Unique)
    if ($arena.Count -ne 1) { $bad += "$s arena after cycle $settled is not one value: $($arena -join ', ')" }
    if ($rest.Count -ne 1) { $bad += "$s rest after cycle $settled is not one value: $($rest -join ', ')" }
    $rows += "$want`t$hog`t$s`t$($arena[0])`t$($rest[0])"
}
if ($bad.Count) {
    Write-Host "[FAIL] capture-commit - $runDir"
    foreach ($b in $bad) { Write-Host "    $b" }
    exit 1
}
New-Item -ItemType Directory -Force -Path $out | Out-Null
$sha = (& git -C $root rev-parse --short=8 HEAD).Trim()
$text = (@("# commit $sha", "steam`thog`tstate`tarena`trest") + $rows) -join "`r`n"
[IO.File]::WriteAllText((Join-Path $out 'commit.txt'), $text + "`r`n")
[IO.File]::WriteAllText((Join-Path $out 'run.txt'), "run=$runDir`r`nexit=$runCode`r`n")
Write-Host "capture-commit: wrote $(Join-Path $out 'commit.txt') from $runDir (test runner exit $runCode)"
foreach ($r in $rows) { Write-Host "    $r" }
if ($runCode -ne 0) { Write-Host "[FAIL] capture-commit - the test runner exited $runCode"; exit 1 }
exit 0
