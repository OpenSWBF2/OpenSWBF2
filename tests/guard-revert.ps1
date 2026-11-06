param(
    [string[]]$Row,
    [switch]$Keep,
    [string]$OutDir,
    [string]$Scratch,
    [string]$RowsFile,
    [int]$CaseTimeout = 30,
    [ValidateSet('', 'Debug', 'Release', 'Test')][string]$Configuration = '',
    [switch]$Full,
    [switch]$ListSelected,
    [string]$ProvenFile
)
$ErrorActionPreference = 'Stop'
if ($Row) { $Row = @($Row | ForEach-Object { $_ -split ',' }) }
$testsDir = $PSScriptRoot
$root = Split-Path $testsDir -Parent
if (-not $RowsFile) { $RowsFile = Join-Path $testsDir 'guardrevert\rows.ps1' }
if (-not $ProvenFile) { $ProvenFile = Join-Path $testsDir 'guardrevert\proven.tsv' }
$ProvenFile = [IO.Path]::GetFullPath($ProvenFile)
$script:ProofColumns = @('Id', 'RowHash', 'DepDigest', 'TriggerDigest', 'Toolset', 'Tree', 'Commit', 'Result', 'Control', 'Config', 'Date')
if (-not $Scratch) { $Scratch = Join-Path $root 'artifacts\guard-revert\wt' }
$Scratch = [IO.Path]::GetFullPath($Scratch)
if ($env:TEMP -and $Scratch.StartsWith([IO.Path]::GetFullPath($env:TEMP), [StringComparison]::OrdinalIgnoreCase)) { Write-Host 'Scratch must not be under TEMP: MSBuild incremental builds miss header changes there.'; exit 2 }
. (Join-Path $testsDir 'smoke.ps1')
. (Join-Path $testsDir 'guardrevert\select.ps1')
$stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
if (-not $OutDir) { $OutDir = Join-Path $root "artifacts\guard-revert\$stamp" }
$script:TestsConfiguration = 'Debug'
$script:GameConfiguration = 'Release'
if ($Configuration) { $script:TestsConfiguration = $Configuration; $script:GameConfiguration = $Configuration }
$script:SmokeDigestConfiguration = $script:GameConfiguration

function Invoke-Git([string]$dir, [string[]]$gitArgs) {
    $ErrorActionPreference = 'Continue'
    $out = & git -C $dir @gitArgs 2>&1
    if ($LASTEXITCODE -ne 0) { throw "git $($gitArgs -join ' ') failed: $out" }
    return $out
}

function Build-Tests([string]$wt, [string]$logPath, [string]$config = $script:TestsConfiguration) {
    $ErrorActionPreference = 'Continue'
    $out = & $script:msbuild (Join-Path $wt 'tests\OpenSWBF2Tests.vcxproj') "/p:Configuration=$config" '/p:Platform=Win32' /m /v:minimal /nologo 2>&1
    $code = $LASTEXITCODE
    $out | Set-Content -Path $logPath -Encoding ASCII
    return ($code -eq 0)
}

function Build-Game([string]$wt, [string]$logPath) {
    $ErrorActionPreference = 'Continue'
    $env:SWBF2_NOSTAGE = 'true'
    $out = & $script:msbuild (Join-Path $wt 'BattlefrontII.vcxproj') "/p:Configuration=$script:GameConfiguration" '/p:Platform=Win32' '/p:SWBF2_NOSTAGE=true' /m /v:minimal /nologo 2>&1
    $code = $LASTEXITCODE
    $out | Set-Content -Path $logPath -Encoding ASCII
    return ($code -eq 0)
}

function Run-Smoke([string]$wt, [string]$tag, $case) {
    $caseDir = Join-Path $OutDir "run-$tag\case"
    New-Item -ItemType Directory -Force -Path $caseDir | Out-Null
    $runDir = Get-SmokeWorkerRoot $root "guard-$script:GameConfiguration"
    $srcBin = Join-Path $root "bin\Win32\$script:GameConfiguration"
    New-SmokeWorkerDir $srcBin $runDir
    $dataLink = Join-Path $runDir 'data'
    try {
        Copy-Item (Join-Path $wt "bin\Win32\$script:GameConfiguration\BattlefrontII.exe") $runDir -Force
        $argLine = $case.Args.Replace('{CASEDIR}', $caseDir)
        $res = Invoke-SmokeCase $case.Name (Join-Path $runDir 'BattlefrontII.exe') $runDir $argLine 120 $caseDir
    }
    finally {
        & cmd /c rmdir "$dataLink" | Out-Null
    }
    $ok = Test-SmokeExpect $case $res
    return New-Object PSObject -Property @{ Ok = $ok; Class = $res.Class; Code = $res.Code; Dumps = @($res.Dumps).Count; LogText = $res.LogText }
}

