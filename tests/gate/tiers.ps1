$ps = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File')
$psc = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-Command')
$run = $ps + @('tests\run.ps1', '-Platform', '$Platform', '-NoBuild')
$cap = 'powershell -NoProfile -ExecutionPolicy Bypass -File tests\baseline\capture-vs2013.ps1 -Platform $Platform'
$cmp = 'powershell -NoProfile -ExecutionPolicy Bypass -File tests\baseline\compare-tables.ps1'
$x87 = 'powershell -NoProfile -ExecutionPolicy Bypass -File tests\baseline\x87-inventory.ps1'
$corpusCase = 'Unit::MovieCorpus::movie corpus decodes every retail stream'
$all = @('plan', 'phase', 'milestone')
$close = @('phase', 'milestone')
$planWin32 = @('plan:Win32', 'phase', 'milestone')

$inHarness = @('tests\hardening.ps1', 'tests\hardening\', 'tests\common.ps1', 'tests\vstools.ps1', 'tests\smoke\')
$inRun = @('tests\run.ps1', 'tests\gate\keys.ps1') + $inHarness
$inTables = @('tests\baseline\compare-tables.ps1', 'tests\baseline\capture-vs2013.ps1', 'tests\baseline\v143\', 'tests\baseline\v143-x64\')
$inSmoke = $inRun + $inTables + @('lib\vendor-pins.props')
$inProjects = @('*.vcxproj', '*.props', '*.sln', '*.filters')
$inHardening = @('src\', 'tests\', 'lib\') + $inProjects
$inT1 = @('src\', 'tests\', 'lib\', 'data\') + $inProjects
$inGame = @('tests\harness\', 'tests\game\', 'tests\excluded.tsv')
$inCorpus = @('src\FFmpegFL\', 'src\RedEngineFL\movie\', 'tests\unit\movie_corpus.cpp', 'tests\main.cpp', 'tests\testutil.*', 'tests\harness\', 'tests\excluded.tsv')

function New-GateBuild([string]$Name, [string]$Config, [string[]]$Tiers) {
    return @{
        Name = $Name; Tiers = $Tiers; Kind = 'build'; Config = $Config; Exe = 'powershell'
        Args = $psc + @(". .\tests\vstools.ps1; `$m = Find-MSBuild; foreach (`$p in 'BattlefrontII.vcxproj', 'tests\OpenSWBF2Tests.vcxproj') { & `$m `$p /p:Configuration=$Config /p:Platform=`$Platform /m /v:minimal /nologo; if (`$LASTEXITCODE -ne 0) { exit `$LASTEXITCODE } }; exit 0")
        Binaries = @("bin\`$Platform\$Config\BattlefrontII.exe", "bin\Tests\`$Platform\$Config\OpenSWBF2Tests.exe")
    }
}

function New-GateRefBuild([string]$Name, [string]$Config, [string[]]$Tiers, [switch]$Game) {
    $proj = 'tests\OpenSWBF2Tests.vcxproj'
    $bin = "bin\Tests\Win32\$Config\OpenSWBF2Tests.exe"
    if ($Game) {
        $proj = 'BattlefrontII.vcxproj'
        $bin = "bin\Win32\$Config\BattlefrontII.exe"
    }
    return @{
        Name = $Name; Tiers = $Tiers; Platform = @('x64'); Kind = 'build'; Config = $Config; Exe = 'powershell'
        Args = $psc + @(". .\tests\vstools.ps1; `$m = Find-MSBuild; & `$m $proj /p:Configuration=$Config /p:Platform=Win32 /m /v:minimal /nologo; exit `$LASTEXITCODE")
        Binaries = @($bin)
    }
}

function Get-GateExes([string]$Tests, [string]$Game) {
    return @("bin\Tests\`$Platform\$Tests\OpenSWBF2Tests.exe", "bin\`$Platform\$Game\BattlefrontII.exe")
}

function New-GateSmoke([string]$Name, [string[]]$Tiers, [string[]]$Platform, [string]$Config, [string[]]$RunArgs, [string]$JudgeKind = 'smoke') {
    $s = @{
        Name = $Name; Tiers = $Tiers; Platform = $Platform; StartsGame = $true; Kind = 'run'; Config = $Config; Needs = @("build-$Config"); JudgeKinds = @($JudgeKind)
        Exe = 'powershell'; Args = $run + $RunArgs + @('-NoHardening'); Binaries = (Get-GateExes $Config $Config); DllDir = "bin\`$Platform\$Config"; Inputs = $inSmoke + $inGame
    }
    return $s
}

function New-GateHardening([string]$Name, [string[]]$Tiers, [string]$Config, [string[]]$RunArgs) {
    return @{
        Name = $Name; Tiers = $Tiers; Platform = @('Win32', 'x64'); Kind = 'run'; Config = $Config; Needs = @("build-$Config"); JudgeKinds = @('hardening')
        Exe = 'powershell'; Args = $run + $RunArgs + @('-Hardening'); Binaries = (Get-GateExes $Config $Config); DllDir = "bin\`$Platform\$Config"; Inputs = $inHardening
    }
}

$saveCross = New-GateSmoke 'savecross-Test' $close @('x64') 'Test' @('-Configuration', 'Test', '-Smoke', 'SaveCross')
$saveCross.Needs += 'build-Test-game-ref'
$saveCross.Binaries += 'bin\Win32\Test\BattlefrontII.exe'

$GateSteps = @(
    (New-GateBuild 'build-Debug' 'Debug' $planWin32),
    (New-GateBuild 'build-Release' 'Release' $planWin32),
    (New-GateBuild 'build-Test' 'Test' $all),
    (New-GateBuild 'build-Asan' 'Asan' $close),
    (New-GateRefBuild 'build-Test-ref' 'Test' $close),
    (New-GateRefBuild 'build-Asan-ref' 'Asan' $close),
    (New-GateRefBuild 'build-Test-game-ref' 'Test' $close -Game),
    @{ Name = 'gate-tests'; Tiers = $all; Platform = @('Win32'); Kind = 'script'; Config = 'Default'; Exe = 'powershell'; Args = $ps + @('tests\gate\gate.tests.ps1'); Inputs = @('tests\gate.ps1', 'tests\gate\keys.ps1', 'tests\gate\gate.tests.ps1', 'tests\common.ps1', 'tests\hardening.ps1', 'tests\vstools.ps1', 'tests\hardening\allow.ps1') },
    @{ Name = 'hardening-tests'; Tiers = $all; Platform = @('Win32'); Kind = 'script'; Config = 'Default'; Exe = 'powershell'; Args = $ps + @('tests\hardening.tests.ps1'); Inputs = @('tests\hardening.ps1', 'tests\hardening\', 'tests\hardening.tests.ps1', 'tests\common.ps1', 'BattlefrontII.vcxproj', 'lib\vendor-pins.props', 'tests\baseline\v143\', 'tests\baseline\v143-x64\', 'tests\baseline\compare-tables.ps1', 'tests\baseline\capture-vs2013.ps1', 'tests\unit\memory_baseline.cpp', 'tests\unit\movie_retail.cpp', 'tests\game\crash.cpp', 'tests\game\standard.cpp', 'tests\game\standard_case.cpp') },
    @{ Name = 'rider-link'; Tiers = $all; Platform = @('Win32'); Kind = 'script'; Config = 'Default'; Exe = 'powershell'; Args = $psc + @('& tools\rider-link\gradlew.bat -p tools\rider-link --offline --no-daemon test buildPlugin; exit $LASTEXITCODE'); Inputs = @('tools\rider-link\') },
    (New-GateHardening 'hardening-Release' $planWin32 'Release' @('-Configuration', 'Release')),
    (New-GateHardening 'hardening-Test' $planWin32 'Test' @('-Configuration', 'Test')),
    (New-GateHardening 'hardening-Asan' $close 'Asan' @('-Asan')),
    @{ Name = 't1-Debug'; Tiers = $all; Platform = @('Win32'); Kind = 'run'; Config = 'Debug'; Needs = @('build-Debug'); JudgeKinds = @('t1'); Exe = 'powershell'; Args = $run + @('-Configuration', 'Debug', '-Smoke', 'None'); Binaries = (Get-GateExes 'Debug' 'Debug'); Inputs = $inT1 },
    @{ Name = 't1-Test'; Tiers = @('phase:x64', 'milestone:x64'); Platform = @('Win32', 'x64'); Kind = 'run'; Config = 'Test'; Needs = @('build-Test', 'build-Test-ref'); JudgeKinds = @('t1'); Exe = 'powershell'; Args = $run + @('-Configuration', 'Test', '-Smoke', 'None'); Binaries = (Get-GateExes 'Test' 'Test') + @('bin\Tests\Win32\Test\OpenSWBF2Tests.exe'); Inputs = $inT1 },
    @{ Name = 't1-Asan'; Tiers = $close; Platform = @('Win32', 'x64'); Kind = 'run'; Config = 'Asan'; Needs = @('build-Asan', 'build-Asan-ref'); JudgeKinds = @('t1'); Exe = 'powershell'; Args = $run + @('-Asan', '-Smoke', 'None'); Binaries = (Get-GateExes 'Asan' 'Asan') + @('bin\Tests\Win32\Asan\OpenSWBF2Tests.exe'); Inputs = $inT1 },
    @{ Name = 'corpus-Asan'; Tiers = $close; Platform = @('Win32', 'x64'); Kind = 'run'; Config = 'Asan'; Needs = @('build-Asan'); JudgeKinds = @('t1'); Exe = 'powershell'; Args = $run + @('-Asan', '-Smoke', 'None', '-Suite', '*[OnRequest]*', '-Filter', $corpusCase); SourceKey = @{ Tlog = 'obj\Tests\$Platform\Asan\OpenSWBF2Tests.tlog\CL.read.1.tlog'; RuntimeDir = 'bin\Tests\$Platform\Asan' }; Inputs = $inCorpus },
    (New-GateSmoke 'standard-Release' $planWin32 @('Win32', 'x64') 'Release' @('-Configuration', 'Release', '-Smoke', 'Standard')),
    (New-GateSmoke 'standard-Test' $planWin32 @('Win32', 'x64') 'Test' @('-Configuration', 'Test', '-Smoke', 'Standard')),
    (New-GateSmoke 'standard-Asan' $close @('Win32') 'Asan' @('-Asan', '-Smoke', 'Standard')),
    (New-GateSmoke 'boot-Release' $close @('Win32', 'x64') 'Release' @('-Configuration', 'Release', '-Smoke', 'Boot')),
    (New-GateSmoke 'vendor-Release' $close @('Win32', 'x64') 'Release' @('-Configuration', 'Release', '-Smoke', 'Vendor')),
    (New-GateSmoke 'twice-Release' $close @('Win32', 'x64') 'Release' @('-Configuration', 'Release', '-Smoke', 'Twice')),
    (New-GateSmoke 'crash-Release' $close @('Win32', 'x64') 'Release' @('-Configuration', 'Release', '-Smoke', 'Crash')),
    (New-GateSmoke 'crash-Test' @('milestone') @('Win32', 'x64') 'Test' @('-Configuration', 'Test', '-Smoke', 'Crash')),
    (New-GateSmoke 'dedicated-Release' $close @('Win32', 'x64') 'Release' @('-Configuration', 'Release', '-Smoke', 'Dedicated')),
    (New-GateSmoke 'boot-Asan' $close @('x64') 'Asan' @('-Asan', '-Smoke', 'Boot')),
    (New-GateSmoke 'dedicated-Asan' $close @('Win32') 'Asan' @('-Asan', '-Smoke', 'Dedicated')),
    (New-GateSmoke 'soak-Asan' @('milestone') @('Win32') 'Asan' @('-Asan', '-Smoke', 'Soak')),
    (New-GateSmoke 'perturb-Test' @('milestone') @('Win32', 'x64') 'Test' @('-Configuration', 'Test', '-Smoke', 'Perturb')),
    (New-GateSmoke 'all-Release-short' @('phase') @('Win32', 'x64') 'Release' @('-Configuration', 'Release', '-Smoke', 'All', '-Soak', 'Short') 'smoke-all'),
    (New-GateSmoke 'all-Release' @('milestone') @('Win32', 'x64') 'Release' @('-Configuration', 'Release', '-Smoke', 'All', '-Soak', 'Long') 'smoke-all'),
    (New-GateSmoke 'all-Test-short' @('milestone') @('Win32', 'x64') 'Test' @('-Configuration', 'Test', '-Smoke', 'All', '-Soak', 'Short') 'smoke-all'),
    $saveCross,
    @{ Name = 'luac-compare'; Tiers = $close; Platform = @('Win32'); Kind = 'run'; Config = 'Release'; Needs = @('build-Release'); JudgeKinds = @('t1'); Exe = 'powershell'; Args = $ps + @('tests\luac-compare.ps1', '-Configuration', 'Release'); Binaries = @('bin\Win32\Release\LuaCompiler.exe', 'bin\x64\Release\LuaCompiler.exe', 'bin\Tests\$Platform\Release\OpenSWBF2Tests.exe'); Inputs = $inT1 },
    @{ Name = 'x87-inventory'; Tiers = $close; Platform = @('Win32'); Kind = 'capture'; Config = 'Release'; Needs = @('build-Release'); JudgeKinds = @('compare'); Compare = @('-X87Inventory'); Outputs = @('artifacts\gate\$Stamp\x87'); Exe = 'powershell'; Args = $psc + @("$x87 -Exe bin\`$Platform\Release\BattlefrontII.exe -Map obj\`$Platform\Release\BattlefrontII.map -Out artifacts\gate\`$Stamp\x87\x87-inventory.tsv; if (`$LASTEXITCODE -ne 0) { exit `$LASTEXITCODE }; $cmp -Tables artifacts\gate\`$Stamp\x87 -X87Inventory; exit `$LASTEXITCODE"); Binaries = @('bin\$Platform\Release\BattlefrontII.exe'); Inputs = @('tests\baseline\x87-inventory.ps1', 'tests\baseline\compare-tables.ps1', 'tests\baseline\v143\', 'tests\vstools.ps1', 'BattlefrontII.vcxproj') },
    @{ Name = 'capture-collect'; Tiers = @('milestone'); Platform = @('Win32'); StartsGame = $true; Kind = 'capture'; Config = 'Release'; Needs = @('build-Release'); JudgeKinds = @('compare'); Compare = @('-Collect'); Outputs = @('artifacts\gate\$Stamp\collect'); Exe = 'powershell'; Args = $psc + @("$cap -NoBuild -Collect -OutDir artifacts\gate\`$Stamp\collect; if (`$LASTEXITCODE -ne 0) { exit `$LASTEXITCODE }; $cmp -Tables artifacts\gate\`$Stamp\collect -Collect; exit `$LASTEXITCODE"); Binaries = @('bin\$Platform\Release\BattlefrontII.exe', 'bin\Tests\$Platform\Release\OpenSWBF2Tests.exe'); DllDir = 'bin\$Platform\Release'; Inputs = $inHarness + $inTables + $inGame },
    @{ Name = 'capture-map'; Tiers = @('milestone'); Platform = @('Win32'); Kind = 'capture'; Config = 'Release'; Needs = @('build-Release'); JudgeKinds = @('compare'); Compare = @('-Map'); Outputs = @('artifacts\gate\$Stamp\map'); Exe = 'powershell'; Args = $psc + @("$cap -NoBuild -Map -OutDir artifacts\gate\`$Stamp\map; if (`$LASTEXITCODE -ne 0) { exit `$LASTEXITCODE }; $cmp -Tables artifacts\gate\`$Stamp\map -Map; exit `$LASTEXITCODE"); Binaries = @('bin\$Platform\Release\BattlefrontII.exe'); Inputs = $inHarness + $inTables },
    @{ Name = 'capture-layouts'; Tiers = @('milestone'); Platform = @('Win32'); Kind = 'capture'; Config = 'Release'; Needs = @('build-Release'); Outputs = @('artifacts\gate\$Stamp\layouts'); Exe = 'powershell'; Args = $ps + @('tests\baseline\capture-vs2013.ps1', '-Platform', '$Platform', '-NoBuild', '-Layouts', '-OutDir', 'artifacts\gate\$Stamp\layouts'); Binaries = @('bin\$Platform\Release\BattlefrontII.exe'); Inputs = $inHarness + $inTables + @('src\') + $inProjects },
    @{ Name = 'debug-digests'; Tiers = @('milestone'); Platform = @('Win32'); StartsGame = $true; Kind = 'run'; Config = 'Debug'; Needs = @('build-Debug'); JudgeKinds = @('smoke', 'compare'); Compare = @('-DebugDigests'); Outputs = @('artifacts\gate\$Stamp\debug-digests'); Exe = 'powershell'; Args = $psc + @("`$b = @(Get-ChildItem artifacts\tests -Directory -ErrorAction SilentlyContinue | ForEach-Object { `$_.Name }); & powershell -NoProfile -ExecutionPolicy Bypass -File tests\run.ps1 -Platform `$Platform -Configuration Debug -SmokeConfiguration Debug -NoBuild -Smoke Twice -NoHardening; if (`$LASTEXITCODE -ne 0) { exit `$LASTEXITCODE }; `$d = @(Get-ChildItem artifacts\tests -Directory | Where-Object { `$b -notcontains `$_.Name } | Sort-Object Name | Select-Object -Last 1); if (`$d.Count -ne 1) { Write-Host 'no Debug Twice run folder'; exit 3 }; $cap -Configuration Debug -DebugDigests -DebugSmokeDir `$d[0].FullName -OutDir artifacts\gate\`$Stamp\debug-digests; if (`$LASTEXITCODE -ne 0) { exit `$LASTEXITCODE }; $cmp -Tables artifacts\gate\`$Stamp\debug-digests -DebugDigests; exit `$LASTEXITCODE"); Binaries = (Get-GateExes 'Debug' 'Debug'); DllDir = 'bin\$Platform\Debug'; Inputs = $inSmoke + $inGame },
    @{ Name = 'verify-v143'; Tiers = @('milestone'); Platform = @('Win32'); Kind = 'capture'; Config = 'Release'; Outputs = @('tests\baseline\v143'); Exe = 'powershell'; Args = $ps + @('tests\baseline\capture-vs2013.ps1', '-Platform', '$Platform', '-Verify', '-Baseline', 'v143'); Inputs = $inHarness + @('tests\baseline\capture-vs2013.ps1', 'tests\baseline\v143\') },
    @{ Name = 'verify-v143-x64'; Tiers = @('milestone'); Platform = @('Win32'); Kind = 'capture'; Config = 'Release'; Outputs = @('tests\baseline\v143-x64'); Exe = 'powershell'; Args = $ps + @('tests\baseline\capture-vs2013.ps1', '-Platform', '$Platform', '-Verify', '-Baseline', 'v143-x64'); Inputs = $inHarness + @('tests\baseline\capture-vs2013.ps1', 'tests\baseline\v143-x64\') },
    @{ Name = 'x64-census'; Tiers = @('milestone'); Platform = @('Win32'); Kind = 'census'; Config = 'Release'; Outputs = @('artifacts\gate\$Stamp\census'); Exe = 'powershell'; Args = $ps + @('tests\x64-census.ps1', '-Name', 'census-gate', '-OutDir', 'artifacts\gate\$Stamp\census'); Inputs = $inT1 }
)
