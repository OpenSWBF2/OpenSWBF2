param(
    [string[]]$Missions = @('geo1c_xl', 'kas2c_xl', 'hot1g_xl'),
    [string]$Control = 'geo1c_xl',
    [string]$OutDir,
    [ValidateSet('Win32', 'x64')][string]$Platform = 'Win32'
)
$ErrorActionPreference = 'Stop'

function Get-SoundOnProblems([string]$LogText, [string]$Class, [bool]$Sound, [string]$Platform, [string]$AddrSpaceRx) {
    $p = @()
    if ($Class -cne 'Pass') { $p += "class=$Class" }
    $opened = $LogText -cmatch 'OPENSWBF2 SOUND open=ok'
    if ($opened -ne $Sound) { $p += "SOUND open=ok present=$([int]$opened)" }
    if ($LogText -cnotmatch 'OPENSWBF2 MEMREPORT offthread count=\d+ sources=\d+ dropped=\d+') { $p += 'no offthread line' }
    if ($LogText -cmatch 'OPENSWBF2 CURSOR warp') { $p += 'CURSOR warp' }
    if ($LogText -cmatch 'OPENSWBF2 FOCUS activated') { $p += 'FOCUS activated' }
    if ($Platform -ceq 'x64' -and ([string]::IsNullOrEmpty($AddrSpaceRx) -or $LogText -cnotmatch $AddrSpaceRx)) { $p += 'no ADDRSPACE line' }
    return $p
}

$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
. (Join-Path $root 'tests\smoke.ps1')
$script:SmokePlatform = $Platform
$exeDir = Join-Path $root "bin\$Platform\Test"
$exe = Join-Path $exeDir 'BattlefrontII.exe'
$reason = Get-SmokePrecheck (Find-GameDataRoot) $exeDir
if ($reason) { Write-Host "[SKIP] soundon - $reason"; exit 0 }
if (-not $OutDir) { $OutDir = New-StampedDir (Join-Path $root 'artifacts\spike32\soundon') }
$OutDir = (New-Item -ItemType Directory -Force -Path $OutDir).FullName
$hog = ''
if ($Platform -eq 'x64') { $hog = '-hogaddrspace ' }

$runs = @()
foreach ($m in $Missions) { $runs += New-Object PSObject -Property @{ Name = $m; Dir = (Join-Path $OutDir "smoke\$m"); Render = '-handsoff -win '; Sound = $true } }
if ($Control) { $runs += New-Object PSObject -Property @{ Name = $Control; Dir = (Join-Path $OutDir "control\$Control"); Render = '-norender -nosound '; Sound = $false } }

$bad = 0
foreach ($r in $runs) {
    New-Item -ItemType Directory -Force -Path $r.Dir | Out-Null
    $argLine = "$($r.Name) $($r.Render)$hog-fixedrate 30 -nowait -logfinalscore -DecompCONMinutes 5 -seed 1 -exitongameover -nofocuspause -log ""$($r.Dir)\run.log"" -logcmds ""LogMemory Verbose"" -hangtimeout 0"
    $res = Invoke-SmokeCase $r.Name $exe $exeDir $argLine $r.Dir
    $log = [string]$res.LogText
    $sound = 'not opened'
    if ($log -cmatch 'OPENSWBF2 SOUND open=ok') { $sound = 'opened' }
    Set-Content -LiteralPath (Join-Path $r.Dir 'sound.txt') -Value $sound -Encoding ASCII
    $problems = @(Get-SoundOnProblems $log $res.Class $r.Sound $Platform $script:SmokeAddrSpaceRxX64)
    $offLine = [regex]::IsMatch($log, 'OPENSWBF2 MEMREPORT offthread count=\d+ sources=\d+ dropped=\d+')
    $tok = 'PASS'
    if ($problems.Count -gt 0) { $tok = 'FAIL'; $bad++ }
    Write-Host ('[{0}] soundon:{1} class={2} sound={3} offthread={4} ({5} ms){6}' -f $tok, $r.Name, $res.Class, $sound, [int]$offLine, $res.Ms, $(if ($problems.Count) { ' - ' + ($problems -join ', ') } else { '' }))
}
Write-Host "Artifacts: $OutDir"
if ($bad -gt 0) { exit 1 }
exit 0
