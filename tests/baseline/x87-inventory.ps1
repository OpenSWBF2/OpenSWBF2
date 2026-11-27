param(
    [string]$Exe,
    [string]$Disasm,
    [string]$Map,
    [string]$Out,
    [string]$Check,
    [string]$Inventory,
    [string]$Causes,
    [string]$Project
)

$ErrorActionPreference = 'Stop'

$script:OffPathDirs = @(
    @{ Path = 'src\RedEngineFL\Graphics\'; Name = 'renderer'; Reason = 'D3D9 renderer: turns sim state into GPU work and never writes sim state' }
    @{ Path = 'src\RedEngineFL\Interface\'; Name = 'ui'; Reason = 'UI toolkit and fonts: screen layout and glyph metrics, not sim state' }
    @{ Path = 'src\RedEngineFL\movie\'; Name = 'movie'; Reason = 'Bink movie playback, outside the mission simulation' }
    @{ Path = 'src\SoundFL\'; Name = 'sound'; Reason = 'audio engine: its float results go to the sound device, not to sim state' }
    @{ Path = 'src\LuaFL\src\lgc.cpp'; Name = 'lua-gc'; Reason = 'Lua collector pacing (fidiv on the GC threshold): decides when to collect, never a script value' }
    @{ Path = 'src\lglcd\'; Name = 'lcd'; Reason = 'Logitech LCD applet output' }
    @{ Path = 'src\Steam\'; Name = 'platform-services'; Reason = 'Steam and Galaxy services: auth, lobby and transport, no sim arithmetic' }
)
$script:NoSource = @{ Name = 'nosource'; Reason = 'no ClCompile item of the game projects builds this object (CRT, import library or vendor code), and no game function outside the off-path directories calls it, directly or through other such objects' }
$script:SimExceptions = @(
    'src\RedEngineFL\Application\RedTerrain.cpp'
    'src\Battlefront2\Source\WeaponAreaEffect.cpp'
    'src\Battlefront2\Source\EntityLight.cpp'
    'src\PebbleFL\Common\PblQuaternion.cpp'
    'src\Battlefront2\Source\AIUtil.cpp'
    'src\Battlefront2\Source\Cloth.cpp'
    'src\Battlefront2\Source\CombatUtil.cpp'
    'src\Battlefront2\Source\Combo.cpp'
    'src\Battlefront2\Source\EntityDroideka.cpp'
    'src\Battlefront2\Source\EntitySoldier.cpp'
    'src\Battlefront2\Source\EntityWalker.cpp'
    'src\Battlefront2\Source\GameLoop.cpp'
    'src\Battlefront2\Source\MountedTurret.cpp'
    'src\Battlefront2\Source\OrdnanceEmitter.cpp'
    'src\Battlefront2\Source\SoldierAnimator.cpp'
    'src\Battlefront2\Source\SoldierAnimatorLowRes.cpp'
    'src\Battlefront2\Source\SpaceAssault.cpp'
    'src\Battlefront2\Source\TentacleSimulator.cpp'
    'src\Battlefront2\Source\Trackable.cpp'
    'src\Battlefront2\Source\WalkerLegPair.cpp'
    'src\Battlefront2\Source\WeaponCannon.cpp'
    'src\RedEngineFL\Graphics\RedSpotLight.cpp'
    'src\RedEngineFL\Application\RedWater.cpp'
    'src\RedEngineFL\Graphics\PC\pcRedRenderer.cpp'
    'src\RedEngineFL\Graphics\RedCamera.cpp'
    'src\Battlefront2\Platform\Pc\pcWeatherEffect.cpp'
    'src\Battlefront2\Source\FirstPerson.cpp'
    'src\Battlefront2\Source\CameraManager.cpp'
    'src\Battlefront2\Source\FreeCamera.cpp'
    'src\Battlefront2\Source\HUDElementMap.cpp'
    'src\Battlefront2\Source\HUDElementTarget.cpp'
    'src\SoundFL\source\snddsp.cpp'
    'src\Battlefront2\Platform\Pc\pcColorRamp.cpp'
    'src\Battlefront2\Source\ChaseCamera.cpp'
    'src\RedEngineFL\Graphics\PC\Shaders\pcRefractionShader.cpp'
    'src\RedEngineFL\Util\RedScreenShot.cpp'
    'src\LuaFL\src\lib\lmathlib.cpp'
    'src\Battlefront2\Source\RepairHelper.cpp'
    'src\PebbleFL\Common\PblMath.cpp'
    'src\Battlefront2\Source\ParticleGeometry.cpp'
    'src\Battlefront2\Platform\Pc\pcHeatShimmerEffect.cpp'
)
$script:CauseLine = '^- `[^`]+` \| (C1|C2|C3|C-X64-\S+) \| '
$script:Header = @('obj', 'function', 'flags', 'pattern', 'verdict', 'evidence')
$script:RuleVerdict = '^(offpath:(' + ((@($script:OffPathDirs | ForEach-Object { $_.Name }) + $script:NoSource.Name | ForEach-Object { [regex]::Escape($_) }) -join '|') + ')$|match:exact$|match:single-rounding$)'
$script:FlagOrder = @('arith', 'callret_precise', 'chain', 'cmp_precise', 'cvt_precise', 'ret_precise', 'store_double_precise', 'transc', 'unknown')

$script:Arith2 = @{}
foreach ($m in 'fadd', 'faddp', 'fsub', 'fsubp', 'fsubr', 'fsubrp', 'fmul', 'fmulp', 'fdiv', 'fdivp', 'fdivr', 'fdivrp') { $script:Arith2[$m] = 'f' }
foreach ($m in 'fiadd', 'fisub', 'fisubr', 'fimul', 'fidiv', 'fidivr') { $script:Arith2[$m] = 'i' }
$script:Arith1 = @('fsqrt', 'frndint', 'fscale', 'fprem', 'fprem1')
$script:Transc = @{ fsin = 0; fcos = 0; f2xm1 = 0; fsincos = 1; fptan = 2; fpatan = -1; fyl2x = -1; fyl2xp1 = -1 }
$script:Consts = @{ fldz = 'E'; fld1 = 'E'; fldpi = 'K'; fldl2e = 'K'; fldl2t = 'K'; fldlg2 = 'K'; fldln2 = 'K' }
$script:Compare = @{ fcom = 0; fcomp = 1; fcompp = 2; fucom = 0; fucomp = 1; fucompp = 2; fcomi = 0; fcomip = 1; fucomi = 0; fucomip = 1; ftst = 0; ficom = 0; ficomp = 1 }
$script:Passive = @('ffree', 'fincstp', 'fdecstp', 'fnop', 'fwait', 'fnstcw', 'fstcw', 'fldcw', 'fnstsw', 'fstsw', 'fninit', 'fnclex', 'fclex', 'fxsave', 'fxrstor', 'fnsave', 'fsave', 'frstor', 'fldenv', 'fnstenv', 'fstenv', 'femms')
$script:ExactSet = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
foreach ($m in 'fld', 'fild', 'fst', 'fstp', 'fxch', 'fldz', 'fld1', 'fchs', 'fabs', 'ffree', 'fnop', 'fwait', 'fincstp', 'fdecstp') { [void]$script:ExactSet.Add($m) }
$script:SingleSet = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
foreach ($m in $script:ExactSet) { [void]$script:SingleSet.Add($m) }
foreach ($m in 'fadd', 'faddp', 'fsub', 'fsubp', 'fsubr', 'fsubrp', 'fmul', 'fmulp', 'fdiv', 'fdivp', 'fdivr', 'fdivrp', 'fsqrt') { [void]$script:SingleSet.Add($m) }

$script:ExactEvidence = 'x87 only loads (float, double, integer), stores, moves and sign ops: the only rounding is a store, which rounds once like the SSE load, store or conversion, at every precision'
$script:SingleEvidence = 'each x87 result is one + - * / or sqrt of two float operands loaded in its block and stored once to a dword: 53 >= 2*24+2, so the double rounding equals one float rounding (SSE single)'

function Get-RepoRoot { return (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) }

function Test-ItemCondition([string]$Condition) {
    if (-not $Condition) { return $true }
    $c = $Condition -replace '\s', ''
    if ($c -ceq "'`$(Platform)'=='Win32'") { return $true }
    if ($c -ceq "'`$(Platform)'=='x64'") { return $false }
    throw "x87-inventory: unknown ClCompile condition $Condition"
}

function Get-ProjectSources([string]$ProjectPath) {
    $root = Get-RepoRoot
    $sources = New-Object System.Collections.Hashtable ([StringComparer]::OrdinalIgnoreCase)
    $seen = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    $queue = New-Object System.Collections.Generic.Queue[string]
    $queue.Enqueue([IO.Path]::GetFullPath($ProjectPath))
    while ($queue.Count) {
        $p = $queue.Dequeue()
        if (-not $seen.Add($p)) { continue }
        $dir = Split-Path $p -Parent
        $x = New-Object Xml.XmlDocument
        $x.Load($p)
        foreach ($g in $x.DocumentElement.ChildNodes) {
            if ($g.LocalName -cne 'ItemGroup') { continue }
            if (-not (Test-ItemCondition $g.GetAttribute('Condition'))) { continue }
            foreach ($it in $g.ChildNodes) {
                if ($it.NodeType -ne [Xml.XmlNodeType]::Element) { continue }
                if ($it.LocalName -ceq 'ClCompile') {
                    if (-not (Test-ItemCondition $it.GetAttribute('Condition'))) { continue }
                    $full = [IO.Path]::GetFullPath((Join-Path $dir $it.GetAttribute('Include')))
                    $rel = $full.Substring($root.TrimEnd('\').Length + 1)
                    $obj = [IO.Path]::GetFileNameWithoutExtension($full) + '.obj'
                    if ($sources.ContainsKey($obj) -and $sources[$obj] -cne $rel) { throw "x87-inventory: $obj is built from $($sources[$obj]) and $rel" }
                    $sources[$obj] = $rel
                }
                elseif ($it.LocalName -ceq 'ProjectReference') {
                    $link = $null
                    foreach ($md in $it.ChildNodes) { if ($md.LocalName -ceq 'LinkLibraryDependencies') { $link = $md.InnerText.Trim() } }
                    if ($link -ceq 'true') { $queue.Enqueue([IO.Path]::GetFullPath((Join-Path $dir $it.GetAttribute('Include')))) }
                }
            }
        }
    }
    return $sources
}

function Get-CauseFiles([string]$Path, $Sources) {
    $byName = New-Object System.Collections.Hashtable ([StringComparer]::OrdinalIgnoreCase)
    foreach ($rel in $Sources.Values) { $byName[[IO.Path]::GetFileName($rel)] = $rel }
    $set = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
    foreach ($line in [IO.File]::ReadAllLines($Path)) {
        if ($line -cnotmatch $script:CauseLine) { continue }
        foreach ($m in [regex]::Matches($line, '[A-Za-z0-9_]+\.(?:cpp|c)\b')) {
            if ($byName.ContainsKey($m.Value)) { [void]$set.Add($byName[$m.Value]) }
        }
    }
    return , $set
}

function Read-MapObjects([string]$Path) {
    $byAddr = @{}
    $rx = [regex]'^\s*0001:[0-9a-f]+\s+(\S+)\s+([0-9a-f]{8})\s+(f?)\s*(?:i\s+)?(\S+)\s*$'
    foreach ($line in [IO.File]::ReadLines($Path)) {
        $m = $rx.Match($line)
        if (-not $m.Success) { continue }
        $a = [Convert]::ToUInt32($m.Groups[2].Value, 16)
        $e = @{ Name = $m.Groups[1].Value; Obj = $m.Groups[4].Value; Func = ($m.Groups[3].Value -ceq 'f') }
        if ($byAddr.ContainsKey($a)) { $byAddr[$a] = @($byAddr[$a]) + @($e) } else { $byAddr[$a] = @($e) }
    }
    return $byAddr
}

function Get-FunctionObj($Entries, [string]$Name) {
    if ($null -eq $Entries) { return '?' }
    $list = @($Entries)
    foreach ($e in $list) { if ($e.Name -ceq $Name) { return $e.Obj } }
    $objs = [string[]]@($list | ForEach-Object { $_.Obj })
    [Array]::Sort($objs, [StringComparer]::Ordinal)
    return $objs[0]
}

function Read-DisasmFunctions([IO.TextReader]$Reader, $Boundaries) {
    $funcs = New-Object System.Collections.Generic.List[object]
    $name = $null
    $lines = $null
    $start = -1
    $hasF = $false
    $calls = $null
    while ($null -ne ($line = $Reader.ReadLine())) {
        if ($line.Length -eq 0) { continue }
        if ($line[0] -ne ' ') {
            if ($null -ne $name) { $funcs.Add(@{ Name = $name; Start = $start; Lines = $(if ($hasF) { $lines } else { $null }); Calls = $calls }) }
            $name = $null
            if ($line.EndsWith(':') -and -not $line.StartsWith('SECTION')) {
                $name = $line.Substring(0, $line.Length - 1)
                $lines = New-Object System.Collections.Generic.List[string]
                $calls = New-Object System.Collections.Generic.List[string]
                $start = -1
                $hasF = $false
            }
            continue
        }
        if ($null -eq $name -or $line.Length -lt 13 -or $line[10] -ne ':') { continue }
        if ($null -ne $Boundaries) { [void]$Boundaries.Add([Convert]::ToInt64($line.Substring(2, 8), 16)) }
        if ($start -lt 0) { $start = [Convert]::ToInt64($line.Substring(2, 8), 16) }
        $lines.Add($line)
        if ($line[12] -ceq 'f') { $hasF = $true }
        elseif ($line.Length -gt 24 -and ([string]::CompareOrdinal($line, 12, 'call ', 0, 5) -eq 0 -or [string]::CompareOrdinal($line, 12, 'jmp ', 0, 4) -eq 0)) {
            $t = $line.Substring(24).Trim()
            if ($t.Length -and $t.IndexOf('[') -lt 0 -and $t -cnotmatch '^[0-9A-F]{8}$') { $calls.Add($t) }
        }
    }
    if ($null -ne $name) { $funcs.Add(@{ Name = $name; Start = $start; Lines = $(if ($hasF) { $lines } else { $null }); Calls = $calls }) }
    return $funcs
}

function Get-OperandKind([string]$Op) {
    $o = $Op.Trim()
    if ($o -ceq 'st') { return 'st(0)' }
    if ($o -cmatch '^st\(\d\)$') { return $o }
    if ($o.StartsWith('tbyte ptr')) { return 'm80' }
    if ($o.StartsWith('qword ptr')) { return 'm64' }
    if ($o.StartsWith('dword ptr')) { return 'm32' }
    if ($o.StartsWith('word ptr')) { return 'm16' }
    if ($o -cmatch '^[a-z]{2,3}$') { return $o }
    return 'm'
}

function Get-StIndex([string]$Kind) {
    if ($Kind -cmatch '^st\((\d)\)$') { return [int]$Matches[1] }
    return -1
}

function Convert-Function($F, [long]$End) {
    $rxData = [regex]'([0-9A-F]{6,8})h\]'
    $parsed = New-Object System.Collections.Generic.List[object]
    $dataStart = $End
    foreach ($line in $F.Lines) {
        $addr = [Convert]::ToInt64($line.Substring(2, 8), 16)
        if ($addr -ge $dataStart) { break }
        $rest = $line.Substring(12)
        $sp = $rest.IndexOf(' ')
        if ($sp -lt 0) { $mn = $rest; $ops = '' } else { $mn = $rest.Substring(0, $sp); $ops = $rest.Substring($sp).Trim() }
        if ($mn -cnotmatch '^[a-z]') { continue }
        foreach ($m in $rxData.Matches($ops)) {
            $t = [Convert]::ToInt64($m.Groups[1].Value, 16)
            if ($t -ge $F.Start -and $t -lt $End -and $t -lt $dataStart) { $dataStart = $t }
        }
        $parsed.Add(@{ A = $addr; M = $mn; O = $ops })
    }
    $labels = New-Object 'System.Collections.Generic.HashSet[long]'
    $ins = New-Object System.Collections.Generic.List[object]
    $hasF = $false
    foreach ($p in $parsed) {
        if ($p.A -ge $dataStart) { break }
        $mn = $p.M
        if ($mn[0] -eq 'f') { $hasF = $true; $ins.Add($p); continue }
        if ($mn -ceq 'call' -or $mn -ceq 'ret') { $ins.Add($p); continue }
        if ($mn[0] -eq 'j' -or $mn.StartsWith('loop')) {
            if ($p.O -cmatch '^[0-9A-F]{8}$') {
                $t = [Convert]::ToInt64($p.O, 16)
                if ($t -ge $F.Start -and $t -lt $dataStart) { [void]$labels.Add($t) }
            }
            $ins.Add($p)
        }
    }
    if (-not $hasF) { return $null }
    return @{ Name = $F.Name; Start = $F.Start; Ins = $ins; Labels = $labels }
}

function Get-Slot($St, [int]$I, $Ctx) {
    if ($I -lt $St.Count) { return $St[$I] }
    $Ctx.In = $true
    return 'U|-1'
}

function Set-Slot($St, [int]$I, [string]$V) {
    while ($St.Count -le $I) { $St.Add('U|-1') }
    $St[$I] = $V
}

function Test-Precise([string]$S) { return ($S[0] -eq 'R' -or $S[0] -eq 'P') }

function Test-FloatInBlock([string]$S, [int]$Block) { return ($S[0] -eq 'E' -and $S.Substring(2) -ceq "$Block") }

function Test-StackHasR($St) {
    foreach ($s in $St) { if ($s[0] -eq 'R') { return $true } }
    return $false
}

function Measure-Function($Fn, $Ret, $ByName, $ByAddr) {
    $flags = @{}
    $retE = $true
    $retSeen = $false
    $tokens = New-Object System.Collections.Generic.List[string]
    $st = New-Object System.Collections.Generic.List[string]
    $ctx = @{ In = $false }
    $block = 0
    $single = $true
    $exact = $true
    $count = 0
    $first = $true
    foreach ($p in $Fn.Ins) {
        if (-not $first -and $Fn.Labels.Contains($p.A)) {
            if (Test-StackHasR $st) { $single = $false }
            if ($st.Count) { $tokens.Add('label') }
            $block++
        }
        $first = $false
        $mn = $p.M
        $ops = $p.O
        if ($mn[0] -ne 'f') {
            if ($mn -ceq 'call') {
                if (Test-StackHasR $st) { $single = $false }
                if ($st.Count) { $tokens.Add('call') }
                $st.Clear()
                $t = $null
                if ($ByName.ContainsKey($ops)) { $t = $ByName[$ops] }
                elseif ($ops -cmatch '^[0-9A-F]{8}$' -and $ByAddr.ContainsKey([Convert]::ToInt64($ops, 16))) { $t = $ByAddr[[Convert]::ToInt64($ops, 16)] }
                if ($null -ne $t -and $Ret[$t] -ceq 'P') {
                    $st.Insert(0, "P|$block")
                    $flags['callret_precise'] = 1
                }
                elseif ($null -ne $t -and $Ret[$t] -ceq 'E') { $st.Insert(0, "E|$block") }
            }
            elseif ($mn -ceq 'ret') {
                if ($st.Count -and (Test-Precise $st[0])) { $flags['ret_precise'] = 1 }
                if ($st.Count) {
                    $tokens.Add('ret')
                    $retSeen = $true
                    if (-not (Test-FloatInBlock $st[0] $block)) { $retE = $false }
                }
                $st.Clear()
                $block++
            }
            elseif ($mn -ceq 'jmp') {
                if (Test-StackHasR $st) { $single = $false }
                if ($st.Count) { $tokens.Add('jmp') }
                $t = $null
                if ($ByName.ContainsKey($ops)) { $t = $ByName[$ops] }
                elseif ($ops -cmatch '^[0-9A-F]{8}$') {
                    $a = [Convert]::ToInt64($ops, 16)
                    if (-not $Fn.Labels.Contains($a) -and $ByAddr.ContainsKey($a)) { $t = $ByAddr[$a] }
                }
                if ($null -ne $t -and $Ret[$t] -ceq 'P') { $flags['ret_precise'] = 1 }
                if ($null -eq $t -or $Ret[$t] -cne 'E') { $retE = $false } else { $retSeen = $true }
                $st.Clear()
                $block++
            }
            else {
                if (Test-StackHasR $st) { $single = $false }
                if ($st.Count) { $tokens.Add('jcc') }
            }
            continue
        }
        $count++
        $parts = @()
        if ($ops) { $parts = @($ops.Split(',') | ForEach-Object { Get-OperandKind $_ }) }
        $tok = $mn
        if ($parts.Count) { $tok += ' ' + ($parts -join ',') }
        if (-not $script:ExactSet.Contains($mn)) { $exact = $false }
        if (-not $script:SingleSet.Contains($mn)) { $single = $false }
        $ctx.In = $false
        if ($mn -ceq 'fld') {
            $i = $(if ($parts.Count) { Get-StIndex $parts[0] } else { -1 })
            if ($i -ge 0) { $st.Insert(0, (Get-Slot $st $i $ctx)) }
            elseif ($parts.Count -and $parts[0] -ceq 'm32') { $st.Insert(0, "E|$block") }
            else { $st.Insert(0, "D|$block") }
        }
        elseif ($mn -ceq 'fild' -or $mn -ceq 'fbld') { $st.Insert(0, "I|$block") }
        elseif ($script:Consts.ContainsKey($mn)) { $st.Insert(0, "$($script:Consts[$mn])|$block") }
        elseif ($mn -ceq 'fst' -or $mn -ceq 'fstp') {
            $v = Get-Slot $st 0 $ctx
            $i = $(if ($parts.Count) { Get-StIndex $parts[0] } else { -1 })
            if ($i -ge 0) { Set-Slot $st $i $v }
            elseif ((Test-Precise $v) -and $parts.Count -and ($parts[0] -ceq 'm64' -or $parts[0] -ceq 'm80')) { $flags['store_double_precise'] = 1 }
            elseif ($v[0] -eq 'R' -and -not ($parts.Count -and $parts[0] -ceq 'm32')) { $single = $false }
            if ($mn -ceq 'fstp' -and $st.Count) { $st.RemoveAt(0) }
        }
        elseif ($mn -ceq 'fist' -or $mn -ceq 'fistp' -or $mn -ceq 'fisttp' -or $mn -ceq 'fbstp') {
            if (Test-Precise (Get-Slot $st 0 $ctx)) { $flags['cvt_precise'] = 1 }
            if ($mn -cne 'fist' -and $st.Count) { $st.RemoveAt(0) }
        }
        elseif ($script:Arith2.ContainsKey($mn)) {
            $flags['arith'] = 1
            $pop = $mn.EndsWith('p')
            $si = @($parts | ForEach-Object { Get-StIndex $_ })
            if ($parts.Count -eq 2 -and $si[0] -ge 0 -and $si[1] -ge 0) { $d = $si[0]; $a = Get-Slot $st $si[0] $ctx; $b = Get-Slot $st $si[1] $ctx }
            elseif ($parts.Count -eq 0) { $d = 1; $a = Get-Slot $st 0 $ctx; $b = Get-Slot $st 1 $ctx }
            elseif ($parts.Count -eq 1 -and $si[0] -ge 0) { $d = 0; $a = Get-Slot $st 0 $ctx; $b = Get-Slot $st $si[0] $ctx }
            else {
                $d = 0
                $a = Get-Slot $st 0 $ctx
                if ($script:Arith2[$mn] -ceq 'i') { $b = "I|$block" }
                elseif ($parts[0] -ceq 'm32') { $b = "E|$block" }
                else { $b = "D|$block" }
            }
            if ((Test-Precise $a) -or (Test-Precise $b)) { $flags['chain'] = 1 }
            if ((Test-FloatInBlock $a $block) -and (Test-FloatInBlock $b $block)) { $r = "R|$block" }
            else { $r = "P|$block"; $single = $false }
            Set-Slot $st $d $r
            if ($pop -and $st.Count) { $st.RemoveAt(0) }
        }
        elseif ($script:Arith1 -ccontains $mn) {
            $flags['arith'] = 1
            $a = Get-Slot $st 0 $ctx
            if (Test-Precise $a) { $flags['chain'] = 1 }
            if ($mn -ceq 'fsqrt' -and (Test-FloatInBlock $a $block)) { Set-Slot $st 0 "R|$block" }
            else { Set-Slot $st 0 "P|$block"; $single = $false }
        }
        elseif ($script:Transc.ContainsKey($mn)) {
            $flags['transc'] = 1
            [void](Get-Slot $st 0 $ctx)
            $eff = $script:Transc[$mn]
            if ($eff -eq -1) { [void](Get-Slot $st 1 $ctx); Set-Slot $st 1 "P|$block"; $st.RemoveAt(0) }
            else {
                Set-Slot $st 0 "P|$block"
                if ($eff -eq 1) { $st.Insert(0, "P|$block") }
                elseif ($eff -eq 2) { $st.Insert(0, "E|$block") }
            }
        }
        elseif ($mn -ceq 'fchs' -or $mn -ceq 'fabs') { Set-Slot $st 0 (Get-Slot $st 0 $ctx) }
        elseif ($mn -ceq 'fxch') {
            $i = $(if ($parts.Count) { Get-StIndex $parts[0] } else { 1 })
            if ($i -lt 0) { $i = 1 }
            $a = Get-Slot $st 0 $ctx
            $b = Get-Slot $st $i $ctx
            Set-Slot $st 0 $b
            Set-Slot $st $i $a
        }
        elseif ($script:Compare.ContainsKey($mn)) {
            $i = 1
            $reg = $true
            if ($parts.Count) {
                $i = Get-StIndex $parts[$parts.Count - 1]
                if ($i -lt 0) { $reg = $false; $i = 1 }
            }
            if ($mn -ceq 'ftst' -or $mn -ceq 'ficom' -or $mn -ceq 'ficomp') { $reg = $false }
            $a = Get-Slot $st 0 $ctx
            $hit = Test-Precise $a
            if ($reg -and (Test-Precise (Get-Slot $st $i $ctx))) { $hit = $true }
            if ($hit) { $flags['cmp_precise'] = 1 }
            for ($k = 0; $k -lt $script:Compare[$mn]; $k++) { if ($st.Count) { $st.RemoveAt(0) } }
        }
        elseif ($mn.StartsWith('fcmov')) {
            $i = $(if ($parts.Count) { Get-StIndex $parts[$parts.Count - 1] } else { 1 })
            if ($i -lt 0) { $i = 1 }
            if (Test-Precise (Get-Slot $st $i $ctx)) { Set-Slot $st 0 "P|$block" }
        }
        elseif ($script:Passive -ccontains $mn) { }
        else { $flags['unknown'] = 1 }
        if ($ctx.In) { $tokens.Add('in') }
        $tokens.Add($tok)
    }
    if (Test-StackHasR $st) { $single = $false }
    $names = @($script:FlagOrder | Where-Object { $flags.ContainsKey($_) })
    $flagText = $(if ($names.Count) { $names -join ',' } else { '-' })
    $sha = [Security.Cryptography.SHA256]::Create()
    try { $h = $sha.ComputeHash([Text.Encoding]::ASCII.GetBytes($tokens -join "`n")) }
    finally { $sha.Dispose() }
    $hex = ([BitConverter]::ToString($h, 0, 8) -replace '-', '').ToLowerInvariant()
    $rule = $null
    if ($flagText -ceq '-' -and $exact) { $rule = 'match:exact' }
    elseif ($flagText -ceq 'arith' -and $single) { $rule = 'match:single-rounding' }
    $kind = '-'
    if ($flags.ContainsKey('ret_precise')) { $kind = 'P' }
    elseif ($retE -and $retSeen) { $kind = 'E' }
    return @{ Flags = $flagText; Pattern = "${count}:$hex"; Rule = $rule; Ret = $kind }
}

function Get-OffPath([string]$Obj, $Sources, $Exceptions) {
    if ($Obj.Contains(':') -or -not $Sources.ContainsKey($Obj)) { return @{ Verdict = "offpath:$($script:NoSource.Name)"; Evidence = "$Obj`: $($script:NoSource.Reason)"; NoSource = $true } }
    $src = $Sources[$Obj]
    if ($Exceptions.Contains($src)) { return $null }
    foreach ($d in $script:OffPathDirs) {
        $hit = $(if ($d.Path.EndsWith('\')) { $src.StartsWith($d.Path, [StringComparison]::OrdinalIgnoreCase) } else { $src -ieq $d.Path })
        if ($hit) { return @{ Verdict = "offpath:$($d.Name)"; Evidence = "$src`: $($d.Reason)"; NoSource = $false } }
    }
    return $null
}

function Get-SimCalledNoSource($Funcs, $Objs, $Sources, $Exceptions) {
    $byName = New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)
    $off = @{}
    $queue = New-Object System.Collections.Generic.Queue[int]
    for ($i = 0; $i -lt $Funcs.Count; $i++) {
        $n = $Funcs[$i].Name
        if ($byName.ContainsKey($n)) { $byName[$n] = @($byName[$n]) + @($i) } else { $byName[$n] = @($i) }
        $obj = $Objs[$i]
        if (-not $off.ContainsKey($obj)) { $off[$obj] = Get-OffPath $obj $Sources $Exceptions }
        if ($null -eq $off[$obj]) { $queue.Enqueue($i) }
    }
    $reached = New-Object 'System.Collections.Generic.HashSet[int]'
    while ($queue.Count) {
        $f = $Funcs[$queue.Dequeue()]
        if ($null -eq $f.Calls) { continue }
        foreach ($t in $f.Calls) {
            if (-not $byName.ContainsKey($t)) { continue }
            foreach ($j in $byName[$t]) {
                $o = $off[$Objs[$j]]
                if ($null -ne $o -and $o.NoSource -and $reached.Add($j)) { $queue.Enqueue($j) }
            }
        }
    }
    $names = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    foreach ($j in $reached) { [void]$names.Add($Funcs[$j].Name) }
    return , $names
}

function Invoke-Disasm([string]$Arguments, $Boundaries) {
    $psi = New-Object Diagnostics.ProcessStartInfo
    $psi.FileName = Find-Dumpbin 'Win32'
    $psi.Arguments = $Arguments
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    $proc = [Diagnostics.Process]::Start($psi)
    $errTask = $proc.StandardError.ReadToEndAsync()
    try { $funcs = Read-DisasmFunctions $proc.StandardOutput $Boundaries }
    finally { $proc.WaitForExit() }
    if ($proc.ExitCode -ne 0) { throw "x87-inventory: dumpbin exited $($proc.ExitCode): $($errTask.Result)" }
    return , $funcs
}

function Add-LostFunctions($Funcs, $MapObjs, $Boundaries) {
    $starts = New-Object System.Collections.Generic.List[long]
    foreach ($f in $Funcs) { if ($f.Start -ge 0) { $starts.Add($f.Start) } }
    if ($starts.Count -eq 0) { return , $Funcs }
    $lo = $starts[0]
    $hi = [Linq.Enumerable]::Max([Collections.Generic.IEnumerable[long]]$Boundaries)
    $map = [long[]]@($MapObjs.Keys | Where-Object { @($MapObjs[$_] | Where-Object { $_.Func }).Count } | ForEach-Object { [long]$_ } | Where-Object { $_ -ge $lo -and $_ -le $hi })
    [Array]::Sort($map)
    $added = New-Object System.Collections.Generic.List[object]
    for ($i = 0; $i -lt $map.Count; $i++) {
        $a = $map[$i]
        if ($Boundaries.Contains($a)) { continue }
        $name = @($MapObjs[[uint32]$a] | Where-Object { $_.Func })[0].Name
        if ($Disasm) { throw "x87-inventory: map function $name at $('{0:X8}' -f $a) starts inside a decoded instruction, and -Disasm cannot decode it again" }
        $end = $(if ($i + 1 -lt $map.Count) { $map[$i + 1] } else { $hi + 1 })
        $got = @(Invoke-Disasm ("/nologo /disasm:nobytes /range:0x{0:X8},0x{1:X8} `"{2}`"" -f $a, ($end - 1), [IO.Path]::GetFullPath($Exe)) $null | Where-Object { $_.Start -ge 0 })
        if ($got.Count -eq 0 -or $got[0].Start -ne $a) { throw "x87-inventory: dumpbin did not decode map function $name at $('{0:X8}' -f $a)" }
        foreach ($g in $got) { if ($g.Start -lt $end) { $added.Add($g) } }
    }
    if ($added.Count -eq 0) { return , $Funcs }
    $all = New-Object System.Collections.Generic.List[object]
    foreach ($f in $Funcs) { if ($f.Start -ge 0) { $all.Add($f) } }
    foreach ($g in $added) { $all.Add($g) }
    $arr = $all.ToArray()
    $keys = [long[]]@($arr | ForEach-Object { $_.Start })
    [Array]::Sort($keys, $arr)
    $out = New-Object System.Collections.Generic.List[object]
    foreach ($f in $arr) { $out.Add($f) }
    return , $out
}

function New-Inventory {
    $root = Get-RepoRoot
    if (-not $Causes) { $script:CausesPath = Join-Path $PSScriptRoot 'v143\CAUSES.md' } else { $script:CausesPath = $Causes }
    if (-not $Project) { $script:ProjectPath = Join-Path $root 'BattlefrontII.vcxproj' } else { $script:ProjectPath = $Project }
    $sources = Get-ProjectSources $script:ProjectPath
    $exceptions = Get-CauseFiles $script:CausesPath $sources
    $built = @($sources.Values)
    foreach ($e in $script:SimExceptions) {
        if ($built -notcontains $e) { throw "x87-inventory: sim exception $e is not a ClCompile item of the game projects" }
        [void]$exceptions.Add($e)
    }
    foreach ($d in $script:OffPathDirs) {
        if ($d.Path.EndsWith('\')) { $ok = @($built | Where-Object { $_.StartsWith($d.Path, [StringComparison]::OrdinalIgnoreCase) }).Count -gt 0 } else { $ok = $built -contains $d.Path }
        if (-not $ok) { throw "x87-inventory: off-path entry $($d.Path) builds no ClCompile item" }
    }
    $mapObjs = Read-MapObjects $Map
    $bounds = New-Object 'System.Collections.Generic.HashSet[long]'
    if ($Disasm) {
        $reader = [IO.File]::OpenText($Disasm)
        try { $funcs = Read-DisasmFunctions $reader $bounds }
        finally { $reader.Dispose() }
    }
    else {
        . (Join-Path (Split-Path $PSScriptRoot -Parent) 'vstools.ps1')
        $funcs = Invoke-Disasm "/nologo /disasm:nobytes `"$([IO.Path]::GetFullPath($Exe))`"" $bounds
    }
    $funcs = Add-LostFunctions $funcs $mapObjs $bounds
    $objs = New-Object System.Collections.Generic.List[string]
    foreach ($f in $funcs) { $objs.Add((Get-FunctionObj $mapObjs[[uint32]$f.Start] $f.Name)) }
    $simCalled = Get-SimCalledNoSource $funcs $objs $sources $exceptions
    $fns = New-Object System.Collections.Generic.List[object]
    for ($i = 0; $i -lt $funcs.Count; $i++) {
        $f = $funcs[$i]
        if ($null -eq $f.Lines -or $f.Start -lt 0) { continue }
        $end = [long]::MaxValue
        for ($j = $i + 1; $j -lt $funcs.Count; $j++) { if ($funcs[$j].Start -ge 0) { $end = $funcs[$j].Start; break } }
        $c = Convert-Function $f $end
        if ($null -ne $c) { $c.Obj = $objs[$i]; $fns.Add($c) }
    }
    $byAddr = @{}
    $once = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    $twice = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    foreach ($f in $funcs) { if ($f.Start -ge 0 -and -not $once.Add($f.Name)) { [void]$twice.Add($f.Name) } }
    $byName = New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)
    foreach ($c in $fns) { if (-not $twice.Contains($c.Name)) { $byName[$c.Name] = $c.Start }; $byAddr[$c.Start] = $c.Start }
    $retp = @{}
    for ($it = 0; $it -lt 50; $it++) {
        $changed = $false
        foreach ($c in $fns) {
            $r = Measure-Function $c $retp $byName $byAddr
            $c.Result = $r
            if ($retp[$c.Start] -cne $r.Ret) { $retp[$c.Start] = $r.Ret; $changed = $true }
        }
        if (-not $changed) { break }
    }
    if ($changed) { throw 'x87-inventory: return kinds did not settle in 50 passes' }
    $rows = New-Object System.Collections.Generic.List[object]
    $keys = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::Ordinal)
    foreach ($c in $fns) {
        $obj = $c.Obj
        $key = "$obj`t$($c.Name)"
        if (-not $keys.Add($key)) { throw "x87-inventory: two functions named $($c.Name) in $obj" }
        $r = $c.Result
        $v = ''
        $ev = ''
        if ($r.Rule -ceq 'match:exact') { $v = $r.Rule; $ev = $script:ExactEvidence }
        elseif ($r.Rule -ceq 'match:single-rounding') { $v = $r.Rule; $ev = $script:SingleEvidence }
        else {
            $o = Get-OffPath $obj $sources $exceptions
            if ($o -and -not ($o.NoSource -and $simCalled.Contains($c.Name))) { $v = $o.Verdict; $ev = $o.Evidence }
        }
        $rows.Add([pscustomobject]@{ Obj = $obj; Function = $c.Name; Flags = $r.Flags; Pattern = $r.Pattern; Verdict = $v; Evidence = $ev })
    }
    return , $rows
}

function Read-InventoryFile([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "x87-inventory: $Path not found" }
    $lines = [IO.File]::ReadAllLines($Path)
    if ($lines.Count -eq 0 -or $lines[0] -cne ($script:Header -join "`t")) { throw "x87-inventory: $Path has no header row" }
    $rows = New-Object System.Collections.Generic.List[object]
    for ($i = 1; $i -lt $lines.Count; $i++) {
        $c = $lines[$i].Split("`t")
        if ($c.Count -ne 6) { throw "x87-inventory: $Path line $($i + 1) has $($c.Count) columns" }
        $rows.Add([pscustomobject]@{ Obj = $c[0]; Function = $c[1]; Flags = $c[2]; Pattern = $c[3]; Verdict = $c[4]; Evidence = $c[5] })
    }
    return , $rows
}

function Get-RowIndex($Rows) {
    $h = New-Object System.Collections.Hashtable ([StringComparer]::Ordinal)
    foreach ($r in $Rows) {
        $k = "$($r.Obj)`t$($r.Function)"
        if ($h.ContainsKey($k)) { throw "x87-inventory: row $($r.Obj) $($r.Function) appears twice" }
        $h[$k] = $r
    }
    return $h
}

function Write-Inventory($Rows, [string]$Path) {
    $arr = @($Rows | ForEach-Object { $_ })
    $keys = [string[]]@($arr | ForEach-Object { "$($_.Obj)`t$($_.Function)" })
    [Array]::Sort($keys, $arr, [StringComparer]::Ordinal)
    $sb = New-Object Text.StringBuilder
    [void]$sb.Append(($script:Header -join "`t") + "`r`n")
    foreach ($r in $arr) { [void]$sb.Append((@($r.Obj, $r.Function, $r.Flags, $r.Pattern, $r.Verdict, $r.Evidence) -join "`t") + "`r`n") }
    $dir = Split-Path ([IO.Path]::GetFullPath($Path)) -Parent
    if (-not (Test-Path -LiteralPath $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
    [IO.File]::WriteAllText([IO.Path]::GetFullPath($Path), $sb.ToString(), (New-Object Text.UTF8Encoding $false))
}

function Compare-Inventory($Pinned, $Fresh) {
    $bad = New-Object System.Collections.Generic.List[string]
    $p = Get-RowIndex $Pinned
    $f = Get-RowIndex $Fresh
    $keys = New-Object System.Collections.Generic.List[string]
    foreach ($k in $p.Keys) { $keys.Add($k) }
    foreach ($k in $f.Keys) { if (-not $p.ContainsKey($k)) { $keys.Add($k) } }
    $arr = $keys.ToArray()
    [Array]::Sort($arr, [StringComparer]::Ordinal)
    foreach ($k in $arr) {
        $name = $k.Replace("`t", ' ')
        if (-not $f.ContainsKey($k)) { $bad.Add("removed $name"); continue }
        $n = $f[$k]
        if (-not $p.ContainsKey($k)) { $bad.Add("new $name flags=$($n.Flags) pattern=$($n.Pattern)"); continue }
        $o = $p[$k]
        if ($o.Flags -cne $n.Flags -or $o.Pattern -cne $n.Pattern) { $bad.Add("changed $name flags $($o.Flags) -> $($n.Flags) pattern $($o.Pattern) -> $($n.Pattern)"); continue }
        if (-not $o.Verdict) { $bad.Add("no verdict $name"); continue }
        $ruleRow = ($o.Verdict -cmatch $script:RuleVerdict) -or ($n.Verdict -cmatch $script:RuleVerdict)
        if ($ruleRow -and ($o.Verdict -cne $n.Verdict -or $o.Evidence -cne $n.Evidence)) { $bad.Add("rule verdict $name pinned '$($o.Verdict)' generated '$($n.Verdict)'") }
    }
    return , $bad
}

$hasInput = [bool]($Exe -or $Disasm)
if ($Check) {
    if (($Inventory -and $hasInput) -or ($Exe -and $Disasm)) { Write-Host 'x87-inventory: -Check takes -Inventory, -Exe or -Disasm, only one of them'; exit 2 }
    if (-not $Inventory -and -not ($hasInput -and $Map)) { Write-Host 'x87-inventory: -Check needs -Inventory or -Exe/-Disasm with -Map'; exit 2 }
    $pinned = Read-InventoryFile $Check
    if ($Inventory) { $fresh = Read-InventoryFile $Inventory } else { $fresh = New-Inventory }
    $bad = Compare-Inventory $pinned $fresh
    if ($bad.Count) {
        Write-Host "[FAIL] x87-inventory rows=$($pinned.Count) problems=$($bad.Count)"
        foreach ($l in $bad) { Write-Host "    $l" }
        exit 1
    }
    Write-Host "[PASS] x87-inventory rows=$($pinned.Count)"
    exit 0
}

if (-not $Out -or -not $hasInput -or -not $Map -or ($Exe -and $Disasm)) { Write-Host 'x87-inventory: pass -Exe or -Disasm, -Map and -Out, or -Check'; exit 2 }
$t0 = Get-Date
$rows = New-Inventory
if (Test-Path -LiteralPath $Out -PathType Leaf) {
    $old = Get-RowIndex (Read-InventoryFile $Out)
    foreach ($r in $rows) {
        if ($r.Verdict) { continue }
        $k = "$($r.Obj)`t$($r.Function)"
        if (-not $old.ContainsKey($k)) { continue }
        $o = $old[$k]
        if ($o.Flags -ceq $r.Flags -and $o.Pattern -ceq $r.Pattern -and $o.Verdict -and $o.Verdict -cnotmatch $script:RuleVerdict) { $r.Verdict = $o.Verdict; $r.Evidence = $o.Evidence }
    }
}
Write-Inventory $rows $Out
$counts = [ordered]@{ rows = $rows.Count; exact = 0; 'single-rounding' = 0; offpath = 0; kept = 0; open = 0 }
foreach ($r in $rows) {
    if ($r.Verdict -ceq 'match:exact') { $counts.exact++ }
    elseif ($r.Verdict -ceq 'match:single-rounding') { $counts['single-rounding']++ }
    elseif ($r.Verdict.StartsWith('offpath:')) { $counts.offpath++ }
    elseif ($r.Verdict) { $counts.kept++ }
    else { $counts.open++ }
}
Write-Host ("x87-inventory: " + (@($counts.Keys | ForEach-Object { "$_=$($counts[$_])" }) -join ' ') + " seconds=$([int]((Get-Date) - $t0).TotalSeconds)")
exit 0