function Run-Case([string]$exe, [string]$wt, [string]$name, [string]$asanDir) {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exe
    $isProbe = $name -like 'strictness probe*'
    $psi.Arguments = '"-tc=' + ($name -replace '"', '\"') + '" --timeout 10' + $(if ($isProbe) { ' -ns' } else { '' })
    $psi.WorkingDirectory = Join-Path $wt 'tests'
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    if ($asanDir) {
        if (Test-Path -LiteralPath $asanDir) { Remove-Item -LiteralPath $asanDir -Recurse -Force }
        New-Item -ItemType Directory -Force -Path $asanDir | Out-Null
        $psi.EnvironmentVariables['ASAN_OPTIONS'] = Get-AsanOptions $asanDir
    }
    $p = [System.Diagnostics.Process]::Start($psi)
    $o = $p.StandardOutput.ReadToEndAsync()
    $e = $p.StandardError.ReadToEndAsync()
    if (-not $p.WaitForExit($CaseTimeout * 1000)) {
        try { $p.Kill() } catch {}
        $p.WaitForExit()
        $report = $null
        if ($asanDir) { $report = Get-AsanReport $asanDir 124 }
        return New-Object PSObject -Property @{ State = 'RED'; Detail = 'timeout'; Code = 124; Text = $o.Result + $e.Result; Report = $report }
    }
    $code = [BitConverter]::ToUInt32([BitConverter]::GetBytes([int]$p.ExitCode), 0)
    $text = $o.Result + $e.Result
    $state = 'RED'
    $detail = ''
    if ($isProbe) {
        if ($code -eq 1 -and $text -match '\[UNEXPECTED-LOG\]') { $state = 'GREEN' }
        else { $state = 'RED' }
    }
    elseif ($code -eq 0) {
        if ($text -match 'test cases:\s+1 \|\s+1 passed') { $state = 'GREEN' }
        else { $state = 'NO-MATCH' }
    }
    elseif ($code -eq 77) { $state = 'SKIP' }
    elseif ($code -eq 2) { $state = 'NO-MATCH' }
    elseif ($code -eq 1) { $state = 'RED' }
    else { $detail = ('crash 0x{0:X8}' -f $code) }
    $report = $null
    if ($asanDir) {
        $report = Get-AsanReport $asanDir $code
        if ($report) {
            $state = 'RED'
            $detail = (@($detail, 'asan report') | Where-Object { $_ }) -join ' '
            $text += "`r`n" + $report
        }
    }
    return New-Object PSObject -Property @{ State = $state; Detail = $detail; Code = $code; Text = $text; Report = $report }
}

function Set-Anchor([string]$path, [string]$anchor, [string]$replacement) {
    $latin = [Text.Encoding]::GetEncoding(28591)
    $bytes = [IO.File]::ReadAllBytes($path)
    $text = $latin.GetString($bytes)
    if ($text.Contains("`r`n")) {
        if (-not $anchor.Contains("`r")) { $anchor = $anchor.Replace("`n", "`r`n") }
        if (-not $replacement.Contains("`r")) { $replacement = $replacement.Replace("`n", "`r`n") }
    }
    $count = 0
    $at = 0
    while (($at = $text.IndexOf($anchor, $at, [StringComparison]::Ordinal)) -ge 0) { $count++; $at += $anchor.Length }
    if ($count -ne 1) { throw "anchor matched $count times in $path" }
    $idx = $text.IndexOf($anchor, [StringComparison]::Ordinal)
    $new = $text.Substring(0, $idx) + $replacement + $text.Substring($idx + $anchor.Length)
    [IO.File]::WriteAllBytes($path, $latin.GetBytes($new))
    return $bytes
}

function Get-GitList([string[]]$gitArgs) {
    return @(((Invoke-Git $root $gitArgs) -join "`n").Split([char]0) | Where-Object { $_ })
}

