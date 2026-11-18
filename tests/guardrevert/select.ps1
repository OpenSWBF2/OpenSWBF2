$script:GuardGlobalTriggers = @(
    'BattlefrontII.vcxproj',
    'BattlefrontII.vcxproj.filters',
    'BattlefrontII.sln',
    'OpenSWBF2.Game.props',
    'OpenSWBF2.Platform.props',
    'lib\',
    'src\LuaFL\LuaFL.vcxproj',
    'src\LuaFL\LuaFL.vcxproj.filters',
    'src\RedEngineFL\RedEngineFL.vcxproj',
    'src\RedEngineFL\RedEngineFL.vcxproj.filters',
    'src\mimalloc\mimalloc.vcxproj',
    'src\mimalloc\mimalloc.vcxproj.filters',
    'src\SoundFL\SoundFL.vcxproj',
    'src\SoundFL\SoundFL.vcxproj.filters',
    'src\Zephyr\ZephyrFL.vcxproj',
    'src\Zephyr\ZephyrFL.vcxproj.filters',
    'src\ZlibFL\ZLibFL.vcxproj',
    'src\ZlibFL\ZLibFL.vcxproj.filters',
    'LuaCompiler.vcxproj',
    'tests\OpenSWBF2Tests.vcxproj',
    'tests\OpenSWBF2Tests.vcxproj.filters',
    'tests\guard-revert.ps1',
    'tests\guardrevert\select.ps1',
    'tests\common.ps1',
    'tests\vstools.ps1',
    'tests\main.cpp',
    'tests\testutil.cpp',
    'tests\testutil.h',
    'tests\third_party\',
    'tests\stubs\'
)

$script:GuardProofFile = 'tests\guardrevert\proven.tsv'

function New-GuardPathSet
{
    New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
}

function Get-GuardSha([string]$Text)
{
    $sha = [Security.Cryptography.SHA256]::Create()
    try
    {
        $b = $sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($Text))
    }
    finally
    {
        $sha.Dispose()
    }
    ([BitConverter]::ToString($b) -replace '-', '').ToLowerInvariant()
}

