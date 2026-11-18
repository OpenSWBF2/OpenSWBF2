param(
    [string]$OutDir,
    [string]$Name = 'census-1',
    [ValidateSet('Release', 'Debug', 'Test', 'Asan')][string[]]$Configurations = @('Release', 'Debug', 'Test', 'Asan')
)
$ErrorActionPreference = 'Stop'
$testsDir = $PSScriptRoot
$root = Split-Path $testsDir -Parent
. (Join-Path $testsDir 'smoke.ps1')
$script:SmokePlatform = 'x64'

function Resolve-RootPath([string]$p) {
    if (-not [IO.Path]::IsPathRooted($p)) { $p = Join-Path $root $p }
    return [IO.Path]::GetFullPath($p).TrimEnd('\')
}

if (-not $OutDir) { $OutDir = Join-Path $testsDir 'baseline\census' }
$OutDir = Resolve-RootPath $OutDir
$logDir = Join-Path $root "artifacts\x64-census\$Name"

$siteCodes = 'C4311', 'C4312', 'C4302', 'C4477'
$codes = $siteCodes + @('C4267', 'C4244')
$msNs = 'http://schemas.microsoft.com/developer/msbuild/2003'

Write-ToolBanner x64
$tools = Get-VsTools x64
$msbuild = Find-MSBuild
$clExe = Join-Path $tools.Bin 'cl.exe'
$clBanner = @(& cmd /c "`"$clExe`" 2>&1" | Where-Object { $_ -and $_.Trim() }) | Select-Object -First 1
$clBanner = ([string]$clBanner).Trim()

function Get-CensusProjects([string]$config) {
    $list = @(Get-GameProjects $root $config | ForEach-Object { @{ Name = $_.Name; Vcxproj = $_.Vcxproj; IntDir = $_.IntDir; TlogDir = $_.TlogDir; Flat = $false } })
    $lc = Join-Path $root "obj\x64\$config\LuaCompiler"
    $list += @{ Name = 'LuaCompiler'; Vcxproj = (Join-Path $root 'LuaCompiler.vcxproj'); IntDir = $lc; TlogDir = (Join-Path $lc 'LuaCompiler.tlog'); Flat = $false }
    $tt = Join-Path $root "obj\Tests\x64\$config"
    $list += @{ Name = 'OpenSWBF2Tests'; Vcxproj = (Join-Path $testsDir 'OpenSWBF2Tests.vcxproj'); IntDir = $tt; TlogDir = (Join-Path $tt 'OpenSWBF2Tests.tlog'); Flat = $true }
    return $list
}

function Get-ProjectUnits($proj) {
    $dir = Split-Path $proj.Vcxproj -Parent
    $xml = New-Object xml
    $xml.Load($proj.Vcxproj)
    $ns = New-Object Xml.XmlNamespaceManager $xml.NameTable
    $ns.AddNamespace('m', $msNs)
    $units = New-Object System.Collections.Generic.List[object]
    foreach ($n in @($xml.SelectNodes('//m:ItemGroup/m:ClCompile[@Include]', $ns))) {
        $inc = $n.GetAttribute('Include')
        $incs = @($inc)
        if ($inc.Contains('*')) {
            $incDir = Split-Path $inc -Parent
            $incs = @(Get-ChildItem -LiteralPath (Join-Path $dir $incDir) -Filter (Split-Path $inc -Leaf) -File | Sort-Object Name | ForEach-Object { Join-Path $incDir $_.Name })
        }
        foreach ($i in $incs) {
            $src = [IO.Path]::GetFullPath((Join-Path $dir $i))
            $objName = [IO.Path]::GetFileNameWithoutExtension($i) + '.obj'
            if ($proj.Flat) { $obj = Join-Path $proj.IntDir $objName }
            else {
                $rel = Split-Path $i -Parent
                $obj = Join-Path $proj.IntDir $objName
                if ($rel) { $obj = Join-Path (Join-Path $proj.IntDir $rel) $objName }
            }
            $units.Add(@{ Src = $src; Obj = [IO.Path]::GetFullPath($obj) })
        }
    }
    return $units.ToArray()
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

$lineRx = New-Object regex '^\s*(?:\d+>)?(.+?)\((\d+)(?:,\d+)?\): (warning|error|fatal error) (C\d{4}): (.*?)(?: \[[^\]]*\])?\s*$'
function Read-Log([string]$log) {
    $sites = @{}
    foreach ($l in [IO.File]::ReadAllLines($log)) {
        $m = $lineRx.Match($l)
        if (-not $m.Success) { continue }
        $kind = 'warning'
        if ($m.Groups[3].Value -ne 'warning') { $kind = 'error' }
        $key = (ConvertTo-RepoPath $m.Groups[1].Value) + '|' + $m.Groups[2].Value + '|' + $kind + '|' + $m.Groups[4].Value
        if (-not $sites.ContainsKey($key)) { $sites[$key] = $m.Groups[5].Value.Trim() }
    }
    return $sites
}

function ConvertTo-CsvField([string]$s) {
    if ($s -match '[",]') { return '"' + $s.Replace('"', '""') + '"' }
    return $s
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

New-Item -ItemType Directory -Force -Path (Join-Path $root 'artifacts') | Out-Null
Set-Content -Path (Join-Path $root 'artifacts\.gitignore') -Value '*' -Encoding ASCII
New-Item -ItemType Directory -Force -Path $logDir | Out-Null

$sites = @{}
$siteConfigs = @{}
$errSites = @{}
$buildRows = New-Object System.Collections.Generic.List[string]
$levels = New-Object 'System.Collections.Generic.SortedSet[string]'
$unitState = @{}
$unitOrder = New-Object System.Collections.Generic.List[string]

foreach ($cfg in $Configurations) {
    $log = Join-Path $logDir "$cfg.log"
    Write-Host "rebuilding BattlefrontII.sln $cfg|x64"
    $start = (Get-Date).AddSeconds(-2)
    & $msbuild (Join-Path $root 'BattlefrontII.sln') '/t:Rebuild' "/p:Configuration=$cfg" '/p:Platform=x64' '/p:SWBF2_NOSTAGE=true' '/m' '/nologo' '/v:quiet' "/flp:logfile=$log;verbosity=normal" | Out-Null
    $rc = $LASTEXITCODE
    Write-Host "  msbuild exit $rc"
    if (-not (Test-Path -LiteralPath $log)) { throw "no log for $cfg" }
    $found = Read-Log $log
    foreach ($k in $found.Keys) {
        $f = $k.Split('|')
        $site = $f[0] + '|' + $f[1] + '|' + $f[3]
        if ($f[2] -eq 'error') {
            if (-not $errSites.ContainsKey($site)) { $errSites[$site] = $found[$k] }
            continue
        }
        if ($codes -notcontains $f[3]) { continue }
        if (-not $sites.ContainsKey($site)) { $sites[$site] = $found[$k]; $siteConfigs[$site] = New-Object System.Collections.Generic.List[string] }
        $siteConfigs[$site].Add($cfg)
    }
    $with = 0
    $total = 0
    foreach ($p in (Get-CensusProjects $cfg)) {
        if (Test-Path -LiteralPath $p.TlogDir) {
            foreach ($cmd in (Read-ClTlog @($p.TlogDir)).Values) {
                foreach ($w in [regex]::Matches($cmd, '(?<=^|\s)/W\d(?=\s|$)')) { [void]$levels.Add("$($p.Name) $($w.Value)") }
            }
        }
        foreach ($u in (Get-ProjectUnits $p)) {
            $key = $p.Name + ',' + (ConvertTo-RepoPath $u.Src)
            if (-not $unitState.ContainsKey($key)) { $unitState[$key] = New-Object System.Collections.Generic.List[string]; $unitOrder.Add($key) }
            $total++
            $item = Get-Item -LiteralPath $u.Obj -ErrorAction SilentlyContinue
            if ($item -and $item.LastWriteTime -ge $start) { $with++ }
            else { $unitState[$key].Add($cfg) }
        }
    }
    $buildRows.Add("| $cfg | $rc | $total | $with | $($total - $with) |")
}

$levelList = @($levels | ForEach-Object { $_.Split(' ')[1] } | Sort-Object -Unique)

$siteRows = New-Object System.Collections.Generic.List[string]
$byFile = @{}
$tot = @{}
foreach ($cd in $codes) { $tot[$cd] = @{ All = 0 }; foreach ($cfg in $Configurations) { $tot[$cd][$cfg] = 0 } }
$onlyOther = New-Object System.Collections.Generic.List[string]
$siteKeys = @($sites.Keys | Sort-Object { $_.Split('|')[0] }, { [int]$_.Split('|')[1] }, { $_.Split('|')[2] })
foreach ($s in $siteKeys) {
    $f = $s.Split('|')
    $tot[$f[2]].All++
    foreach ($cfg in ($siteConfigs[$s] | Sort-Object -Unique)) { $tot[$f[2]][$cfg]++ }
    if (-not $byFile.ContainsKey($f[0])) { $byFile[$f[0]] = @{} }
    $byFile[$f[0]][$f[2]] = 1 + [int]$byFile[$f[0]][$f[2]]
    if ($siteCodes -notcontains $f[2]) { continue }
    $siteRows.Add((@((ConvertTo-CsvField $f[0]), $f[1], $f[2], '', '', (ConvertTo-CsvField $sites[$s])) -join ','))
    if ($siteConfigs[$s] -notcontains 'Release') { $onlyOther.Add("| ``$($f[0])`` | $($f[1]) | $($f[2]) | $((@($siteConfigs[$s] | Sort-Object -Unique)) -join ', ') |") }
}
$fileRows = foreach ($k in (Get-Sorted $byFile.Keys)) { (@((ConvertTo-CsvField $k)) + @($codes | ForEach-Object { [int]$byFile[$k][$_] })) -join ',' }

$errByFile = @{}
foreach ($s in $errSites.Keys) { $k = $s.Split('|')[0].ToLowerInvariant(); $errByFile[$k] = 1 + [int]$errByFile[$k] }
$tuRows = New-Object System.Collections.Generic.List[string]
$missing = New-Object System.Collections.Generic.List[string]
$projStats = [ordered]@{}
foreach ($key in $unitOrder) {
    $p = $key.Split(',')
    if (-not $projStats.Contains($p[0])) { $projStats[$p[0]] = @(0, 0) }
    $projStats[$p[0]][0]++
    $flag = 'yes'
    if ($unitState[$key].Count) { $flag = 'no'; $missing.Add("$($p[0]): ``$($p[1])`` ($((@($unitState[$key])) -join ', '))") }
    else { $projStats[$p[0]][1]++ }
    $tuRows.Add("$key,$flag,$([int]$errByFile[$p[1].ToLowerInvariant()])")
}

$commit = (& git -C $root rev-parse HEAD).Trim()
$dirty = @(& git -C $root status --porcelain --untracked-files=no)
$clean = 'yes'
if ($dirty.Count -gt 0) { $clean = 'no' }

$md = New-Object System.Collections.Generic.List[string]
$md.Add("# x64 census: $Name")
$md.Add('')
$md.Add('Generated by `tests\x64-census.ps1`. `BattlefrontII.sln /t:Rebuild /p:Platform=x64 /p:SWBF2_NOSTAGE=true` for each configuration, at the warning level the projects set. Sites are unique (file, line, code) over all configurations. `' + $Name + '.csv` lists one row per C4311, C4312, C4302 and C4477 site; `' + $Name + '-files.csv` counts every code per file.')
$md.Add('')
$md.Add('| Key | Value |')
$md.Add('|-----|-------|')
$md.Add('| toolset | v143 |')
$md.Add("| cl | $clBanner |")
$md.Add("| MSVC tools version | $($tools.ToolsVersion) |")
$md.Add("| configurations | $($Configurations -join ', ') |")
$md.Add("| platform | x64 |")
$md.Add("| warning level (cl command tlogs) | $($levelList -join ', ') |")
$md.Add("| commit | $commit |")
$md.Add("| tracked tree clean | $clean |")
$md.Add('')
$md.Add('## Totals')
$md.Add('')
$md.Add('| Code | sites | ' + ($Configurations -join ' | ') + ' |')
$md.Add('|------|-------|' + (($Configurations | ForEach-Object { '-----' }) -join '|') + '|')
foreach ($cd in $codes) { $md.Add("| $cd | $($tot[$cd].All) | " + (($Configurations | ForEach-Object { $tot[$cd][$_] }) -join ' | ') + ' |') }
$md.Add('')
$md.Add('## Builds')
$md.Add('')
$md.Add('| Configuration | msbuild exit | units | with fresh x64 object | without |')
$md.Add('|---------------|--------------|-------|-----------------------|---------|')
foreach ($r in $buildRows) { $md.Add($r) }
$md.Add('')
$md.Add('## Units')
$md.Add('')
$md.Add('| Project | TUs | with x64 object | without |')
$md.Add('|---------|-----|-----------------|---------|')
foreach ($k in $projStats.Keys) { $md.Add("| $k | $($projStats[$k][0]) | $($projStats[$k][1]) | $($projStats[$k][0] - $projStats[$k][1]) |") }
$md.Add('')
$md.Add('A unit has an x64 object when the build of every configuration wrote its object.')
$md.Add('')
$md.Add('## Units without an x64 object')
$md.Add('')
if ($missing.Count) { foreach ($m in (Get-Sorted $missing)) { $md.Add("- $m") } } else { $md.Add('None.') }
$md.Add('')
$md.Add('## x64 errors')
$md.Add('')
if ($errSites.Count) {
    $md.Add('| File | Line | Code | Text |')
    $md.Add('|------|------|------|------|')
    foreach ($s in (Get-Sorted $errSites.Keys)) { $f = $s.Split('|'); $md.Add("| ``$($f[0])`` | $($f[1]) | $($f[2]) | $($errSites[$s].Replace('|', '\|')) |") }
}
else { $md.Add('None.') }
$md.Add('')
$md.Add('## Sites not in Release')
$md.Add('')
if ($onlyOther.Count) {
    $md.Add('| File | Line | Code | Configurations |')
    $md.Add('|------|------|------|----------------|')
    foreach ($r in $onlyOther) { $md.Add($r) }
}
else { $md.Add('None.') }

$csvPath = Join-Path $OutDir "$Name.csv"
$filesPath = Join-Path $OutDir "$Name-files.csv"
$tuPath = Join-Path $OutDir "$Name-tu.csv"
$mdPath = Join-Path $OutDir "$Name.md"
Write-CensusText $csvPath (@('file,line,code,family,plan,text') + $siteRows)
Write-CensusText $filesPath (@('file,' + ($codes -join ',')) + @($fileRows))
Write-CensusText $tuPath (@('project,tu,x64_obj,x64_errors') + (Get-Sorted $tuRows))
Write-CensusText $mdPath $md

foreach ($cd in $codes) { Write-Host ("{0}: {1} sites" -f $cd, $tot[$cd].All) }
foreach ($k in $projStats.Keys) { Write-Host ("{0}: {1}/{2} TUs with an x64 object" -f $k, $projStats[$k][1], $projStats[$k][0]) }
Write-Host "x64 error sites: $($errSites.Count)"
Write-Host "wrote $csvPath"
Write-Host "wrote $filesPath"
Write-Host "wrote $tuPath"
Write-Host "wrote $mdPath"
