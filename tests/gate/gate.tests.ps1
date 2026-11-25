$ErrorActionPreference = 'Stop'
$testsDir = Split-Path $PSScriptRoot -Parent
. (Join-Path $PSScriptRoot 'keys.ps1')
$tmp = Join-Path $env:TEMP ('openswbf2-gatetests-' + [guid]::NewGuid().ToString('N'))
$repo = Join-Path $tmp 'repo'
$ev = Join-Path $tmp 'evidence'
$marks = Join-Path $tmp 'marks'
$stepsFile = Join-Path $tmp 'steps.ps1'
$noInputsFile = Join-Path $tmp 'steps-noinputs.ps1'
$gate = Join-Path $repo 'tests\gate.ps1'
$recFile = Join-Path $ev 'steps.tsv'
$script:failed = 0

function Write-Result([string]$Name, $Why) {
    if ($Why) { $script:failed++; Write-Host "FAILED ${Name}: $Why" }
    else { Write-Host "OK $Name" }
}

function Invoke-Gate([string]$GateArgs, [string]$Steps = $stepsFile) {
    Get-ChildItem -LiteralPath $marks -File | Remove-Item -Force
    $env:OPENSWBF2_GATETEST_MARKS = $marks
    try {
        $o = @(& powershell -NoProfile -ExecutionPolicy Bypass -Command "& '$gate' -StepsFile '$Steps' -EvidenceDir '$ev' $GateArgs; exit `$LASTEXITCODE" 2>&1 | ForEach-Object { "$_" })
        $c = $LASTEXITCODE
    }
    finally { Remove-Item Env:\OPENSWBF2_GATETEST_MARKS -ErrorAction SilentlyContinue }
    $m = @(Get-ChildItem -LiteralPath $marks -File | ForEach-Object { $_.Name } | Sort-Object)
    New-Object PSObject -Property @{ Code = $c; Marks = $m; Text = ($o -join "`n") }
}

function Test-Carried($R, [string]$Name) { return $R.Text -match "(?m)^\[CARRY\] gate:$Name " }

function Get-Rec([string]$Name) { return (Read-StepRecords $recFile)["$Name`tWin32`tDebug"] }

function Get-RunWhy($R, [string]$Name, [int]$Code = 0) {
    if ($R.Code -ne $Code -or $R.Marks -notcontains $Name -or (Test-Carried $R $Name)) { return "exit $($R.Code), marks $($R.Marks -join ','): $($R.Text)" }
    return $null
}

function Get-CarryWhy($R, [string]$Name) {
    if ($R.Code -ne 0 -or $R.Marks -contains $Name -or -not (Test-Carried $R $Name)) { return "exit $($R.Code), marks $($R.Marks -join ','): $($R.Text)" }
    return $null
}

function Write-Tlog {
    $u = $repo.ToUpperInvariant()
    $lines = @("^$u\TESTS\UNIT\MOVIE_CORPUS.CPP", "$u\TESTS\UNIT\MOVIE_CORPUS.CPP", "$u\SRC\COMMON\SHARED.H", 'C:\OUTSIDE\SDK.H', "^$u\TESTS\UNIT\OTHER.CPP", "$u\SRC\COMMON\UNRELATED.H")
    New-Item -ItemType Directory -Force -Path (Split-Path $tlog -Parent) | Out-Null
    Set-Content -LiteralPath $tlog -Encoding Unicode -Value $lines
    [IO.File]::SetLastWriteTimeUtc($tlog, [DateTime]::UtcNow.AddMinutes(1))
}

function Set-RepoFile([string]$Rel, [string]$Text) {
    $p = Join-Path $repo $Rel
    New-Item -ItemType Directory -Force -Path (Split-Path $p -Parent) | Out-Null
    [IO.File]::WriteAllText($p, $Text)
}