function Get-GuardTestInfo([string]$Root)
{
    $root = $Root.TrimEnd('\')
    $sources = @{}
    $dups = @{}
    $literals = @{}
    foreach ($f in (Get-ChildItem -LiteralPath (Join-Path $root 'tests') -Recurse -File -Filter '*.cpp' | Where-Object { $_.FullName -notmatch '\\tests\\third_party\\' }))
    {
        $rel = $f.FullName.Substring($root.Length + 1)
        $text = [IO.File]::ReadAllText($f.FullName)
        foreach ($m in [regex]::Matches($text, 'TEST_CASE\s*\(\s*"((?:[^"\\]|\\.)*)"'))
        {
            $name = $m.Groups[1].Value -replace '\\(.)', '$1'
            if ($sources.ContainsKey($name) -and $sources[$name] -ne $rel)
            {
                $dups[$name] = $true
            }
            $sources[$name] = $rel
        }
        $lits = New-Object System.Collections.Generic.List[string]
        foreach ($m in [regex]::Matches($text, '"((?:[^"\\\r\n]|\\.)*)"'))
        {
            $v = $m.Groups[1].Value
            if ($v -match '^(?:src\\\\|src/|\.\./src/|data\\\\|data/|\.\./data/|tests\\\\|tests/|\.\./tests/)')
            {
                $lits.Add((($v -replace '^\.\./', '').Replace('\\', '\').Replace('/', '\')))
            }
            elseif ($v -match '^corpus(?:$|\\\\|/)')
            {
                $lits.Add('tests\' + $v.Replace('\\', '\').Replace('/', '\'))
            }
        }
        if ($lits.Count)
        {
            $literals[$rel] = @($lits | Select-Object -Unique)
        }
    }
    foreach ($name in $dups.Keys)
    {
        $sources.Remove($name)
    }
    @{ Sources = $sources; Literals = $literals }
}

function ConvertTo-GuardSorted($Value, [string]$Prefix)
{
    if ($null -eq $Value)
    {
        return $null
    }
    if ($Value -is [string])
    {
        if ($Prefix)
        {
            return [regex]::Replace($Value, [regex]::Escape($Prefix), '<root>\', 'IgnoreCase')
        }
        return $Value
    }
    if ($Value -is [Collections.IDictionary])
    {
        $o = [ordered]@{}
        $keys = [string[]]@($Value.Keys)
        [Array]::Sort($keys, [StringComparer]::Ordinal)
        foreach ($k in $keys)
        {
            $o[$k] = ConvertTo-GuardSorted $Value[$k] $Prefix
        }
        return $o
    }
    if ($Value -is [Management.Automation.PSCustomObject])
    {
        $o = [ordered]@{}
        $keys = [string[]]@($Value.PSObject.Properties | ForEach-Object { $_.Name })
        [Array]::Sort($keys, [StringComparer]::Ordinal)
        foreach ($k in $keys)
        {
            $o[$k] = ConvertTo-GuardSorted $Value.$k $Prefix
        }
        return $o
    }
    if ($Value -is [Collections.IEnumerable])
    {
        $list = New-Object Collections.ArrayList
        foreach ($x in $Value)
        {
            [void]$list.Add((ConvertTo-GuardSorted $x $Prefix))
        }
        return , $list.ToArray()
    }
    $Value
}

function Get-GuardCaseData([string]$Path)
{
    if (-not [IO.File]::Exists($Path))
    {
        return $null
    }
    $errs = $null
    $ast = [Management.Automation.Language.Parser]::ParseFile($Path, [ref]$null, [ref]$errs)
    if (@($errs).Count)
    {
        return $null
    }
    $parts = New-Object 'System.Collections.Generic.List[string]'
    foreach ($b in @($ast.ParamBlock, $ast.DynamicParamBlock, $ast.BeginBlock, $ast.ProcessBlock))
    {
        if ($null -ne $b)
        {
            $parts.Add($b.Extent.Text)
        }
    }
    if ($null -ne $ast.EndBlock)
    {
        foreach ($s in $ast.EndBlock.Statements)
        {
            if ($s -is [Management.Automation.Language.FunctionDefinitionAst] -and $s.Name -eq 'Get-SmokeCases')
            {
                continue
            }
            $parts.Add($s.Extent.Text)
        }
    }
    ($parts -join "`n") -replace "`r`n", "`n"
}

function Get-GuardCaseTexts($Rows, [string]$DefaultConfig, [string]$Root)
{
    $prefix = $Root.TrimEnd('\') + '\'
    $texts = @{}
    $sets = @{}
    $savedConfig = $script:SmokeDigestConfiguration
    $savedPerturb = $env:OPENSWBF2_PERTURB_MISSIONS
    try
    {
        foreach ($row in $Rows)
        {
            $kind = Get-GuardRowKind $row
            $cases = @()
            if ($kind -eq 'Smoke')
            {
                $cfg = [string]$row.Config
                if (-not $cfg)
                {
                    $cfg = $DefaultConfig
                }
                if (-not $sets.ContainsKey($cfg))
                {
                    $script:SmokeDigestConfiguration = $cfg
                    $set = @{}
                    foreach ($w in 'Crash', 'Standard')
                    {
                        foreach ($c in (Get-SmokeCases $w @() $false $null))
                        {
                            $set[$c.Name] = $c
                        }
                    }
                    $sets[$cfg] = $set
                }
                $c = $sets[$cfg][[string]$row.Case]
                if ($null -ne $c)
                {
                    $cases = @($c)
                }
            }
            elseif ($kind -eq 'Perturb')
            {
                $script:SmokeDigestConfiguration = 'Test'
                $env:OPENSWBF2_PERTURB_MISSIONS = @($row.Missions) -join ','
                try
                {
                    $cases = @(Add-MiOptionsExpect @(Add-SerialTieExpect @(Get-SmokeCases 'Perturb' @() $false $null)) -1)
                }
                catch
                {
                    $cases = @()
                }
            }
            else
            {
                continue
            }
            if ($cases.Count -eq 0)
            {
                continue
            }
            $lines = New-Object 'System.Collections.Generic.List[string]'
            foreach ($c in $cases)
            {
                $lines.Add("case`t" + (ConvertTo-Json (ConvertTo-GuardSorted $c $prefix) -Compress -Depth 20))
                $lines.Add("digest`t" + [string](Get-DigestExpected $c.Name))
                foreach ($a in @(Get-SmokeAllowed $c.Name))
                {
                    $lines.Add("allowed`t$a")
                }
                foreach ($m in [regex]::Matches([string]$c.Args, '(?:^| )-displayini\s+(?:"([^"]*)"|(\S+))'))
                {
                    $p = $m.Groups[2].Value
                    if ($m.Groups[1].Success)
                    {
                        $p = $m.Groups[1].Value
                    }
                    $h = '-'
                    if ($p -notmatch '\{CASEDIR\}' -and [IO.File]::Exists($p))
                    {
                        $h = Get-GuardSha ([Convert]::ToBase64String([IO.File]::ReadAllBytes($p)))
                    }
                    $lines.Add("displayini`t" + (ConvertTo-GuardSorted $p $prefix) + "`t$h")
                }
            }
            $texts[[string]$row.Id] = $lines -join "`n"
        }
    }
    finally
    {
        $script:SmokeDigestConfiguration = $savedConfig
        $env:OPENSWBF2_PERTURB_MISSIONS = $savedPerturb
    }
    $texts
}

function Read-ClReadTlog([string]$Path, [string]$Root)
{
    if (-not $Path -or -not [IO.File]::Exists($Path))
    {
        return $null
    }
    if (-not $Root.EndsWith('\'))
    {
        $Root += '\'
    }
    $cmp = [StringComparison]::OrdinalIgnoreCase
    $rl = $Root.Length
    $time = [IO.File]::GetLastWriteTimeUtc($Path)
    $text = [IO.File]::ReadAllText($Path)
    $sets = New-Object 'System.Collections.Generic.Dictionary[string,System.Collections.Generic.HashSet[string]]' ([StringComparer]::OrdinalIgnoreCase)
    $cur = New-Object 'System.Collections.Generic.List[System.Collections.Generic.HashSet[string]]'
    foreach ($l in $text.Split([char[]]"`r`n", [StringSplitOptions]::RemoveEmptyEntries))
    {
        if ($l[0] -eq [char]'^')
        {
            $cur.Clear()
            foreach ($s in $l.Substring(1).Split('|'))
            {
                if ($s.StartsWith($Root, $cmp))
                {
                    $rel = $s.Substring($rl)
                    $set = $null
                    if (-not $sets.TryGetValue($rel, [ref]$set))
                    {
                        $set = New-GuardPathSet
                        [void]$set.Add($rel)
                        $sets[$rel] = $set
                    }
                    $cur.Add($set)
                }
            }
        }
        elseif ($cur.Count -gt 0 -and $l.StartsWith($Root, $cmp))
        {
            $rel = $l.Substring($rl)
            foreach ($set in $cur)
            {
                [void]$set.Add($rel)
            }
        }
    }
    $fileStale = New-Object 'System.Collections.Generic.Dictionary[string,bool]' ([StringComparer]::OrdinalIgnoreCase)
    $stale = New-GuardPathSet
    foreach ($kv in $sets.GetEnumerator())
    {
        foreach ($p in $kv.Value)
        {
            $b = $false
            if (-not $fileStale.TryGetValue($p, [ref]$b))
            {
                $full = $Root + $p
                $b = (-not [IO.File]::Exists($full)) -or ([IO.File]::GetLastWriteTimeUtc($full) -gt $time)
                $fileStale[$p] = $b
            }
            if ($b)
            {
                [void]$stale.Add($kv.Key)
                break
            }
        }
    }
    [pscustomobject]@{ Path = $Path; Root = $Root; Time = $time; Sets = $sets; Stale = $stale }
}

function Read-ClReadTlogs([string[]]$Paths, [string]$Root)
{
    $merged = $null
    foreach ($p in $Paths)
    {
        $r = Read-ClReadTlog $p $Root
        if ($null -eq $r)
        {
            return $null
        }
        if ($null -eq $merged)
        {
            $merged = $r
            continue
        }
        foreach ($kv in $r.Sets.GetEnumerator())
        {
            $set = $null
            if ($merged.Sets.TryGetValue($kv.Key, [ref]$set))
            {
                $set.UnionWith($kv.Value)
            }
            else
            {
                $merged.Sets[$kv.Key] = $kv.Value
            }
        }
        $merged.Stale.UnionWith($r.Stale)
    }
    $merged
}

function Get-GuardRowHash($Row)
{
    $keys = [string[]]@($Row.Keys)
    [Array]::Sort($keys, [StringComparer]::Ordinal)
    $lines = foreach ($k in $keys)
    {
        $v = $Row[$k]
        if ($v -is [array])
        {
            $v = @($v | ForEach-Object { [string]$_ }) -join "`n"
        }
        "$k=$v"
    }
    Get-GuardSha ((@($lines) -join "`n") -replace "`r`n", "`n")
}

function Get-GuardRowPlatform($Row)
{
    if ($Row.Platform)
    {
        return [string]$Row.Platform
    }
    'Win32'
}

function Select-GuardPlatformRows($Rows, [string]$Platform)
{
    $Rows | Where-Object { (Get-GuardRowPlatform $_) -ceq $Platform }
}

function Get-GuardRowKind($Row)
{
    if ($Row.Kind)
    {
        return [string]$Row.Kind
    }
    if ($Row.Asan -eq $true)
    {
        return 'Asan'
    }
    'T1'
}

function Test-GuardGenerated([string]$Path)
{
    $Path.StartsWith('obj\', [StringComparison]::OrdinalIgnoreCase) -or $Path.StartsWith('bin\', [StringComparison]::OrdinalIgnoreCase)
}

function Get-GuardKeysUnder($Ctx, [string]$Prefix)
{
    $v = $Ctx.Under[$Prefix]
    if ($null -eq $v)
    {
        $list = New-Object 'System.Collections.Generic.List[string]'
        foreach ($k in $Ctx.Keys)
        {
            if ($k.StartsWith($Prefix, [StringComparison]::OrdinalIgnoreCase) -and $k -ne $script:GuardProofFile)
            {
                $list.Add($k)
            }
        }
        $v = $list.ToArray()
        $Ctx.Under[$Prefix] = $v
    }
    , $v
}

function Get-GuardUsers($Ctx, $Reads, [string]$Tag, [string]$File)
{
    $key = "$Tag|$File"
    $v = $Ctx.Users[$key]
    if ($null -eq $v)
    {
        $list = New-Object 'System.Collections.Generic.List[string]'
        foreach ($kv in $Reads.Sets.GetEnumerator())
        {
            if ($kv.Value.Contains($File))
            {
                $list.Add($kv.Key)
            }
        }
        $v = $list.ToArray()
        $Ctx.Users[$key] = $v
    }
    , $v
}

function Get-GuardTuInfo($Ctx, $Reads, [string]$Tag, [string]$Tu)
{
    $key = "$Tag|$Tu"
    $info = $Ctx.Tu[$key]
    if ($null -ne $info)
    {
        return $info
    }
    $paths = [string[]]@($Reads.Sets[$Tu])
    [Array]::Sort($paths, [StringComparer]::OrdinalIgnoreCase)
    $sb = New-Object Text.StringBuilder
    $unmapped = $null
    foreach ($p in $paths)
    {
        if ($Ctx.Hashes.ContainsKey($p))
        {
            [void]$sb.Append($p.ToLowerInvariant()).Append("`t").Append([string]$Ctx.Hashes[$p]).Append("`n")
        }
        elseif ($null -eq $unmapped -and -not (Test-GuardGenerated $p))
        {
            $unmapped = $p
        }
    }
    $info = @{ Digest = (Get-GuardSha $sb.ToString()); Unmapped = $unmapped; Stale = $Reads.Stale.Contains($Tu) }
    $Ctx.Tu[$key] = $info
    $info
}

function Get-GuardChangedSince([string]$Root, [string[]]$Commits)
{
    $untracked = @(Get-KeyGitList $Root @('ls-files', '--others', '--exclude-standard', '-z'))
    $changed = @{}
    foreach ($c in @($Commits | Where-Object { $_ } | Select-Object -Unique))
    {
        try
        {
            $diff = @(Get-KeyGitList $Root @('-c', 'core.quotepath=off', 'diff', '--name-only', '--no-renames', '-z', $c, '--'))
        }
        catch
        {
            $changed[$c] = $null
            continue
        }
        $set = New-GuardPathSet
        foreach ($p in ($diff + $untracked))
        {
            [void]$set.Add($p.Replace('/', '\'))
        }
        $changed[$c] = $set
    }
    return $changed
}

function Select-GuardRows($In)
{
    $cmp = [StringComparison]::OrdinalIgnoreCase
    $hashes = $In.Hashes
    if ($null -eq $hashes)
    {
        $hashes = @{}
    }
    $proofs = $In.Proofs
    if ($null -eq $proofs)
    {
        $proofs = @{}
    }
    $triggers = $In.GlobalTriggers
    if ($null -eq $triggers)
    {
        $triggers = $script:GuardGlobalTriggers
    }
    $toolset = [string]$In.Toolset
    $platform = [string]$In.Platform
    $asanReads = $In.AsanReads
    if (-not $In.ContainsKey('AsanReads'))
    {
        $asanReads = $In.TestsReads
    }
    $testsGameTUs = @($In.TestsProjectGameTUs | Where-Object { $_ })
    $testsGameSet = New-GuardPathSet
    foreach ($g in $testsGameTUs)
    {
        [void]$testsGameSet.Add($g)
    }
    $testSources = $In.TestSources
    if ($null -eq $testSources)
    {
        $testSources = @{}
    }
    $testLiterals = $In.TestLiterals
    if ($null -eq $testLiterals)
    {
        $testLiterals = @{}
    }
    $caseTexts = $In.CaseTexts
    if ($null -eq $caseTexts)
    {
        $caseTexts = @{}
    }
    $caseData = $In.CaseData
    $caseDataDigest = $null
    if ($null -ne $caseData)
    {
        $caseDataDigest = Get-GuardSha (([string]$caseData) -replace "`r`n", "`n")
    }
    $ctx = @{ Hashes = $hashes; Keys = [string[]]@($hashes.Keys); Under = @{}; Users = @{}; Tu = @{} }

    $tlines = New-Object 'System.Collections.Generic.List[string]'
    foreach ($t in $triggers)
    {
        if ($t.EndsWith('\'))
        {
            foreach ($k in (Get-GuardKeysUnder $ctx $t))
            {
                $tlines.Add($k.ToLowerInvariant() + "`t" + [string]$hashes[$k])
            }
        }
        elseif ($hashes.ContainsKey($t))
        {
            $tlines.Add($t.ToLowerInvariant() + "`t" + [string]$hashes[$t])
        }
        else
        {
            $tlines.Add($t.ToLowerInvariant() + "`t-")
        }
    }
    $tarr = $tlines.ToArray()
    [Array]::Sort($tarr, [StringComparer]::Ordinal)
    $triggerDigest = Get-GuardSha ($tarr -join "`n")
    $sourceExt = @('.c', '.cpp', '.cc', '.h', '.hpp', '.inl')

    foreach ($row in $In.Rows)
    {
        $kind = Get-GuardRowKind $row
        $rowHash = Get-GuardRowHash $row
        $file = [string]$row.File
        $files = New-GuardPathSet
        $tus = New-GuardPathSet
        $stale = $null
        $unmapped = $null
        $unmappedLit = $null
        $unknownTest = $null
        $noTlog = $null
        $unknownCase = $null
        $extra = New-Object 'System.Collections.Generic.List[string]'
        [void]$files.Add($file)

        $reads = $null
        $tag = $null
        if ($kind -eq 'T1' -or $kind -eq 'Asan' -or $kind -eq 'AsanCanary')
        {
            if ($kind -eq 'T1')
            {
                $reads = $In.TestsReads
                $tag = 'tests'
            }
            else
            {
                $reads = $asanReads
                $tag = 'asan'
            }
            if ($null -eq $reads)
            {
                $noTlog = "$tag tlog"
            }
            else
            {
                foreach ($u in (Get-GuardUsers $ctx $reads $tag $file))
                {
                    [void]$tus.Add($u)
                }
                $compiled = $testsGameSet.Contains($file) -or ($file.StartsWith('tests\', $cmp) -and $file.EndsWith('.cpp', $cmp))
                if ($compiled -and -not $reads.Sets.ContainsKey($file))
                {
                    $noTlog = $file
                }
                foreach ($g in $testsGameTUs)
                {
                    [void]$files.Add($g)
                    if ($reads.Sets.ContainsKey($g))
                    {
                        [void]$tus.Add($g)
                    }
                    elseif ($null -eq $noTlog)
                    {
                        $noTlog = $g
                    }
                }
            }
            foreach ($t in @($row.Tests))
            {
                if (-not $t)
                {
                    continue
                }
                $src = [string]$testSources[$t]
                if (-not $src)
                {
                    if ($null -eq $unknownTest)
                    {
                        $unknownTest = $t
                    }
                    continue
                }
                [void]$files.Add($src)
                if ($null -ne $reads)
                {
                    if ($reads.Sets.ContainsKey($src))
                    {
                        [void]$tus.Add($src)
                    }
                    elseif ($null -eq $noTlog)
                    {
                        $noTlog = $src
                    }
                }
                foreach ($lit in @($testLiterals[$src]))
                {
                    if (-not $lit)
                    {
                        continue
                    }
                    if ($hashes.ContainsKey($lit))
                    {
                        [void]$files.Add($lit)
                        continue
                    }
                    $under = Get-GuardKeysUnder $ctx ($lit.TrimEnd('\') + '\')
                    if ($under.Count -gt 0)
                    {
                        $files.UnionWith([string[]]$under)
                    }
                    elseif ($null -eq $unmappedLit)
                    {
                        $unmappedLit = $lit
                    }
                }
            }
        }
        elseif ($kind -eq 'Hardening' -or $kind -eq 'Smoke' -or $kind -eq 'Perturb' -or $kind -eq 'Tlog')
        {
            [void]$files.Add('tests\smoke.ps1')
            if ($null -eq $caseDataDigest)
            {
                $unknownCase = 'case data'
            }
            else
            {
                $extra.Add("casedata`t$caseDataDigest")
            }
            if ($kind -eq 'Smoke' -or $kind -eq 'Perturb')
            {
                $ct = $caseTexts[[string]$row.Id]
                if (-not $ct)
                {
                    if ($kind -eq 'Smoke')
                    {
                        $unknownCase = "case $($row.Case)"
                    }
                    else
                    {
                        $unknownCase = 'case perturb ' + (@($row.Missions) -join ',')
                    }
                }
                else
                {
                    $extra.Add("case`t" + (Get-GuardSha (([string]$ct) -replace "`r`n", "`n")))
                }
            }
            $reads = $In.GameReads
            $tag = 'game'
            if ($kind -eq 'Tlog')
            {
                $reads = $null
            }
            elseif ($null -eq $reads)
            {
                $noTlog = 'game tlog'
            }
            else
            {
                $users = Get-GuardUsers $ctx $reads $tag $file
                foreach ($u in $users)
                {
                    [void]$tus.Add($u)
                }
                if ($kind -eq 'Smoke' -or $kind -eq 'Perturb')
                {
                    foreach ($u in $reads.Sets.Keys)
                    {
                        [void]$tus.Add($u)
                    }
                }
                $isSrc = $file.StartsWith('src\', $cmp) -and ($sourceExt -contains [IO.Path]::GetExtension($file).ToLowerInvariant())
                if ($isSrc -and $users.Count -eq 0)
                {
                    $noTlog = $file
                }
            }
            if ($kind -eq 'Smoke' -or $kind -eq 'Perturb')
            {
                foreach ($pre in @('data\scripts\'))
                {
                    $under = Get-GuardKeysUnder $ctx $pre
                    if ($under.Count -gt 0)
                    {
                        $files.UnionWith([string[]]$under)
                    }
                }
            }
        }

        $lines = New-Object 'System.Collections.Generic.List[string]'
        $lines.AddRange($extra)
        foreach ($p in $files)
        {
            if ($p -eq $script:GuardProofFile)
            {
                continue
            }
            if ($hashes.ContainsKey($p))
            {
                $lines.Add($p.ToLowerInvariant() + "`t" + [string]$hashes[$p])
            }
            elseif ($null -eq $unmapped -and -not (Test-GuardGenerated $p))
            {
                $unmapped = $p
            }
        }
        foreach ($tu in $tus)
        {
            $info = Get-GuardTuInfo $ctx $reads $tag $tu
            $lines.Add("tu:$tag`:" + $tu.ToLowerInvariant() + "`t" + $info.Digest)
            if ($info.Stale -and $null -eq $stale)
            {
                $stale = $tu
            }
            if ($null -ne $info.Unmapped -and $null -eq $unmapped)
            {
                $unmapped = $info.Unmapped
            }
        }
        $arr = $lines.ToArray()
        [Array]::Sort($arr, [StringComparer]::Ordinal)
        $depDigest = Get-GuardSha ("row`t$rowHash`nplatform`t$platform`ntoolset`t$toolset`ntriggers`t$triggerDigest`n" + ($arr -join "`n"))

        $doubt = $null
        if ($null -ne $stale)
        {
            $doubt = "stale tlog $stale"
        }
        elseif ($null -ne $unmapped)
        {
            $doubt = "unmapped $unmapped"
        }
        elseif ($null -ne $unmappedLit)
        {
            $doubt = "unmapped literal $unmappedLit"
        }
        elseif ($null -ne $unknownTest)
        {
            $doubt = "unknown test $unknownTest"
        }
        elseif ($null -ne $unknownCase)
        {
            $doubt = "unknown $unknownCase"
        }
        elseif ($null -ne $noTlog)
        {
            $doubt = "no tlog $noTlog"
        }

        $proof = $proofs["$($row.Id)`t$platform"]
        $reason = $null
        if (-not $platform)
        {
            $reason = 'platform'
        }
        elseif ($null -eq $proof)
        {
            $reason = 'no record'
        }
        elseif ([string]$proof.Platform -cne $platform)
        {
            $reason = 'platform'
        }
        elseif ($proof.RowHash -ne $rowHash)
        {
            $reason = 'row changed'
        }
        elseif ($proof.Toolset -ne $toolset)
        {
            $reason = 'toolset'
        }
        elseif ($null -ne $doubt)
        {
            $reason = $doubt
        }
        elseif ($proof.TriggerDigest -ne $triggerDigest)
        {
            $reason = 'global trigger'
        }
        elseif ($proof.DepDigest -ne $depDigest)
        {
            $reason = 'deps changed'
        }
        $commit = $null
        if ($null -ne $proof)
        {
            $commit = $proof.Commit
        }
        $action = 'CARRY'
        if ($null -ne $reason)
        {
            $action = 'SELECT'
        }
        if ($In.Narrow -and $action -eq 'SELECT' -and @('no record', 'row changed', 'platform') -notcontains $reason)
        {
            $direct = New-Object 'System.Collections.Generic.List[string]'
            $direct.Add($file)
            foreach ($t in @($row.Tests))
            {
                if ($t -and $testSources[$t])
                {
                    $direct.Add([string]$testSources[$t])
                }
            }
            $changedSet = $null
            if ($null -ne $In.Changed -and $commit -and $In.Changed.ContainsKey([string]$commit))
            {
                $changedSet = $In.Changed[[string]$commit]
            }
            if ($null -eq $changedSet)
            {
                $reason = "proof commit $commit unknown"
            }
            else
            {
                $touched = $null
                foreach ($p in $direct)
                {
                    if ($changedSet.Contains($p))
                    {
                        $touched = $p
                        break
                    }
                }
                if ($null -ne $touched)
                {
                    $reason = "touched $touched"
                }
                else
                {
                    $action = 'SKIP'
                }
            }
        }
        [pscustomobject]@{
            Id = $row.Id
            Kind = $kind
            Action = $action
            Reason = $reason
            Doubt = $doubt
            Commit = $commit
            DepDigest = $depDigest
            RowHash = $rowHash
            TriggerDigest = $triggerDigest
            Deps = $(if ($In.WithDeps) { , $arr } else { $null })
        }
    }
}
