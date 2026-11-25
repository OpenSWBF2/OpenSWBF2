if ($PSScriptRoot) { . (Join-Path $PSScriptRoot 'vstools.ps1') }
$script:HarnessCommonLoaded = $true
$script:StallSeconds = 60
$script:SectionPollMs = 10
$script:StallStarvedWaits = @('FreePage', 'PageIn', 'PageOut', 'VirtualMemory', 'SystemAllocation', 'Suspended')
$script:SmokePlatform = 'Win32'
$script:AsanRuntimeNames = @{ Win32 = 'clang_rt.asan_dynamic-i386.dll'; x64 = 'clang_rt.asan_dynamic-x86_64.dll' }
$script:WorkerPlatforms = @('Win32', 'x64')
$script:WorkerConfigurations = @('Debug', 'Release', 'Test', 'Asan')
$script:WorkerKinds = @('smoke', 'guard', 'perturb', 'laa')
$script:WorkerIndexMax = [Environment]::ProcessorCount

function Get-AsanRuntimeName { return $script:AsanRuntimeNames[$script:SmokePlatform] }

function Get-AsanOptions([string]$caseDir) {
    if ($caseDir.Contains("'")) { throw "ASan log_path cannot be quoted for a path that contains an apostrophe: $caseDir" }
    return "exitcode=66:log_path='$caseDir\asan'"
}

function Get-AsanReport([string]$caseDir, $code) {
    $files = @(Get-ChildItem -LiteralPath $caseDir -Filter 'asan.*' -File -ErrorAction SilentlyContinue)
    if ($files.Count -gt 0) {
        $text = 'ASan report:'
        foreach ($f in $files) { $text += "`r`n" + ((Get-Content -LiteralPath $f.FullName -TotalCount 60) -join "`r`n") }
        return $text
    }
    if ($code -eq 66) { return 'ASan exit 66 without an asan.* report: the ASan runtime failed before main (check ASAN_OPTIONS)' }
    return $null
}

function Get-AsanCanaryRx([string]$label = 'use-after-poison', [string]$tail = '') {
    return "$label on address[^\r\n]*\r?\n\s*(?:READ|WRITE) of size 1 at [^\r\n]*\r?\n\s*#0 [^\r\n]*asan_canary\.cpp$tail"
}

function Get-AsanCanaryState($code, $report, [string]$regex) {
    if ($code -ne 0 -and $null -ne $report -and $report -match $regex) { return 'REPORTED' }
    if ($code -eq 0 -and $null -eq $report) { return 'CLEAN' }
    return 'OTHER'
}

function Find-GameDataRoot {
    $candidate = $null
    if ($env:SWBF2_GAMEDATA) {
        $candidate = $env:SWBF2_GAMEDATA
    }
    else {
        foreach ($view in 'Registry64', 'Registry32') {
            $base = $null
            $sub = $null
            try {
                $base = [Microsoft.Win32.RegistryKey]::OpenBaseKey('LocalMachine', $view)
                $sub = $base.OpenSubKey('SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 6060')
                if ($sub) {
                    $loc = $sub.GetValue('InstallLocation')
                    if ($loc) { $candidate = "$loc\GameData"; break }
                }
            }
            catch {}
            finally {
                if ($sub) { $sub.Close() }
                if ($base) { $base.Close() }
            }
        }
    }
    if ($candidate -and (Test-Path -LiteralPath (Join-Path $candidate 'data\_lvl_pc') -PathType Container)) { return $candidate }
    return $null
}

$script:SmokeRequiredDlls = @{ Win32 = @('Galaxy.dll', 'GalaxyPeer.dll', 'openswbf2-ffmpeg.dll', 'steam_api.dll'); x64 = @('openswbf2-ffmpeg.dll') }