function Get-GuardHashes {
    $h = New-Object System.Collections.Hashtable ([StringComparer]::OrdinalIgnoreCase)
    foreach ($e in (Get-GitList @('ls-files', '-s', '-z'))) {
        $tab = $e.IndexOf("`t")
        $h[$e.Substring($tab + 1).Replace('/', '\')] = $e.Split(' ')[1]
    }
    $live = @()
    foreach ($p in (@(Get-GitList @('diff', '--name-only', '-z', 'HEAD')) + @(Get-GitList @('ls-files', '--others', '--exclude-standard', '-z')))) {
        $k = $p.Replace('/', '\')
        if (Test-Path -LiteralPath (Join-Path $root $k) -PathType Leaf) { $live += $p }
        else { $h.Remove($k) }
    }
    if ($live.Count) {
        $ErrorActionPreference = 'Continue'
        $out = @($live | & git -C $root hash-object --stdin-paths)
        if ($LASTEXITCODE -ne 0 -or $out.Count -ne $live.Count) { throw 'git hash-object failed' }
        for ($i = 0; $i -lt $live.Count; $i++) { $h[$live[$i].Replace('/', '\')] = "$($out[$i])".Trim() }
    }
    return $h
}

function Get-GuardTestInfo {
    $sources = @{}
    $dups = @{}
    $literals = @{}
    foreach ($f in (Get-ChildItem -LiteralPath $testsDir -Recurse -File -Filter '*.cpp' | Where-Object { $_.FullName -notmatch '\\tests\\third_party\\' })) {
        $rel = $f.FullName.Substring($root.Length + 1)
        $text = [IO.File]::ReadAllText($f.FullName)
        foreach ($m in [regex]::Matches($text, 'TEST_CASE\s*\(\s*"((?:[^"\\]|\\.)*)"')) {
            $name = $m.Groups[1].Value -replace '\\(.)', '$1'
            if ($sources.ContainsKey($name) -and $sources[$name] -ne $rel) { $dups[$name] = $true }
            $sources[$name] = $rel
        }
        $lits = New-Object System.Collections.Generic.List[string]
        foreach ($m in [regex]::Matches($text, '"((?:[^"\\\r\n]|\\.)*)"')) {
            $v = $m.Groups[1].Value
            if ($v -match '^(?:src\\\\|src/|\.\./src/|data\\\\|data/|\.\./data/)') { $lits.Add((($v -replace '^\.\./', '').Replace('\\', '\').Replace('/', '\'))) }
            elseif ($v -match '^corpus(?:$|\\\\|/)') { $lits.Add('tests\' + $v.Replace('\\', '\').Replace('/', '\')) }
        }
        if ($lits.Count) { $literals[$rel] = @($lits | Select-Object -Unique) }
    }
    foreach ($name in $dups.Keys) { $sources.Remove($name) }
    return @{ Sources = $sources; Literals = $literals }
}

function Get-TestsProjectGameTUs {
    $xml = [IO.File]::ReadAllText((Join-Path $testsDir 'OpenSWBF2Tests.vcxproj'))
    return @([regex]::Matches($xml, '<ClCompile\s+Include="\.\.\\(src\\[^"]+)"') | ForEach-Object { $_.Groups[1].Value })
}

function Read-GuardProofs([string]$path) {
    $proofs = @{}
    if (-not (Test-Path -LiteralPath $path)) { return $proofs }
    $lines = @([IO.File]::ReadAllLines($path) | Where-Object { $_ })
    if ($lines.Count -lt 2) { return $proofs }
    $cols = $lines[0].Split("`t")
    foreach ($l in ($lines | Select-Object -Skip 1)) {
        $f = $l.Split("`t")
        $o = [ordered]@{}
        for ($i = 0; $i -lt $cols.Count; $i++) { $o[$cols[$i]] = $(if ($i -lt $f.Count) { $f[$i] } else { '' }) }
        if ($o.Id) { $proofs[$o.Id] = [pscustomobject]$o }
    }
    return $proofs
}

function Add-GuardProof($r, [string]$ctl, [string]$result) {
    if (-not $script:TreeClean) { return }
    $d = $script:Decisions[$r.Id]
    $dep = $d.DepDigest
    if ($d.Doubt) { $dep = '-' }
    $date = [DateTime]::UtcNow.ToString('yyyy-MM-ddTHH:mm:ssZ')
    $line = @($r.Id, $d.RowHash, $dep, $d.TriggerDigest, $script:Toolset, $script:Tree, $script:Commit, $result, $ctl, $script:ProofConfig, $date) -join "`t"
    [IO.File]::AppendAllText($script:PartialFile, $line + "`r`n")
}

function Merge-GuardProofs {
    if (-not $script:PartialFile -or -not (Test-Path -LiteralPath $script:PartialFile)) { return }
    $byId = New-Object 'System.Collections.Generic.SortedDictionary[string,string]' ([StringComparer]::Ordinal)
    $old = Read-GuardProofs $ProvenFile
    foreach ($id in $old.Keys) {
        $p = $old[$id]
        $byId[$id] = @($script:ProofColumns | ForEach-Object { [string]$p.$_ }) -join "`t"
    }
    foreach ($l in [IO.File]::ReadAllLines($script:PartialFile)) {
        if ($l) { $byId[$l.Split("`t")[0]] = $l }
    }
    New-Item -ItemType Directory -Force -Path (Split-Path $ProvenFile -Parent) | Out-Null
    $text = ((@($script:ProofColumns -join "`t") + @($byId.Values)) -join "`r`n") + "`r`n"
    [IO.File]::WriteAllText($ProvenFile, $text)
}

try {
    . $RowsFile
    $rows = @($GuardRevertRows)
    $dupIds = @($rows | Group-Object { $_.Id } | Where-Object { $_.Count -gt 1 } | ForEach-Object { $_.Name })
    if ($dupIds.Count) { throw "duplicate row Id: $($dupIds -join ', ')" }
    if ($Row) { $rows = @($rows | Where-Object { $Row -contains $_.Id }) }
    if ($rows.Count -eq 0) { Write-Host 'No rows selected.'; exit 2 }
    $vsTools = Find-VsTools
    $script:Toolset = 'unknown'
    if ($vsTools) { $script:Toolset = $vsTools.ToolsVersion }
    $script:ProofConfig = "$script:TestsConfiguration/$script:GameConfiguration"
    $statusArgs = @('status', '--porcelain', '--untracked-files=normal', '--', '.', ':!tests/guardrevert/proven.tsv')
    $rootPrefix = $root.TrimEnd('\') + '\'
    if ($ProvenFile.StartsWith($rootPrefix, [StringComparison]::OrdinalIgnoreCase)) {
        $statusArgs += ':!' + $ProvenFile.Substring($rootPrefix.Length).Replace('\', '/')
    }
    New-Item -ItemType Directory -Force -Path (Join-Path $root 'artifacts') | Out-Null
    Set-Content -Path (Join-Path $root 'artifacts\.gitignore') -Value '*' -Encoding ASCII
    $before = (Invoke-Git $root $statusArgs) -join "`n"
    $script:TreeClean = -not $before
    $script:Tree = "$(Invoke-Git $root @('rev-parse', 'HEAD^{tree}'))".Trim()
    $script:Commit = "$(Invoke-Git $root @('rev-parse', 'HEAD'))".Trim()
    $script:TreeChanged = $false
    $proofs = Read-GuardProofs $ProvenFile
    $testInfo = Get-GuardTestInfo
    $tlogRoot = $root.TrimEnd('\') + '\'
    $selIn = @{
        Rows = $rows
        Proofs = $proofs
        Hashes = (Get-GuardHashes)
        TestsReads = (Read-ClReadTlog (Join-Path $root "obj\Tests\$script:TestsConfiguration\OpenSWBF2Tests.tlog\CL.read.1.tlog") $tlogRoot)
        AsanReads = (Read-ClReadTlog (Join-Path $root 'obj\Tests\Asan\OpenSWBF2Tests.tlog\CL.read.1.tlog') $tlogRoot)
        GameReads = (Read-ClReadTlog (Join-Path $root "obj\Win32\$script:GameConfiguration\BattlefrontII.tlog\CL.read.1.tlog") $tlogRoot)
        TestsProjectGameTUs = (Get-TestsProjectGameTUs)
        TestSources = $testInfo.Sources
        TestLiterals = $testInfo.Literals
        GlobalTriggers = $script:GuardGlobalTriggers
        Toolset = $script:Toolset
    }
    $script:Decisions = @{}
    foreach ($d in @(Select-GuardRows $selIn)) {
        $p = $proofs[$d.Id]
        if ($d.Action -eq 'CARRY' -and $p.Config -ne $script:ProofConfig) { $d.Action = 'SELECT'; $d.Reason = "config $($p.Config)" }
        if ($Row) { $d.Action = 'SELECT'; $d.Reason = 'named' }
        elseif ($Full -and $d.Action -eq 'CARRY') { $d.Action = 'SELECT'; $d.Reason = 'full' }
        $script:Decisions[$d.Id] = $d
    }
    $carryLines = @()
    $listLines = @()
    foreach ($r in $rows) {
        $d = $script:Decisions[$r.Id]
        if ($d.Action -eq 'SELECT') { $listLines += "SELECT $($d.Id) $($d.Kind) $($d.Reason)" }
        else {
            $p = $proofs[$d.Id]
            $line = "CARRY $($d.Id) $($d.Kind) $($p.Commit) $($p.Tree) $($p.Date)"
            $carryLines += $line
            $listLines += $line
        }
    }
    $rows = @($rows | Where-Object { $script:Decisions[$_.Id].Action -eq 'SELECT' })
    $totals = "selected $($rows.Count) carried $($carryLines.Count)"
    if ($ListSelected) {
        $listLines | ForEach-Object { Write-Host $_ }
        Write-Host $totals
        exit 0
    }
    if ($rows.Count -eq 0) {
        $carryLines | ForEach-Object { Write-Host $_ }
        Write-Host $totals
        exit 0
    }
    if (-not $script:TreeClean) { Write-Host 'PROOFS NOT RECORDED: tree not clean' }
    $selectedIds = @($rows | ForEach-Object { $_.Id })
    $smokeRows = @($rows | Where-Object { $_.Kind -eq 'Smoke' })
    $hardRows = @($rows | Where-Object { $_.Kind -eq 'Hardening' })
    $tlogRows = @($rows | Where-Object { $_.Kind -eq 'Tlog' })
    $asanRows = @($rows | Where-Object { $_.Kind -eq 'AsanCanary' -or $_.Asan })
    $rows = @($rows | Where-Object { $_.Kind -ne 'Smoke' -and $_.Kind -ne 'Hardening' -and $_.Kind -ne 'Tlog' -and $_.Kind -ne 'AsanCanary' -and -not $_.Asan })
    $gameData = Find-GameDataRoot
    $smokeCases = @{}
    foreach ($w in 'Crash', 'Standard') {
        foreach ($c in (Get-SmokeCases $w @() $false $null)) {
            if ($smokeCases.ContainsKey($c.Name)) { throw "smoke case '$($c.Name)' is defined more than once" }
            $smokeCases[$c.Name] = $c
        }
    }

    Assert-Runtimes $GameRuntimes

    $msbuild = Find-MSBuild
    New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
    $script:PartialFile = Join-Path $OutDir 'proven.partial.tsv'
    New-Item -ItemType Directory -Force -Path $Scratch | Out-Null
    $wt = Join-Path $Scratch "gr-$stamp"
    $table = @()
    $failed = $false
    $added = $false
    $cleanupFailed = $false

    try {
        Invoke-Git $root @('worktree', 'add', '--detach', $wt, 'HEAD') | Out-Null
        $added = $true
        $patch = Join-Path $OutDir 'overlay.patch'
        Invoke-Git $root @('diff', 'HEAD', '--binary', "--output=$patch") | Out-Null
        if ((Get-Item $patch).Length -gt 0) {
            Invoke-Git $wt @('apply', '--whitespace=nowarn', $patch) | Out-Null
        }
        foreach ($rel in (Invoke-Git $root @('ls-files', '--others', '--exclude-standard'))) {
            $rel = "$rel".Trim()
            if (-not $rel) { continue }
            $dst = Join-Path $wt $rel
            New-Item -ItemType Directory -Force -Path (Split-Path $dst -Parent) | Out-Null
            Copy-Item -LiteralPath (Join-Path $root $rel) -Destination $dst -Force
        }

        if ($rows.Count -gt 0) {
            $exe = Join-Path $wt "bin\Tests\$script:TestsConfiguration\OpenSWBF2Tests.exe"
            $controlOk = Build-Tests $wt (Join-Path $OutDir 'build-control.log')
            if (-not $controlOk) { throw "control build failed, see $(Join-Path $OutDir 'build-control.log')" }

            $control = @{}
            foreach ($name in ($rows | ForEach-Object { $_.Tests } | Select-Object -Unique)) {
                $control[$name] = Run-Case $exe $wt $name
                Set-Content -Path (Join-Path $OutDir (('control-' + ($name -replace '[^A-Za-z0-9_.-]', '_')) + '.txt')) -Value $control[$name].Text -Encoding ASCII
            }

            foreach ($r in $rows) {
                $path = Join-Path $wt $r.File
                $orig = $null
                $result = 'BUILD-FAIL'
                $detail = ''
                $code = ''
                $ctl = 'GREEN'
                foreach ($name in $r.Tests) { if ($control[$name].State -ne 'GREEN') { $ctl = $control[$name].State } }
                try {
                    $orig = Set-Anchor $path $r.Anchor $r.Replacement
                    $ok = Build-Tests $wt (Join-Path $OutDir ("build-$($r.Id).log"))
                    if ($ok) {
                        $states = @()
                        $codes = @()
                        $details = @()
                        foreach ($name in $r.Tests) {
                            $c = Run-Case $exe $wt $name
                            Set-Content -Path (Join-Path $OutDir (("$($r.Id)-" + ($name -replace '[^A-Za-z0-9_.-]', '_')) + '.txt')) -Value $c.Text -Encoding ASCII
                            $states += $c.State
                            $codes += $c.Code
                            if ($c.Detail) { $details += $c.Detail }
                        }
                        $code = ($codes -join ',')
                        $detail = ($details -join ',')
                        if ($states -contains 'NO-MATCH') { $result = 'NO-MATCH' }
                        elseif ($states -contains 'RED') { $result = 'RED' }
                        elseif ($states -contains 'SKIP') { $result = 'SKIP' }
                        else { $result = 'STILL-GREEN' }
                    }
                }
                catch {
                    $result = 'ERROR'
                    $detail = $_.Exception.Message
                }
                finally {
                    if ($orig) { [IO.File]::WriteAllBytes($path, $orig) }
                }
                $table += New-Object PSObject -Property @{ Id = $r.Id; Tests = ($r.Tests -join ' | '); Control = $ctl; Reverted = $result; Exit = $code; Detail = $detail }
                if ($result -ne 'RED' -or $ctl -ne 'GREEN') { $failed = $true }
                else { Add-GuardProof $r 'GREEN' 'RED' }
                Write-Host ('{0,-12} control={1,-8} reverted={2,-11} exit={3} {4}' -f $r.Id, $ctl, $result, $code, $detail)
            }
        }

        if ($asanRows.Count -gt 0) {
            $asanExe = Join-Path $wt 'bin\Tests\Asan\OpenSWBF2Tests.exe'
            $asanOut = Join-Path $OutDir 'asan'
            $safe = { param($s) $s -replace '[^A-Za-z0-9_.-]', '_' }
            $asanState = {
                param($r, $c)
                if ($r.Kind -eq 'AsanCanary') { return (Get-AsanCanaryState $c.Code $c.Report $r.Report) }
                return $c.State
            }
            if (-not (Build-Tests $wt (Join-Path $OutDir 'build-asan-control.log') 'Asan')) { throw "asan control build failed, see $(Join-Path $OutDir 'build-asan-control.log')" }
            $asanControl = @{}
            foreach ($name in ($asanRows | ForEach-Object { $_.Tests } | Select-Object -Unique)) {
                $asanControl[$name] = Run-Case $asanExe $wt $name (Join-Path $asanOut ('control-' + (& $safe $name)))
                Set-Content -Path (Join-Path $OutDir ('asan-control-' + (& $safe $name) + '.txt')) -Value $asanControl[$name].Text -Encoding ASCII
            }
            foreach ($r in $asanRows) {
                $path = Join-Path $wt $r.File
                $orig = $null
                $result = 'BUILD-FAIL'
                $detail = ''
                $code = ''
                $want = 'GREEN'
                if ($r.Kind -eq 'AsanCanary') { $want = 'REPORTED' }
                $ctl = 'GREEN'
                foreach ($name in $r.Tests) {
                    $s = & $asanState $r $asanControl[$name]
                    if ($s -ne $want) { $ctl = $s }
                }
                try {
                    $orig = Set-Anchor $path $r.Anchor $r.Replacement
                    if (Build-Tests $wt (Join-Path $OutDir "build-$($r.Id).log") 'Asan') {
                        $states = @()
                        $codes = @()
                        $details = @()
                        foreach ($name in $r.Tests) {
                            $c = Run-Case $asanExe $wt $name (Join-Path $asanOut ("$($r.Id)-" + (& $safe $name)))
                            Set-Content -Path (Join-Path $OutDir ("asan-$($r.Id)-" + (& $safe $name) + '.txt')) -Value $c.Text -Encoding ASCII
                            $states += (& $asanState $r $c)
                            $codes += $c.Code
                            if ($c.Detail) { $details += $c.Detail }
                        }
                        $code = ($codes -join ',')
                        $detail = ($details -join ',')
                        if ($r.Kind -eq 'AsanCanary') {
                            if ($states -contains 'REPORTED') { $result = 'STILL-GREEN' }
                            elseif (@($states | Where-Object { $_ -ne 'CLEAN' }).Count) { $result = 'WRONG-RED' }
                            else { $result = 'RED' }
                        }
                        elseif ($states -contains 'NO-MATCH') { $result = 'NO-MATCH' }
                        elseif ($states -contains 'RED') { $result = 'RED' }
                        elseif ($states -contains 'SKIP') { $result = 'SKIP' }
                        else { $result = 'STILL-GREEN' }
                    }
                }
                catch {
                    $result = 'ERROR'
                    $detail = $_.Exception.Message
                }
                finally {
                    if ($orig) { [IO.File]::WriteAllBytes($path, $orig) }
                }
                $table += New-Object PSObject -Property @{ Id = $r.Id; Tests = ($r.Tests -join ' | '); Control = $ctl; Reverted = $result; Exit = $code; Detail = $detail }
                if ($result -ne 'RED' -or $ctl -ne 'GREEN') { $failed = $true }
                elseif ($r.Kind -eq 'AsanCanary') { Add-GuardProof $r 'REPORTED' 'CLEAN' }
                else { Add-GuardProof $r 'GREEN' 'RED' }
                Write-Host ('{0,-12} control={1,-8} reverted={2,-11} exit={3} {4}' -f $r.Id, $ctl, $result, $code, $detail)
            }
        }

        if ($hardRows.Count -gt 0) {
            $wtExe = Join-Path $wt "bin\Win32\$script:GameConfiguration\BattlefrontII.exe"
            $binNames = @((@(Get-HardeningChecks '' '' 0 0 0) + @(Get-ImportChecks '')) | ForEach-Object { $_.Name })
            $getChecks = {
                param([string[]]$names)
                $res = @()
                if (@($names | Where-Object { $binNames -contains $_ }).Count) {
                    $db = Find-Dumpbin
                    $hdr = (& $db /headers $wtExe) -join "`n"
                    $imp = (& $db /imports $wtExe) -join "`n"
                    $res += @(Get-HardeningChecks $hdr '' 0 0 0) + @(Get-ImportChecks $imp)
                }
                $srcNames = @($names | Where-Object { $binNames -notcontains $_ })
                if ($srcNames.Count) { $res += @(Get-SourceChecks $wt -Only $srcNames) }
                return @($res | Where-Object { $names -contains $_.Name })
            }
            if (-not (Build-Game $wt (Join-Path $OutDir 'build-hardening-control.log'))) { throw "hardening control build failed, see $(Join-Path $OutDir 'build-hardening-control.log')" }
            $hardControl = @{}
            $hardNames = @($hardRows | ForEach-Object { $_.Check } | Select-Object -Unique)
            $hardAll = @(& $getChecks $hardNames)
            foreach ($name in $hardNames) {
                $hardControl[$name] = [bool](@($hardAll | Where-Object { $_.Name -eq $name }) | Select-Object -First 1).Ok
            }
            foreach ($r in $hardRows) {
                $path = Join-Path $wt $r.File
                $orig = $null
                $result = 'BUILD-FAIL'
                $detail = ''
                $ctl = 'GREEN'
                if (-not $hardControl[$r.Check]) { $ctl = 'FAIL' }
                try {
                    $orig = Set-Anchor $path $r.Anchor $r.Replacement
                    if (Build-Game $wt (Join-Path $OutDir "build-$($r.Id).log")) {
                        $chk = @(& $getChecks @($r.Check)) | Select-Object -First 1
                        $detail = "check=$($r.Check) ok=$($chk.Ok)"
                        if ($chk.Ok) { $result = 'STILL-GREEN' } else { $result = 'RED' }
                    }
                }
                catch {
                    $result = 'ERROR'
                    $detail = $_.Exception.Message
                }
                finally {
                    if ($orig) { [IO.File]::WriteAllBytes($path, $orig) }
                }
                $table += New-Object PSObject -Property @{ Id = $r.Id; Tests = "hardening:$($r.Check)"; Control = $ctl; Reverted = $result; Exit = ''; Detail = $detail }
                if ($result -ne 'RED' -or $ctl -ne 'GREEN') { $failed = $true }
                else { Add-GuardProof $r 'GREEN' 'RED' }
                Write-Host ('{0,-12} control={1,-8} reverted={2,-11} {3}' -f $r.Id, $ctl, $result, $detail)
            }
        }

        if ($tlogRows.Count -gt 0) {
            $buildProj = {
                param($project, $logPath)
                if ($project -eq 'tests') { return (Build-Tests $wt $logPath) }
                return (Build-Game $wt $logPath)
            }
            $tlogDirs = @{ game = (Join-Path $wt "obj\Win32\$script:GameConfiguration\BattlefrontII.tlog"); tests = (Join-Path $wt "obj\Tests\$script:TestsConfiguration\OpenSWBF2Tests.tlog") }
            $getFlag = {
                param($name)
                $game = $null
                $tests = $null
                if (Test-Path -LiteralPath $tlogDirs.game) { $game = Get-ClTlogCounts $tlogDirs.game }
                if (Test-Path -LiteralPath $tlogDirs.tests) { $tests = Get-ClTlogCounts $tlogDirs.tests }
                return (Get-FlagChecks $game $tests | Where-Object { $_.Name -eq $name })
            }
            foreach ($project in ($tlogRows | ForEach-Object { $_.Project } | Select-Object -Unique)) {
                $log = Join-Path $OutDir "build-tlog-control-$project.log"
                if (-not (& $buildProj $project $log)) { throw "tlog control build failed, see $log" }
            }
            $tlogControl = @{}
            $tlogProven = @()
            foreach ($name in ($tlogRows | ForEach-Object { $_.Check } | Select-Object -Unique)) {
                $tlogControl[$name] = & $getFlag $name
            }
            foreach ($r in $tlogRows) {
                $path = Join-Path $wt $r.File
                $orig = $null
                $result = 'BUILD-FAIL'
                $ctlChk = $tlogControl[$r.Check]
                $detail = "control: $($ctlChk.Detail)"
                $ctl = 'GREEN'
                if (-not $ctlChk.Ok) { $ctl = 'FAIL' }
                try {
                    $orig = Set-Anchor $path $r.Anchor $r.Replacement
                    Get-ChildItem -LiteralPath $tlogDirs[$r.Project] -Filter 'cl.command*.tlog' -ErrorAction SilentlyContinue | Remove-Item -Force
                    $built = & $buildProj $r.Project (Join-Path $OutDir "build-$($r.Id).log")
                    $fresh = (Get-ClTlogCounts $tlogDirs[$r.Project]).Total
                    if ($built -or $fresh -gt 0) {
                        $chk = & $getFlag $r.Check
                        $detail = "reverted: $($chk.Detail)"
                        if (-not $built) { $detail = "build failed after $fresh compiles; $detail" }
                        if ($chk.Ok) { $result = 'STILL-GREEN' } else { $result = 'RED' }
                    }
                }
                catch {
                    $result = 'ERROR'
                    $detail = $_.Exception.Message
                }
                finally {
                    if ($orig) { [IO.File]::WriteAllBytes($path, $orig) }
                }
                $table += New-Object PSObject -Property @{ Id = $r.Id; Tests = "tlog:$($r.Check)"; Control = $ctl; Reverted = $result; Exit = ''; Detail = $detail }
                if ($result -ne 'RED' -or $ctl -ne 'GREEN') { $failed = $true }
                else { $tlogProven += $r }
                Write-Host ('{0,-16} control={1,-8} reverted={2,-11} {3}' -f $r.Id, $ctl, $result, $detail)
            }
            foreach ($project in ($tlogRows | ForEach-Object { $_.Project } | Select-Object -Unique)) {
                $log = Join-Path $OutDir "build-tlog-restored-$project.log"
                if (-not (& $buildProj $project $log)) { throw "tlog restored build failed, see $log" }
            }
            $restoredOk = $true
            foreach ($name in $tlogControl.Keys) {
                $chk = & $getFlag $name
                Write-Host ('tlog restored {0}: ok={1} {2}' -f $name, $chk.Ok, $chk.Detail)
                if (-not $chk.Ok) { $failed = $true; $restoredOk = $false }
            }
            if ($restoredOk) { foreach ($r in $tlogProven) { Add-GuardProof $r 'GREEN' 'RED' } }
        }

        if ($smokeRows.Count -gt 0) {
            $pre = Get-SmokePrecheck $gameData (Join-Path $root "bin\Win32\$script:GameConfiguration")
            if ($pre) { throw "smoke rows need a prior game build and data: $pre" }
            if (-not (Build-Game $wt (Join-Path $OutDir 'build-smoke-control.log'))) { throw "smoke control build failed, see $(Join-Path $OutDir 'build-smoke-control.log')" }
            $smokeControl = @{}
            foreach ($name in ($smokeRows | ForEach-Object { $_.Case } | Select-Object -Unique)) {
                $smokeControl[$name] = Run-Smoke $wt "control-$name" $smokeCases[$name]
            }
            foreach ($r in $smokeRows) {
                $path = Join-Path $wt $r.File
                $orig = $null
                $result = 'BUILD-FAIL'
                $detail = ''
                $code = ''
                $ctlRes = $smokeControl[$r.Case]
                $ctl = 'GREEN'
                if (-not $ctlRes.Ok) { $ctl = 'FAIL-' + $ctlRes.Class }
                try {
                    $orig = Set-Anchor $path $r.Anchor $r.Replacement
                    if (Build-Game $wt (Join-Path $OutDir "build-$($r.Id).log")) {
                        $s = Run-Smoke $wt $r.Id $smokeCases[$r.Case]
                        $code = ('0x{0:X8}' -f $s.Code)
                        $detail = "class=$($s.Class) dumps=$($s.Dumps)"
                        if ($s.Ok) { $result = 'STILL-GREEN' }
                        elseif ($r.RedRx -and $s.LogText -cnotmatch $r.RedRx) { $result = 'WRONG-RED' }
                        else { $result = 'RED' }
                    }
                }
                catch {
                    $result = 'ERROR'
                    $detail = $_.Exception.Message
                }
                finally {
                    if ($orig) { [IO.File]::WriteAllBytes($path, $orig) }
                }
                $table += New-Object PSObject -Property @{ Id = $r.Id; Tests = "smoke:$($r.Case)"; Control = $ctl; Reverted = $result; Exit = $code; Detail = $detail }
                if ($result -ne 'RED' -or $ctl -ne 'GREEN') { $failed = $true }
                else { Add-GuardProof $r 'GREEN' 'RED' }
                Write-Host ('{0,-12} control={1,-8} reverted={2,-11} exit={3} {4}' -f $r.Id, $ctl, $result, $code, $detail)
            }
        }
    }
    finally {
        $ErrorActionPreference = 'Continue'
        if ($added -and -not $Keep) {
            $reparse = $null
            try { $reparse = @(Get-ReparsePoints $wt) }
            catch { Write-Host "REFUSING to remove ${wt}: reparse point scan failed: $($_.Exception.Message)"; $cleanupFailed = $true }
            if ($reparse -and $reparse.Count -gt 0) {
                Write-Host "REFUSING to remove ${wt}: reparse points:"
                $reparse | ForEach-Object { Write-Host "    $_" }
                $cleanupFailed = $true
            }
            elseif ($null -ne $reparse) {
                & git -C $wt reset --hard -q 2>&1 | Out-Null
                & git -C $wt clean -fdq 2>&1 | Out-Null
                & git -C $root worktree remove $wt 2>&1 | Out-Null
                if ($LASTEXITCODE -ne 0 -or (Test-Path -LiteralPath $wt)) {
                    Write-Host "Cleanup failed: git worktree remove left $wt in place."
                    $cleanupFailed = $true
                }
            }
        }
        & git -C $root worktree prune 2>&1 | Out-Null
        $script:TreeChanged = $true
        try {
            $after = (Invoke-Git $root $statusArgs) -join "`n"
            $headAfter = "$(Invoke-Git $root @('rev-parse', 'HEAD'))".Trim()
            $script:TreeChanged = ($after -ne $before) -or ($headAfter -ne $script:Commit)
        }
        catch { Write-Host "STATUS CHECK FAILED: $($_.Exception.Message)" }
        if ($script:TreeChanged) { Write-Host 'PROOFS NOT RECORDED: main tree changed during the run' }
        else {
            try { Merge-GuardProofs }
            catch { Write-Host "PROOF MERGE FAILED: $($_.Exception.Message)"; $cleanupFailed = $true }
        }
    }

    $lines = @()
    $lines += ('{0,-12} {1,-8} {2,-11} {3,-10} {4}' -f 'Id', 'Control', 'Reverted', 'Exit', 'Tests')
    foreach ($t in $table) {
        $lines += ('{0,-12} {1,-8} {2,-11} {3,-10} {4} {5}' -f $t.Id, $t.Control, $t.Reverted, $t.Exit, $t.Tests, $t.Detail)
    }
    Write-Host ''
    $carryLines | ForEach-Object { Write-Host $_ }
    Write-Host $totals
    $lines | ForEach-Object { Write-Host $_ }
    $lines | Set-Content -Path (Join-Path $OutDir 'summary.txt') -Encoding ASCII
    Write-Host "Artifacts: $OutDir"

    $ranIds = @($table | ForEach-Object { $_.Id })
    $missing = @($selectedIds | Where-Object { $ranIds -notcontains $_ })
    if ($table.Count -ne $selectedIds.Count) {
        Write-Host "ERROR: $($table.Count) of $($selectedIds.Count) selected rows ran. Missing: $($missing -join ', ')"
        $failed = $true
    }
}
catch {
    Write-Host "ERROR: $($_.Exception.Message)"
    Write-Host $_.InvocationInfo.PositionMessage
    if ($_.ScriptStackTrace) { Write-Host $_.ScriptStackTrace }
    exit 1
}

if ($script:TreeChanged) { Write-Host 'WARNING: main tree status changed during the run.'; exit 1 }
if ($failed -or $cleanupFailed) { exit 1 }
exit 0
