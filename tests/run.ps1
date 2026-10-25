param(
    [string]$Configuration = 'Debug',
    [switch]$NoBuild,
    [string]$Suite,
    [string]$Filter,
    [switch]$Fuzz,
    [int]$Seed = 1,
    [int]$Iterations = 100000,
    [int]$Timeout = 10
)
$ErrorActionPreference = 'Stop'
$testsDir = $PSScriptRoot
$root = Split-Path $testsDir -Parent

foreach ($dll in 'd3dx9_43.dll', 'msvcr120.dll') {
    if (-not (Test-Path (Join-Path $env:windir "SysWOW64\$dll"))) {
        Write-Host "Preflight failed: $dll missing from SysWOW64. Install the DirectX June 2010 redist and the VS2013 (vcredist 2013 x86) runtime."
        exit 2
    }
}

if (-not $NoBuild) {
    $msbuild = Join-Path ${env:ProgramFiles(x86)} 'MSBuild\12.0\Bin\MSBuild.exe'
    if (-not (Test-Path $msbuild)) {
        $key = 'HKLM:\SOFTWARE\WOW6432Node\Microsoft\MSBuild\ToolsVersions\12.0'
        if (Test-Path $key) { $msbuild = Join-Path (Get-ItemProperty $key).MSBuildToolsPath 'MSBuild.exe' }
    }
    if (-not (Test-Path $msbuild)) { Write-Host 'MSBuild 12.0 not found (install VS2013 Update 5).'; exit 2 }
    & $msbuild (Join-Path $testsDir 'OpenSWBF2Tests.vcxproj') "/p:Configuration=$Configuration" '/p:Platform=Win32' /m /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) { Write-Host 'Build failed.'; exit 2 }
}

$exe = $env:OPENSWBF2_TEST_EXE
if (-not $exe) { $exe = Join-Path $root "bin\Tests\$Configuration\OpenSWBF2Tests.exe" }
if (-not (Test-Path $exe)) { Write-Host "Test exe not found: $exe"; exit 2 }

$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$outDir = Join-Path $root "artifacts\tests\$stamp"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null
Set-Content -Path (Join-Path $root 'artifacts\.gitignore') -Value '*' -Encoding ASCII

function Start-Exe([string]$argLine) {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exe
    $psi.Arguments = $argLine
    $psi.WorkingDirectory = $testsDir
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    $p = [System.Diagnostics.Process]::Start($psi)
    $o = $p.StandardOutput.ReadToEndAsync()
    $e = $p.StandardError.ReadToEndAsync()
    return @{ Proc = $p; Out = $o; Err = $e }
}

$allSuites = 'unit', 'characterisation', 'regression', 'known_bug', 'corpus', 'fuzz', 'stress'
if ($Fuzz) { $suites = @('stress') }
elseif ($Suite) { $suites = @($Suite) }
else { $suites = $allSuites | Where-Object { $_ -ne 'stress' } }

$cases = @()
$listMs = 30000
if ($env:OPENSWBF2_LIST_TIMEOUT_MS) { $listMs = [int]$env:OPENSWBF2_LIST_TIMEOUT_MS }
foreach ($s in $suites) {
    $h = Start-Exe "-ts=$s --list-test-cases"
    if (-not $h.Proc.WaitForExit($listMs)) {
        try { $h.Proc.Kill() } catch {}
        Write-Host "Listing timed out for suite '$s'."
        exit 2
    }
    if ($h.Proc.ExitCode -ne 0) {
        Write-Host "Listing failed for suite '$s' (exit code $($h.Proc.ExitCode))."
        exit 2
    }
    $lines = ($h.Out.Result -split "`r?`n")
    $in = $false
    $before = $cases.Count
    foreach ($l in $lines) {
        if ($l -like '=====*') { $in = -not $in; continue }
        if ($in -and $l.Trim() -ne '') { $cases += New-Object PSObject -Property @{ Name = $l.TrimEnd(); Suite = $s } }
    }
    if ($cases.Count -eq $before -and -not $Filter -and $s -ne 'regression') {
        Write-Host "Suite '$s' listed 0 cases."
        exit 2
    }
}
if ($Filter) { $cases = @($cases | Where-Object { $_.Name -like $Filter }) }
if ($cases.Count -eq 0) { Write-Host 'No cases run (suite empty or filter matched nothing).'; exit 2 }