try {
    New-Item -ItemType Directory -Force -Path (Join-Path $repo 'tests\gate'), (Join-Path $repo 'tests\hardening'), $marks | Out-Null
    foreach ($f in 'gate.ps1', 'gate\keys.ps1', 'hardening.ps1', 'common.ps1', 'hardening\allow.ps1', 'vstools.ps1') { Copy-Item -LiteralPath (Join-Path $testsDir $f) -Destination (Join-Path $repo "tests\$f") }
    $tlog = Join-Path $repo 'artifacts\tl\CL.read.1.tlog'
    Set-RepoFile '.gitignore' "artifacts/`n"
    Set-RepoFile 'src\a.cpp' "int a;`r`n"
    Set-RepoFile 'src\movie\dec.c' "int dec;`r`n"
    Set-RepoFile 'src\common\shared.h' "int shared;`r`n"
    Set-RepoFile 'src\common\unrelated.h' "int unrelated;`r`n"
    Set-RepoFile 'tests\unit\movie_corpus.cpp' "int corpus;`r`n"
    Set-RepoFile 'tests\unit\other.cpp' "int other;`r`n"
    Set-RepoFile 'fail\c.txt' "c`r`n"
    Set-RepoFile 'artifacts\rt\clang_rt.asan_dynamic-i386.dll' 'runtime'
    Set-Content -LiteralPath (Join-Path $repo 'tests\stub.ps1') -Encoding ASCII -Value @(
        'param([string]$Name, [string]$Kind)',
        '[IO.File]::WriteAllText((Join-Path $env:OPENSWBF2_GATETEST_MARKS $Name), ''x'')',
        'if ($Kind -eq ''run'') {',
        '    $d = Join-Path (Split-Path $PSScriptRoot -Parent) (''artifacts\tests\'' + $Name + ''-'' + [guid]::NewGuid().ToString(''N''))',
        '    New-Item -ItemType Directory -Force -Path $d | Out-Null',
        '    $line = $(if ($env:OPENSWBF2_GATETEST_BADRUN -ceq $Name) { "[FAIL] stub $Name" } else { "[PASS] stub $Name" })',
        '    [IO.File]::WriteAllText((Join-Path $d ''results.txt''), "$line`r`n")',
        '    $failed = $(if ($env:OPENSWBF2_GATETEST_BADINDEX -ceq $Name) { 1 } else { 0 })',
        '    [IO.File]::WriteAllText((Join-Path $d ''index.json''), "{ `"failed`": $failed, `"tests`": [] }`r`n")',
        '    if ($env:OPENSWBF2_GATETEST_BADUNIT -ceq $Name) { New-Item -ItemType Directory -Force -Path (Join-Path $d ''unit'') | Out-Null; [IO.File]::WriteAllText((Join-Path $d ''unit\index.json''), "{ `"failed`": 1, `"tests`": [] }`r`n") }',
        '}',
        'if ($env:OPENSWBF2_GATETEST_FAIL -ceq $Name) { Write-Output "stub $Name fails"; exit 1 }',
        'Write-Output "stub $Name"',
        'exit 0'
    )
    $common = @(
        'function Get-ToolchainInfo { New-Object PSObject -Property @{ Cl = ''stub-cl''; ToolsVersion = ''stub-tools''; Ucrt = ''stub-ucrt'' } }',
        'function Get-SteamRunning { ''yes'' }',
        'function Get-SteamOverlayState { ''off'' }',
        'function Find-GameDataRoot { $null }',
        'function Get-GameDataManifestHash { ''stub-gamedata'' }',
        'function Get-GatePreflightInput([string[]]$Platforms) {',
        '    @{ Steam = ''yes''; Overlay = ''off''; RulePaths = @(); GamePlatforms = @(); Platforms = @($Platforms); AllowRules = @(); Wer = @(); FreeRamKB = $script:RamFloorKB; RepoDrive = ''C:''; RepoFree = [long]$script:DiskDropBytes; GameData = ''G:\GameData''; Toolchain = @(@{ Platform = ''Win32''; MSBuild = $true; Dumpbin = $true; RuntimeDir = ''S:\''; MissingRuntimes = @() }) }',
        '}',
        '$stub = @(''-NoProfile'', ''-ExecutionPolicy'', ''Bypass'', ''-File'', ''tests\stub.ps1'')'
    )
    $step = { param([string]$name, [string]$kind, [string]$tiers, [string]$rest) "    @{ Name = '$name'; Tiers = @($tiers); Kind = '$kind'; Config = 'Debug'; Exe = 'powershell'; Args = `$stub + @('-Name', '$name', '-Kind', '$kind')$rest }" }
    Set-Content -LiteralPath $stepsFile -Encoding ASCII -Value ($common + @(
        '$GateSteps = @(',
        ((& $step 'build' 'build' "'plan', 'phase'" '') + ','),
        ((& $step 'fail-c' 'run' "'plan'" "; Needs = @('build'); Inputs = @('fail\')") + ','),
        ((& $step 'run-a' 'run' "'plan'" "; Needs = @('build'); Inputs = @('src\a.cpp', 'tests\stub.ps1')") + ','),
        ((& $step 'run-b' 'run' "'phase'" "; Inputs = @('src\')") + ','),
        (& $step 'corpus' 'run' "'plan'" "; Needs = @('build'); Inputs = @('src\movie\', 'tests\unit\movie_corpus.cpp'); SourceKey = @{ Tlog = 'artifacts\tl\CL.read.1.tlog'; RuntimeDir = 'artifacts\rt' }"),
        ')'
    ))
    Set-Content -LiteralPath $noInputsFile -Encoding ASCII -Value ($common + @('$GateSteps = @(', ((& $step 'build' 'build' "'plan'" '') + ','), (& $step 'bare' 'run' "'plan'" ''), ')'))
    Write-Tlog
    $gitArgs = @('-c', 'user.name=gatetests', '-c', 'user.email=gatetests@invalid', '-c', 'core.autocrlf=false')
    foreach ($a in @(@('init', '-q'), @('add', '-A'), @('commit', '-q', '-m', 'gatetests'))) {
        $o = & git -C $repo @gitArgs @a 2>&1
        if ($LASTEXITCODE -ne 0) { throw "git $($a -join ' '): $o" }
    }

    $r = Invoke-Gate '-Tier plan' $noInputsFile
    Write-Result 'a run step with no Inputs is a definition error and exits 2 before any step' $(if ($r.Code -ne 2 -or $r.Marks.Count -or $r.Text -notmatch 'gate: bare: no Inputs') { "exit $($r.Code), marks $($r.Marks -join ','): $($r.Text)" })

    $env:OPENSWBF2_GATETEST_FAIL = 'fail-c'
    try { $r = Invoke-Gate '-Tier plan' }
    finally { Remove-Item Env:\OPENSWBF2_GATETEST_FAIL -ErrorAction SilentlyContinue }
    $why = $null
    if ($r.Code -ne 1 -or ($r.Marks -join ',') -cne 'build,fail-c' -or $r.Text -notmatch '(?m)^\[FAIL\] gate:fail-c ') { $why = "first run: exit $($r.Code), marks $($r.Marks -join ','): $($r.Text)" }
    elseif ((Get-Rec 'fail-c').Result -cne 'FAIL') { $why = 'no FAIL record for fail-c' }
    else {
        $r = Invoke-Gate '-Tier plan -Step fail-c'
        if ($r.Code -ne 0 -or ($r.Marks -join ',') -cne 'build,fail-c' -or $r.Text -notmatch '(?m)^gate: tier plan partial') { $why = "re-run alone: exit $($r.Code), marks $($r.Marks -join ','): $($r.Text)" }
        elseif ((Get-Rec 'fail-c').Result -cne 'PASS') { $why = 'the re-run alone recorded no PASS' }
    }
    Write-Result 'the first failing step stops the run with exit 1 before later steps, and a later run of that step alone records its pass' $why

    $r0 = Invoke-Gate '-Tier plan'
    $r1 = Invoke-Gate '-Tier plan'
    $why = $null
    if ($r0.Code -ne 0 -or $r0.Text -notmatch '(?m)^gate: tier plan passed') { $why = "first full run: exit $($r0.Code): $($r0.Text)" }
    elseif (-not (Get-Rec 'run-a') -or -not (Get-Rec 'corpus')) { $why = 'no records after a full run' }
    else { $why = (Get-CarryWhy $r1 'run-a') + (Get-CarryWhy $r1 'corpus') + (Get-CarryWhy $r1 'fail-c') }
    Write-Result 'an unchanged tree carries every run step without starting it' $why

    Set-RepoFile 'src\a.cpp' "int b;`r`n"
    $r = Invoke-Gate '-Tier plan'
    $why = Get-RunWhy $r 'run-a'
    if (-not $why) { $why = (Get-CarryWhy $r 'corpus') + (Get-CarryWhy $r 'fail-c') }
    Write-Result 'a change to a file in a step''s Inputs runs that step and carries the others' $why

    foreach ($bad in 'OPENSWBF2_GATETEST_BADRUN', 'OPENSWBF2_GATETEST_BADINDEX', 'OPENSWBF2_GATETEST_BADUNIT') {
        Set-RepoFile 'src\a.cpp' "int $bad;`r`n"
        Set-Item -Path "Env:\$bad" -Value 'run-a'
        try { $rb = Invoke-Gate '-Tier plan' }
        finally { Remove-Item -Path "Env:\$bad" -ErrorAction SilentlyContinue }
        $rn = Invoke-Gate '-Tier plan'
        $why = Get-RunWhy $rb 'run-a'
        if (-not $why) { $why = Get-RunWhy $rn 'run-a' }
        if (-not $why -and $rn.Text -notmatch '(?m)^gate: run-a Win32 run \(evidence judged FAIL: run ') { $why = "no evidence-judged reason: $($rn.Text)" }
        Write-Result "a carried step whose archived run records a failure ($bad) runs again" $why
    }

    Set-RepoFile '.planning\phase\note.md' "note`r`n"
    Set-RepoFile 'NOTES.md' "notes`r`n"
    Set-RepoFile 'src\other.cpp' "int other;`r`n"
    $r = Invoke-Gate '-Tier plan'
    Write-Result 'a change outside a step''s Inputs (.planning, a root .md, another src file) carries it' ((Get-CarryWhy $r 'run-a') + (Get-CarryWhy $r 'corpus') + (Get-CarryWhy $r 'fail-c'))

    Remove-Item -LiteralPath $recFile -Force
    $r = Invoke-Gate '-Tier plan'
    $why = (Get-RunWhy $r 'run-a') + (Get-RunWhy $r 'corpus')
    if (-not $why -and $r.Text -notmatch '(?m)^gate: run-a Win32 run \(no record with this RunKey\)') { $why = "no 'no record' reason: $($r.Text)" }
    Write-Result 'a missing record runs the step' $why

    $gone = (Get-Rec 'run-a').Evidence
    Remove-Item -LiteralPath (Join-Path $ev $gone) -Recurse -Force
    $r = Invoke-Gate '-Tier plan'
    $why = Get-RunWhy $r 'run-a'
    if (-not $why -and $r.Text -notmatch "evidence $gone is missing") { $why = "no missing-evidence reason: $($r.Text)" }
    if (-not $why) { $why = Get-CarryWhy $r 'corpus' }
    Write-Result 'a record whose evidence folder is gone runs the step' $why

    $rp = Invoke-Gate '-Tier phase -Step run-b'
    $rw = Invoke-Gate '-Tier plan -Step run-b'
    $why = $null
    if ($rp.Code -ne 0 -or ($rp.Marks -join ',') -cne 'run-b' -or $rp.Text -notmatch '(?m)^gate: tier phase partial') { $why = "-Tier phase -Step run-b: exit $($rp.Code), marks $($rp.Marks -join ','): $($rp.Text)" }
    elseif ($rw.Code -ne 0 -or -not (Test-Carried $rw 'run-b') -or $rw.Marks.Count) { $why = "-Tier plan -Step run-b, a phase-only step: exit $($rw.Code), marks $($rw.Marks -join ','): $($rw.Text)" }
    else {
        $rs = Invoke-Gate '-Tier phase -Step run-a'
        Set-RepoFile 'src\a.cpp' "int c;`r`n"
        $rs = Invoke-Gate '-Tier phase -Step run-a'
        $rt = Invoke-Gate '-Tier plan'
        if ($rs.Code -ne 0 -or $rs.Marks -notcontains 'run-a') { $why = "-Tier phase -Step run-a after a change: exit $($rs.Code), marks $($rs.Marks -join ','): $($rs.Text)" }
        else { $why = Get-CarryWhy $rt 'run-a' }
    }
    Write-Result '-Step runs a step of another tier, and a record written by -Step carries into a whole run of another tier' $why

    $rb = Invoke-Gate '-Tier phase -Step build'
    $ru = Invoke-Gate '-Tier plan -Step nope'
    $why = $null
    if ($rb.Code -ne 0 -or ($rb.Marks -join ',') -cne 'build' -or $rb.Text -notmatch '(?m)^gate: tier phase partial') { $why = "-Step build: exit $($rb.Code), marks $($rb.Marks -join ','): $($rb.Text)" }
    elseif ($ru.Code -ne 2 -or $ru.Marks.Count -or $ru.Text -notmatch 'gate: no step nope') { $why = "-Step nope: exit $($ru.Code), marks $($ru.Marks -join ','): $($ru.Text)" }
    Write-Result '-Step accepts a build step alone and refuses an unknown name with exit 2 before any step' $why

    Set-RepoFile 'OpenSWBF2.Platform.props' "<ProjectConfiguration Include=`"Release|x64`" />`r`n"
    $lo = @(& powershell -NoProfile -ExecutionPolicy Bypass -File $gate -Tier plan -StepsFile $stepsFile -EvidenceDir $ev -Platform 'Win32,x64' -ListOrder 2>&1 | ForEach-Object { "$_" }) -join "`n"
    $loCode = $LASTEXITCODE
    $lb = @(& powershell -NoProfile -ExecutionPolicy Bypass -File $gate -Tier plan -StepsFile $stepsFile -EvidenceDir $ev -Platform 'Win32,arm' -ListOrder 2>&1 | ForEach-Object { "$_" }) -join "`n"
    $lbCode = $LASTEXITCODE
    Remove-Item -LiteralPath (Join-Path $repo 'OpenSWBF2.Platform.props') -Force
    $why = $null
    if ($loCode -ne 0 -or $lo -notmatch '(?m)^gate: plan Win32 order build' -or $lo -notmatch '(?m)^gate: plan x64 order build') { $why = "-File -Platform 'Win32,x64': exit ${loCode}: $lo" }
    elseif ($lbCode -ne 2 -or $lb -notmatch "gate: -Platform takes Win32 and x64, not 'Win32,arm'") { $why = "-File -Platform 'Win32,arm': exit ${lbCode}: $lb" }
    Write-Result '-Platform under -File takes a comma list of Win32 and x64 and refuses any other name with exit 2' $why

    $r = Invoke-Gate '-Tier plan'
    Write-Result 'corpus key: carried when nothing it reads changed' (Get-CarryWhy $r 'corpus')

    Set-RepoFile 'src\common\unrelated.h' "int unrelated2;`r`n"
    Set-RepoFile 'tests\unit\other.cpp' "int other2;`r`n"
    [IO.File]::SetLastWriteTimeUtc($tlog, [DateTime]::UtcNow.AddMinutes(1))
    $r = Invoke-Gate '-Tier plan'
    Write-Result 'corpus key: a TU outside its sources and that TU''s headers do not run it' (Get-CarryWhy $r 'corpus')

    Set-RepoFile 'src\movie\dec.c' "int dec2;`r`n"
    $r = Invoke-Gate '-Tier plan'
    Write-Result 'corpus key: a change to a listed source runs it' (Get-RunWhy $r 'corpus')

    Set-RepoFile 'src\common\shared.h' "int shared2;`r`n"
    [IO.File]::SetLastWriteTimeUtc($tlog, [DateTime]::UtcNow.AddMinutes(1))
    $r = Invoke-Gate '-Tier plan'
    Write-Result 'corpus key: a change to a header in the include closure from the tlog runs it' (Get-RunWhy $r 'corpus')

    Set-RepoFile 'src\common\shared.h' "int shared3;`r`n"
    [IO.File]::SetLastWriteTimeUtc($tlog, [DateTime]::UtcNow.AddMinutes(-60))
    $r = Invoke-Gate '-Tier plan'
    $why = Get-RunWhy $r 'corpus'
    if (-not $why -and $r.Text -notmatch '(?m)^gate: corpus Win32 run \(key invalid: tlog\)') { $why = "no stale-tlog reason: $($r.Text)" }
    Write-Result 'corpus key: a tlog older than a file it read runs it' $why
    [IO.File]::SetLastWriteTimeUtc($tlog, [DateTime]::UtcNow.AddMinutes(1))
    [void](Invoke-Gate '-Tier plan')

    $rec = Get-Rec 'corpus'
    Merge-KeyedRecords $recFile $script:StepColumns $script:StepKeyColumns @() @("corpus`tWin32`tDebug")
    $r = Invoke-Gate '-Tier plan'
    Write-Result 'corpus key: a missing record runs it' $(if (-not $rec) { 'no corpus record before the drop' } else { Get-RunWhy $r 'corpus' })

    Remove-Item -LiteralPath $tlog -Force
    $r = Invoke-Gate '-Tier plan'
    $why = Get-RunWhy $r 'corpus'
    if (-not $why -and $r.Text -notmatch '(?m)^gate: corpus Win32 run \(key invalid: tlog\)') { $why = "no missing-tlog reason: $($r.Text)" }
    Write-Result 'corpus key: a missing tlog runs it' $why
    Write-Tlog

    $orphan = Join-Path $ev ('a' * 64)
    New-Item -ItemType Directory -Force -Path $orphan | Out-Null
    [IO.File]::WriteAllText((Join-Path $orphan 'exit.txt'), "0`r`n")
    for ($i = 0; $i -lt 4; $i++) { $r = Invoke-Gate '-Tier plan' }
    $runs = @(Get-ChildItem -LiteralPath (Join-Path $repo 'artifacts\gate') -Directory | Sort-Object Name)
    $newest = $runs[-1].Name
    $env:OPENSWBF2_GATETEST_FAIL = 'run-a'
    Set-RepoFile 'src\a.cpp' "int d;`r`n"
    try { $rf = Invoke-Gate '-Tier plan' }
    finally { Remove-Item Env:\OPENSWBF2_GATETEST_FAIL -ErrorAction SilentlyContinue }
    $failRun = @(Get-ChildItem -LiteralPath (Join-Path $repo 'artifacts\gate') -Directory | Sort-Object Name)[-1].Name
    $rr = Invoke-Gate '-Tier plan -Resume'
    $after = @(Get-ChildItem -LiteralPath (Join-Path $repo 'artifacts\gate') -Directory | Sort-Object Name | ForEach-Object { $_.Name })
    $named = @((Read-StepRecords $recFile).Values | ForEach-Object { $_.Evidence })
    $why = $null
    if ($r.Code -ne 0 -or $runs.Count -ne 3) { $why = "after four runs $($runs.Count) run folders remain (exit $($r.Code))" }
    elseif ([IO.Directory]::Exists($orphan)) { $why = 'the evidence folder no record names is still there' }
    elseif (@($named | Where-Object { -not [IO.Directory]::Exists((Join-Path $ev $_)) }).Count) { $why = 'a named evidence folder was deleted' }
    elseif ($rf.Code -ne 1 -or $failRun -ceq $newest) { $why = "the failing run: exit $($rf.Code), folder $failRun" }
    elseif ($rr.Code -ne 0 -or $rr.Text -notmatch "(?m)^gate: tier plan platform Win32 run .*\\$([regex]::Escape($failRun))\r?$" -or $rr.Text -notmatch '(?m)^\[RESUMED\] gate:build ') { $why = "-Resume did not continue $($failRun): exit $($rr.Code): $($rr.Text)" }
    elseif ($after.Count -ne 3 -or $after -cnotcontains $failRun) { $why = "after the resume the run folders are $($after -join ', ')" }
    Write-Result 'retention keeps the three newest run folders and the one -Resume continues, and deletes evidence no record names' $why
}
catch { $script:failed++; Write-Host "FAILED: gate tests threw: $($_.Exception.Message) $($_.InvocationInfo.PositionMessage)" }
finally {
    Remove-Item Env:\OPENSWBF2_GATETEST_FAIL -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $tmp -Recurse -Force -ErrorAction SilentlyContinue
}
if ($script:failed) { Write-Host "gate tests: $($script:failed) failed"; exit 1 }
Write-Host 'gate tests: all passed'
exit 0