function Get-SmokePrecheck($gameDataRoot, $exeDir, $asan = $false, [ValidateSet('Win32', 'x64')][string]$platform = $script:SmokePlatform) {
    if (-not $gameDataRoot) { return 'no retail GameData found (set SWBF2_GAMEDATA or install Steam App 6060)' }
    if (-not (Test-Path -LiteralPath (Join-Path $exeDir 'BattlefrontII.exe'))) { return "BattlefrontII.exe not found in $exeDir" }
    $dlls = @($script:SmokeRequiredDlls[$platform])
    if ($asan) { $dlls += $script:AsanRuntimeNames[$platform] }
    foreach ($dll in $dlls) {
        if (-not (Test-Path -LiteralPath (Join-Path $exeDir $dll))) { return "$dll missing beside the exe in $exeDir" }
    }
    $data = Get-Item -LiteralPath (Join-Path $exeDir 'data') -Force -ErrorAction SilentlyContinue
    if (-not $data -or ($data.Attributes -band [IO.FileAttributes]::ReparsePoint) -or -not (Test-Path -LiteralPath (Join-Path $exeDir 'data\_lvl_pc\shell.lvl'))) { return "staged data missing in $exeDir (build without SWBF2_NOSTAGE)" }
    return $null
}

function New-StallState {
    return @{ Progress = $null; Seen = $false; CpuMs = $null; ProcCpuMs = $null; SilenceMs = 0.0; SilentMaxMs = 0.0; SilentMaxAt = $null; Stalled = $false; Kill = $false }
}

function Step-StallClock($state, $sample) {
    $s = $state.Clone()
    $cpu = 0.0
    if ($null -ne $sample.CpuMs -and $null -ne $s.CpuMs) { $cpu = [Math]::Min([Math]::Max(0.0, [double]$sample.CpuMs - [double]$s.CpuMs), [double]$sample.IntervalMs) }
    $s.CpuMs = $sample.CpuMs
    $proc = 0.0
    if ($null -ne $sample.ProcCpuMs -and $null -ne $s.ProcCpuMs) { $proc = [Math]::Min([Math]::Max(0.0, [double]$sample.ProcCpuMs - [double]$s.ProcCpuMs), [double]$sample.IntervalMs) }
    $s.ProcCpuMs = $sample.ProcCpuMs
    if ($null -ne $sample.Progress -and $sample.Progress -cne $s.Progress) {
        $s.Progress = $sample.Progress
        $s.Seen = $true
        $s.SilenceMs = 0.0
        return $s
    }
    $add = $cpu
    if ($sample.State -eq 'Wait' -and $script:StallStarvedWaits -notcontains $sample.WaitReason) {
        if ($sample.OtherReady) { $add = $proc }
        else { $add = [double]$sample.IntervalMs }
    }
    $s.SilenceMs += $add
    if ($s.SilenceMs -gt $s.SilentMaxMs) {
        $s.SilentMaxMs = $s.SilenceMs
        $s.SilentMaxAt = $s.Progress
    }
    if ($s.SilenceMs -ge $script:StallSeconds * 1000) {
        if ($s.Stalled -or -not $s.Seen) { $s.Kill = $true }
        $s.Stalled = $true
        $s.SilenceMs = 0.0
    }
    return $s
}

function Read-ProgressBlock([int]$processId, $block) {
    if ($null -eq $block) {
        try { $map = [IO.MemoryMappedFiles.MemoryMappedFile]::OpenExisting("Local\OpenSWBF2Progress.$processId", [IO.MemoryMappedFiles.MemoryMappedFileRights]::ReadWrite) }
        catch [IO.FileNotFoundException] { return $null }
        $block = New-Object PSObject -Property @{ Map = $map; View = $map.CreateViewAccessor(0, 12); Turns = 0; Steps = 0 }
    }
    $block.Turns = $block.View.ReadUInt32(0)
    $block.Steps = $block.View.ReadUInt32(4)
    return $block
}

function Get-SmokeMainThread($p, $threadId) {
    $p.Refresh()
    $best = $null
    $bestStart = $null
    foreach ($t in @($p.Threads)) {
        if ($null -ne $threadId) {
            if ($t.Id -eq $threadId) { return $t }
            continue
        }
        try { $st = $t.StartTime } catch { continue }
        if ($null -eq $best -or $st -lt $bestStart) { $best = $t; $bestStart = $st }
    }
    if ($null -ne $threadId) { return Get-SmokeMainThread $p $null }
    return $best
}

