$script:VsToolsByPlatform = @{}
$T1Runtimes = 'd3dx9_43.dll', 'vcruntime140.dll', 'msvcp140.dll'
$GameRuntimes = 'vcruntime140.dll', 'msvcp140.dll'
$GalaxyRuntimes = 'msvcr120.dll', 'msvcp120.dll'
$GalaxyOffGroups = 'Vendor', 'None'

function Get-VsTools([ValidateSet('Win32','x64')][string]$Platform = 'Win32') {
    $tools = Find-VsTools $Platform
    if (-not $tools) {
        Write-Host 'Visual Studio 2022 with the C++ x86/x64 build tools not found.'
        exit 2
    }
    return $tools
}

function Get-VsToolsTarget([ValidateSet('Win32','x64')][string]$Platform) {
    if ($Platform -eq 'x64') { return 'x64' }
    return 'x86'
}

function Find-VsTools([ValidateSet('Win32','x64')][string]$Platform = 'Win32') {
    if ($script:VsToolsByPlatform.ContainsKey($Platform)) { return $script:VsToolsByPlatform[$Platform] }
    $vs = $null
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (Test-Path -LiteralPath $vswhere) {
        $vs = & $vswhere -latest -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | Select-Object -First 1
    }
    $tools = $null
    if ($vs) {
        $verFile = Join-Path $vs 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt'
        if (Test-Path -LiteralPath $verFile) {
            $ver = (Get-Content -LiteralPath $verFile | Select-Object -First 1).Trim()
            $tools = New-Object PSObject -Property @{
                Install = $vs
                ToolsVersion = $ver
                Bin = Join-Path $vs "VC\Tools\MSVC\$ver\bin\HostX86\$(Get-VsToolsTarget $Platform)"
                MSBuild = Join-Path $vs 'MSBuild\Current\Bin\MSBuild.exe'
            }
        }
    }
    if (-not $tools -or -not (Test-Path -LiteralPath $tools.Bin) -or -not (Test-Path -LiteralPath $tools.MSBuild)) { return $null }
    $script:VsToolsByPlatform[$Platform] = $tools
    return $tools
}

function Find-MSBuild { return (Get-VsTools).MSBuild }

function Find-VcBin([ValidateSet('Win32','x64')][string]$Platform = 'Win32') { return (Get-VsTools $Platform).Bin }

function Find-Dumpbin([ValidateSet('Win32','x64')][string]$Platform = 'Win32') { return (Join-Path (Find-VcBin $Platform) 'dumpbin.exe') }

function Find-Editbin([ValidateSet('Win32','x64')][string]$Platform = 'Win32') { return (Join-Path (Find-VcBin $Platform) 'editbin.exe') }

function Get-MissingRuntimes([string]$dir, [string[]]$dlls) {
    return @($dlls | Where-Object { -not (Test-Path -LiteralPath (Join-Path $dir $_)) })
}

function Get-PlatformRuntimes([string[]]$dlls, [ValidateSet('Win32','x64')][string]$Platform = 'Win32') {
    if ($Platform -eq 'x64') { return @($dlls) + 'vcruntime140_1.dll' }
    return @($dlls)
}

function Get-GameRuntimes([ValidateSet('Win32','x64')][string]$Platform, [string[]]$Groups) {
    if ($Platform -eq 'Win32' -and @($Groups | Where-Object { $_ -and $GalaxyOffGroups -notcontains $_ }).Count) { return @($GameRuntimes) + $GalaxyRuntimes }
    return @($GameRuntimes)
}

function Assert-Runtimes([string[]]$dlls, [ValidateSet('Win32','x64')][string]$Platform = 'Win32') {
    $dir = Get-SystemDllDir $Platform
    $missing = @(Get-MissingRuntimes $dir (Get-PlatformRuntimes $dlls $Platform))
    if ($missing.Count -gt 0) {
        $arch = Get-VsToolsTarget $Platform
        Write-Host "Preflight failed: missing from $(Split-Path $dir -Leaf): $($missing -join ', '). Install the DirectX June 2010 redist, the VC++ 2015-2022 $arch redistributable and the VS2013 $arch runtime."
        exit 2
    }
}

function Get-SystemDllDir([ValidateSet('Win32','x64')][string]$Platform = 'Win32') {
    if ($Platform -eq 'Win32') { return Join-Path $env:windir 'SysWOW64' }
    if ([Environment]::Is64BitProcess) { return Join-Path $env:windir 'System32' }
    return Join-Path $env:windir 'Sysnative'
}

function Get-ToolchainInfo([ValidateSet('Win32','x64')][string]$Platform = 'Win32') {
    $t = Get-VsTools $Platform
    $cl = Join-Path $t.Bin 'cl.exe'
    $banner = [string](@(& cmd /c "`"$cl`" 2>&1" | Where-Object { $_ -and $_.Trim() }) | Select-Object -First 1)
    $clVersion = ''
    if ($banner -match 'Version (\S+)') { $clVersion = $Matches[1] }
    $ucrt = Join-Path (Get-SystemDllDir $Platform) 'ucrtbase.dll'
    $ucrtVersion = ''
    if (Test-Path -LiteralPath $ucrt) { $ucrtVersion = (Get-Item -LiteralPath $ucrt).VersionInfo.FileVersion }
    $nt = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion'
    New-Object PSObject -Property @{ Banner = $banner; Cl = $clVersion; ToolsVersion = $t.ToolsVersion; Ucrt = $ucrtVersion; Windows = "$($nt.CurrentBuild).$($nt.UBR)" }
}

function Write-ToolBanner([ValidateSet('Win32','x64')][string]$Platform = 'Win32') {
    $i = Get-ToolchainInfo $Platform
    Write-Host "toolchain: $($i.Banner) (MSVC $($i.ToolsVersion))"
    Write-Host "ucrtbase: $($i.Ucrt)"
    Write-Host "windows: $($i.Windows)"
}
