$script:GuardGlobalTriggers = @(
    'BattlefrontII.vcxproj',
    'BattlefrontII.vcxproj.filters',
    'BattlefrontII.sln',
    'LuaCompiler.vcxproj',
    'tests\OpenSWBF2Tests.vcxproj',
    'tests\OpenSWBF2Tests.vcxproj.filters',
    'tests\guard-revert.ps1',
    'tests\guardrevert\select.ps1',
    'tests\smoke.ps1',
    'tests\run.ps1',
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
        elseif ($kind -eq 'Hardening' -or $kind -eq 'Smoke')
        {
            $reads = $In.GameReads
            $tag = 'game'
            if ($null -eq $reads)
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
                if ($kind -eq 'Smoke')
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
            if ($kind -eq 'Smoke')
            {
                foreach ($pre in 'data\scripts\', 'tests\smoke\')
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
        $depDigest = Get-GuardSha ("row`t$rowHash`ntoolset`t$toolset`ntriggers`t$triggerDigest`n" + ($arr -join "`n"))

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
        elseif ($null -ne $noTlog)
        {
            $doubt = "no tlog $noTlog"
        }

        $proof = $proofs[$row.Id]
        $reason = $null
        if ($null -eq $proof)
        {
            $reason = 'no record'
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
        }
    }
}
