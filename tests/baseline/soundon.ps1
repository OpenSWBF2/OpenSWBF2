param(
    [string[]]$Missions = @('geo1c_xl', 'kas2c_xl', 'hot1g_xl'),
    [string]$Control = 'geo1c_xl',
    [string]$OutDir,
    [string]$PollExe,
    [string]$PollOut,
    [string]$PollStop,
    [ValidateSet('Win32', 'x64')][string]$Platform = 'Win32'
)
$ErrorActionPreference = 'Stop'

if ($PollExe) {
    $started = $false
    $opened = $false
    while (-not $opened -and -not (Test-Path -LiteralPath $PollStop)) {
        $procs = @(Get-Process -Name 'BattlefrontII' -ErrorAction SilentlyContinue | Where-Object { try { $_.Path -ieq $PollExe } catch { $false } })
        if ($procs.Count -gt 0) {
            $started = $true
            foreach ($p in $procs) {
                try { if (@($p.Modules | Where-Object { $_.ModuleName -ieq 'AudioSes.dll' }).Count -gt 0) { $opened = $true } } catch {}
            }
        }
        elseif ($started) { break }
        if (-not $opened) { Start-Sleep -Milliseconds 250 }
    }
    $state = 'not opened'
    if ($opened) { $state = 'opened' }
    Set-Content -LiteralPath $PollOut -Value $state -Encoding ASCII
    if (-not $started) { exit 3 }
    exit 0
}

$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
. (Join-Path $root 'tests\smoke.ps1')
$script:SmokePlatform = $Platform
$exeDir = Join-Path $root "bin\$Platform\Test"
$exe = Join-Path $exeDir 'BattlefrontII.exe'
$reason = Get-SmokePrecheck (Find-GameDataRoot) $exeDir
if ($reason) { Write-Host "[SKIP] soundon - $reason"; exit 0 }
if (-not $OutDir) { $OutDir = New-StampedDir (Join-Path $root 'artifacts\spike32\soundon') }
$ps32 = Join-Path $env:windir 'System32\WindowsPowerShell\v1.0\powershell.exe'
$wow = Join-Path $env:windir 'SysWOW64\WindowsPowerShell\v1.0\powershell.exe'
if ($Platform -eq 'Win32' -and (Test-Path -LiteralPath $wow)) { $ps32 = $wow }

$runs = @()
foreach ($m in $Missions) { $runs += New-Object PSObject -Property @{ Name = $m; Dir = (Join-Path $OutDir "smoke\$m"); Render = ''; Want = 'opened' } }
if ($Control) { $runs += New-Object PSObject -Property @{ Name = $Control; Dir = (Join-Path $OutDir "control\$Control"); Render = '-norender -nosound '; Want = 'not opened' } }

$bad = 0
foreach ($r in $runs) {
    New-Item -ItemType Directory -Force -Path $r.Dir | Out-Null
    $soundFile = Join-Path $r.Dir 'sound.txt'
    $stopFile = Join-Path $r.Dir 'sound-stop.txt'
    Remove-Item -LiteralPath $stopFile -Force -ErrorAction SilentlyContinue
    $argLine = "$($r.Name) $($r.Render)-fixedrate 30 -nowait -logfinalscore -DecompCONMinutes 5 -seed 1 -exitongameover -nofocuspause -log ""$($r.Dir)\run.log"" -logcmds ""LogMemory Verbose"" -hangtimeout 0"
    $poll = Start-Process -FilePath $ps32 -ArgumentList @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', """$PSCommandPath""", '-PollExe', """$exe""", '-PollOut', """$soundFile""", '-PollStop', """$stopFile""") -WindowStyle Hidden -PassThru
    $res = Invoke-SmokeCase $r.Name $exe $exeDir $argLine $r.Dir
    Set-Content -LiteralPath $stopFile -Value 'exited' -Encoding ASCII
    $poll.WaitForExit()
    $sound = ''
    if (Test-Path -LiteralPath $soundFile) { $sound = (Get-Content -LiteralPath $soundFile -TotalCount 1) }
    $offLine = [regex]::IsMatch($res.LogText, 'OPENSWBF2 MEMREPORT offthread count=\d+ sources=\d+ dropped=\d+')
    $ok = ($res.Class -eq 'Pass') -and ($sound -eq $r.Want) -and $offLine
    if (-not $ok) { $bad++ }
    $tok = 'PASS'
    if (-not $ok) { $tok = 'FAIL' }
    Write-Host ('[{0}] soundon:{1} class={2} sound={3} offthread={4} ({5} ms)' -f $tok, $r.Name, $res.Class, $sound, [int]$offLine, $res.Ms)
}
Write-Host "Artifacts: $OutDir"
if ($bad -gt 0) { exit 1 }
exit 0
