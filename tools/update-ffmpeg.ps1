param(
    [string]$Version = '9.0.2',
    [string]$Sha256 = '8c3850283eb25fa026482078a04051e0be17347b09ef81a0849bec15a96e002e',
    [string]$KeyFingerprint = 'FCF986EA15E6E293A5644F10B4322F04D67658D8',
    [string]$PatchSha256 = '8e0158c1095d60f704d4aadd231066081f34c93934960666d92e6b390d343064',
    [string]$TxPatchSha256 = '67cc7007829f377493d9c5f4f7fae718765e1242dc1d09a2c63619724fa16e02',
    [string]$WorkDir,
    [string]$OutDir,
    [switch]$Verify
)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$root = Split-Path $PSScriptRoot -Parent
function Get-VsTools([ValidateSet('Win32','x64')][string]$Platform = 'Win32') {
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
                Bin = Join-Path $vs "VC\Tools\MSVC\$ver\bin\HostX86\$(if ($Platform -eq 'x64') { 'x64' } else { 'x86' })"
                MSBuild = Join-Path $vs 'MSBuild\Current\Bin\MSBuild.exe'
            }
        }
    }
    if (-not $tools -or -not (Test-Path -LiteralPath $tools.Bin) -or -not (Test-Path -LiteralPath $tools.MSBuild)) {
        Write-Host 'Visual Studio 2022 with the C++ x86/x64 build tools not found.'
        exit 2
    }
    return $tools
}
if (-not $WorkDir) { $WorkDir = Join-Path $root 'artifacts\ffmpeg' }
if (-not $OutDir) { $OutDir = Join-Path $root 'src\FFmpegFL' }
$WorkDir = [IO.Path]::GetFullPath($WorkDir)
$OutDir = [IO.Path]::GetFullPath($OutDir)
$patchCommit = '5195d6f9729ad00b93e4d0f0bea309416fb4713e'
$patches = @(
    @{ File = (Join-Path $root 'src\FFmpegFL\patches\5195d6f9.patch'); Sha256 = $PatchSha256; Line = "- Local patch: patches/5195d6f9.patch, upstream commit $patchCommit (avcodec/bink: check init_get_bits8() for failure, CVE-2026-125068), SHA-256 {0}" },
    @{ File = (Join-Path $root 'src\FFmpegFL\patches\tx-subtx-free.patch'); Sha256 = $TxPatchSha256; Line = '- Local patch: patches/tx-subtx-free.patch, not upstream (avutil/tx: free the sub-transform array of a failed codelet; an allocation failure in a binkaudio_dct open leaked one block, cause C-FFMPEG-TX-LEAK in tests/baseline/v143/CAUSES.md), SHA-256 {0}' }
)
$archs = @(
    @{ Platform = 'Win32'; VcArch = 'x86'; Arch = 'x86'; Os = 'win32' },
    @{ Platform = 'x64'; VcArch = 'x64'; Arch = 'x86_64'; Os = 'win64' }
)
$commonArgs = @('--toolchain=msvc', '--enable-static', '--disable-shared', '--disable-programs', '--disable-doc', '--disable-debug', '--disable-autodetect', '--disable-network', '--disable-asm', '--disable-pthreads', '--disable-w32threads', '--disable-os2threads', '--disable-everything', '--disable-avdevice', '--disable-avfilter', '--disable-swresample', '--disable-swscale', '--enable-demuxer=bink', '--enable-decoder=bink,binkaudio_dct,binkaudio_rdft', '--malloc-prefix=ossf2_')
$defines = @('HAVE_AV_CONFIG_H', '_ISOC11_SOURCE', '_FILE_OFFSET_BITS=64', '_LARGEFILE_SOURCE', 'WIN32_LEAN_AND_MEAN', '_USE_MATH_DEFINES', '_CRT_NONSTDC_NO_WARNINGS', '_WIN32_WINNT=0x0600', '_CRT_SECURE_NO_WARNINGS')
$libs = @('libavutil', 'libavcodec', 'libavformat')
$dropSources = @('libavcodec/file_open.c', 'libavformat/file_open.c')
$archDiffNames = @('FFMPEG_CONFIGURATION', 'CC_IDENT', 'OS_NAME', 'EXTERN_PREFIX', 'EXTERN_ASM', 'HAVE_MM_EMPTY')
$ownSources = @('openswbf2/ossf2_alloc.c', 'openswbf2-ffmpeg.def')
$repoUrl = 'https://github.com/OpenSWBF2/OpenSWBF2'
$notOwned = @('^FFmpegFL\.vcxproj', '\.def$', '^openswbf2\\', '^patches\\')
$crlf = "`r`n"
$utf8 = New-Object Text.UTF8Encoding($false)

function Fail([string]$msg) {
    Write-Host "update-ffmpeg: $msg"
    exit 1
}

function Get-FileSha256([string]$path) {
    $sha = [Security.Cryptography.SHA256]::Create()
    $stream = [IO.File]::OpenRead($path)
    try { $bytes = $sha.ComputeHash($stream) } finally { $stream.Dispose() }
    return -join ($bytes | ForEach-Object { $_.ToString('x2') })
}