function Wait-SmokeProcess($p) {
    $state = New-StallState
    $block = $null
    $mainId = $null
    $flagged = $false
    $killed = $false
    $clock = [System.Diagnostics.Stopwatch]::StartNew()
    $last = 0.0
    try {
        while (-not $p.WaitForExit($(if ($null -eq $block) { $script:SectionPollMs } else { 1000 }))) {
            if ($null -eq $block) {
                try { $block = Read-ProgressBlock $p.Id $null }
                catch {
                    if ($p.HasExited) { break }
                    throw
                }
                if ($clock.Elapsed.TotalMilliseconds - $last -lt 1000) { continue }
            }
            $sample = @{ Progress = $null; State = 'Unknown'; WaitReason = $null; CpuMs = $null; ProcCpuMs = $null; OtherReady = $false; IntervalMs = 0.0 }
            try {
                $block = Read-ProgressBlock $p.Id $block
                if ($block) { $sample.Progress = "$($block.Turns)/$($block.Steps)" }
                $t = Get-SmokeMainThread $p $mainId
                if ($t) {
                    $mainId = $t.Id
                    $sample.State = [string]$t.ThreadState
                    if ($sample.State -eq 'Wait') { $sample.WaitReason = [string]$t.WaitReason }
                    $sample.CpuMs = $t.TotalProcessorTime.TotalMilliseconds
                    $sample.ProcCpuMs = $p.TotalProcessorTime.TotalMilliseconds
                    foreach ($o in @($p.Threads)) {
                        if ($o.Id -eq $mainId) { continue }
                        try { $os = [string]$o.ThreadState } catch { continue }
                        if ($os -eq 'Ready' -or $os -eq 'Standby') { $sample.OtherReady = $true; break }
                    }
                }
            }
            catch {
                if ($p.HasExited) { break }
                throw
            }
            $now = $clock.Elapsed.TotalMilliseconds
            $sample.IntervalMs = $now - $last
            $last = $now
            $state = Step-StallClock $state $sample
            if ($state.Kill) {
                & taskkill /PID $p.Id /T /F 2>&1 | Out-Null
                $killed = $true
                break
            }
            if ($state.Stalled -and -not $flagged) {
                $block.View.Write(8, [int]1)
                $flagged = $true
            }
        }
        $p.WaitForExit()
        $block = Read-ProgressBlock $p.Id $block
        $turns = $null
        $steps = $null
        if ($block) { $turns = $block.Turns; $steps = $block.Steps }
    }
    finally {
        if ($block) {
            $block.View.Dispose()
            $block.Map.Dispose()
        }
    }
    return New-Object PSObject -Property @{ Stalled = $state.Stalled; Killed = $killed; SilentMaxS = $state.SilentMaxMs / 1000.0; SilentMaxAt = $state.SilentMaxAt; Turns = $turns; Steps = $steps }
}

$script:OutputHeadBytes = 1048576
$script:OutputTailBytes = 1048576

