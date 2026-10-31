param(
    [string]$OutDir,
    [string]$Name = 'census-1',
    [string]$Scratch
)
$ErrorActionPreference = 'Stop'
$testsDir = $PSScriptRoot
$root = Split-Path $testsDir -Parent
. (Join-Path $testsDir 'smoke.ps1')

function Resolve-RootPath([string]$p) {
    if (-not [IO.Path]::IsPathRooted($p)) { $p = Join-Path $root $p }
    return [IO.Path]::GetFullPath($p).TrimEnd('\')
}

if (-not $OutDir) { $OutDir = Join-Path $testsDir 'baseline\census' }
if (-not $Scratch) { $Scratch = Join-Path $root 'artifacts\x64-census' }
$OutDir = Resolve-RootPath $OutDir
$Scratch = Resolve-RootPath $Scratch
$artifacts = Join-Path $root 'artifacts'
if (-not $Scratch.StartsWith($artifacts + '\', [StringComparison]::OrdinalIgnoreCase)) { throw "scratch folder must be under artifacts: $Scratch" }

$codes = 'C4311', 'C4312', 'C4302', 'C4267', 'C4244'
$platforms = 'Win32', 'x64'
$msNs = 'http://schemas.microsoft.com/developer/msbuild/2003'
$projects = @(
    @{ Name = 'BattlefrontII'; Path = Join-Path $root 'BattlefrontII.vcxproj' },
    @{ Name = 'LuaCompiler'; Path = Join-Path $root 'LuaCompiler.vcxproj' },
    @{ Name = 'OpenSWBF2Tests'; Path = Join-Path $root 'tests\OpenSWBF2Tests.vcxproj' }
)

Write-ToolBanner
$tools = Get-VsTools
$msbuild = Find-MSBuild
$clExe = Join-Path $tools.Bin 'cl.exe'
$clBanner = @(& cmd /c "`"$clExe`" 2>&1" | Where-Object { $_ -and $_.Trim() }) | Select-Object -First 1
$clBanner = ([string]$clBanner).Trim()

function Remove-Scratch {
    if (-not (Test-Path -LiteralPath $Scratch)) { return }
    $rp = @(Get-ReparsePoints $Scratch)
    if ($rp.Count -gt 0) { throw "scratch folder holds reparse points, not deleting: $($rp -join ', ')" }
    Remove-Item -LiteralPath $Scratch -Recurse -Force
}

function Get-FullItemPath([string]$dir, [string]$inc) {
    $leaf = Split-Path $inc -Leaf
    $parent = Split-Path $inc -Parent
    $base = $dir
    if ($parent) { $base = Join-Path $dir $parent }
    return Join-Path ([IO.Path]::GetFullPath($base)) $leaf
}

function ConvertTo-IncludeDirs([string]$value, [string]$dir) {
    $value = $value.Replace('$(MSBuildThisFileDirectory)', $dir + '\')
    $out = foreach ($tok in $value.Split(';')) {
        if (-not $tok -or $tok.Contains('$(') -or $tok.Contains('%(') -or [IO.Path]::IsPathRooted($tok)) { $tok }
        else { [IO.Path]::GetFullPath((Join-Path $dir $tok)).TrimEnd('\') }
    }
    return ($out -join ';')
}

function New-ProjectCopy($proj) {
    $src = $proj.Path
    $dir = Split-Path $src -Parent
    $copyDir = Join-Path $Scratch $proj.Name
    New-Item -ItemType Directory -Force -Path $copyDir | Out-Null
    $xml = New-Object xml
    $xml.Load($src)
    $ns = New-Object Xml.XmlNamespaceManager $xml.NameTable
    $ns.AddNamespace('m', $msNs)

    foreach ($n in @($xml.SelectNodes('//m:ItemGroup/*[@Include]', $ns))) {
        if ($n.LocalName -eq 'ProjectConfiguration') { continue }
        if ($n.LocalName -eq 'ProjectReference') { [void]$n.ParentNode.RemoveChild($n); continue }
        $n.SetAttribute('Include', (Get-FullItemPath $dir $n.GetAttribute('Include')))
    }
    foreach ($n in @($xml.SelectNodes('//m:ObjectFileName', $ns))) { [void]$n.ParentNode.RemoveChild($n) }
    foreach ($n in @($xml.SelectNodes('//m:Target', $ns))) {
        if ('BuildLuaLvls', 'StageGameData' -contains $n.GetAttribute('Name')) { [void]$n.ParentNode.RemoveChild($n) }
    }
    foreach ($n in @($xml.SelectNodes('//m:AdditionalIncludeDirectories', $ns))) { $n.InnerText = ConvertTo-IncludeDirs $n.InnerText $dir }
    foreach ($n in @($xml.SelectNodes('//m:OutDir', $ns))) { $n.InnerText = "$copyDir\bin\`$(Platform)\" }
    foreach ($n in @($xml.SelectNodes('//m:IntDir', $ns))) { $n.InnerText = "$copyDir\obj\`$(Platform)\" }

    $cfgGroup = $xml.SelectSingleNode("//m:ItemGroup[@Label='ProjectConfigurations']", $ns)
    $pc = $xml.CreateElement('ProjectConfiguration', $msNs)
    $pc.SetAttribute('Include', 'Release|x64')
    foreach ($kv in @(@('Configuration', 'Release'), @('Platform', 'x64'))) {
        $e = $xml.CreateElement($kv[0], $msNs)
        $e.InnerText = $kv[1]
        [void]$pc.AppendChild($e)
    }
    [void]$cfgGroup.AppendChild($pc)

    foreach ($n in @($xml.SelectNodes('//m:WarningLevel', $ns))) { $n.InnerText = 'Level4' }
    foreach ($cc in @($xml.SelectNodes('//m:ItemDefinitionGroup/m:ClCompile', $ns))) {
        if (-not $cc.SelectSingleNode('m:WarningLevel', $ns)) {
            $e = $xml.CreateElement('WarningLevel', $msNs)
            $e.InnerText = 'Level4'
            [void]$cc.AppendChild($e)
        }
    }

    $tus = New-Object System.Collections.Generic.List[string]
    foreach ($n in @($xml.SelectNodes('//m:ItemGroup/m:ClCompile[@Include]', $ns))) {
        $inc = $n.GetAttribute('Include')
        if ($inc.Contains('*')) {
            foreach ($f in @(Get-ChildItem -Path (Split-Path $inc -Parent) -Filter (Split-Path $inc -Leaf) -File)) { $tus.Add($f.FullName) }
        }
        else { $tus.Add($inc) }
    }

    $copyPath = Join-Path $copyDir ($proj.Name + '.vcxproj')
    if ($copyPath -eq $src) { throw "refusing to overwrite $src" }
    $xml.Save($copyPath)
    return @{ Name = $proj.Name; Copy = $copyPath; Dir = $copyDir; Tus = $tus.ToArray() }
}

$script:CaseCache = @{}
function Get-TrueCase([string]$rel) {
    $key = $rel.ToLowerInvariant()
    if ($script:CaseCache.ContainsKey($key)) { return $script:CaseCache[$key] }
    $cur = $root
    $parts = foreach ($seg in $rel.Split('\')) {
        $hit = @([IO.Directory]::GetFileSystemEntries($cur, $seg)) | Select-Object -First 1
        if ($hit) { $seg = Split-Path $hit -Leaf }
        $cur = Join-Path $cur $seg
        $seg
    }
    $res = $parts -join '\'
    $script:CaseCache[$key] = $res
    return $res
}

$script:Anchors = @()
if ($tools.Install) { $script:Anchors += , @($tools.Install.TrimEnd('\'), '<VS>') }
if ($env:DXSDK_DIR) { $script:Anchors += , @($env:DXSDK_DIR.TrimEnd('\'), '<DXSDK>') }
$script:Anchors += , @((Join-Path ${env:ProgramFiles(x86)} 'Windows Kits'), '<WindowsKits>')

function ConvertTo-RepoPath([string]$p) {
    $full = [IO.Path]::GetFullPath($p.Trim())
    if ($full.StartsWith($root + '\', [StringComparison]::OrdinalIgnoreCase)) { return Get-TrueCase $full.Substring($root.Length + 1) }
    foreach ($a in $script:Anchors) {
        if ($full.StartsWith($a[0] + '\', [StringComparison]::OrdinalIgnoreCase)) { return $a[1] + $full.Substring($a[0].Length) }
    }
    throw "diagnostic outside the repo and known SDK roots: $full"
}

$lineRx = New-Object regex '^\s*(?:\d+>)?(.+?)\((\d+)(?:,\d+)?\): (warning|error|fatal error) (C\d{4})'
function Read-Log([string]$log) {
    $sites = New-Object 'System.Collections.Generic.HashSet[string]'
    foreach ($l in [IO.File]::ReadAllLines($log)) {
        $m = $lineRx.Match($l)
        if (-not $m.Success) { continue }
        $kind = 'warning'
        if ($m.Groups[3].Value -ne 'warning') { $kind = 'error' }
        [void]$sites.Add((ConvertTo-RepoPath $m.Groups[1].Value) + '|' + $m.Groups[2].Value + '|' + $kind + '|' + $m.Groups[4].Value)
    }
    return $sites
}

function Write-CensusText([string]$path, $lines) {
    $text = (@($lines) -join "`r`n") + "`r`n"
    if ($text -match '[A-Za-z]:\\') { throw "absolute path left in $(Split-Path $path -Leaf)" }
    if ($env:USERPROFILE -and $text.IndexOf($env:USERPROFILE, [StringComparison]::OrdinalIgnoreCase) -ge 0) { throw "user profile left in $(Split-Path $path -Leaf)" }
    if ($env:USERNAME -and $text.IndexOf($env:USERNAME, [StringComparison]::OrdinalIgnoreCase) -ge 0) { throw "user name left in $(Split-Path $path -Leaf)" }
    New-Item -ItemType Directory -Force -Path (Split-Path $path -Parent) | Out-Null
    [IO.File]::WriteAllText($path, $text, [Text.Encoding]::ASCII)
}

function Get-Sorted($items) {
    $arr = [string[]]@($items)
    [Array]::Sort($arr, [StringComparer]::Ordinal)
    return , $arr
}

Remove-Scratch
New-Item -ItemType Directory -Force -Path (Join-Path $root 'artifacts') | Out-Null
Set-Content -Path (Join-Path $root 'artifacts\.gitignore') -Value '*' -Encoding ASCII
New-Item -ItemType Directory -Force -Path $Scratch | Out-Null

$copies = foreach ($p in $projects) { New-ProjectCopy $p }

$warn = @{}
$seen = @{}
foreach ($plat in $platforms) { $warn[$plat] = @{}; $seen[$plat] = New-Object 'System.Collections.Generic.HashSet[string]' }
$errSites = New-Object 'System.Collections.Generic.HashSet[string]'
$errByFile = @{}
$tuRows = New-Object System.Collections.Generic.List[string]
$objStats = New-Object System.Collections.Generic.List[object]
$missing = New-Object System.Collections.Generic.List[string]

foreach ($c in $copies) {
    foreach ($plat in $platforms) {
        $log = Join-Path $c.Dir "$plat.log"
        Write-Host "compiling $($c.Name) $plat"
        & $msbuild $c.Copy '/t:ClCompile' '/p:Configuration=Release' "/p:Platform=$plat" '/m' '/nologo' '/v:quiet' "/flp:logfile=$log;verbosity=normal" | Out-Null
        Write-Host "  msbuild exit $LASTEXITCODE"
        if ($plat -eq 'Win32' -and $LASTEXITCODE -ne 0) { throw "Win32 baseline compile failed for $($c.Name), see $log" }
        if (-not (Test-Path -LiteralPath $log)) { throw "no log for $($c.Name) $plat" }
        foreach ($s in (Read-Log $log)) {
            $f = $s.Split('|')
            if ($f[2] -eq 'warning') {
                if (($codes -contains $f[3]) -and $seen[$plat].Add($s)) {
                    if (-not $warn[$plat].ContainsKey($f[0])) { $warn[$plat][$f[0]] = @{} }
                    $w = $warn[$plat][$f[0]]
                    $w[$f[3]] = 1 + [int]$w[$f[3]]
                }
            }
            elseif ($plat -eq 'x64' -and $errSites.Add($s)) {
                $k = $f[0].ToLowerInvariant()
                $errByFile[$k] = 1 + [int]$errByFile[$k]
            }
        }
    }
}

foreach ($c in $copies) {
    $objDir = Join-Path $c.Dir 'obj\x64'
    $with = 0
    foreach ($tu in $c.Tus) {
        $rel = ConvertTo-RepoPath $tu
        $n = [int]$errByFile[$rel.ToLowerInvariant()]
        $flag = 'no'
        if (Test-Path -LiteralPath (Join-Path $objDir ([IO.Path]::GetFileNameWithoutExtension($tu) + '.obj'))) { $flag = 'yes'; $with++ }
        else { $missing.Add("$($c.Name),$rel") }
        $tuRows.Add("$($c.Name),$rel,$flag,$n")
    }
    $objStats.Add(@($c.Name, $with, $c.Tus.Count))
}

$files = New-Object 'System.Collections.Generic.HashSet[string]'
foreach ($plat in $platforms) { foreach ($k in $warn[$plat].Keys) { [void]$files.Add($k) } }
$tot = @{}
foreach ($k in 'x64', 'Win32', 'delta') { $tot[$k] = @{}; foreach ($cd in $codes) { $tot[$k][$cd] = 0 } }
$rows = foreach ($f in (Get-Sorted $files)) {
    $x = foreach ($cd in $codes) { $v = 0; if ($warn['x64'].ContainsKey($f)) { $v = [int]$warn['x64'][$f][$cd] }; $v }
    $y = foreach ($cd in $codes) { $v = 0; if ($warn['Win32'].ContainsKey($f)) { $v = [int]$warn['Win32'][$f][$cd] }; $v }
    $d = for ($i = 0; $i -lt $codes.Count; $i++) { $x[$i] - $y[$i] }
    for ($i = 0; $i -lt $codes.Count; $i++) { $tot['x64'][$codes[$i]] += $x[$i]; $tot['Win32'][$codes[$i]] += $y[$i]; $tot['delta'][$codes[$i]] += $d[$i] }
    (@($f) + $x + $y + $d) -join ','
}
$header = 'file,' + ((@('x64', 'x86', 'delta') | ForEach-Object { $p = $_; ($codes | ForEach-Object { "$($p)_$_" }) -join ',' }) -join ',')

$hist = @{}
foreach ($s in $errSites) { $cd = $s.Split('|')[3]; $hist[$cd] = 1 + [int]$hist[$cd] }

$commit = (& git -C $root rev-parse HEAD).Trim()
$dirty = @(& git -C $root status --porcelain --untracked-files=no)
$clean = 'yes'
if ($dirty.Count -gt 0) { $clean = 'no' }

$md = New-Object System.Collections.Generic.List[string]
$md.Add("# x64 census: $Name")
$md.Add('')
$md.Add('Generated by `tests\x64-census.ps1`. Compile-only (`/t:ClCompile`) builds of throwaway copies of BattlefrontII, LuaCompiler and OpenSWBF2Tests; the real projects and solution are not changed. Counts are unique (file, line, code) sites.')
$md.Add('')
$md.Add('| Key | Value |')
$md.Add('|-----|-------|')
$md.Add('| toolset | v143 |')
$md.Add("| cl | $clBanner |")
$md.Add("| MSVC tools version | $($tools.ToolsVersion) |")
$md.Add('| configuration | Release |')
$md.Add('| platforms | Win32 /W4, x64 /W4 |')
$md.Add("| commit | $commit |")
$md.Add("| tracked tree clean | $clean |")
$md.Add('')
$md.Add('## Totals')
$md.Add('')
$md.Add('| Code | x64 | x86 | delta |')
$md.Add('|------|-----|-----|-------|')
foreach ($cd in $codes) { $md.Add("| $cd | $($tot['x64'][$cd]) | $($tot['Win32'][$cd]) | $($tot['delta'][$cd]) |") }
$md.Add('')
$md.Add('## x64 objects')
$md.Add('')
$md.Add('| Project | TUs | with x64 object | without |')
$md.Add('|---------|-----|-----------------|---------|')
foreach ($o in $objStats) { $md.Add("| $($o[0]) | $($o[2]) | $($o[1]) | $($o[2] - $o[1]) |") }
$md.Add('')
$md.Add('`x64_errors` in `' + $Name + '-tu.csv` counts the unique x64 error sites inside the TU file itself. The `/MP` log does not say which TU an error in a shared header came from, so header errors appear only in the histogram below.')
$md.Add('')
$md.Add('## Lower bound')
$md.Add('')
$md.Add('This census is a lower bound. On x64, `operator new(unsigned int)` is not a valid allocation function, so most game TUs fail with C2821 and the errors that cascade from it. cl stops early in those TUs, before it reaches most front-end warnings, so their C4311/C4312/C4302/C4267/C4244 sites are undercounted. The counts will rise once Phase 22 (PTR-01) lets those TUs compile. Census #2 must compare like-for-like TUs (use the TU list in `' + $Name + '-tu.csv`). No numeric target is set.')
$md.Add('')
$md.Add('## x64 error codes')
$md.Add('')
$md.Add("Unique x64 error sites: $($errSites.Count).")
$md.Add('')
$md.Add('| Code | Sites |')
$md.Add('|------|-------|')
foreach ($cd in ($hist.Keys | Sort-Object @{ Expression = { $hist[$_] }; Descending = $true }, @{ Expression = { $_ }; Ascending = $true })) { $md.Add("| $cd | $($hist[$cd]) |") }
$md.Add('')
$md.Add('## TUs without an x64 object')
$md.Add('')
foreach ($m in (Get-Sorted $missing)) { $f = $m.Split(','); $md.Add("- $($f[0]): ``$($f[1])``") }

$csvPath = Join-Path $OutDir "$Name.csv"
$tuPath = Join-Path $OutDir "$Name-tu.csv"
$mdPath = Join-Path $OutDir "$Name.md"
Write-CensusText $csvPath (@($header) + @($rows))
Write-CensusText $tuPath (@('project,tu,x64_obj,x64_errors') + (Get-Sorted $tuRows))
Write-CensusText $mdPath $md

foreach ($cd in $codes) { Write-Host ("{0}: x64 {1}, x86 {2}, delta {3}" -f $cd, $tot['x64'][$cd], $tot['Win32'][$cd], $tot['delta'][$cd]) }
foreach ($o in $objStats) { Write-Host ("{0}: {1}/{2} TUs with an x64 object" -f $o[0], $o[1], $o[2]) }
Write-Host "x64 error sites: $($errSites.Count)"
Write-Host "wrote $csvPath"
Write-Host "wrote $tuPath"
Write-Host "wrote $mdPath"

Remove-Scratch