function Get-TextSha256([string]$text) {
    $sha = [Security.Cryptography.SHA256]::Create()
    $bytes = $sha.ComputeHash($utf8.GetBytes($text))
    return -join ($bytes | ForEach-Object { $_.ToString('x2') })
}

function ConvertTo-Posix([string]$path) {
    $p = $path -replace '\\', '/'
    if ($p -match '^([A-Za-z]):/(.*)$') { return '/' + $Matches[1].ToLowerInvariant() + '/' + $Matches[2] }
    return $p
}

function Reset-Dir([string]$path) {
    if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path -Recurse -Force }
    New-Item -ItemType Directory -Path $path -Force | Out-Null
}

function Write-Bytes([string]$path, [byte[]]$bytes) {
    $dir = Split-Path $path -Parent
    if (-not (Test-Path -LiteralPath $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
    [IO.File]::WriteAllBytes($path, $bytes)
}

function Write-CrlfText([string]$path, [string[]]$lines) {
    Write-Bytes $path $utf8.GetBytes((($lines -join $crlf) + $crlf))
}

function Get-GitRoot {
    $exec = (& git --exec-path).Trim()
    return [IO.Path]::GetFullPath((Join-Path $exec '..\..\..'))
}

function Invoke-Native([string]$exe, [string[]]$argv, [string]$log) {
    $ErrorActionPreference = 'Continue'
    $out = @(& $exe @argv 2>&1 | ForEach-Object { "$_" })
    $code = $LASTEXITCODE
    if ($log) { [IO.File]::WriteAllLines($log, [string[]]$out) }
    return @{ Code = $code; Out = $out }
}

function Invoke-Bash([string]$command, [string]$log) {
    return Invoke-Native $script:bash @('-c', $command) $log
}

function Get-VcEnv([string]$vcArch) {
    $tools = Get-VsTools 'Win32'
    $vcvars = Join-Path $tools.Install 'VC\Auxiliary\Build\vcvarsall.bat'
    if (-not (Test-Path -LiteralPath $vcvars)) { Fail "vcvarsall.bat not found at $vcvars" }
    $lines = & cmd.exe /c "call `"$vcvars`" $vcArch >nul 2>&1 && set"
    if ($LASTEXITCODE -ne 0) { Fail "vcvarsall $vcArch failed" }
    $envMap = @{}
    foreach ($l in $lines) {
        $i = $l.IndexOf('=')
        if ($i -gt 0) { $envMap[$l.Substring(0, $i)] = $l.Substring($i + 1) }
    }
    if ($envMap['VSCMD_ARG_TGT_ARCH'] -ne $vcArch) { Fail "vcvarsall $vcArch gave target '$($envMap['VSCMD_ARG_TGT_ARCH'])'" }
    $envMap['VSLANG'] = '1033'
    return $envMap
}

function Invoke-WithEnv([hashtable]$envMap, [scriptblock]$block) {
    $saved = @{}
    foreach ($k in $envMap.Keys) {
        $saved[$k] = [Environment]::GetEnvironmentVariable($k, 'Process')
        [Environment]::SetEnvironmentVariable($k, $envMap[$k], 'Process')
    }
    try { return & $block }
    finally {
        foreach ($k in $saved.Keys) { [Environment]::SetEnvironmentVariable($k, $saved[$k], 'Process') }
    }
}

function Get-Download([string]$url, [string]$path) {
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri $url -OutFile $path -UseBasicParsing
}

function Read-ConfigMak([string]$path) {
    $vars = @{}
    foreach ($l in [IO.File]::ReadAllLines($path)) {
        if ($l -match '^([!A-Za-z0-9_]+)=(.*)$') { $vars[$Matches[1]] = $Matches[2] }
    }
    return $vars
}

function Expand-Make([string]$text, [hashtable]$vars, [int]$depth = 0) {
    if ($depth -gt 32) { throw "make expansion too deep: $text" }
    $sb = New-Object Text.StringBuilder
    $i = 0
    while ($i -lt $text.Length) {
        $c = $text[$i]
        if ($c -eq '$' -and $i + 1 -lt $text.Length -and $text[$i + 1] -eq '(') {
            $level = 0
            $j = $i + 1
            for (; $j -lt $text.Length; $j++) {
                if ($text[$j] -eq '(') { $level++ }
                elseif ($text[$j] -eq ')') { $level--; if ($level -eq 0) { break } }
            }
            if ($j -ge $text.Length) { throw "unbalanced make reference: $text" }
            $inner = Expand-Make $text.Substring($i + 2, $j - $i - 2) $vars ($depth + 1)
            if ($inner -match '^([^:\s]+):([^=]*)=(.*)$') {
                $name = $Matches[1]; $from = $Matches[2]; $to = $Matches[3]
                if ($from.IndexOf('%') -lt 0) { throw "unsupported substitution reference: $inner" }
                $val = ''
                if ($vars.ContainsKey($name)) { $val = Expand-Make $vars[$name] $vars ($depth + 1) }
                $fromParts = $from.Split('%'); $toParts = $to.Split('%')
                $words = @($val -split '\s+' | Where-Object { $_ })
                $mapped = foreach ($w in $words) {
                    if ($w.StartsWith($fromParts[0]) -and $w.EndsWith($fromParts[1]) -and $w.Length -ge ($fromParts[0].Length + $fromParts[1].Length)) {
                        $stem = $w.Substring($fromParts[0].Length, $w.Length - $fromParts[0].Length - $fromParts[1].Length)
                        if ($toParts.Count -eq 2) { $toParts[0] + $stem + $toParts[1] } else { $to }
                    } else { $w }
                }
                [void]$sb.Append((@($mapped) -join ' '))
            } elseif ($inner -match '^[!A-Za-z0-9_\-\.]+$') {
                if ($vars.ContainsKey($inner)) { [void]$sb.Append((Expand-Make $vars[$inner] $vars ($depth + 1))) }
            } else {
                throw "unsupported make function: $inner"
            }
            $i = $j + 1
        } else {
            [void]$sb.Append($c)
            $i++
        }
    }
    return $sb.ToString()
}

function Read-Makefile([string]$path, [hashtable]$vars, [string]$srcRoot, [bool]$optional) {
    if (-not (Test-Path -LiteralPath $path)) {
        if ($optional) { return }
        Fail "missing Makefile $path"
    }
    $raw = [IO.File]::ReadAllText($path) -replace "`r", ''
    $joined = $raw -replace "\\\n", ' '
    $stack = New-Object Collections.Stack
    $active = $true
    foreach ($line in $joined.Split("`n")) {
        if ($line.StartsWith("`t")) { continue }
        $hash = $line.IndexOf('#')
        if ($hash -ge 0) { $line = $line.Substring(0, $hash) }
        $t = $line.Trim()
        if (-not $t) { continue }
        if ($t -match '^(ifdef|ifndef)\s+(\S+)$') {
            $stack.Push($active)
            $def = $vars.ContainsKey($Matches[2]) -and $vars[$Matches[2]]
            if ($Matches[1] -eq 'ifndef') { $def = -not $def }
            $active = $active -and $def
            continue
        }
        if ($t -eq 'else') {
            if ($stack.Count -eq 0) { Fail "else without if in $path" }
            $outer = $stack.Peek()
            $active = $outer -and (-not $active)
            continue
        }
        if ($t -eq 'endif') {
            if ($stack.Count -eq 0) { Fail "endif without if in $path" }
            $active = $stack.Pop()
            continue
        }
        if ($t -match '^(ifeq|ifneq|define|endef)\b') { Fail "unsupported make directive '$t' in $path" }
        if (-not $active) { continue }
        if ($t -match '^(-?)include\s+(.+)$') {
            $opt = $Matches[1] -eq '-'
            $target = Expand-Make $Matches[2].Trim() $vars
            Read-Makefile ($target -replace '/', '\') $vars $srcRoot ($optional -or $opt)
            continue
        }
        if ($t -match '^([^\s=:+]+)\s*(\+=|:=|\?=|=)\s*(.*)$') {
            $name = Expand-Make $Matches[1] $vars
            $op = $Matches[2]
            $value = $Matches[3]
            if ($op -eq ':=') {
                try { $value = Expand-Make $value $vars } catch { $value = '$(unsupported ' + $name + ')' }
            }
            if ($op -eq '+=') {
                if ($vars.ContainsKey($name) -and $vars[$name]) { $vars[$name] = $vars[$name] + ' ' + $value } else { $vars[$name] = $value }
            } elseif ($op -eq '?=') {
                if (-not $vars.ContainsKey($name)) { $vars[$name] = $value }
            } else {
                $vars[$name] = $value
            }
        }
    }
    if ($stack.Count -ne 0) { Fail "unterminated conditional in $path" }
}

function Get-SourceList([string]$srcRoot, [string]$configMak) {
    $config = Read-ConfigMak $configMak
    $all = New-Object 'System.Collections.Generic.SortedSet[string]' -ArgumentList ([StringComparer]::Ordinal)
    foreach ($lib in $libs) {
        $vars = @{}
        foreach ($k in $config.Keys) { $vars[$k] = $config[$k] }
        $vars['SRC_PATH'] = $srcRoot -replace '\\', '/'
        $vars['SUBDIR'] = "$lib/"
        Read-Makefile (Join-Path $srcRoot "$lib\Makefile") $vars $srcRoot $false
        foreach ($sub in @($vars['ARCH'], $vars['INTRINSICS'])) {
            if ($sub) { Read-Makefile (Join-Path $srcRoot "$lib\$sub\Makefile") $vars $srcRoot $true }
        }
        Read-Makefile (Join-Path $srcRoot 'ffbuild\arch.mak') $vars $srcRoot $false
        $objs = @()
        foreach ($name in 'OBJS', 'OBJS-yes', 'STLIBOBJS', 'STLIBOBJS-yes') {
            if ($vars.ContainsKey($name)) { $objs += @((Expand-Make $vars[$name] $vars) -split '\s+' | Where-Object { $_ }) }
        }
        foreach ($o in $objs) {
            if (-not $o.EndsWith('.o')) { Fail "object without .o suffix in ${lib}: $o" }
            $rel = [IO.Path]::GetFullPath((Join-Path (Join-Path $srcRoot $lib) ($o.Substring(0, $o.Length - 2) + '.c')))
            if (-not $rel.StartsWith($srcRoot + '\')) { Fail "object outside the source tree: $o" }
            if (-not (Test-Path -LiteralPath $rel)) { Fail "no .c source for $lib object $o" }
            [void]$all.Add(($rel.Substring($srcRoot.Length + 1) -replace '\\', '/'))
        }
    }
    $list = @($all | Where-Object { $dropSources -notcontains $_ })
    foreach ($d in $dropSources) { if ($all -notcontains $d) { Fail "expected duplicate source $d is no longer selected" } }
    return $list
}

function Get-HeaderClosure([string]$srcRoot, [string]$buildDir, [string[]]$sources, [hashtable]$envMap) {
    $clArgs = @('/nologo', '/showIncludes', '/Zs', '/TC', '/std:c17')
    foreach ($d in $defines) { $clArgs += "/D$d" }
    $clArgs += @("/I$buildDir", "/I$srcRoot", ('/I' + (Join-Path $srcRoot 'compat\atomics\win32')))
    $clArgs += @($sources | ForEach-Object { $_ -replace '/', '\' })
    $r = Invoke-WithEnv $envMap {
        Push-Location -LiteralPath $srcRoot
        try { Invoke-Native 'cl.exe' $clArgs $null } finally { Pop-Location }
    }
    if ($r.Code -ne 0) { $r.Out | Select-Object -Last 40 | ForEach-Object { Write-Host $_ }; Fail 'cl /showIncludes failed' }
    $out = $r.Out
    $set = New-Object 'System.Collections.Generic.SortedSet[string]' -ArgumentList ([StringComparer]::Ordinal)
    foreach ($l in $out) {
        if ($l -match '^Note: including file:\s*(.+)$') {
            $p = [IO.Path]::GetFullPath($Matches[1].Trim())
            if ($p.StartsWith($srcRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
                [void]$set.Add(($p.Substring($srcRoot.Length + 1) -replace '\\', '/'))
            }
        }
    }
    return @($set)
}

function Get-ConfigDefines([string]$text) {
    $map = @{}
    foreach ($l in ($text -replace "`r", '').Split("`n")) {
        if ($l -match '^#define\s+(\S+)') { $map[$Matches[1]] = $l }
    }
    return $map
}

function Edit-ConfigH([string]$path, [string]$platform) {
    $fixed = @{
        'CONFIG_THIS_YEAR' = '2026'
        'CC_IDENT' = '"Microsoft (R) C/C++ Optimizing Compiler Version 19.44.35230 for ' + @{ Win32 = 'x86'; x64 = 'x64' }[$platform] + '"'
    }
    if (-not @{ Win32 = 1; x64 = 1 }[$platform]) { Fail "Edit-ConfigH: unknown platform '$platform'" }
    $text = [IO.File]::ReadAllText($path)
    foreach ($k in 'CONFIG_GPL 0', 'CONFIG_NONFREE 0', 'CONFIG_GPLV3 0') {
        if ($text -notmatch ("(?m)^#define $k$")) { Fail "$path does not define $k" }
    }
    foreach ($pair in @(@('HAVE_ALIGNED_MALLOC', '1', '0'), @('HAVE_POSIX_MEMALIGN', '0', '1'))) {
        $found = [regex]::Matches($text, "(?m)^#define $($pair[0]) (\S+)$")
        if ($found.Count -ne 1) { Fail "$path has $($found.Count) definitions of $($pair[0])" }
        if ($found[0].Groups[1].Value -ne $pair[1]) { Fail "$path has $($pair[0]) $($found[0].Groups[1].Value), expected $($pair[1])" }
        $text = $text.Replace("#define $($pair[0]) $($pair[1])`n", "#define $($pair[0]) $($pair[2])`n")
    }
    foreach ($name in 'CONFIG_THIS_YEAR', 'CC_IDENT') {
        $form = @{ 'CONFIG_THIS_YEAR' = '\d{4}'; 'CC_IDENT' = '"Microsoft \(R\) C/C\+\+ Optimizing Compiler Version [0-9.]+ for x(86|64)"' }[$name]
        $found = [regex]::Matches($text, "(?m)^#define $name (.*)$")
        if ($found.Count -ne 1) { Fail "$path has $($found.Count) definitions of $name" }
        if ($found[0].Groups[1].Value -notmatch "^$form$") { Fail "$path has $name $($found[0].Groups[1].Value), not the expected form" }
        $text = $text.Remove($found[0].Index, $found[0].Length).Insert($found[0].Index, "#define $name $($fixed[$name])")
    }
    [IO.File]::WriteAllBytes($path, $utf8.GetBytes($text))
}

function Compare-Trees([string]$expected, [string]$actual) {
    $diffs = @()
    $list = {
        param($dir)
        $m = @{}
        if (Test-Path -LiteralPath $dir) {
            foreach ($f in Get-ChildItem -LiteralPath $dir -Recurse -File) {
                $rel = $f.FullName.Substring($dir.Length + 1)
                $skip = $false
                foreach ($p in $notOwned) { if ($rel -match $p) { $skip = $true } }
                if (-not $skip) { $m[$rel] = $f.FullName }
            }
        }
        $m
    }
    $e = & $list $expected
    $a = & $list $actual
    foreach ($k in ($e.Keys | Sort-Object)) {
        if (-not $a.ContainsKey($k)) { $diffs += "missing in ${actual}: $k"; continue }
        if ($k -eq 'UPSTREAM.md') {
            $et = [IO.File]::ReadAllText($e[$k]) -replace '(?m)^Generated: .*$', ''
            $at = [IO.File]::ReadAllText($a[$k]) -replace '(?m)^Generated: .*$', ''
            if ($et -cne $at) { $diffs += "differs: $k" }
        } elseif ((Get-FileSha256 $e[$k]) -ne (Get-FileSha256 $a[$k])) { $diffs += "differs: $k" }
    }
    foreach ($k in ($a.Keys | Sort-Object)) {
        if (-not $e.ContainsKey($k)) { $diffs += "not generated: $k" }
    }
    return $diffs
}

$gitRoot = Get-GitRoot
$script:bash = Join-Path $gitRoot 'bin\bash.exe'
$gpg = Join-Path $gitRoot 'usr\bin\gpg.exe'
$gpgv = Join-Path $gitRoot 'usr\bin\gpgv.exe'
foreach ($tool in $script:bash, $gpg, $gpgv) { if (-not (Test-Path -LiteralPath $tool)) { Fail "Git for Windows tool not found: $tool" } }
foreach ($pt in $patches) {
    if (-not (Test-Path -LiteralPath $pt.File)) { Fail "missing patch $($pt.File)" }
    $pt.Hash = Get-FileSha256 $pt.File
    if ($pt.Hash -ne $pt.Sha256) { Fail "patch $($pt.File) SHA-256 $($pt.Hash), expected $($pt.Sha256)" }
}

New-Item -ItemType Directory -Path $WorkDir -Force | Out-Null
$dl = Join-Path $WorkDir 'dl'
New-Item -ItemType Directory -Path $dl -Force | Out-Null
$tarName = "ffmpeg-$Version.tar.xz"
$tarUrl = "https://ffmpeg.org/releases/$tarName"
$tar = Join-Path $dl $tarName
$asc = "$tar.asc"
$keyUrl = 'https://ffmpeg.org/ffmpeg-devel.asc'
$key = Join-Path $dl 'ffmpeg-devel.asc'
if (-not (Test-Path -LiteralPath $tar)) { Get-Download $tarUrl $tar }
$tarHash = Get-FileSha256 $tar
if ($tarHash -ne $Sha256) { Remove-Item -LiteralPath $tar -Force; Fail "$tarName SHA-256 $tarHash, expected $Sha256" }
Get-Download "$tarUrl.asc" $asc
Get-Download $keyUrl $key
$gpgDir = Join-Path $WorkDir 'gnupg'
Reset-Dir $gpgDir
$keyring = Join-Path $gpgDir 'ffmpeg.gpg'
$r = Invoke-Native $gpg @('--batch', '--yes', '--dearmor', '--output', ($keyring -replace '\\', '/'), ($key -replace '\\', '/')) $null
if ($r.Code -ne 0) { $r.Out | ForEach-Object { Write-Host $_ }; Fail 'gpg --dearmor of the FFmpeg release key failed' }
$r = Invoke-Native $gpgv @('--status-fd', '1', '--keyring', ($keyring -replace '\\', '/'), ($asc -replace '\\', '/'), ($tar -replace '\\', '/')) $null
if ($r.Code -ne 0) { $r.Out | ForEach-Object { Write-Host $_ }; Fail "signature check of $tarName failed" }
$status = $r.Out
$valid = @($status | Where-Object { $_ -match '^\[GNUPG:\] VALIDSIG ' })
if ($valid.Count -ne 1) { Fail "signature check of $tarName gave $($valid.Count) VALIDSIG lines" }
$sigPrimary = ($valid[0] -split '\s+')[-1]
if ($sigPrimary -ne $KeyFingerprint) { Fail "$tarName is signed by $sigPrimary, expected $KeyFingerprint" }
if (-not ($status | Where-Object { $_ -match '^\[GNUPG:\] GOODSIG ' })) { Fail "no GOODSIG for $tarName" }
Write-Host "update-ffmpeg: $tarName SHA-256 $tarHash, signed by $sigPrimary"

$extract = Join-Path $WorkDir 'src'
Reset-Dir $extract
$r = Invoke-Bash "tar -xJf '$(ConvertTo-Posix $tar)' -C '$(ConvertTo-Posix $extract)'" $null
if ($r.Code -ne 0) { $r.Out | ForEach-Object { Write-Host $_ }; Fail 'tar extraction failed' }
$srcRoot = Join-Path $extract "ffmpeg-$Version"
if (-not (Test-Path -LiteralPath (Join-Path $srcRoot 'configure'))) { Fail "no configure in $srcRoot" }
if (((Get-Content -LiteralPath (Join-Path $srcRoot 'VERSION') -Raw).Trim()) -ne $Version) { Fail "VERSION file is not $Version" }
foreach ($pt in $patches) {
    $r = Invoke-Bash "cd '$(ConvertTo-Posix $srcRoot)' && patch -p1 --no-backup-if-mismatch -F0 --batch -i '$(ConvertTo-Posix $pt.File)'" (Join-Path $WorkDir ('patch-' + [IO.Path]::GetFileNameWithoutExtension($pt.File) + '.log'))
    $r.Out | ForEach-Object { Write-Host "patch: $_" }
    if ($r.Code -ne 0 -or ($r.Out | Where-Object { $_ -match 'fuzz|FAILED|reject|offset' })) { Fail "patch $($pt.File) did not apply exactly" }
}

$builds = @{}
foreach ($a in $archs) {
    $buildDir = Join-Path $WorkDir ('build-' + $a.Platform)
    Reset-Dir $buildDir
    $envMap = Get-VcEnv $a.VcArch
    $cfgArgs = @($commonArgs[0], "--arch=$($a.Arch)", "--target-os=$($a.Os)") + $commonArgs[1..($commonArgs.Count - 1)]
    Write-Host "update-ffmpeg: configure $($a.Platform): $($cfgArgs -join ' ')"
    $cmd = "cd '$(ConvertTo-Posix $buildDir)' && '$(ConvertTo-Posix $srcRoot)/configure' $($cfgArgs -join ' ')"
    $r = Invoke-WithEnv $envMap { Invoke-Bash $cmd (Join-Path $WorkDir "configure-$($a.Platform).log") }
    if ($r.Code -ne 0) {
        $cl = Join-Path $buildDir 'ffbuild\config.log'
        if (Test-Path -LiteralPath $cl) { Get-Content -LiteralPath $cl -Tail 40 | ForEach-Object { Write-Host $_ } }
        Fail "configure $($a.Platform) failed"
    }
    $r = Invoke-Bash "cd '$(ConvertTo-Posix $buildDir)' && sh '$(ConvertTo-Posix $srcRoot)/ffbuild/version.sh' '$(ConvertTo-Posix $srcRoot)' libavutil/ffversion.h" $null
    if ($r.Code -ne 0) { Fail "version.sh failed for $($a.Platform)" }
    $ffv = [IO.File]::ReadAllText((Join-Path $buildDir 'libavutil\ffversion.h'))
    if ($ffv -notmatch ('(?m)^#define FFMPEG_VERSION "' + [regex]::Escape($Version) + '"$')) { Fail "ffversion.h for $($a.Platform) does not carry $Version" }
    $configH = Join-Path $buildDir 'config.h'
    Copy-Item -LiteralPath $configH -Destination "$configH.orig" -Force
    Edit-ConfigH $configH $a.Platform
    $builds[$a.Platform] = @{ Dir = $buildDir; Env = $envMap; Args = $cfgArgs }
}

$x86Defs = Get-ConfigDefines ([IO.File]::ReadAllText((Join-Path $builds['Win32'].Dir 'config.h')))
$x64Defs = Get-ConfigDefines ([IO.File]::ReadAllText((Join-Path $builds['x64'].Dir 'config.h')))
foreach ($k in (@($x86Defs.Keys) + @($x64Defs.Keys) | Sort-Object -Unique)) {
    if ($x86Defs[$k] -ne $x64Defs[$k] -and $archDiffNames -notcontains $k) { Fail "config.h differs between Win32 and x64 in $k" }
}

$sources = $null
foreach ($a in $archs) {
    $s = Get-SourceList $srcRoot (Join-Path $builds[$a.Platform].Dir 'ffbuild\config.mak')
    if ($null -eq $sources) { $sources = $s }
    elseif (($s -join "`n") -ne ($sources -join "`n")) { Fail "the source list differs between Win32 and $($a.Platform)" }
}
$headerSet = New-Object 'System.Collections.Generic.SortedSet[string]' -ArgumentList ([StringComparer]::Ordinal)
foreach ($a in $archs) {
    foreach ($h in (Get-HeaderClosure $srcRoot $builds[$a.Platform].Dir $sources $builds[$a.Platform].Env)) { [void]$headerSet.Add($h) }
}
$closure = @($headerSet | Where-Object { $sources -notcontains $_ })
$headers = @($closure | Where-Object { $_.EndsWith('.h') })
$closureOther = @($closure | Where-Object { -not $_.EndsWith('.h') })
Write-Host "update-ffmpeg: $($sources.Count) .c sources, $($headers.Count) .h headers, $($closureOther.Count) other included files"

$unusedRx = '\b(CONFIG_THIS_YEAR|CC_IDENT)\b'
foreach ($f in @($sources) + @($closure)) {
    if ([IO.File]::ReadAllText((Join-Path $srcRoot ($f -replace '/', '\'))) -match $unusedRx) { Fail "$f uses $($Matches[1]), which the config edit fixes as unused" }
}

$stage = Join-Path $WorkDir 'out'
Reset-Dir $stage
foreach ($f in @($sources) + @($closure)) {
    $from = Join-Path $srcRoot ($f -replace '/', '\')
    Write-Bytes (Join-Path $stage ($f -replace '/', '\')) ([IO.File]::ReadAllBytes($from))
}
foreach ($a in $archs) {
    Write-Bytes (Join-Path $stage "config\$($a.Platform)\config.h") ([IO.File]::ReadAllBytes((Join-Path $builds[$a.Platform].Dir 'config.h')))
}
$commonFiles = @('config_components.h', 'libavutil\avconfig.h', 'libavutil\ffversion.h', 'libavcodec\codec_list.c', 'libavcodec\parser_list.c', 'libavcodec\bsf_list.c', 'libavformat\demuxer_list.c', 'libavformat\muxer_list.c', 'libavformat\protocol_list.c')
foreach ($c in $commonFiles) {
    $b86 = [IO.File]::ReadAllBytes((Join-Path $builds['Win32'].Dir $c))
    $b64 = [IO.File]::ReadAllBytes((Join-Path $builds['x64'].Dir $c))
    if ([Convert]::ToBase64String($b86) -ne [Convert]::ToBase64String($b64)) { Fail "$c differs between Win32 and x64" }
    Write-Bytes (Join-Path $stage "config\common\$c") $b86
}
Write-Bytes (Join-Path $stage 'COPYING.LGPLv2.1') ([IO.File]::ReadAllBytes((Join-Path $srcRoot 'COPYING.LGPLv2.1')))

$diffText = ''
foreach ($pt in $patches) { $diffText += $utf8.GetString([IO.File]::ReadAllBytes($pt.File)) }
foreach ($a in $archs) {
    $label = "config/$($a.Platform)/config.h"
    $orig = ConvertTo-Posix (Join-Path $builds[$a.Platform].Dir 'config.h.orig')
    $edit = ConvertTo-Posix (Join-Path $builds[$a.Platform].Dir 'config.h')
    $r = Invoke-Bash "diff -u --label 'a/$label' --label 'b/$label' '$orig' '$edit'" $null
    if ($r.Code -ne 1) { Fail "diff of the $label edit returned $($r.Code)" }
    $diffText += "diff -u a/$label b/$label`n" + (($r.Out -join "`n") + "`n")
}
foreach ($own in $ownSources) {
    $ownPath = Join-Path $OutDir ($own -replace '/', '\')
    if (-not (Test-Path -LiteralPath $ownPath)) { Fail "missing local source $own" }
    $r = Invoke-Bash "diff -u --label /dev/null --label 'b/$own' /dev/null '$(ConvertTo-Posix $ownPath)'" $null
    if ($r.Code -ne 1) { Fail "diff of the local source $own returned $($r.Code)" }
    $diffText += "diff -u /dev/null b/$own`n" + (($r.Out -join "`n") + "`n")
}
Write-Bytes (Join-Path $stage 'changes.diff') $utf8.GetBytes($diffText)

$props = @('<?xml version="1.0" encoding="utf-8"?>', '<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">', '  <ItemGroup>')
$props += @($sources | ForEach-Object { '    <ClCompile Include="$(MSBuildThisFileDirectory)' + ($_ -replace '/', '\') + '" />' })
$props += @('  </ItemGroup>', '</Project>')
Write-CrlfText (Join-Path $stage 'FFmpegFL.files.props') $props

$vendored = [string[]](@($sources) + @($closure))
[Array]::Sort($vendored, [StringComparer]::Ordinal)
$listHash = Get-TextSha256 (($vendored -join "`n") + "`n")
$notice = @(
    "This software uses code of FFmpeg (https://ffmpeg.org) licensed under the GNU Lesser General Public License version 2.1 or later (LGPLv2.1+). The license text is in COPYING.LGPLv2.1.",
    '',
    "FFmpeg version: $Version",
    "Source: $tarUrl",
    "Source SHA-256: $Sha256",
    '',
    'openswbf2-ffmpeg.dll is built from a subset of that source with these local changes: the patches 5195d6f9.patch (upstream commit 5195d6f9729ad00b93e4d0f0bea309416fb4713e, CVE-2026-125068) and tx-subtx-free.patch, an edit of each generated config.h (HAVE_ALIGNED_MALLOC 0, HAVE_POSIX_MEMALIGN 1, and fixed CONFIG_THIS_YEAR and CC_IDENT values), the allocator shim openswbf2/ossf2_alloc.c and the export list openswbf2-ffmpeg.def.',
    '',
    "The complete source of this DLL, with every local change and the project files that build it, is published in the OpenSWBF2 repository at $repoUrl under src/FFmpegFL, at the commit this build was made from. There, changes.diff holds every change against the FFmpeg source above, and UPSTREAM.md records how the subset was made."
)
Write-CrlfText (Join-Path $stage 'NOTICE.txt') $notice
$upstream = @(
    '# FFmpeg subset',
    '',
    "Generated: $((Get-Date).ToString('yyyy-MM-dd'))",
    '',
    "- Version: $Version",
    "- Source: $tarUrl",
    "- Source SHA-256: $Sha256",
    "- Signature: $tarUrl.asc, checked against $keyUrl, primary key fingerprint $KeyFingerprint",
    ($patches[0].Line -f $patches[0].Hash),
    ($patches[1].Line -f $patches[1].Hash),
    "- Configure Win32: configure $($builds['Win32'].Args -join ' ')",
    "- Configure x64: configure $($builds['x64'].Args -join ' ')",
    '- Config edit: each config.h gets HAVE_ALIGNED_MALLOC 1 -> 0 and HAVE_POSIX_MEMALIGN 0 -> 1, so av_malloc, av_realloc and av_free reach ossf2_posix_memalign, ossf2_realloc and ossf2_free instead of the unprefixed CRT _aligned_* functions (cause C-FFMPEG-ALLOC-CONFIG in tests/baseline/v143/CAUSES.md).',
    '- Config fixed values: each config.h gets CONFIG_THIS_YEAR 2026 and the CC_IDENT of cl 19.44.35230 for its platform in place of the configure date and the installed compiler, so -Verify does not depend on the calendar or the Visual Studio update. No vendored source uses either name, and the script fails if one does (cause C-FFMPEG-CONFIG-FIXED in tests/baseline/v143/CAUSES.md).',
    "- Local sources: $($ownSources -join ' and ') are ours, compiled into or linked with the DLL.",
    '- changes.diff holds both patches, both config.h edits and the local sources as new files.',
    "- Dropped sources: $($dropSources -join ', ') (per-library copies of libavutil/file_open.c that would define ff_open twice in one DLL)",
    "- Sources: $($sources.Count) .c files from the OBJS, OBJS-yes, STLIBOBJS and STLIBOBJS-yes lists of $($libs -join ', ')",
    "- Headers: $($headers.Count) .h files from the cl /showIncludes closure of those sources on Win32 and x64",
    "- Other included files: $($closureOther.Count) template sources that other sources include and that are not compiled on their own ($($closureOther -join ', '))",
    '- Counts: libavutil/hwcontext_stub.c is selected by OBJS-$(!CONFIG_VULKAN), which configure turns on (!CONFIG_VULKAN=yes in ffbuild/config.mak), so make compiles it too. A count that ignores the !CONFIG_ form gets one source fewer, and a closure count that takes the included templates for headers gets the headers plus the templates.',
    "- File list SHA-256: $listHash (the sorted relative paths of the vendored sources and closure files, one per line, LF)",
    '- Generated files: config/Win32/config.h, config/x64/config.h and config/common/ (identical on both platforms)',
    '- Regenerate: powershell -NoProfile -ExecutionPolicy Bypass -File tools/update-ffmpeg.ps1; check: the same with -Verify',
    '',
    '## Exemptions',
    '',
    'FFmpegFL.vcxproj builds openswbf2-ffmpeg.dll under OpenSWBF2.Game.props. Each exemption from those flags is per warning or per file, with its cause in tests/baseline/v143/CAUSES.md:',
    '- /wd4146 on every FFmpegFL source, after /sdl on the command line (C-FFMPEG-WD4146)',
    '- /wd4703 on libavutil/encryption_info.c, libavformat/url.c, libavformat/avio.c, libavformat/av1.c and libavcodec/raw.c only (C-FFMPEG-WD4703)',
    '- Optimization MinSpace on libavcodec/avcodec.c in Debug only (C-FFMPEG-DEBUG-O1)',
    '- bcrypt.dll import kept (C-FFMPEG-BCRYPT): avformat_alloc_context -> av_opt_set_defaults -> av_opt_set_defaults2 -> set_string_color -> av_parse_color -> av_get_random_seed -> BCryptOpenAlgorithmProvider, BCryptGenRandom, BCryptCloseAlgorithmProvider. These are static calls from an export the decoder needs, so /OPT:REF keeps them; the /VERBOSE:REF link discards none of them. Measured links that export only avformat_alloc_context, avformat_open_input, avcodec_open2 or avutil_version each import bcrypt.dll too.',
    '- Link: /OPT:REF /OPT:ICF and /Gy in every configuration. Debug compiles at /Od, which does not imply /Gy, and without /Gy the linker cannot drop ff_tempfile (libavutil/file_open.c), whose free of the tempnam result is the only CRT heap call left after the config edit; the Debug DLL then imports free and the filesystem functions. With /Gy no Debug, Release or Test DLL imports api-ms-win-crt-heap-l1-1-0.dll.',
    '- Asan: the DLL imports malloc, calloc and free from api-ms-win-crt-heap-l1-1-0.dll because VCASAN.lib(vcasan.obj), which /fsanitize=address links into every image, references them. No FFmpegFL object references them after /OPT:REF.'
)
Write-CrlfText (Join-Path $stage 'UPSTREAM.md') $upstream

if ($Verify) {
    $diffs = Compare-Trees $stage $OutDir
    if ($diffs.Count -gt 0) {
        $diffs | ForEach-Object { Write-Host "update-ffmpeg: $_" }
        Fail "$($diffs.Count) difference(s) between the regenerated subset and $OutDir"
    }
    Write-Host "update-ffmpeg: $OutDir matches the regenerated subset"
    exit 0
}

if (Test-Path -LiteralPath $OutDir) {
    foreach ($f in Get-ChildItem -LiteralPath $OutDir -Recurse -File) {
        $rel = $f.FullName.Substring($OutDir.Length + 1)
        $skip = $false
        foreach ($p in $notOwned) { if ($rel -match $p) { $skip = $true } }
        if (-not $skip) { Remove-Item -LiteralPath $f.FullName -Force }
    }
    Get-ChildItem -LiteralPath $OutDir -Recurse -Directory | Sort-Object { $_.FullName.Length } -Descending | Where-Object { -not (Get-ChildItem -LiteralPath $_.FullName -Force) } | Remove-Item -Force
}
foreach ($f in Get-ChildItem -LiteralPath $stage -Recurse -File) {
    $rel = $f.FullName.Substring($stage.Length + 1)
    Write-Bytes (Join-Path $OutDir $rel) ([IO.File]::ReadAllBytes($f.FullName))
}
Write-Host "update-ffmpeg: wrote the subset to $OutDir"
exit 0