function Initialize-OutputCapture {
    if ('OpenSWBF2OutputCapture' -as [type]) { return }
    Add-Type -TypeDefinition @'
public sealed class OpenSWBF2OutputCapture
{
    readonly System.IO.Stream mStream;
    readonly System.Text.Encoding mEncoding;
    readonly byte[] mHead;
    readonly byte[] mTail;
    readonly System.Threading.Thread mThread;
    int mHeadLength;
    long mTailWritten;
    bool mComplete;

    public OpenSWBF2OutputCapture(System.IO.Stream stream, System.Text.Encoding encoding, int headBytes, int tailBytes)
    {
        mStream = stream;
        mEncoding = encoding;
        mHead = new byte[headBytes];
        mTail = new byte[tailBytes];
        mThread = new System.Threading.Thread(Pump);
        mThread.IsBackground = true;
        mThread.Start();
    }

    void Pump()
    {
        byte[] buf = new byte[65536];
        try
        {
            int n;
            while ((n = mStream.Read(buf, 0, buf.Length)) > 0)
            {
                int at = 0;
                if (mHeadLength < mHead.Length)
                {
                    at = System.Math.Min(n, mHead.Length - mHeadLength);
                    System.Buffer.BlockCopy(buf, 0, mHead, mHeadLength, at);
                    mHeadLength += at;
                }
                while (at < n)
                {
                    int pos = (int)(mTailWritten % mTail.Length);
                    int take = System.Math.Min(n - at, mTail.Length - pos);
                    System.Buffer.BlockCopy(buf, at, mTail, pos, take);
                    mTailWritten += take;
                    at += take;
                }
            }
            mComplete = true;
        }
        catch (System.IO.IOException)
        {
        }
        catch (System.ObjectDisposedException)
        {
        }
    }

    public void Wait()
    {
        mThread.Join();
    }

    public bool Complete
    {
        get { mThread.Join(); return mComplete; }
    }

    public long Omitted
    {
        get { mThread.Join(); return System.Math.Max(0L, mTailWritten - mTail.Length); }
    }

    public string Text
    {
        get
        {
            mThread.Join();
            string head = mEncoding.GetString(mHead, 0, mHeadLength);
            if (mTailWritten <= mTail.Length)
            {
                return head + mEncoding.GetString(mTail, 0, (int)mTailWritten);
            }
            int start = (int)(mTailWritten % mTail.Length);
            byte[] tail = new byte[mTail.Length];
            System.Buffer.BlockCopy(mTail, start, tail, 0, mTail.Length - start);
            System.Buffer.BlockCopy(mTail, 0, tail, mTail.Length - start, start);
            return head + "\r\n[OUTPUT TRUNCATED: " + Omitted + " bytes omitted]\r\n" + mEncoding.GetString(tail);
        }
    }
}
'@
}

function New-OutputCapture($stream, $encoding) {
    Initialize-OutputCapture
    return [OpenSWBF2OutputCapture]::new($stream, $encoding, $script:OutputHeadBytes, $script:OutputTailBytes)
}

function Invoke-T1Exe([string]$exe, [string]$workDir, [string]$argLine, [string]$asanOptions) {
    $psi = New-Object System.Diagnostics.ProcessStartInfo
    $psi.FileName = $exe
    $psi.Arguments = $argLine
    $psi.WorkingDirectory = $workDir
    $psi.UseShellExecute = $false
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true
    $psi.CreateNoWindow = $true
    if ($asanOptions) { $psi.EnvironmentVariables['ASAN_OPTIONS'] = $asanOptions }
    Initialize-OutputCapture
    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $p = [System.Diagnostics.Process]::Start($psi)
    $o = New-OutputCapture $p.StandardOutput.BaseStream $p.StandardOutput.CurrentEncoding
    $e = New-OutputCapture $p.StandardError.BaseStream $p.StandardError.CurrentEncoding
    $wait = Wait-SmokeProcess $p
    $sw.Stop()
    $out = $o.Text
    $code = [BitConverter]::ToUInt32([BitConverter]::GetBytes([int]$p.ExitCode), 0)
    return New-Object PSObject -Property @{ Code = $code; Out = $out; Text = $out + $e.Text; Omitted = $o.Omitted + $e.Omitted; Stalled = $wait.Stalled; Killed = $wait.Killed; SilentMaxS = $wait.SilentMaxS; Ms = $sw.ElapsedMilliseconds }
}

function Get-T1Token($res) {
    if ($res.Killed -or ($res.Stalled -and $res.Code -eq 124)) {
        $m = [regex]::Match([string]$res.Text, '\[STALL\] ([^\r\n]*)')
        $detail = 'no answer to the stall flag'
        if ($m.Success) { $detail = $m.Groups[1].Value.TrimEnd() }
        return New-Object PSObject -Property @{ Token = 'STALL'; Detail = $detail }
    }
    $token = 'CRASH'
    $detail = ('0x{0:X8}' -f $res.Code)
    if ($res.Code -eq 0) {
        $detail = ''
        if ($res.Text -match 'test cases:\s+1 \|\s+1 passed') { $token = 'PASS' }
        else { $token = 'FAIL'; $detail = 'case did not run exactly once' }
    }
    elseif ($res.Code -eq 1) { $token = 'FAIL'; $detail = '' }
    elseif ($res.Code -eq 77) { $token = 'SKIP'; $detail = '' }
    return New-Object PSObject -Property @{ Token = $token; Detail = $detail }
}