$results = @()
$bad = 0
foreach ($c in $cases) {
    $argLine = '"-tc=' + ($c.Name -replace '"', '\"') + '" --timeout ' + $Timeout
    if ($Fuzz) { $argLine += " --fuzz-seed=$Seed --fuzz-count=$Iterations" }
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $h = Start-Exe $argLine
    $outerMs = [int64]($Timeout + 10) * 1000
    if ($Fuzz) { $outerMs = [Math]::Max($outerMs, [int64]$Iterations / 5 + 60000) }
    $outer = $h.Proc.WaitForExit([int][Math]::Min($outerMs, [int]::MaxValue))
    if (-not $outer) { try { $h.Proc.Kill() } catch {}; $h.Proc.WaitForExit() }
    $sw.Stop()
    $text = $h.Out.Result + $h.Err.Result
    $code = [BitConverter]::ToUInt32([BitConverter]::GetBytes([int]$h.Proc.ExitCode), 0)
    $detail = ''
    if (-not $outer) { $st = 'TIMEOUT' }
    elseif ($code -eq 0) {
        if ($text -match 'test cases:\s+1 \|\s+1 passed') { $st = 'PASS' }
        else { $st = 'FAIL'; $detail = 'case did not run exactly once' }
    }
    elseif ($code -eq 1) { $st = 'FAIL' }
    elseif ($code -eq 77) { $st = 'SKIP' }
    elseif ($code -eq 124) { $st = 'TIMEOUT' }
    else { $st = 'CRASH'; $detail = ('0x{0:X8}' -f $code) }

    $known = ($c.Suite -eq 'known_bug')
    $ms = $sw.ElapsedMilliseconds
    $safe = ($c.Name -replace '[^A-Za-z0-9_.-]', '_')
    $file = "$safe.txt"
    $n = 2
    while (Test-Path (Join-Path $outDir $file)) { $file = "$safe-$n.txt"; $n++ }
    Set-Content -Path (Join-Path $outDir $file) -Value $text -Encoding ASCII

    if ($known -and ($st -eq 'TIMEOUT' -or ($st -eq 'FAIL' -and $code -eq 1 -and -not $detail -and $text -match 'test cases:\s+1 \|'))) {
        $under = $st
        $line = "[KNOWN] $($c.Name) ($under) ($ms ms)"
        $final = 'KNOWN'
    }
    else {
        $line = "[$st] $($c.Name) ($ms ms)"
        if ($detail -and $st -ne 'CRASH') { $line += " - $detail" }
        elseif ($st -eq 'CRASH') { $line = "[CRASH] $($c.Name) $detail ($ms ms)" }
        $final = $st
        if ($st -ne 'PASS' -and $st -ne 'SKIP') { $bad++ }
    }
    Write-Host $line
    $results += New-Object PSObject -Property @{ Line = $line; Final = $final; Known = $known }
    if ($final -in 'FAIL', 'TIMEOUT', 'CRASH', 'KNOWN') { Write-Host "    details: $(Join-Path $outDir $file)" }
    if ($known -and $st -eq 'PASS') {
        $w = "WARNING: known_bug case passed, bug may be fixed: $($c.Name)"
        Write-Host $w
        $results += New-Object PSObject -Property @{ Line = $w; Final = 'WARN'; Known = $true }
    }
}

$normal = @($results | Where-Object { -not $_.Known -and $_.Final -ne 'WARN' })
$kn = @($results | Where-Object { $_.Known -and $_.Final -ne 'WARN' })
$summary = @()
$summary += ''
$summary += 'Results'
foreach ($t in 'PASS', 'FAIL', 'SKIP', 'TIMEOUT', 'CRASH') {
    $summary += ('  {0,-8} {1}' -f $t, @($normal | Where-Object { $_.Final -eq $t }).Count)
}
$summary += 'Known bugs'
$summary += ('  {0,-8} {1}' -f 'KNOWN', @($kn | Where-Object { $_.Final -eq 'KNOWN' }).Count)
$summary += ('  {0,-8} {1}' -f 'PASS', @($kn | Where-Object { $_.Final -eq 'PASS' }).Count)
$summary | ForEach-Object { Write-Host $_ }
(($results | ForEach-Object { $_.Line }) + $summary) | Set-Content -Path (Join-Path $outDir 'results.txt') -Encoding ASCII
Write-Host "Artifacts: $outDir"

if ($bad -gt 0) { exit 1 }
exit 0
