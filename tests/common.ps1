if ($PSScriptRoot) { . (Join-Path $PSScriptRoot 'vstools.ps1') }
$script:HarnessCommonLoaded = $true
$script:SmokePlatform = 'Win32'
$script:AsanRuntimeNames = @{ Win32 = 'clang_rt.asan_dynamic-i386.dll'; x64 = 'clang_rt.asan_dynamic-x86_64.dll' }
$script:WorkerPlatforms = @('Win32', 'x64')
$script:WorkerConfigurations = @('Debug', 'Release', 'Test', 'Asan')
$script:WorkerKinds = @('smoke')
$script:WorkerIndexMax = [Environment]::ProcessorCount

function Get-AsanRuntimeName { return $script:AsanRuntimeNames[$script:SmokePlatform] }

function Find-GameDataRoot {
    $candidate = $null
    if ($env:SWBF2_GAMEDATA) {
        $candidate = $env:SWBF2_GAMEDATA
    }
    else {
        foreach ($view in 'Registry64', 'Registry32') {
            $base = $null
            $sub = $null
            try {
                $base = [Microsoft.Win32.RegistryKey]::OpenBaseKey('LocalMachine', $view)
                $sub = $base.OpenSubKey('SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 6060')
                if ($sub) {
                    $loc = $sub.GetValue('InstallLocation')
                    if ($loc) { $candidate = "$loc\GameData"; break }
                }
            }
            catch {}
            finally {
                if ($sub) { $sub.Close() }
                if ($base) { $base.Close() }
            }
        }
    }
    if ($candidate -and (Test-Path -LiteralPath (Join-Path $candidate 'data\_lvl_pc') -PathType Container)) { return $candidate }
    return $null
}

$script:SmokeRequiredDlls = @{ Win32 = @('Galaxy.dll', 'GalaxyPeer.dll', 'openswbf2-ffmpeg.dll', 'steam_api.dll'); x64 = @('openswbf2-ffmpeg.dll') }

function Get-SmokePrecheck($gameDataRoot, $exeDir, $asan = $false, [ValidateSet('Win32', 'x64')][string]$platform = $script:SmokePlatform) {
    if (-not $gameDataRoot) { return 'no retail GameData found (set SWBF2_GAMEDATA or install Steam App 6060)' }
    if (-not (Test-Path -LiteralPath (Join-Path $exeDir 'BattlefrontII.exe'))) { return "BattlefrontII.exe not found in $exeDir" }
    $dlls = @($script:SmokeRequiredDlls[$platform])
    if ($asan) { $dlls += $script:AsanRuntimeNames[$platform] }
    foreach ($dll in $dlls) {
        if (-not (Test-Path -LiteralPath (Join-Path $exeDir $dll))) { return "$dll missing beside the exe in $exeDir" }
    }
    $data = Get-Item -LiteralPath (Join-Path $exeDir 'data') -Force -ErrorAction SilentlyContinue
    if (-not $data -or ($data.Attributes -band [IO.FileAttributes]::ReparsePoint) -or -not (Test-Path -LiteralPath (Join-Path $exeDir 'data\_lvl_pc\shell.lvl'))) { return "staged data missing in $exeDir (build without SWBF2_NOSTAGE)" }
    return $null
}

function New-StampedDir([string]$Parent) {
    [void][IO.Directory]::CreateDirectory($Parent)
    $m = New-Object System.Threading.Mutex($false, 'Local\OpenSWBF2-stamped-dir')
    try { [void]$m.WaitOne() }
    catch [System.Threading.AbandonedMutexException] {}
    try {
        $base = Get-Date -Format 'yyyyMMdd-HHmmss'
        $name = $base
        $i = 1
        while ([IO.Directory]::Exists((Join-Path $Parent $name))) { $i++; $name = "$base-$i" }
        $dir = Join-Path $Parent $name
        [void][IO.Directory]::CreateDirectory($dir)
        return $dir
    }
    finally {
        $m.ReleaseMutex()
        $m.Dispose()
    }
}

function Get-WorkerPath([string]$Root, [string]$Platform, [string]$Configuration, [string]$Kind, [int]$Index) {
    if (-not $Root) { throw 'worker path needs a root' }
    if ($script:WorkerPlatforms -cnotcontains $Platform) { throw "worker platform '$Platform' is not one of $($script:WorkerPlatforms -join ', ')" }
    if ($script:WorkerConfigurations -cnotcontains $Configuration) { throw "worker configuration '$Configuration' is not one of $($script:WorkerConfigurations -join ', ')" }
    if ($script:WorkerKinds -cnotcontains $Kind) { throw "worker kind '$Kind' is not one of $($script:WorkerKinds -join ', ')" }
    if ($Index -lt 1 -or $Index -gt $script:WorkerIndexMax) { throw "worker index $Index is outside 1..$($script:WorkerIndexMax)" }
    return Join-Path $Root "artifacts\smoke-workers\$Platform\$Configuration\$Kind\w$Index"
}

function Get-ReparsePoints([string]$dir) {
    $found = New-Object System.Collections.Generic.List[string]
    $top = New-Object System.IO.DirectoryInfo $dir
    if (-not $top.Exists) { return }
    if (($top.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { return $top.FullName }
    $pending = New-Object System.Collections.Generic.Stack[System.IO.DirectoryInfo]
    $pending.Push($top)
    while ($pending.Count -gt 0) {
        $cur = $pending.Pop()
        foreach ($e in $cur.EnumerateFileSystemInfos()) {
            if (($e.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { $found.Add($e.FullName) }
            elseif (($e.Attributes -band [IO.FileAttributes]::Directory) -ne 0) { $pending.Push((New-Object System.IO.DirectoryInfo $e.FullName)) }
        }
    }
    return $found.ToArray()
}

function Get-DigestKey($caseDir) {
    $f = Join-Path $caseDir 'run.log'
    if (-not (Test-Path -LiteralPath $f)) { return $null }
    $m = Select-String -LiteralPath $f -Pattern 'Sim digest .*' | Select-Object -Last 1
    if (-not $m) { return $null }
    return $m.Matches[0].Value
}