function Get-GuardT1State($res, [bool]$isProbe) {
    if ((Get-T1Token $res).Token -eq 'STALL') { return New-Object PSObject -Property @{ State = 'RED'; Detail = 'stall'; Code = 124 } }
    $state = 'RED'
    $detail = ''
    if ($isProbe) {
        if ($res.Code -eq 1 -and $res.Text -match '\[UNEXPECTED-LOG\]') { $state = 'GREEN' }
    }
    elseif ($res.Code -eq 0) {
        if ($res.Text -match 'test cases:\s+1 \|\s+1 passed') { $state = 'GREEN' }
        else { $state = 'NO-MATCH' }
    }
    elseif ($res.Code -eq 77) { $state = 'SKIP' }
    elseif ($res.Code -eq 2) { $state = 'NO-MATCH' }
    elseif ($res.Code -ne 1) { $detail = ('crash 0x{0:X8}' -f $res.Code) }
    elseif ([string]$res.Text -match 'reference child: ') { $state = 'CHILD-FAIL'; $detail = 'reference child' }
    return New-Object PSObject -Property @{ State = $state; Detail = $detail; Code = $res.Code }
}

function New-StampedDir([string]$Parent) {
    [void][IO.Directory]::CreateDirectory($Parent)
    $m = New-Object System.Threading.Mutex($false, 'Local\OpenSWBF2-stamped-dir')
    try { [void]$m.WaitOne() }
    catch [System.Threading.AbandonedMutexException] {}
    try {
        $base = Get-Date -Format 'yyyyMMdd-HHmmss'
        $name = $base
        $i = 1
        while ([IO.Directory]::Exists((Join-Path $Parent $name))) { $i++; $name = "$base-$i" }
        $dir = Join-Path $Parent $name
        [void][IO.Directory]::CreateDirectory($dir)
        return $dir
    }
    finally {
        $m.ReleaseMutex()
        $m.Dispose()
    }
}

function Get-WorkerPath([string]$Root, [string]$Platform, [string]$Configuration, [string]$Kind, [int]$Index) {
    if (-not $Root) { throw 'worker path needs a root' }
    if ($script:WorkerPlatforms -cnotcontains $Platform) { throw "worker platform '$Platform' is not one of $($script:WorkerPlatforms -join ', ')" }
    if ($script:WorkerConfigurations -cnotcontains $Configuration) { throw "worker configuration '$Configuration' is not one of $($script:WorkerConfigurations -join ', ')" }
    if ($script:WorkerKinds -cnotcontains $Kind) { throw "worker kind '$Kind' is not one of $($script:WorkerKinds -join ', ')" }
    if ($Index -lt 1 -or $Index -gt $script:WorkerIndexMax) { throw "worker index $Index is outside 1..$($script:WorkerIndexMax)" }
    return Join-Path $Root "artifacts\smoke-workers\$Platform\$Configuration\$Kind\w$Index"
}

function Get-WorkerPaths([string]$Root, [string]$Platform, [string]$Configuration, [string]$Kind, [int]$Base, [int]$Count) {
    if ($Base -lt 0 -or $Count -lt 1 -or $Base + $Count -gt $script:WorkerIndexMax) { throw "workers $($Base + 1)..$($Base + $Count) are outside 1..$($script:WorkerIndexMax)" }
    return @(1..$Count | ForEach-Object { Get-WorkerPath $Root $Platform $Configuration $Kind ($Base + $_) })
}

function Get-ReparsePoints([string]$dir) {
    $found = New-Object System.Collections.Generic.List[string]
    $top = New-Object System.IO.DirectoryInfo $dir
    if (-not $top.Exists) { return }
    if (($top.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { return $top.FullName }
    $pending = New-Object System.Collections.Generic.Stack[System.IO.DirectoryInfo]
    $pending.Push($top)
    while ($pending.Count -gt 0) {
        $cur = $pending.Pop()
        foreach ($e in $cur.EnumerateFileSystemInfos()) {
            if (($e.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { $found.Add($e.FullName) }
            elseif (($e.Attributes -band [IO.FileAttributes]::Directory) -ne 0) { $pending.Push((New-Object System.IO.DirectoryInfo $e.FullName)) }
        }
    }
    return $found.ToArray()
}
