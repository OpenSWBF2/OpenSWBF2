param([string]$Configuration = 'Debug', [ValidateSet('Win32', 'x64')][string]$Platform = 'Win32')
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'hardening.ps1')
$script:SmokePlatform = $Platform
$tmp = Join-Path $env:TEMP ('openswbf2-hardening-tests-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $tmp | Out-Null
$failed = 0
$n = 0
try {
    $goodHdr = "    Characteristics`n         102 characteristics`n    DLL characteristics`n            8140 DLL characteristics`n                   Dynamic base`n                   NX compatible`n                   Application can handle large (>2GB) addresses`n   1C13000 [   38D38] RVA [size] of Base Relocation Directory`n  .reloc name`n"
    $cookieCfg = "            0085C930 Security Cookie`n"
    $goodCfg = "$cookieCfg            008327F0 Safe Exception Handler Table`n                 7CF Safe Exception Handler Count`n"
    $oldHdr = "    Characteristics`n         103 characteristics`n                   Relocations stripped`n                   NX compatible`n         0 [       0] RVA [size] of Base Relocation Directory`n"
    $hardScenarios = @(
        @('hardening good text passes all', $goodHdr, $goodCfg, 736, 0, 736, @('laa', 'dynamicbase', 'nxcompat', 'reloc', 'safeseh', 'gs', 'sdl'), @()),
        @('hardening old exe fails dynamicbase and reloc', $oldHdr, $goodCfg, 736, 0, 736, @('nxcompat', 'safeseh', 'gs', 'sdl'), @('dynamicbase', 'reloc', 'laa')),
        @('hardening safeseh count 0 fails', $goodHdr, "$cookieCfg                 0 Safe Exception Handler Count`n", 736, 0, 736, @('laa', 'dynamicbase', 'nxcompat', 'reloc', 'gs', 'sdl'), @('safeseh')),
        @('hardening missing security cookie fails gs', $goodHdr, "                 7CF Safe Exception Handler Count`n", 736, 0, 736, @('laa', 'dynamicbase', 'nxcompat', 'reloc', 'safeseh', 'sdl'), @('gs')),
        @('hardening /GS- entries fail gs', $goodHdr, $goodCfg, 736, 3, 736, @('laa', 'dynamicbase', 'nxcompat', 'reloc', 'safeseh', 'sdl'), @('gs')),
        @('hardening empty tlog fails gs and sdl', $goodHdr, $goodCfg, 0, 0, 0, @('laa', 'reloc', 'safeseh'), @('gs', 'sdl')),
        @('hardening missing /sdl entry fails sdl', $goodHdr, $goodCfg, 736, 0, 735, @('laa', 'dynamicbase', 'nxcompat', 'reloc', 'safeseh', 'gs'), @('sdl')),
        @('hardening non-LAA header fails laa', $goodHdr.Replace('Application can handle large (>2GB) addresses', 'Something else'), $goodCfg, 736, 0, 736, @('dynamicbase', 'nxcompat', 'reloc', 'safeseh', 'gs', 'sdl'), @('laa'))
    )
    foreach ($sc in $hardScenarios) {
        $n++
        $got = Get-HardeningChecks $sc[1] $sc[2] $sc[3] $sc[4] $sc[5]
        $why = $null
        foreach ($name in $sc[6]) { if (-not ($got | Where-Object { $_.Name -eq $name }).Ok) { $why = "$name should pass" } }
        foreach ($name in $sc[7]) { if (($got | Where-Object { $_.Name -eq $name }).Ok) { $why = "$name should fail" } }
        if ($why) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $why" }
        else { Write-Host "OK $n $($sc[0])" }
    }
    function New-FlagBag($total, $an, $il, $io, $extra) {
        $p = @{ Total = $total; GsMinus = 0; Sdl = $total; AlignedNewOff = $an; InlineOff = $il; InlineOn = $io; ThrowingNew = 0; Gl = 0; FpFast = 0; FpContract = 0; We4311 = $total; We4312 = $total; We4302 = $total; We4477 = $total; WeAll = $total; WeMissing = @() }
        if ($extra) { foreach ($k in $extra.Keys) { $p[$k] = $extra[$k] } }
        return New-Object PSObject -Property $p
    }
    $allFlags = @('flag-alignednew', 'flag-inline', 'flag-alignednew-tests', 'flag-banned')
    $flagScenarios = @(
        @('flags all good pass', (New-FlagBag 748 748 0 748 $null), (New-FlagBag 97 97 0 0 $null), $allFlags, @()),
        @('flags alignednew 747/748 fails', (New-FlagBag 748 747 0 748 $null), (New-FlagBag 97 97 0 0 $null), @('flag-inline', 'flag-alignednew-tests', 'flag-banned'), @('flag-alignednew')),
        @('flags one /Zc:inline- fails', (New-FlagBag 748 748 1 748 $null), (New-FlagBag 97 97 0 0 $null), @('flag-alignednew', 'flag-alignednew-tests', 'flag-banned'), @('flag-inline')),
        @('flags missing /Zc:inline fails', (New-FlagBag 748 748 0 747 $null), (New-FlagBag 97 97 0 0 $null), @('flag-alignednew', 'flag-alignednew-tests', 'flag-banned'), @('flag-inline')),
        @('flags tests missing alignednew fails', (New-FlagBag 748 748 0 748 $null), (New-FlagBag 97 0 0 0 $null), @('flag-alignednew', 'flag-inline', 'flag-banned'), @('flag-alignednew-tests')),
        @('flags one /GL fails banned', (New-FlagBag 748 748 0 748 @{ Gl = 1 }), (New-FlagBag 97 97 0 0 $null), @('flag-alignednew', 'flag-inline', 'flag-alignednew-tests'), @('flag-banned')),
        @('flags one tests /fp:fast fails banned', (New-FlagBag 748 748 0 748 $null), (New-FlagBag 97 97 0 0 @{ FpFast = 1 }), @('flag-alignednew', 'flag-inline', 'flag-alignednew-tests'), @('flag-banned')),
        @('flags empty game tlog fails', (New-FlagBag 0 0 0 0 $null), $null, @('flag-banned'), @('flag-alignednew', 'flag-inline'))
    )
    foreach ($sc in $flagScenarios) {
        $n++
        $got = @(Get-FlagChecks $sc[1] $sc[2])
        $why = $null
        foreach ($name in $sc[3]) { if (-not ($got | Where-Object { $_.Name -eq $name }).Ok) { $why = "$name should pass" } }
        foreach ($name in $sc[4]) { if (($got | Where-Object { $_.Name -eq $name }).Ok) { $why = "$name should fail" } }
        if (-not $sc[2] -and ($got | Where-Object { $_.Name -eq 'flag-alignednew-tests' })) { $why = 'flag-alignednew-tests must be omitted without a tests tlog' }
        if ($why) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $why" }
        else { Write-Host "OK $n $($sc[0])" }
    }
    $n++
    $tlogDir = Join-Path $env:TEMP ("openswbf2-tlog-" + [guid]::NewGuid().ToString('N'))
    try {
        New-Item -ItemType Directory -Path $tlogDir | Out-Null
        Set-Content -Encoding Unicode -Path (Join-Path $tlogDir 'CL.command.1.tlog') -Value @(
            '^C:\X\A.CPP',
            '/c /IC:\X\GL\INC /Zc:alignedNew- /Zc:inline- /sdl /GS C:\X\A.CPP',
            '^C:\X\B.CPP',
            '/c /IC:\X\GL\INC /Zc:alignedNew- /Zc:inline /sdl /GS /Fo"C:\X\GL\B.OBJ" C:\X\B.CPP'
        )
        $tc = Get-ClTlogCounts $tlogDir
        $why = $null
        if ($tc.Total -ne 2) { $why = "Total=$($tc.Total)" }
        elseif ($tc.AlignedNewOff -ne 2) { $why = "AlignedNewOff=$($tc.AlignedNewOff)" }
        elseif ($tc.InlineOff -ne 1) { $why = "InlineOff=$($tc.InlineOff)" }
        elseif ($tc.InlineOn -ne 1) { $why = "InlineOn=$($tc.InlineOn)" }
        elseif ($tc.Gl -ne 0) { $why = "Gl=$($tc.Gl) matched a path" }
        elseif ($tc.Sdl -ne 2 -or $tc.GsMinus -ne 0) { $why = "Sdl=$($tc.Sdl) GsMinus=$($tc.GsMinus)" }
        if ($why) { $failed++; Write-Host "FAILED ${n}: UTF-16 tlog flag counts: $why" }
        else { Write-Host "OK $n UTF-16 tlog flag counts" }
    }
    finally {
        Remove-Item -Recurse -Force $tlogDir -ErrorAction SilentlyContinue
    }
    $n++
    $spacerDir = Join-Path $env:TEMP ("openswbf2-spacer-" + [guid]::NewGuid().ToString('N'))
    try {
        New-Item -ItemType Directory -Path (Join-Path $spacerDir 'src') | Out-Null
        New-Item -ItemType Directory -Path (Join-Path $spacerDir 'tests') | Out-Null
        Set-Content -Encoding ASCII -Path (Join-Path $spacerDir 'src\x.h') -Value @(
            'unsigned char acAllowed[0x10];',
            'unsigned char acAllowed[0x10];',
            'char mGap[0x20];',
            'char mPad[12];',
            'char acName[64];',
            'char mRun[16 + 0x4];',
            'unsigned char m[2][0x10];',
            'static const unsigned char kTable[0x4] = { 1, 2, 3, 4 };',
            'mutable char mBuf[0x8];',
            'alignas(4) unsigned char mAligned[0x8];',
            'BYTE mByte[0x8];',
            'uint8_t mU8[0x8];',
            'signed char mS[0x8];',
            'unsigned __int8 mI8[0x8];',
            'char a[4], pad2[0x10];',
            'char mTail[sizeof(int) - 1];',
            '// char mComment[0x10];',
            'char* apNames[0x10];',
            'struct S { bool b; volatile char mVol[0x2]; };',
            'char acSized[0x10];'
        )
        $allow = @('src\x.h|acAllowed|[0x10]', 'src\x.h|acSized|[16]', 'src\x.h|mMissing|[0x4]')
        $sc = @(Get-SourceChecks $spacerDir $allow | Where-Object { $_.Name -eq 'char-spacer' })
        $want = ((@(2, 3, 4, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 19, 20) | ForEach-Object { "src\x.h:$_" }) + 'unused allow src\x.h|acSized|[16]' + 'unused allow src\x.h|mMissing|[0x4]') -join ', '
        if ($sc.Count -ne 1 -or $sc[0].Ok -or $sc[0].Detail -ne $want) { $failed++; Write-Host "FAILED ${n}: char-spacer flags every spacer form outside the exact allow-list: $($sc.Detail)" }
        else { Write-Host "OK $n char-spacer flags every spacer form outside the exact allow-list" }
    }
    finally {
        Remove-Item -Recurse -Force $spacerDir -ErrorAction SilentlyContinue
    }
    $n++
    $offsetDir = Join-Path $env:TEMP ("openswbf2-offset-" + [guid]::NewGuid().ToString('N'))
    try {
        New-Item -ItemType Directory -Path (Join-Path $offsetDir 'src') | Out-Null
        New-Item -ItemType Directory -Path (Join-Path $offsetDir 'tests') | Out-Null
        Set-Content -Encoding ASCII -Path (Join-Path $offsetDir 'src\x.cpp') -Value @(
            'p = (Foo*)((char*)pObj + 0x10);',
            'x = *(int*)((char*)this + 0x1c);',
            'q = (const Bar*)((unsigned char*)pObj->mRec - 8);',
            'a = (const sockaddr*)(pPeer + 1);',
            'r = reinterpret_cast<Baz*>(pRec + 2);',
            's = reinterpret_cast<int*>(reinterpret_cast<char*>(pObj) + 4);',
            'Read((char*)mBitmap + 1, n);',
            't = (Node*)((char*)pLink - offsetof(Node, mLink));',
            'u = (Node*)((char*)pLink - (int)offsetof(Node, mLink));',
            'v = (int*)(pMe->mCtrl);',
            'w = (char*)pA - (char*)pB;',
            '// c = (Foo*)((char*)pObj + 0x10);',
            'y = (Foo*)((char*)pAllowed + 4);',
            'y = (Foo*)((char*)pAllowed + 4);',
            'z = (Foo*)( (char*)pSpaced  +  4 );',
            'k = (float)(fA + fB);',
            'h = (char*)p + iSize;',
            'n = (IMAGE_NT_HEADERS*)((char*)pBase + pDos->e_lfanew);',
            'm = (IMAGE_NT_HEADERS*)((char*)pBase + 0x3c);',
            'g = ((Damageable*)((char*)pUnit->GetGameObject() + 320))->Vanish();',
            'e = (Node*)(pList->Head(1) + 2);',
            'c = (char*)pRec->Buf()[0] + 4;',
            'b = Get(a)->Value() + 2;'
        )
        $allow = @('src\x.cpp|(Foo*)((char*)pAllowed + 4)', 'src\x.cpp|(Foo*)( (char*)pSpaced + 4 )', 'src\x.cpp|(Foo*)((char*)pMissing + 4)', 'src\y.cpp|(Foo*)((char*)pAllowed + 4)')
        $sc = @(Get-SourceChecks $offsetDir @() $allow | Where-Object { $_.Name -eq 'offset-hack' })
        $want = ((@(1, 2, 3, 4, 5, 6, 7, 14, 19, 20, 21, 22) | ForEach-Object { "src\x.cpp:$_" }) + 'unused allow src\x.cpp|(Foo*)((char*)pMissing + 4)' + 'unused allow src\y.cpp|(Foo*)((char*)pAllowed + 4)') -join ', '
        if ($sc.Count -ne 1 -or $sc[0].Ok -or $sc[0].Detail -ne $want) { $failed++; Write-Host "FAILED ${n}: offset-hack flags every byte-offset form outside the exact allow-list: $($sc.Detail)" }
        else { Write-Host "OK $n offset-hack flags every byte-offset form outside the exact allow-list" }
    }
    finally {
        Remove-Item -Recurse -Force $offsetDir -ErrorAction SilentlyContinue
    }
    $n++
    $sizeDir = Join-Path $env:TEMP ("openswbf2-sizelit-" + [guid]::NewGuid().ToString('N'))
    try {
        New-Item -ItemType Directory -Path (Join-Path $sizeDir 'src\a'), (Join-Path $sizeDir 'src\LuaFL') | Out-Null
        Set-Content -Encoding ASCII -Path (Join-Path $sizeDir 'src\a\x.cpp') -Value @(
            'MemoryPool Foo::sMemoryPool("Foo", 0x30);',
            'MemoryPool Bar::sMemoryPool("Bar", sizeof(Bar));',
            'MemoryPool Baz::sMemoryPool("Baz", 0);',
            'FreeList Foo::smFreeList(0xcc, 1, true, 4);',
            'FreeList Bar::smFreeList(sizeof(Bar), 1, true, 4);',
            'gPool.Setup(uCount, 0xFD0, 0);',
            'Agent::sMemoryPool.Setup(600, 0x358, Agent::sMemoryPool.GetGrow());',
            'gPool.Setup(Count(), sizeof(Foo), 0);',
            'pPool->Create(2, 16, 0);',
            'mLock.Create(1, 1);',
            'Foo::SetupMemoryPool(16, 0x40, Foo::sMemoryPool.GetGrow());',
            'mValue.Setup(1.0f, 2.0f, 3.0f);',
            '// gPool.Setup(uCount, 0xFD0, 0);',
            'MemoryPool kAllowed("allowed", 16);',
            'Weapon::UpdateMemoryPoolSize(0x150);',
            'Weapon::UpdateMemoryPoolSize(sizeof(Weapon));',
            'p = RedAllocFromHeap(h, n * 4, 0);',
            'p = RedAllocFromHeap(h, n * sizeof(T*), 0);',
            'p = (T**)RedAllocFromHeap(',
            '    h, (unsigned int)n + 4, 0);',
            'p = RedAllocFromHeap(h, n * 4 * sizeof(T), 0);',
            'pPool->Create(c, uBin * 4, 0);',
            'p = RedAllocFromHeap(h, n * 40, 0);',
            'pLua->Create(c, uLua * 4, 0);',
            'p = RedAllocFromHeap(h, 4 * n, 0);',
            'p = RedAllocFromHeap(h, 40 * n, 0);',
            'const unsigned int uiBytes = n * 4;',
            'q = RedAllocFromHeap(h, uiBytes, 0);',
            'const unsigned int uiLead = 4 * n;',
            'Foo::SetupMemoryPool(16, uiLead, Foo::sMemoryPool.GetGrow());',
            'const unsigned int uiOk = n * sizeof(T*);',
            'r = RedAllocFromHeap(h, uiOk, 0);',
            'bool bSame = uiOk == n * 4;',
            'r = RedAllocFromHeap(h, bSame, 0);'
        )
        Set-Content -Encoding ASCII -Path (Join-Path $sizeDir 'src\LuaFL\l.cpp') -Value 'MemoryPool Vendor::sMemoryPool("Vendor", 8);'
        $allow = @('src\a\x.cpp|MemoryPool kAllowed("allowed", 16)|x64:safe:raw blocks', 'src\a\x.cpp|->Create(c, uLua * 4, 0)|x64:safe:4-byte bins', 'src\a\x.cpp|MemoryPool kMissing("missing", 16)|x64:safe:raw blocks', 'src\a\x.cpp|.Create(9, 9, 9)')
        $c = Get-X64SizeLiteralCheck $sizeDir $allow
        $want = (@('allow without an x64:safe reason src\a\x.cpp|.Create(9, 9, 9)') + (@(1, 4, 6, 7, 9, 22, 11, 15, 17, 19, 25, 30, 28) | ForEach-Object { "src\a\x.cpp:$_" }) + 'unused allow src\a\x.cpp|MemoryPool kMissing("missing", 16)') -join ', '
        $why = $null
        if ($c.Name -ne 'x64-size-literal' -or $c.Ok -or $c.Detail -ne $want) { $why = "ok=$($c.Ok): $($c.Detail)" }
        Set-Content -Encoding ASCII -Path (Join-Path $sizeDir 'src\a\x.cpp') -Value @('MemoryPool Bar::sMemoryPool("Bar", sizeof(Bar));', 'gPool.Setup(uCount, sizeof(Foo), 0);')
        $clean = Get-X64SizeLiteralCheck $sizeDir @()
        if (-not $why -and (-not $clean.Ok -or $clean.Detail)) { $why = "clean tree: ok=$($clean.Ok): $($clean.Detail)" }
        if ($why) { $failed++; Write-Host "FAILED ${n}: x64-size-literal flags every literal pool size outside the exact allow-list: $why" }
        else { Write-Host "OK $n x64-size-literal flags every literal pool size outside the exact allow-list" }
    }
    finally {
        Remove-Item -Recurse -Force $sizeDir -ErrorAction SilentlyContinue
    }
    $n++
    $verdictDir = Join-Path $env:TEMP ("openswbf2-verdict-" + [guid]::NewGuid().ToString('N'))
    try {
        New-Item -ItemType Directory -Path (Join-Path $verdictDir 'src') | Out-Null
        New-Item -ItemType Directory -Path (Join-Path $verdictDir 'tests') | Out-Null
        Set-Content -Encoding ASCII -Path (Join-Path $verdictDir 'src\x.cpp') -Value @(
            'unsigned char acSafe[0x10];',
            'unsigned char acBare[0x10];',
            'char mOr[0x10 | 0x1];',
            'q = (Foo*)((char*)pA + (iA | iB));'
        )
        $bad = @(Get-SourceChecks $verdictDir @('src\x.cpp|acSafe|[0x10]|x64:safe:byte buffer', 'src\x.cpp|acBare|[0x10]', 'src\x.cpp|mOr|[0x10|0x1]|x64:unsafe:bits') @('src\x.cpp|(Foo*)((char*)pA + (iA | iB))|x64:fixed:', 'src\x.cpp|(Foo*)((char*)pA + 4)|x64:safe: ') -Only 'allow-x64-verdict')
        $wantBad = 'no verdict src\x.cpp|acBare|[0x10], no verdict src\x.cpp|mOr|[0x10|0x1]|x64:unsafe:bits, no verdict src\x.cpp|(Foo*)((char*)pA + (iA | iB))|x64:fixed:, no verdict src\x.cpp|(Foo*)((char*)pA + 4)|x64:safe: '
        $good = @(Get-SourceChecks $verdictDir @('src\x.cpp|acSafe|[0x10]|x64:safe:byte buffer', 'src\x.cpp|acBare|[0x10]|x64:fixed:sized by sizeof', 'src\x.cpp|mOr|[0x10|0x1]|x64:safe:bit mask') @('src\x.cpp|(Foo*)((char*)pA + (iA | iB))|x64:fixed:offset in bytes') -Only @('allow-x64-verdict', 'char-spacer', 'offset-hack'))
        $why = $null
        if ($bad.Count -ne 1 -or $bad[0].Ok -or $bad[0].Detail -cne $wantBad) { $why = "missing, malformed and empty verdicts: $($bad.Detail)" }
        elseif ($good.Count -ne 3 -or @($good | Where-Object { -not $_.Ok }).Count) { $why = "all verdicts, keys with an inner |: $(@($good | ForEach-Object { '{0}={1} {2}' -f $_.Name, $_.Ok, $_.Detail }) -join '; ')" }
        if ($why) { $failed++; Write-Host "FAILED ${n}: allow-x64-verdict: $why" }
        else { Write-Host "OK $n allow-x64-verdict fails a missing, malformed or empty verdict, passes when every entry has one, and keeps a key with an inner | whole" }
    }
    finally {
        Remove-Item -Recurse -Force $verdictDir -ErrorAction SilentlyContinue
    }
    $n++
    $serialDir = Join-Path $env:TEMP ("openswbf2-serial-" + [guid]::NewGuid().ToString('N'))
    try {
        New-Item -ItemType Directory -Path (Join-Path $serialDir 'src\LuaFL') | Out-Null
        New-Item -ItemType Directory -Path (Join-Path $serialDir 'tests') | Out-Null
        Set-Content -Encoding ASCII -Path (Join-Path $serialDir 'src\x.cpp') -Value @(
            'static int sThreadActivationKey = 0;',
            'int a = sThreadActivationKey;',
            'unsigned b = pObj->mCollisionSerial;',
            'x = sCollisionSerialNext + 1;',
            'k = NextThreadActivationKey();',
            'int mCollisionSerialTies = 0;',
            'q = pA->mCollisionSerial == pB->mCollisionSerial;'
        )
        Set-Content -Encoding ASCII -Path (Join-Path $serialDir 'src\LuaFL\l.cpp') -Value @(
            'h = sCollisionSerialNext;'
        )
        $allow = @('src\x.cpp|static int sThreadActivationKey = 0;', 'src\x.cpp|int stale = sThreadActivationKey;')
        $sc = @(Get-SourceChecks $serialDir @() @() @() $allow | Where-Object { $_.Name -eq 'serial-value' })
        $want = ((@('src\LuaFL\l.cpp:1') + (@(2, 3, 4, 7) | ForEach-Object { "src\x.cpp:$_" })) + 'unused allow src\x.cpp|int stale = sThreadActivationKey;') -join ', '
        if ($sc.Count -ne 1 -or $sc[0].Ok -or $sc[0].Detail -ne $want) { $failed++; Write-Host "FAILED ${n}: serial-value flags every serial read outside the exact allow-list: $($sc.Detail)" }
        else { Write-Host "OK $n serial-value flags every serial read outside the exact allow-list" }
    }
    finally {
        Remove-Item -Recurse -Force $serialDir -ErrorAction SilentlyContinue
    }
    $n++
    $handleDir = Join-Path $env:TEMP ("openswbf2-handle-" + [guid]::NewGuid().ToString('N'))
    try {
        New-Item -ItemType Directory -Path (Join-Path $handleDir 'src\LuaFL') | Out-Null
        New-Item -ItemType Directory -Path (Join-Path $handleDir 'src\PebbleFL\Common') | Out-Null
        Set-Content -Encoding ASCII -Path (Join-Path $handleDir 'src\x.cpp') -Value @(
            'int a = PBL_HANDLE_ID(p);',
            'return a ? b - 1 : 0xdddddddd;',
            'h.mSavedHandleId = 0;',
            'return (float)(pOwner->PblHandled::GetHandleId() & 0xf);',
            'if ((unsigned int)pOld != 0xDDDDDDDDu)',
            'if (object->GetHandleId() == (unsigned int)debugId)',
            'if (t->GetTracker() != (Tracker*)0xdddddddd)',
            'if (state->handle.GetHandleId() == netHostTraceHandleId)',
            'if (ptr->GetHandleId() != id)',
            'h = p;',
            'ok = d.mGameObject->GetHandleId() == d.mGameObjectId;',
            'x = GameObjectHandleId(p);',
            'if (id != ptr->GetHandleId())',
            'if (p->GetHandleId() == uSaved)',
            'if (p->GetHandleId() != h.GetHandleId())',
            'if (p->GetHandleId() == saved.id_)',
            'if (o->GetGameObject() && o->GetGameObject()->PblHandled::GetHandleId())',
            'p = reinterpret_cast<Tracker*>(0xdddddddd);',
            'if (static_cast<unsigned int>(v) == 0xDDDDDDDD)',
            'if (v != 0xDDDDDDDDUL)',
            'if (p->GetHandleId())',
            'if (p->GetHandleId() == 0)',
            'if (object->GetHandleId() == 110)',
            'if (!mSkip && !(PblHandled::GetHandleId() & 1) == even)',
            'if (GetIndex(i, 1, s->PblHandled::GetHandleId()) && ok)',
            'if ((int)v.GetHandleId() % 30 != b)',
            'if (pObject->GetHandleId() == (unsigned int)debugWriteHandleId)',
            'if (!map.FindId(pObject->GetHandleId()))'
        )
        Set-Content -Encoding ASCII -Path (Join-Path $handleDir 'src\PebbleFL\Common\PblHandle.h') -Value @(
            'unsigned int mHandleId;'
        )
        Set-Content -Encoding ASCII -Path (Join-Path $handleDir 'src\LuaFL\l.cpp') -Value @(
            'int a = PBL_HANDLE_ID(p);'
        )
        $sc = @(Get-SourceChecks $handleDir -Only 'handle-raw')
        $want = (@(1, 3, 5, 7, 9, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22) | ForEach-Object { "src\x.cpp:$_" }) -join ', '
        if ($sc.Count -ne 1 -or $sc[0].Ok -or $sc[0].Detail -ne $want) { $failed++; Write-Host "FAILED ${n}: handle-raw flags raw handle reads and freed-fill compares outside PblHandle: $($sc.Detail)" }
        else { Write-Host "OK $n handle-raw flags raw handle reads and freed-fill compares outside PblHandle" }
    }
    finally {
        Remove-Item -Recurse -Force $handleDir -ErrorAction SilentlyContinue
    }
    $miGood = '/c /ISRC\MIMALLOC\INCLUDE /D MI_PAGE_CACHE_OFFSET=0 /D MI_DEBUG=2 /D MI_SECURE=0 /D MI_NO_GETENV=1 /D MI_USE_RTLGENRANDOM /GS C:\X\SRC\MIMALLOC\SRC\STATIC.C'
    $miRelease = $miGood.Replace('MI_DEBUG=2 /D MI_SECURE=0', 'MI_DEBUG=0 /D MI_SECURE=4')
    $miScenarios = @(
        @('mimalloc-defines good static.c line passes', $miGood, 'Debug', $true, $true),
        @('mimalloc-defines quoted asan line passes', '/c /fsanitize=address /D "MI_PAGE_CACHE_OFFSET=0" /D "MI_DEBUG=0" /D "MI_SECURE=0" /D "MI_TRACK_ASAN=1" /D "MI_NO_GETENV=1" /D "MI_USE_RTLGENRANDOM" C:\X\SRC\MIMALLOC\SRC\STATIC.C', 'Asan', $true, $true),
        @('mimalloc-defines missing MI_NO_GETENV fails', $miGood.Replace(' /D MI_NO_GETENV=1', ''), 'Debug', $false, $true),
        @('mimalloc-defines missing MI_PAGE_CACHE_OFFSET=0 fails', $miGood.Replace(' /D MI_PAGE_CACHE_OFFSET=0', ''), 'Debug', $false, $true),
        @('mimalloc-defines MI_PAGE_CACHE_OFFSET=1 fails', $miGood.Replace('MI_PAGE_CACHE_OFFSET=0', 'MI_PAGE_CACHE_OFFSET=1'), 'Debug', $false, $true),
        @('mimalloc-defines asan without MI_TRACK_ASAN fails', $miGood.Replace('/c ', '/c /fsanitize=address '), 'Debug', $false, $true),
        @('mimalloc-defines MI_TRACK_ASAN without asan fails', $miGood.Replace('/c ', '/c /D MI_TRACK_ASAN=1 '), 'Debug', $false, $true),
        @('mimalloc-defines release line passes', $miRelease, 'Release', $true, $true),
        @('mimalloc-defines release with MI_SECURE=0 fails', $miRelease.Replace('MI_SECURE=4', 'MI_SECURE=0'), 'Release', $false, $true),
        @('mimalloc-defines debug values under release fail', $miGood, 'Release', $false, $true),
        @('mimalloc-defines without tests tlog checks the game only', $miGood, 'Debug', $true, $false)
    )
    foreach ($sc in $miScenarios) {
        $n++
        $miDir = Join-Path $env:TEMP ("openswbf2-midef-" + [guid]::NewGuid().ToString('N'))
        try {
            New-Item -ItemType Directory -Path $miDir | Out-Null
            Set-Content -Encoding Unicode -Path (Join-Path $miDir 'CL.command.1.tlog') -Value @(
                '^C:\X\A.CPP',
                '/c /Zc:alignedNew- /Zc:inline /sdl /GS C:\X\A.CPP',
                '^C:\X\SRC\MIMALLOC\SRC\STATIC.C',
                $sc[1]
            )
            $bag = Get-ClTlogCounts $miDir
            $testsBag = $null
            if ($sc[4]) { $testsBag = $bag }
            $got = @(Get-FlagChecks $bag $testsBag $sc[2] $sc[2] | Where-Object { $_.Name -eq 'mimalloc-defines' })
            if ($got.Count -ne 1 -or $got[0].Ok -ne $sc[3]) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $($got.Detail)" }
            else { Write-Host "OK $n $($sc[0])" }
        }
        finally {
            Remove-Item -Recurse -Force $miDir -ErrorAction SilentlyContinue
        }
    }
    $libGood = '/c /Zc:alignedNew- /Zc:inline /sdl /GS C:\X\LIB\B.CPP'
    $libScenarios = @(
        @('library tlog all good passes', $libGood, @()),
        @('library tlog /Zc:inline- fails flag-inline', $libGood.Replace('/Zc:inline ', '/Zc:inline- '), @('flag-inline')),
        @('library tlog /GS- fails flag-banned and gs', $libGood.Replace('/GS ', '/GS- '), @('flag-banned', 'gs')),
        @('library tlog without /sdl fails sdl', $libGood.Replace('/sdl ', ''), @('sdl')),
        @('library tlog without /Zc:alignedNew- fails flag-alignednew', $libGood.Replace('/Zc:alignedNew- ', ''), @('flag-alignednew')),
        @('missing mimalloc tlog dir fails mimalloc-defines', $libGood, @('mimalloc-defines', 'flag-alignednew', 'flag-inline'))
    )
    foreach ($sc in $libScenarios) {
        $n++
        $libRoot = Join-Path $env:TEMP ("openswbf2-libtlog-" + [guid]::NewGuid().ToString('N'))
        try {
            $exeTl = Join-Path $libRoot 'BattlefrontII.tlog'
            $libTl = Join-Path $libRoot 'lib\SoundFL\SoundFL.tlog'
            $miTl = Join-Path $libRoot 'lib\mimalloc\mimalloc.tlog'
            New-Item -ItemType Directory -Force -Path $exeTl, $libTl | Out-Null
            Set-Content -Encoding Unicode -Path (Join-Path $exeTl 'CL.command.1.tlog') -Value @('^C:\X\A.CPP', '/c /Zc:alignedNew- /Zc:inline /sdl /GS C:\X\A.CPP')
            Set-Content -Encoding Unicode -Path (Join-Path $libTl 'CL.command.1.tlog') -Value @('^C:\X\LIB\B.CPP', $sc[1])
            if ($sc[0] -notmatch 'missing mimalloc') {
                New-Item -ItemType Directory -Force -Path $miTl | Out-Null
                Set-Content -Encoding Unicode -Path (Join-Path $miTl 'CL.command.1.tlog') -Value @('^C:\X\SRC\MIMALLOC\SRC\STATIC.C', '/c /Zc:alignedNew- /Zc:inline /sdl /GS /D MI_PAGE_CACHE_OFFSET=0 /D MI_DEBUG=2 /D MI_SECURE=0 /D MI_NO_GETENV=1 /D MI_USE_RTLGENRANDOM C:\X\SRC\MIMALLOC\SRC\STATIC.C')
            }
            $bag = Get-ClTlogCounts @($exeTl, $libTl, $miTl)
            $got = @(Get-FlagChecks $bag $null 'Debug' 'Debug') + @(Get-HardeningChecks $goodHdr $goodCfg $bag.Total $bag.GsMinus $bag.Sdl)
            $why = $null
            foreach ($c in $got) {
                if (@('flag-alignednew', 'flag-inline', 'flag-banned', 'mimalloc-defines', 'gs', 'sdl') -notcontains $c.Name) { continue }
                $want = $sc[2] -notcontains $c.Name
                if ($c.Ok -ne $want) { $why = "$($c.Name) ok=$($c.Ok): $($c.Detail)" }
            }
            if ($why) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $why" }
            else { Write-Host "OK $n $($sc[0])" }
        }
        finally {
            Remove-Item -Recurse -Force $libRoot -ErrorAction SilentlyContinue
        }
    }
    $weGood = '/c /Zc:alignedNew- /Zc:inline /sdl /GS /we4311 /we"4312" /we4302 /we4477 C:\X\A.CPP'
    $weLuac = @('^C:\X\L.CPP', $weGood.Replace('A.CPP', 'L.CPP'))
    $weScenarios = @(
        @('x64-truncation passes plain and quoted /we on every line', 'x64', @('^C:\X\A.CPP', $weGood), $true, $true, $null, $weLuac),
        @('x64-truncation fails a line without /we4477 and names it', 'x64', @('^C:\X\A.CPP', $weGood, '^C:\X\B.CPP', $weGood.Replace(' /we4477', '').Replace('A.CPP', 'B.CPP')), $true, $false, 'first missing C:\X\B.CPP', $weLuac),
        @('x64-truncation fails a /we44770 lookalike', 'x64', @('^C:\X\A.CPP', $weGood.Replace('/we4477', '/we44770')), $true, $false, 'first missing C:\X\A.CPP', $weLuac),
        @('x64-truncation fails without a tests tlog', 'x64', @('^C:\X\A.CPP', $weGood), $false, $false, 'tests cl entries=0', $weLuac),
        @('x64-truncation fails a LuaCompiler line without /we4302 and names it', 'x64', @('^C:\X\A.CPP', $weGood), $true, $false, 'luac cl entries=1 /we4311=1 /we4312=1 /we4302=0 /we4477=1 all four=0 first missing C:\X\L.CPP', @('^C:\X\L.CPP', $weGood.Replace(' /we4302', '').Replace('A.CPP', 'L.CPP'))),
        @('x64-truncation fails without a LuaCompiler tlog', 'x64', @('^C:\X\A.CPP', $weGood), $true, $false, 'luac cl entries=0', $null),
        @('x64-truncation is not emitted on Win32', 'Win32', @('^C:\X\A.CPP', $weGood), $true, $null, $null, $null)
    )
    foreach ($sc in $weScenarios) {
        $n++
        $weDir = Join-Path $env:TEMP ("openswbf2-we-" + [guid]::NewGuid().ToString('N'))
        $wePlat = $script:SmokePlatform
        try {
            New-Item -ItemType Directory -Path $weDir | Out-Null
            Set-Content -Encoding Unicode -Path (Join-Path $weDir 'CL.command.1.tlog') -Value $sc[2]
            $bag = Get-ClTlogCounts $weDir
            $testsBag = $null
            if ($sc[3]) { $testsBag = New-FlagBag 97 97 0 0 $null }
            $luacBag = $null
            if ($sc[6]) {
                $weLuacDir = Join-Path $weDir 'luac'
                New-Item -ItemType Directory -Path $weLuacDir | Out-Null
                Set-Content -Encoding Unicode -Path (Join-Path $weLuacDir 'CL.command.1.tlog') -Value $sc[6]
                $luacBag = Get-ClTlogCounts $weLuacDir
            }
            $script:SmokePlatform = $sc[1]
            $got = @(Get-FlagChecks $bag $testsBag 'Debug' 'Debug' $luacBag | Where-Object { $_.Name -eq 'x64-truncation' })
            $why = $null
            if ($null -eq $sc[4]) { if ($got.Count) { $why = 'emitted' } }
            elseif ($got.Count -ne 1) { $why = "rows=$($got.Count)" }
            elseif ($got[0].Ok -ne $sc[4]) { $why = "ok=$($got[0].Ok): $($got[0].Detail)" }
            elseif ($sc[5] -and $got[0].Detail.IndexOf($sc[5]) -lt 0) { $why = "detail lacks '$($sc[5])': $($got[0].Detail)" }
            if ($why) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $why" }
            else { Write-Host "OK $n $($sc[0])" }
        }
        finally {
            $script:SmokePlatform = $wePlat
            Remove-Item -Recurse -Force $weDir -ErrorAction SilentlyContinue
        }
    }
    $objScenarios = @(
        @('x64-objects passes when every unit is built and linked', @('One', 'Two'), @('One', 'Two'), $true, $null),
        @('x64-objects fails a unit with no object and names it', @('One'), @('One', 'Two'), $false, 'TWO.OBJ not built'),
        @('x64-objects fails a stale object the link did not read', @('One', 'Two'), @('One'), $false, 'TWO.OBJ not linked')
    )
    foreach ($sc in $objScenarios) {
        $n++
        $objRoot = Join-Path $env:TEMP ("openswbf2-x64obj-" + [guid]::NewGuid().ToString('N'))
        $objPlat = $script:SmokePlatform
        try {
            $objInt = Join-Path $objRoot 'obj\x64\Test'
            New-Item -ItemType Directory -Force -Path (Join-Path $objInt 'src\a'), (Join-Path $objInt 'BattlefrontII.tlog') | Out-Null
            Set-Content -Encoding UTF8 -Path (Join-Path $objRoot 'BattlefrontII.vcxproj') -Value @(
                '<?xml version="1.0" encoding="utf-8"?>',
                '<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">',
                '  <ItemGroup>',
                '    <ClCompile Include="src\a\One.cpp" />',
                '    <ClCompile Include="Two.cpp" />',
                '  </ItemGroup>',
                '</Project>'
            )
            $objPath = @{ One = (Join-Path $objInt 'src\a\ONE.OBJ'); Two = (Join-Path $objInt 'TWO.OBJ') }
            foreach ($o in $sc[1]) { Set-Content -Path $objPath[$o] -Value 'obj' -Encoding ASCII }
            Set-Content -Encoding Unicode -Path (Join-Path $objInt 'BattlefrontII.tlog\link.command.1.tlog') -Value @(('^' + ((@($sc[2] | ForEach-Object { $objPath[$_] }) + (Join-Path $objInt 'BATTLEFRONTII.RES')) -join '|')), '/OUT:"X.EXE"')
            $script:SmokePlatform = 'x64'
            $c = Get-X64ObjectsCheck $objRoot 'Test'
            $why = $null
            if ($c.Ok -ne $sc[3]) { $why = "ok=$($c.Ok): $($c.Detail)" }
            elseif ($c.Detail -notmatch '^units=2 ') { $why = "units: $($c.Detail)" }
            elseif ($sc[4] -and $c.Detail.IndexOf($sc[4], [StringComparison]::OrdinalIgnoreCase) -lt 0) { $why = "detail lacks '$($sc[4])': $($c.Detail)" }
            if ($why) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $why" }
            else { Write-Host "OK $n $($sc[0])" }
        }
        finally {
            $script:SmokePlatform = $objPlat
            Remove-Item -Recurse -Force $objRoot -ErrorAction SilentlyContinue
        }
    }
    $masmScenarios = @(
        @('x64-objects counts an x64 MASM item that is built and linked', 'x64', @('One', 'Fast'), @('One', 'Fast'), $true, 'masm=1', $null),
        @('x64-objects fails an x64 MASM object the link did not read', 'x64', @('One', 'Fast'), @('One'), $false, 'masm=1', 'FAST.OBJ not linked'),
        @('x64-objects fails an x64 MASM item with no object', 'x64', @('One'), @('One'), $false, 'masm=1', 'FAST.OBJ not built'),
        @('x64-objects skips an x64 MASM item on Win32', 'Win32', @('One'), @('One'), $true, 'masm=0', $null)
    )
    foreach ($sc in $masmScenarios) {
        $n++
        $maRoot = Join-Path $env:TEMP ("openswbf2-x64masm-" + [guid]::NewGuid().ToString('N'))
        $maPlat = $script:SmokePlatform
        try {
            $maInt = Join-Path $maRoot "obj\$($sc[1])\Test"
            New-Item -ItemType Directory -Force -Path (Join-Path $maInt 'src\a'), (Join-Path $maInt 'BattlefrontII.tlog') | Out-Null
            Set-Content -Encoding UTF8 -Path (Join-Path $maRoot 'BattlefrontII.vcxproj') -Value @(
                '<?xml version="1.0" encoding="utf-8"?>',
                '<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">',
                '  <ItemGroup>',
                '    <ClCompile Include="One.cpp" />',
                '    <MASM Include="src\a\Fast.asm" Condition="''$(Platform)''==''x64''" />',
                '  </ItemGroup>',
                '</Project>'
            )
            $maPath = @{ One = (Join-Path $maInt 'ONE.OBJ'); Fast = (Join-Path $maInt 'src\a\FAST.OBJ') }
            foreach ($o in $sc[2]) { Set-Content -Path $maPath[$o] -Value 'obj' -Encoding ASCII }
            Set-Content -Encoding Unicode -Path (Join-Path $maInt 'BattlefrontII.tlog\link.command.1.tlog') -Value @(('^' + (@($sc[3] | ForEach-Object { $maPath[$_] }) -join '|')), '/OUT:"X.EXE"')
            $script:SmokePlatform = $sc[1]
            $c = Get-X64ObjectsCheck $maRoot 'Test'
            $why = $null
            if ($c.Ok -ne $sc[4]) { $why = "ok=$($c.Ok): $($c.Detail)" }
            elseif ($c.Detail -notmatch ('^units=1 missing=\d+ ' + $sc[5] + '(?: |$)')) { $why = "counts: $($c.Detail)" }
            elseif ($sc[6] -and $c.Detail.IndexOf($sc[6], [StringComparison]::OrdinalIgnoreCase) -lt 0) { $why = "detail lacks '$($sc[6])': $($c.Detail)" }
            if ($why) { $failed++; Write-Host "FAILED ${n}: $($sc[0]): $why" }
            else { Write-Host "OK $n $($sc[0])" }
        }
        finally {
            $script:SmokePlatform = $maPlat
            Remove-Item -Recurse -Force $maRoot -ErrorAction SilentlyContinue
        }
    }
    $n++
    $ocRoot =Join-Path $env:TEMP ("openswbf2-x64objcond-" + [guid]::NewGuid().ToString('N'))
    $ocPlat = $script:SmokePlatform
    $ocWhy = $null
    $ocMutWhy = $null
    try {
        New-Item -ItemType Directory -Force -Path $ocRoot | Out-Null
        Set-Content -Encoding UTF8 -Path (Join-Path $ocRoot 'BattlefrontII.vcxproj') -Value @(
            '<?xml version="1.0" encoding="utf-8"?>',
            '<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">',
            '  <ItemGroup>',
            '    <ClCompile Include="All.cpp" />',
            '    <ClCompile Include="Win.cpp" Condition="''$(Platform)''==''Win32''" />',
            '  </ItemGroup>',
            '  <ItemGroup Condition="''$(Platform)''==''x64''">',
            '    <ClCompile Include="Wide.cpp" />',
            '  </ItemGroup>',
            '</Project>'
        )
        foreach ($oc in @(@('x64', @('All', 'Wide')), @('Win32', @('All', 'Win')))) {
            $ocInt = Join-Path $ocRoot "obj\$($oc[0])\Test"
            New-Item -ItemType Directory -Force -Path (Join-Path $ocInt 'BattlefrontII.tlog') | Out-Null
            $ocObjs = @($oc[1] | ForEach-Object { Join-Path $ocInt "$_.obj" })
            foreach ($o in $ocObjs) { Set-Content -Path $o -Value 'obj' -Encoding ASCII }
            Set-Content -Encoding Unicode -Path (Join-Path $ocInt 'BattlefrontII.tlog\link.command.1.tlog') -Value @(('^' + ($ocObjs -join '|')), '/OUT:"X.EXE"')
            $script:SmokePlatform = $oc[0]
            $c = Get-X64ObjectsCheck $ocRoot 'Test'
            if (-not $c.Ok -or $c.Detail -notmatch '^units=2 missing=0') { $ocWhy = "$($oc[0]) ok=$($c.Ok): $($c.Detail)"; break }
        }
        if (-not $ocWhy) {
            $ocAnchor = 'if (-not (Test-MsBuildItemIncluded $item $script:SmokePlatform $config)) { continue }'
            $ocDef = (Get-Command Get-X64ObjectsCheck).Definition
            $ocHits = ([regex]::Matches($ocDef, [regex]::Escape($ocAnchor))).Count
            if ($ocHits -ne 1) { $ocWhy = "mutant anchor found $ocHits times in Get-X64ObjectsCheck" }
            else {
                $script:SmokePlatform = 'x64'
                $ocMut = & {
                    Set-Item -Path function:Get-X64ObjectsCheck -Value ([scriptblock]::Create($ocDef.Replace($ocAnchor, '')))
                    Get-X64ObjectsCheck $ocRoot 'Test'
                }
                if ($ocMut.Ok -or $ocMut.Detail -notmatch '\\Win\.obj not built') { $ocWhy = "a Get-X64ObjectsCheck mutant that ignores Condition did not report Win.obj missing on x64: ok=$($ocMut.Ok) $($ocMut.Detail)" }
                else { $ocMutWhy = $ocMut.Detail }
            }
        }
        if (-not $ocWhy) {
            $ocCond = @(
                @("'`$(Platform)'=='x64' and '`$(Configuration)'=='Release'", 'x64', 'Release', $true),
                @("'`$(Platform)'=='x64' and '`$(Configuration)'=='Release'", 'x64', 'Test', $false),
                @("'`$(Configuration)'=='Test'", 'Win32', 'Test', $true),
                @('', 'Win32', 'Test', $true)
            )
            foreach ($cc in $ocCond) {
                if ((Test-MsBuildCondition $cc[0] $cc[1] $cc[2]) -ne $cc[3]) { $ocWhy = "condition '$($cc[0])' on $($cc[1])|$($cc[2]) is not $($cc[3])"; break }
            }
        }
        if (-not $ocWhy) {
            foreach ($bad in @("'`$(Platform)'!='x64'", "Exists('x.dll')", "'`$(Platform)'=='x64' or '`$(Platform)'=='Win32'", "'`$(SWBF2_VENDOR_X64)'==''")) {
                $threw = $false
                try { [void](Test-MsBuildCondition $bad 'x64' 'Test') } catch { $threw = $_.Exception.Message -like 'unsupported MSBuild condition:*' }
                if (-not $threw) { $ocWhy = "unsupported condition '$bad' did not throw"; break }
            }
        }
    }
    catch { $ocWhy = "threw: $_" }
    finally {
        $script:SmokePlatform = $ocPlat
        Remove-Item -Recurse -Force $ocRoot -ErrorAction SilentlyContinue
    }
    if ($ocWhy) { $failed++; Write-Host "FAILED ${n}: x64-objects honours item and ItemGroup Condition per platform: $ocWhy" }
    else { Write-Host "OK $n x64-objects honours item and ItemGroup Condition per platform (x64 needs All and Wide, Win32 needs All and Win), unsupported condition forms throw; mutant without Condition fails: $ocMutWhy" }
    $n++
    $mapGood = @(
        ' 0002:0000156c 00000004H .CRT$XIB                DATA',
        ' 0002:00001578 00000004H .CRT$XLB                DATA',
        ' 0002:0000157c 00000004H .CRT$XLY                DATA',
        ' 0002:0000156c       __mi_crt_callback_init     0085456c     static.obj',
        ' 0002:00001578       __mi_tls_callback_pre      00854578     static.obj',
        ' 0002:0000157c       __mi_tls_callback_post     0085457c     static.obj'
    ) -join "`r`n"
    $hookCases = @(
        @('good map', $mapGood, $true),
        @('no __mi_crt_callback_init', ($mapGood -replace '(?m)^.*__mi_crt_callback_init.*\r?\n', ''), $false),
        @('XIB symbol outside .CRT$XIB', $mapGood.Replace('0002:0000156c       __mi_crt_callback_init', '0002:00001570       __mi_crt_callback_init'), $false),
        @('XLY symbol from another object', $mapGood.Replace('0085457c     static.obj', '0085457c     other.obj'), $false),
        @('no map', $null, $false)
    )
    $why = $null
    foreach ($hc in $hookCases) {
        $r = Get-MiCrtHookCheck $hc[1]
        if ($r.Name -ne 'mimalloc-crt-hooks' -or $r.Ok -ne $hc[2]) { $why = "$($hc[0]) ok=$($r.Ok) $($r.Detail)" }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: mimalloc-crt-hooks map fixtures: $why" }
    else { Write-Host "OK $n mimalloc-crt-hooks passes the three static.obj hooks and fails a missing, misplaced or foreign hook" }
    $n++
    $rlRelease = @(
        ' 0002:00004a4c       ??_C@_09LGAOCJJ@riderlink@ 0087ea4c     CommandLine.obj',
        ' 0002:00004a58       ??_C@_0N@DNHDPIMN@riderlinkdir@ 0087ea58     CommandLine.obj',
        ' 0002:00004a68       ??_C@_0O@BCFKFPPB@riderlinkhold@ 0087ea68     CommandLine.obj',
        ' 0002:00024870       ??_C@_0CA@DCCPKP@?9riderlink?5is?5not?5in?5this?5build@ 0089e870     pcMain.obj',
        ' 0003:0002898c       ?gLinkSwitchIgnored@@3_NA  0092598c     CommandLine.obj'
    ) -join "`r`n"
    $rlFunction = ' 0001:00050730       ?Start@RiderLink@@YAX_N@Z  00451730 f   RiderLink.obj'
    $rlProtocol = ' 0001:00050d50       ?Parse@RiderLinkProtocol@@YA?AW4Message@1@PBDIPADAAI@Z 00451d50 f   RiderLinkProtocol.obj'
    $rlTap = ' 0003:005aa9c0       ?g_fnLineTap@RedWarning@@3P6AXPBD@ZA 00eba9c0     RedWarning.obj'
    $rlCases = @(
        @('Release map with only the lower-case literals', $rlRelease, 'Release', $true),
        @('Release map with a RiderLink function', "$rlRelease`r`n$rlFunction", 'Release', $false),
        @('Release map with a RiderLinkProtocol function', "$rlRelease`r`n$rlProtocol", 'Release', $false),
        @('Release map with the tap slot', "$rlRelease`r`n$rlTap", 'Release', $false),
        @('Test map without link symbols', $rlRelease, 'Test', $false),
        @('Test map with link symbols', "$rlRelease`r`n$rlFunction`r`n$rlProtocol`r`n$rlTap", 'Test', $true),
        @('Debug map with a link function', "$rlRelease`r`n$rlFunction", 'Debug', $true),
        @('no map', $null, 'Release', $false),
        @('no map on Test', $null, 'Test', $false)
    )
    $why = $null
    foreach ($rc in $rlCases) {
        $r = Get-RiderLinkMapCheck $rc[1] $rc[2]
        if ($r.Name -ne 'riderlink-map' -or $r.Ok -ne $rc[3]) { $why = "$($rc[0]) ok=$($r.Ok) $($r.Detail)" }
    }
    if ($why) { $failed++; Write-Host "FAILED ${n}: riderlink-map map fixtures: $why" }
    else { Write-Host "OK $n riderlink-map passes a Release map holding only the lower-case switch and Warning literals and a Test map with link symbols, and fails link code or the tap slot in Release, a Test map without link symbols and a missing map" }
    $n++
    $addrDir = Join-Path $env:TEMP ("openswbf2-addrorder-" + [guid]::NewGuid().ToString('N'))
    try {
        foreach ($d in 'src\Battlefront2\Source', 'src\RedEngineFL') { New-Item -ItemType Directory -Path (Join-Path $addrDir $d) | Out-Null }
        $addrLines = @('int k = (int)this;', 'if ((unsigned int)pA > 1) {}', 'if (1 < (unsigned int)pB) {}', 'if (reinterpret_cast<uintptr_t>(pA) > 1) {}', 'if (1 > reinterpret_cast<int>(pA)) {}', 'bool b = PblPtrAfter(x, y);', 'if (pOther > pObject) {}', 'int c = iCount > 3;', 'pGroup->mThreads.Add(&mNode, this, (int)pOwner);', 'if (ppA > ppB) {}', 'bool e = (const char*)pA > (const char*)pB;', 'bool d = std::less<const void*>()(pA, pB);', 'bool f = obj > other;', 'mList.Add(&mNode, this, (int)iKey);', 'n = (char*)pA - (char*)pB;')
        Set-Content -Encoding ASCII -Path (Join-Path $addrDir 'src\Battlefront2\Source\a.cpp') -Value $addrLines
        Set-Content -Encoding ASCII -Path (Join-Path $addrDir 'src\RedEngineFL\a.cpp') -Value $addrLines
        $bad = @(Get-SourceChecks $addrDir -addrOrderAllow @() -Only 'addr-order')
        $addrWant = (@(1..7) + @(9..12) | ForEach-Object { "src\Battlefront2\Source\a.cpp:$_" }) -join ', '
        $allowed = @(Get-SourceChecks $addrDir -addrOrderAllow @('src\Battlefront2\Source\a.cpp|if (pOther > pObject) {}') -Only 'addr-order')
        $unused = @(Get-SourceChecks $addrDir -addrOrderAllow @('src\Battlefront2\Source\a.cpp|if (pOther > pObject) {}', 'src\Battlefront2\Source\a.cpp|int nope;') -Only 'addr-order')
        if ($bad.Count -ne 1 -or $bad[0].Ok -or $bad[0].Detail -ne $addrWant) { $failed++; Write-Host "FAILED ${n}: addr-order flags pointer order under Battlefront2 and PebbleFL only: $($bad.Detail)" }
        elseif ($allowed[0].Detail -cmatch ':7(,|$)') { $failed++; Write-Host "FAILED ${n}: addr-order allow entry did not suppress its line: $($allowed[0].Detail)" }
        elseif ($unused[0].Detail -cnotmatch 'unused allow src\\Battlefront2\\Source\\a\.cpp\|int nope;') { $failed++; Write-Host "FAILED ${n}: addr-order did not report an unused allow entry: $($unused[0].Detail)" }
        else { Write-Host "OK $n addr-order flags pointer order under Battlefront2 and PebbleFL only and keeps its allow list exact" }
    }
    finally {
        Remove-Item -Recurse -Force $addrDir -ErrorAction SilentlyContinue
    }
    $n++
    $crtBanDir = Join-Path $env:TEMP ("openswbf2-crtban-" + [guid]::NewGuid().ToString('N'))
    try {
        foreach ($d in 'src\Foo', 'src\mimalloc\src', 'src\PebbleFL\Common', 'src\FFmpegFL\libavutil', 'src\RedEngineFL\Memory') { New-Item -ItemType Directory -Path (Join-Path $crtBanDir $d) | Out-Null }
        Set-Content -Encoding ASCII -Path (Join-Path $crtBanDir 'src\Foo\a.cpp') -Value @('void* p = malloc(1);', 'void* q = calloc (1, 2);', 'p = realloc(p, 4);', 'free(p);', 'char* s = _strdup("x");', 'size_t z = _msize(q);', 'pBackend->free(p);', 'mFree(p);', 'RedFreeToHeap(0, p, 0);', 'PblCrtFree(p);', 'int free_count = 0;', 'std::free(p);')
        Set-Content -Encoding ASCII -Path (Join-Path $crtBanDir 'src\mimalloc\src\x.c') -Value @('void* p = malloc(1);')
        Set-Content -Encoding ASCII -Path (Join-Path $crtBanDir 'src\PebbleFL\Common\PblCrtHeap.cpp') -Value @('void* p = malloc(1);', 'free(p);')
        foreach ($f in 'src\FFmpegFL\libavutil\mem.c', 'src\RedEngineFL\Memory\m.cpp') { Set-Content -Encoding ASCII -Path (Join-Path $crtBanDir $f) -Value @('void* p = _aligned_malloc(1, 16);') }
        $bad = @(Get-SourceChecks $crtBanDir -Only 'crt-heap-ban')
        Remove-Item -LiteralPath (Join-Path $crtBanDir 'src\Foo\a.cpp'), (Join-Path $crtBanDir 'src\RedEngineFL\Memory\m.cpp')
        $good = @(Get-SourceChecks $crtBanDir -Only 'crt-heap-ban')
        if ($bad.Count -ne 1 -or $bad[0].Ok -or $bad[0].Detail -ne 'src\Foo\a.cpp:1, src\Foo\a.cpp:2, src\Foo\a.cpp:3, src\Foo\a.cpp:4, src\Foo\a.cpp:5, src\Foo\a.cpp:6, src\RedEngineFL\Memory\m.cpp:1') { $failed++; Write-Host "FAILED ${n}: crt-heap-ban flags CRT heap calls outside src\mimalloc, src\FFmpegFL and PblCrtHeap.cpp: $($bad.Detail)" }
        elseif ($good.Count -ne 1 -or -not $good[0].Ok) { $failed++; Write-Host "FAILED ${n}: crt-heap-ban flags CRT heap calls outside src\mimalloc, src\FFmpegFL and PblCrtHeap.cpp: allowed files flagged $($good.Detail)" }
        else { Write-Host "OK $n crt-heap-ban flags CRT heap calls outside src\mimalloc, src\FFmpegFL and PblCrtHeap.cpp" }
    }
    finally {
        Remove-Item -Recurse -Force $crtBanDir -ErrorAction SilentlyContinue
    }
    $n++
    $miApiDir = Join-Path $env:TEMP ("openswbf2-miapi-" + [guid]::NewGuid().ToString('N'))
    try {
        foreach ($d in 'src\Foo', 'src\mimalloc\src', 'src\RedEngineFL\Memory') { New-Item -ItemType Directory -Path (Join-Path $miApiDir $d) | Out-Null }
        Set-Content -Encoding ASCII -Path (Join-Path $miApiDir 'src\Foo\a.cpp') -Value @('void* p = mi_malloc(1);', 'mi_heap_t* h = mi_heap_new();', 'void* q = mi_heap_malloc (h, 1);', 'mi_free(q);', 'int v = MI_VERSION;', 'int mi_count = 0;', 'mi_option_set(mi_option_verbose, 1);')
        Set-Content -Encoding ASCII -Path (Join-Path $miApiDir 'src\mimalloc\src\x.c') -Value @('void* p = mi_malloc(1);')
        Set-Content -Encoding ASCII -Path (Join-Path $miApiDir 'src\RedEngineFL\Memory\RedMemoryMi.cpp') -Value @('void* p = mi_malloc(1);')
        $bad = @(Get-SourceChecks $miApiDir -Only 'mi-api-ban')
        Remove-Item -LiteralPath (Join-Path $miApiDir 'src\Foo\a.cpp')
        $good = @(Get-SourceChecks $miApiDir -Only 'mi-api-ban')
        if ($bad.Count -ne 1 -or $bad[0].Ok -or $bad[0].Detail -ne 'src\Foo\a.cpp:1, src\Foo\a.cpp:2, src\Foo\a.cpp:3, src\Foo\a.cpp:4, src\Foo\a.cpp:7') { $failed++; Write-Host "FAILED ${n}: mi-api-ban flags mimalloc calls outside src\mimalloc and RedMemoryMi.cpp: $($bad.Detail)" }
        elseif ($good.Count -ne 1 -or -not $good[0].Ok) { $failed++; Write-Host "FAILED ${n}: mi-api-ban flags mimalloc calls outside src\mimalloc and RedMemoryMi.cpp: allowed files flagged $($good.Detail)" }
        else { Write-Host "OK $n mi-api-ban flags mimalloc calls outside src\mimalloc and RedMemoryMi.cpp" }
    }
    finally {
        Remove-Item -Recurse -Force $miApiDir -ErrorAction SilentlyContinue
    }
    $n++
    $newSizeDir = Join-Path $env:TEMP ("openswbf2-newsize-" + [guid]::NewGuid().ToString('N'))
    try {
        foreach ($d in 'src\Foo', 'tests\stubs', 'tests\third_party') { New-Item -ItemType Directory -Path (Join-Path $newSizeDir $d) | Out-Null }
        Set-Content -Encoding ASCII -Path (Join-Path $newSizeDir 'src\Foo\a.cpp') -Value @('void* Foo::operator new(size_t uSize);')
        Set-Content -Encoding ASCII -Path (Join-Path $newSizeDir 'tests\stubs\s.cpp') -Value @('int x;', 'void* AIPath::operator new(unsigned int uSize) { return 0; }')
        Set-Content -Encoding ASCII -Path (Join-Path $newSizeDir 'tests\third_party\t.h') -Value @('void* operator new(unsigned int n);')
        $bad = @(Get-SourceChecks $newSizeDir -Only 'new-size-t')
        Set-Content -Encoding ASCII -Path (Join-Path $newSizeDir 'tests\stubs\s.cpp') -Value @('int x;', 'void* AIPath::operator new(size_t uSize) { return 0; }')
        $good = @(Get-SourceChecks $newSizeDir -Only 'new-size-t')
        if ($bad.Count -ne 1 -or $bad[0].Ok -or $bad[0].Detail -ne 'tests\stubs\s.cpp:2') { $failed++; Write-Host "FAILED ${n}: new-size-t flags operator new(unsigned int in tests outside tests\third_party: $($bad.Detail)" }
        elseif ($good.Count -ne 1 -or -not $good[0].Ok) { $failed++; Write-Host "FAILED ${n}: new-size-t flags operator new(unsigned int in tests outside tests\third_party: size_t flagged $($good.Detail)" }
        else { Write-Host "OK $n new-size-t flags operator new(unsigned int in tests outside tests\third_party" }
    }
    finally {
        Remove-Item -Recurse -Force $newSizeDir -ErrorAction SilentlyContinue
    }
    $n++
    $readersDir = Join-Path $env:TEMP ("openswbf2-readers-" + [guid]::NewGuid().ToString('N'))
    try {
        foreach ($d in 'tests\baseline', 'tests\unit', 'tests\game') { New-Item -ItemType Directory -Path (Join-Path $readersDir $d) | Out-Null }
        foreach ($fd in 'v143', 'v143-x64') { Copy-Item -LiteralPath (Join-Path (Split-Path $PSScriptRoot -Parent) "tests\baseline\$fd") -Destination (Join-Path $readersDir 'tests\baseline') -Recurse }
        $readerMap = [IO.File]::ReadAllText((Join-Path (Split-Path $PSScriptRoot -Parent) 'tests\baseline\compare-tables.ps1'))
        $readerT1 = @([regex]::Matches($readerMap, "Reader = 't1:([^']+)'") | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
        $readerCpp = @([regex]::Matches($readerMap, "'(tests\\game\\[^']+\.cpp)'") | ForEach-Object { $_.Groups[1].Value } | Sort-Object -Unique)
        foreach ($f in @('tests\baseline\compare-tables.ps1', 'tests\baseline\capture-vs2013.ps1') + $readerT1 + $readerCpp) { Copy-Item -LiteralPath (Join-Path (Split-Path $PSScriptRoot -Parent) $f) -Destination (Join-Path $readersDir $f) }
        $good = @(Get-SourceChecks $readersDir -Only 'baseline-readers')
        Set-Content -Encoding ASCII -Path (Join-Path $readersDir 'tests\baseline\v143\extra-table.txt') -Value 'x'
        $bad = @(Get-SourceChecks $readersDir -Only 'baseline-readers')
        Remove-Item -LiteralPath (Join-Path $readersDir 'tests\baseline\v143\extra-table.txt')
        Set-Content -Encoding ASCII -Path (Join-Path $readersDir 'tests\baseline\v143-x64\extra-table.txt') -Value 'x'
        $badX64 = @(Get-SourceChecks $readersDir -Only 'baseline-readers')
        if ($good.Count -ne 1 -or -not $good[0].Ok) { $failed++; Write-Host "FAILED ${n}: baseline-readers fails a v143 or v143-x64 file with no reader: unmodified copy flagged $($good.Detail)" }
        elseif ($bad.Count -ne 1 -or $bad[0].Ok -or $bad[0].Detail -cne 'v143\extra-table.txt has no reader') { $failed++; Write-Host "FAILED ${n}: baseline-readers fails a v143 file with no reader: $($bad.Detail)" }
        elseif ($badX64.Count -ne 1 -or $badX64[0].Ok -or $badX64[0].Detail -cne 'v143-x64\extra-table.txt has no reader') { $failed++; Write-Host "FAILED ${n}: baseline-readers fails a v143-x64 file with no reader: $($badX64.Detail)" }
        else { Write-Host "OK $n baseline-readers fails a v143 file and a v143-x64 file with no reader" }
    }
    finally {
        Remove-Item -Recurse -Force $readersDir -ErrorAction SilentlyContinue
    }
    $n++
    $impGood = "Dump of file C:\X\BattlefrontII.exe`n`n  Section contains the following imports:`n`n    KERNEL32.dll`n              1000 Import Address Table`n                 1A0 GetTickCount`n`n    api-ms-win-crt-environment-l1-1-0.dll`n                   0 getenv`n"
    $impBad = $impGood + "`n    bcrypt.dll`n                   0 BCryptGenRandom`n"
    function Get-SelImportRow($imp, [string]$plat, [string]$name) { return , @(Get-ImportChecks $imp $plat | Where-Object { $_.Name -eq $name }) }
    foreach ($plat in 'Win32', 'x64') {
        $okGood = Get-SelImportRow $impGood $plat 'import-modules'
        $okBad = Get-SelImportRow $impBad $plat 'import-modules'
        $okEmpty = Get-SelImportRow '' $plat 'import-modules'
        if ($okGood.Count -ne 1 -or -not $okGood[0].Ok -or $okBad[0].Ok -or $okBad[0].Detail -notmatch 'bcrypt\.dll' -or $okEmpty[0].Ok -or $okEmpty[0].Detail -notmatch 'kernel32\.dll not found') { $failed++; Write-Host "FAILED ${n}: import-modules allows only listed DLLs on ${plat}: good=$($okGood.Ok) bad=$($okBad.Ok) empty=$($okEmpty.Ok)" }
        else { Write-Host "OK $n import-modules allows only listed DLLs on $plat" }
        $n++
    }
    $d3dxImpGood = $impGood + "`n    d3dx9_43.dll`n             1404C3DB0 Import Address Table`n             1405B63E8 Import Name Table`n                     0 time date stamp`n                     0 Index of first forwarder reference`n`n                          B7 D3DXLoadSurfaceFromSurface`n                           7 D3DXCheckTextureRequirements`n`n    WS2_32.dll`n                   0 Ordinal 23`n"
    $d3dxImpExtra = $d3dxImpGood.Replace("7 D3DXCheckTextureRequirements`n", "7 D3DXCheckTextureRequirements`n                          C1 D3DXMatrixRotationAxis`n")
    $d3dxImpMissing = $d3dxImpGood.Replace("                          B7 D3DXLoadSurfaceFromSurface`n", '')
    $why = $null
    $dg = Get-SelImportRow $d3dxImpGood 'x64' 'd3dx-imports'
    $de = Get-SelImportRow $d3dxImpExtra 'x64' 'd3dx-imports'
    $dm = Get-SelImportRow $d3dxImpMissing 'x64' 'd3dx-imports'
    $dw = Get-SelImportRow $d3dxImpExtra 'Win32' 'd3dx-imports'
    if ($dg.Count -ne 1 -or -not $dg[0].Ok) { $why = "good listing: $($dg.Ok) $($dg.Detail)" }
    elseif ($de.Count -ne 1 -or $de[0].Ok -or $de[0].Detail -cne 'extra: D3DXMatrixRotationAxis') { $why = "extra name: $($de.Ok) $($de.Detail)" }
    elseif ($dm.Count -ne 1 -or $dm[0].Ok -or $dm[0].Detail -cne 'missing: D3DXLoadSurfaceFromSurface') { $why = "missing name: $($dm.Ok) $($dm.Detail)" }
    elseif ($dw.Count -ne 0) { $why = 'Win32 has a d3dx-imports row' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: d3dx-imports passes only the two render imports on x64: $why" }
    else { Write-Host "OK $n d3dx-imports passes only the two render imports on x64, names an extra or missing import and is x64 only" }
    $n++
    $crtImpGood = $impGood + "`n    api-ms-win-crt-math-l1-1-0.dll`n             1404C4998 Import Address Table`n             1405B6F00 Import Name Table`n                     0 time date stamp`n                     0 Index of first forwarder reference`n`n                          4F asin`n                          55 atan2`n                         117 sqrtf`n                          BF floorf`n                          9D sin`n"
    $crtImpBad = $crtImpGood.Replace("                          9D sin`n", "                          9D sin`n                          A0 sinf`n                          60 atan2f`n")
    $why = $null
    $cg = Get-SelImportRow $crtImpGood 'x64' 'crt-float-imports'
    $cb = Get-SelImportRow $crtImpBad 'x64' 'crt-float-imports'
    $cn = Get-SelImportRow $impGood 'x64' 'crt-float-imports'
    $cw = Get-SelImportRow $crtImpBad 'Win32' 'crt-float-imports'
    if ($cg.Count -ne 1 -or -not $cg[0].Ok) { $why = "good listing: $($cg.Ok) $($cg.Detail)" }
    elseif ($cb.Count -ne 1 -or $cb[0].Ok -or $cb[0].Detail -cne 'float CRT imports: atan2f sinf') { $why = "float imports: $($cb.Ok) $($cb.Detail)" }
    elseif ($cn.Count -ne 1 -or $cn[0].Ok -or $cn[0].Detail -cne 'api-ms-win-crt-math-l1-1-0.dll not in the import table') { $why = "no math module: $($cn.Ok) $($cn.Detail)" }
    elseif ($cw.Count -ne 0) { $why = 'Win32 has a crt-float-imports row' }
    if ($why) { $failed++; Write-Host "FAILED ${n}: crt-float-imports fails a float CRT import on x64: $why" }
    else { Write-Host "OK $n crt-float-imports passes the double routines, names each float CRT import, fails a listing without the math module and is x64 only" }
    $n++
    $ctObjs = @('C:\X\OBJ\A.OBJ', 'C:\X\OBJ\PCCRTTRIGX64.OBJ', 'C:\X\OBJ\B.OBJ')
    $ctSym = "Dump of file C:\X\OBJ\A.OBJ`n`nFile Type: COFF OBJECT`n`nCOFF SYMBOL TABLE`n00A 00000000 UNDEF  notype ()    External     | PcCrtSinX64`n00B 00000000 UNDEF  notype ()    External     | sinh`n`nDump of file C:\X\OBJ\PCCRTTRIGX64.OBJ`n`nFile Type: COFF OBJECT`n`nCOFF SYMBOL TABLE`n00C 00000000 UNDEF  notype       External     | __imp_sin`n`nDump of file C:\X\OBJ\B.OBJ`n`nFile Type: COFF OBJECT`n`nCOFF SYMBOL TABLE`n010 00000000 SECT4  notype ()    External     | cos`n"
    $ctBad = $ctSym.Replace("00B 00000000 UNDEF  notype ()    External     | sinh`n", "00B 00000000 UNDEF  notype ()    External     | sinh`n00C 00000000 UNDEF  notype ()    External     | cos`n00D 00000000 UNDEF  notype       External     | __imp_tan`n")
    $ctMissing = $ctSym.Substring(0, $ctSym.IndexOf('Dump of file C:\X\OBJ\B.OBJ'))
    $why = $null
    $tg = Get-CrtTrigRouteCheck $ctObjs $ctSym
    $tb = Get-CrtTrigRouteCheck $ctObjs $ctBad
    $tm = Get-CrtTrigRouteCheck $ctObjs $ctMissing
    $th = Get-CrtTrigRouteCheck @('C:\X\OBJ\A.OBJ') $ctSym
    if ($tg.Name -ne 'crt-trig-route' -or -not $tg.Ok) { $why = "good listing: $($tg.Ok) $($tg.Detail)" }
    elseif ($tb.Ok -or $tb.Detail -cne 'objects=3; A.OBJ references cos,tan') { $why = "extra reference: $($tb.Ok) $($tb.Detail)" }
    elseif ($tm.Ok -or $tm.Detail -cne 'objects=3; no symbols for 1 objects: C:\X\OBJ\B.OBJ') { $why = "missing object: $($tm.Ok) $($tm.Detail)" }
    elseif ($th.Ok -or $th.Detail -cne 'objects=1; pcCrtTrigX64.obj not linked') { $why = "no helper: $($th.Ok) $($th.Detail)" }
    if ($why) { $failed++; Write-Host "FAILED ${n}: crt-trig-route fails an object other than the helper that references sin, cos or tan: $why" }
    else { Write-Host "OK $n crt-trig-route passes the helper's references, names an extra referencing object, fails an object with no symbols and a link without the helper" }
    $n++
    $impDelay = $impGood + "`n  Section contains the following delay load imports:`n`n    steam_api.dll`n              00000001 Characteristics`n              0041C958 Address of HMODULE`n              0041C944 Import Address Table`n              0041B7EC Import Name Table`n              0041B808 Bound Import Name Table`n              00000000 Unload Import Name Table`n                     0 time date stamp`n`n          0040125A             313 SteamAPI_Shutdown`n`n  Summary`n`n        1000 .data`n"
    $impVc1 = $impGood + "`n    VCRUNTIME140_1.dll`n                   0 __CxxFrameHandler4`n"
    $delayAllowSaved = $script:ImportDelayAllow
    foreach ($plat in 'Win32', 'x64') {
        $why = $null
        try {
            $script:ImportDelayAllow = @{ Win32 = @(); x64 = @() }
            $gm = Get-SelImportRow $impGood $plat 'import-modules'
            $gd = Get-SelImportRow $impGood $plat 'import-delay-modules'
            $dm = Get-SelImportRow $impDelay $plat 'import-modules'
            $dd = Get-SelImportRow $impDelay $plat 'import-delay-modules'
            if ($gm.Count -ne 1 -or $gd.Count -ne 1 -or -not $gm[0].Ok -or -not $gd[0].Ok) { $why = "static-only fixture: modules=$($gm.Ok) delay=$($gd.Ok)" }
            elseif ($dd.Count -ne 1 -or $dd[0].Ok -or $dd[0].Detail -cne 'delay imports outside the allow list: steam_api.dll') { $why = "unlisted delay import: ok=$($dd.Ok) detail=$($dd.Detail)" }
            elseif (-not $dm[0].Ok) { $why = "static half saw the delay import: $($dm[0].Detail)" }
            else {
                $script:ImportDelayAllow[$plat] = @('steam_api.dll')
                $ld = Get-SelImportRow $impDelay $plat 'import-delay-modules'
                if (-not $ld[0].Ok) { $why = "listed delay import failed: $($ld[0].Detail)" }
            }
        }
        finally { $script:ImportDelayAllow = $delayAllowSaved }
        if ($why) { $failed++; Write-Host "FAILED ${n}: import-delay-modules on ${plat} fails only an unlisted delay import: $why" }
        else { Write-Host "OK $n import-delay-modules on $plat fails an unlisted delay import, passes a listed one and keeps it out of import-modules" }
        $n++
    }
    $bw = Get-SelImportRow $impVc1 'Win32' 'import-modules'
    $bx = Get-SelImportRow $impVc1 'x64' 'import-modules'
    if (-not $bx[0].Ok -or $bw[0].Ok -or $bw[0].Detail -notmatch 'vcruntime140_1\.dll') { $failed++; Write-Host "FAILED ${n}: import-modules lists are per platform: Win32=$($bw[0].Ok) x64=$($bx[0].Ok) $($bw[0].Detail)" }
    else { Write-Host "OK $n import-modules lists are per platform (vcruntime140_1.dll passes on x64, fails on Win32)" }
    $n++
    $bx64 = Get-SelImportRow ($impGood + "`n    binkw64.dll`n                   0 BinkOpen`n") 'x64' 'import-modules'
    if ($bx64.Count -ne 1 -or $bx64[0].Ok -or $bx64[0].Detail -notmatch 'binkw64\.dll') { $failed++; Write-Host "FAILED ${n}: x64 import-modules fails a binkw64.dll import: ok=$($bx64.Ok) $($bx64.Detail)" }
    else { Write-Host "OK $n x64 import-modules fails a binkw64.dll import (x64 plays movies through openswbf2-ffmpeg.dll and links no Bink)" }
    $n++
    $impAnchor = 'Section contains the following delay load imports:'
    $impDef = (Get-Command Get-ImportChecks).Definition
    $impHits = ([regex]::Matches($impDef, [regex]::Escape($impAnchor))).Count
    if ($impHits -ne 1) { $failed++; Write-Host "FAILED ${n}: delay-import mutant anchor found $impHits times in Get-ImportChecks" }
    else {
        $impMut = @(& {
            Set-Item -Path function:Get-ImportChecks -Value ([scriptblock]::Create($impDef.Replace($impAnchor, 'OPENSWBF2 NO SUCH DUMPBIN HEADER')))
            Get-ImportChecks $impDelay 'Win32' | Where-Object { $_.Name -eq 'import-delay-modules' }
        })
        if ($impMut.Count -ne 1 -or -not $impMut[0].Ok) { $failed++; Write-Host "FAILED ${n}: a Get-ImportChecks mutant that ignores the delay section still fails the delay fixture: ok=$($impMut.Ok) $($impMut.Detail)" }
        else { Write-Host "OK $n delay-import fixture discriminates: a Get-ImportChecks mutant that ignores the delay section passes import-delay-modules on it" }
    }
    $n++
    $ucDir = Join-Path $tmp 'unicows'
    $ucOut = Join-Path $ucDir 'out'
    $ucOutBad = Join-Path $ucDir 'outbad'
    $ucSrc = Join-Path $ucDir 'src'
    $ucSrcBad = Join-Path $ucDir 'srcbad'
    foreach ($d in $ucOut, $ucOutBad, $ucSrc, $ucSrcBad) { New-Item -ItemType Directory -Force -Path $d | Out-Null }
    Set-Content -LiteralPath (Join-Path $ucOutBad 'unicows.dll') -Value 'x' -Encoding ASCII
    Set-Content -LiteralPath (Join-Path $ucSrc 'a.cpp') -Value 'int a;' -Encoding ASCII
    Set-Content -LiteralPath (Join-Path $ucSrcBad 'a.cpp') -Value 'LoadLibraryA("unicows.dll");' -Encoding ASCII
    $ucShipped = [IO.File]::ReadAllText((Join-Path (Split-Path $PSScriptRoot -Parent) 'BattlefrontII.vcxproj'))
    $ucProj = '<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003"><Target Name="StageGameData"><ItemGroup><GameDataFile Include="$(SWBF2_GAMEDATA)\steam_api.dll" />{0}</ItemGroup><Copy SourceFiles="{1}" DestinationFolder="$(OutDir)" /><Delete Files="$(OutDir)unicows.dll" /></Target></Project>'
    $ucCopyGood = '@(GameDataFile)'
    $ucCases = @(
        @('the shipped project with a clean output, worker list, imports and src', $true, $ucOut, $ucShipped, $null, $impGood, $ucSrc, $null),
        @('a project with only the Delete of the stale copy', $true, $ucOut, ($ucProj -f '', $ucCopyGood), @('BattlefrontII.exe', 'steam_appid.txt'), $impGood, $ucSrc, $null),
        @('unicows.dll in the output folder', $false, $ucOutBad, $ucShipped, $null, $impGood, $ucSrc, 'staged in '),
        @('a GameDataFile item naming unicows', $false, $ucOut, ($ucProj -f '<GameDataFile Include="$(SWBF2_GAMEDATA)\unicows.dll" />', $ucCopyGood), $null, $impGood, $ucSrc, 'vcxproj GameDataFile'),
        @('a VendorFile item naming unicows', $false, $ucOut, ($ucProj -f '<VendorFile Include="$(SWBF2_VENDOR_X64)\unicows.dll" />', $ucCopyGood), $null, $impGood, $ucSrc, 'vcxproj VendorFile'),
        @('a Copy whose SourceFiles names unicows', $false, $ucOut, ($ucProj -f '', '$(SWBF2_GAMEDATA)\unicows.dll'), $null, $impGood, $ucSrc, 'vcxproj Copy'),
        @('unicows.dll in the worker list', $false, $ucOut, $ucShipped, @('BattlefrontII.exe', 'unicows.dll'), $impGood, $ucSrc, 'worker list'),
        @('unicows.dll in the static import section', $false, $ucOut, $ucShipped, $null, ($impGood + "`n    UNICOWS.dll`n                   0 GetModuleHandleW`n"), $ucSrc, 'static import'),
        @('unicows.dll in the delay import section', $false, $ucOut, $ucShipped, $null, ($impDelay.Replace('    steam_api.dll', '    unicows.dll')), $ucSrc, 'delay import'),
        @('a src file naming unicows', $false, $ucOut, $ucShipped, $null, $impGood, $ucSrcBad, 'named in ')
    )
    foreach ($uc in $ucCases) {
        $c = Get-UnicowsCheck '' $Platform $uc[2] $uc[5] $uc[3] $uc[4] $uc[6]
        $why = $null
        if ($c.Name -ne 'unicows-staged' -or $c.Ok -ne $uc[1]) { $why = "ok=$($c.Ok) $($c.Detail)" }
        elseif ($uc[7] -and $c.Detail -notlike "*$($uc[7])*") { $why = "detail does not name '$($uc[7])': $($c.Detail)" }
        elseif ($uc[1] -and $c.Detail) { $why = "passing case has detail: $($c.Detail)" }
        $verb = 'fails'
        if ($uc[1]) { $verb = 'passes' }
        if ($why) { $failed++; Write-Host "FAILED ${n}: unicows-staged $verb on $($uc[0]): $why" }
        else { Write-Host "OK $n unicows-staged $verb on $($uc[0])" }
        $n++
    }
    $dpRoot = Split-Path $PSScriptRoot -Parent
    $dpPins = [IO.File]::ReadAllText((Join-Path $dpRoot 'lib\vendor-pins.props'))
    $dpProj = [IO.File]::ReadAllText((Join-Path $dpRoot 'BattlefrontII.vcxproj'))
    $dpWin32Verify = '<VerifyFileHash Condition="''$(Platform)''==''Win32''" File="$(D3dxDll)" Algorithm="SHA256" Hash="$(D3dxWin32Sha256)" />'
    $dpX64Verify = '<VerifyFileHash Condition="''$(Platform)''==''x64''" File="$(D3dxDll)" Algorithm="SHA256" Hash="$(D3dxX64Sha256)" />'
    $dpNupkgVerify = '<VerifyFileHash File="$(D3dxNupkg)" Algorithm="SHA256" Hash="$(D3dxNupkgSha256)" />'
    $dpX64Pin = (Get-VendorPins $dpRoot)['D3dxX64Sha256']
    $dpCases = @(
        @('the shipped pins and project', $true, $dpPins, $dpProj, $null),
        @('a 63-digit DLL pin', $false, $dpPins.Replace($dpX64Pin, $dpX64Pin.Substring(1)), $dpProj, 'D3dxX64Sha256 is not 64 hex digits'),
        @('an upper-case nupkg pin', $false, $dpPins.Replace('ead0906ae8a26c18', 'EAD0906AE8A26C18'), $dpProj, 'D3dxNupkgSha256 is not 64 hex digits'),
        @('a project without the Win32 DLL VerifyFileHash', $false, $dpPins, $dpProj.Replace($dpWin32Verify, ''), 'against $(D3dxWin32Sha256)'),
        @('a project without the x64 DLL VerifyFileHash', $false, $dpPins, $dpProj.Replace($dpX64Verify, ''), 'against $(D3dxX64Sha256)'),
        @('a project whose cache VerifyFileHash is conditional', $false, $dpPins, $dpProj.Replace($dpNupkgVerify, $dpNupkgVerify.Replace('<VerifyFileHash ', '<VerifyFileHash Condition="false" ')), 'of $(D3dxNupkg) against'),
        @('a project that does not import the pins', $false, $dpPins, $dpProj.Replace('<Import Project="lib\vendor-pins.props" />', ''), 'does not import'),
        @('a project that extracts only a missing cached DLL', $false, $dpPins, $dpProj.Replace('<Unzip Condition="''$(D3dxDllHash)'' != ''$(D3dxDllSha256)''"', '<Unzip Condition="!Exists(''$(D3dxDll)'')"'), 'not conditioned on the cached DLL hash')
    )
    foreach ($dp in $dpCases) {
        $c = Get-D3dxPinsCheck $dpRoot $dp[2] $dp[3]
        $why = $null
        if ($dp[3] -ceq $dpProj -and $dp[2] -ceq $dpPins -and -not $dp[1]) { $why = 'fixture did not change its input' }
        elseif ($c.Name -ne 'd3dx9-pins' -or $c.Ok -ne $dp[1]) { $why = "ok=$($c.Ok) $($c.Detail)" }
        elseif ($dp[4] -and $c.Detail -notlike "*$($dp[4])*") { $why = "detail does not name '$($dp[4])': $($c.Detail)" }
        elseif ($dp[1] -and $c.Detail) { $why = "passing case has detail: $($c.Detail)" }
        $verb = $(if ($dp[1]) { 'passes' } else { 'fails' })
        if ($why) { $failed++; Write-Host "FAILED ${n}: d3dx9-pins $verb on $($dp[0]): $why" }
        else { Write-Host "OK $n d3dx9-pins $verb on $($dp[0])" }
        $n++
    }
    $fmHeap = @('Startup_RedInitHeap:', '  0000000140355560: sub         rsp,48h', '  0000000140355573: call        ?Install@pcCrash@@YAXXZ', '  0000000140355578: call        ?PcMathStartup@@YAXXZ', '  000000014035557D: lea         rcx,[??_C@_08DCLPFPD@heapinit@]')
    $fmMath = @('?PcMathStartup@@YAXXZ:', '  0000000140026D60: xor         ecx,ecx', '  0000000140026D62: jmp         qword ptr [__imp__set_FMA3_enable]', '  0000000140026D69: CC CC CC CC CC CC CC                             IIIIIII')
    $fmText = { param($h, $m) (@('Dump of file x.exe', '', 'File Type: EXECUTABLE IMAGE', '') + $h + @('', '  Summary', '      4BF000 .text', 'Dump of file x.exe') + $m + @('?Next@@YAXXZ:', '  0000000140026D70: call        qword ptr [__imp__set_FMA3_enable]')) -join "`r`n" }
    $fmCases = @(
        @('the shipped tail jump with ecx zeroed', $true, $fmHeap, $fmMath, $null),
        @('a direct call through the import thunk after mov ecx,0', $true, $fmHeap, @($fmMath[0], '  0000000140026D60: sub         rsp,28h', '  0000000140026D64: mov         ecx,0', '  0000000140026D69: call        _set_FMA3_enable', '  0000000140026D6E: add         rsp,28h', '  0000000140026D72: ret'), $null),
        @('argument 1', $false, $fmHeap, @($fmMath[0], '  0000000140026D60: mov         ecx,1', $fmMath[2]), 'does not pass 0'),
        @('ecx set to 0 then overwritten', $false, $fmHeap, @($fmMath[0], $fmMath[1], '  0000000140026D62: mov         ecx,eax', $fmMath[2]), 'does not pass 0'),
        @('ecx set to 0 before another call', $false, $fmHeap, @($fmMath[0], $fmMath[1], '  0000000140026D62: call        ?Other@@YAXXZ', $fmMath[2]), 'does not pass 0'),
        @('no call to _set_FMA3_enable', $false, $fmHeap, @($fmMath[0], $fmMath[1], '  0000000140026D62: ret'), 'does not call _set_FMA3_enable'),
        @('another function called from Startup_RedInitHeap', $false, @($fmHeap[0], $fmHeap[1], $fmHeap[2], '  0000000140355578: call        ?PcMathStartupX@@YAXXZ'), $fmMath, 'does not call PcMathStartup'),
        @('no PcMathStartup call', $false, @($fmHeap[0], $fmHeap[1], $fmHeap[2], $fmHeap[4]), $fmMath, 'does not call PcMathStartup'),
        @('no Startup_RedInitHeap', $false, @(), $fmMath, 'Startup_RedInitHeap not found'),
        @('no PcMathStartup', $false, $fmHeap, @(), 'PcMathStartup not found')
    )
    foreach ($fm in $fmCases) {
        $c = Get-Fma3OffCheck (& $fmText $fm[2] $fm[3])
        $why = $null
        if ($c.Name -ne 'fma3-off' -or $c.Ok -ne $fm[1]) { $why = "ok=$($c.Ok) $($c.Detail)" }
        elseif ($fm[4] -and $c.Detail -notlike "*$($fm[4])*") { $why = "detail does not name '$($fm[4])': $($c.Detail)" }
        elseif ($fm[1] -and $c.Detail) { $why = "passing case has detail: $($c.Detail)" }
        $verb = $(if ($fm[1]) { 'passes' } else { 'fails' })
        if ($why) { $failed++; Write-Host "FAILED ${n}: fma3-off $verb on $($fm[0]): $why" }
        else { Write-Host "OK $n fma3-off $verb on $($fm[0])" }
        $n++
    }
    $dsDir = Join-Path $tmp 'd3dx9-staged'
    $dsGoodBytes = [Text.Encoding]::ASCII.GetBytes('good')
    $dsSha = [Security.Cryptography.SHA256]::Create()
    $dsGoodPin = ([BitConverter]::ToString($dsSha.ComputeHash($dsGoodBytes)) -replace '-', '').ToLowerInvariant()
    $dsSha.Dispose()
    $dsPins = $dpPins.Replace($dpX64Pin, $dsGoodPin)
    $dsCases = @(
        @('a staged folder whose copy matches the pin', 'PASS', $true, 'good', $null),
        @('a staged folder whose copy has another hash', 'FAIL', $true, 'bad', 'pin is'),
        @('a staged folder without the copy', 'FAIL', $true, $null, 'missing'),
        @('an output folder that was not staged', 'SKIP', $false, $null, 'not staged')
    )
    $dsI = 0
    foreach ($ds in $dsCases) {
        $dsI++
        $d = Join-Path $dsDir "c$dsI"
        New-Item -ItemType Directory -Force -Path $d | Out-Null
        if ($ds[2]) { New-Item -ItemType Directory -Force -Path (Join-Path $d 'data') | Out-Null }
        if ($ds[3]) { [IO.File]::WriteAllBytes((Join-Path $d 'd3dx9_43.dll'), [Text.Encoding]::ASCII.GetBytes($ds[3])) }
        $c = Get-D3dxStagedCheck $dpRoot 'x64' $d $dsPins
        $line = Get-D3dxStagedLine $c
        $why = $null
        if ($c.Status -ne $ds[1]) { $why = "status $($c.Status): $line" }
        elseif ($ds[4] -and $line -notlike "*$($ds[4])*") { $why = "line does not name '$($ds[4])': $line" }
        elseif ($ds[1] -eq 'PASS' -and $line -cne '[PASS] vendor:d3dx9-staged') { $why = "unexpected line: $line" }
        elseif ($ds[1] -eq 'SKIP' -and $line -cne '[SKIP] vendor:d3dx9-staged - not staged') { $why = "unexpected line: $line" }
        if ($why) { $failed++; Write-Host "FAILED ${n}: vendor:d3dx9-staged is $($ds[1]) on $($ds[0]): $why" }
        else { Write-Host "OK $n vendor:d3dx9-staged is $($ds[1]) on $($ds[0])" }
        $n++
    }
    $n++
    $c = Get-D3dxStagedCheck $dpRoot 'Win32' (Join-Path $dsDir 'c1') $dsPins
    if ($c.Status -ne 'FAIL') { $failed++; Write-Host "FAILED ${n}: vendor:d3dx9-staged on Win32 judged an x64-pinned copy $($c.Status)" }
    else { Write-Host "OK $n vendor:d3dx9-staged judges each platform against its own pin" }
    $n++
    $spPin = (Get-VendorPins $dpRoot)['SteamApi64Sha256']
    $spVerify = [regex]::Match($dpProj, '<VerifyFileHash [^>]*File="\$\(SWBF2_VENDOR_X64\)\\steam_api64\.dll"[^>]*/>').Value
    $spCopy = [regex]::Match($dpProj, '<Copy [^>]*SourceFiles="\$\(SWBF2_VENDOR_X64\)\\steam_api64\.dll"[^>]*/>').Value
    $spCases = @(
        @('the shipped pin and project', $true, $dpPins, $dpProj, $null),
        @('a 63-digit steam_api64 pin', $false, $dpPins.Replace($spPin, $spPin.Substring(1)), $dpProj, 'SteamApi64Sha256 is not 64 hex digits'),
        @('a Copy without a VerifyFileHash', $false, $dpPins, $dpProj.Replace($spVerify, ''), 'without a VerifyFileHash'),
        @('a VerifyFileHash after the Copy', $false, $dpPins, $dpProj.Replace($spVerify + "`r`n    " + $spCopy, $spCopy + "`r`n    " + $spVerify), 'without a VerifyFileHash'),
        @('a VerifyFileHash with another condition', $false, $dpPins, $dpProj.Replace($spVerify, $spVerify.Replace('<VerifyFileHash Condition="', '<VerifyFileHash Condition="false and ')), 'without a VerifyFileHash'),
        @('a project that never copies steam_api64.dll', $false, $dpPins, $dpProj.Replace($spCopy, ''), 'has no Copy')
    )
    foreach ($sp in $spCases) {
        $c = Get-SteamApi64PinsCheck $dpRoot $sp[2] $sp[3]
        $why = $null
        if (-not $spVerify -or -not $spCopy) { $why = 'StageVendor steam_api64 steps not found in BattlefrontII.vcxproj' }
        elseif ($sp[3] -ceq $dpProj -and $sp[2] -ceq $dpPins -and -not $sp[1]) { $why = 'fixture did not change its input' }
        elseif ($c.Name -ne 'steam-api64-pins' -or $c.Ok -ne $sp[1]) { $why = "ok=$($c.Ok) $($c.Detail)" }
        elseif ($sp[4] -and $c.Detail -notlike "*$($sp[4])*") { $why = "detail does not name '$($sp[4])': $($c.Detail)" }
        elseif ($sp[1] -and $c.Detail) { $why = "passing case has detail: $($c.Detail)" }
        $verb = $(if ($sp[1]) { 'passes' } else { 'fails' })
        if ($why) { $failed++; Write-Host "FAILED ${n}: steam-api64-pins $verb on $($sp[0]): $why" }
        else { Write-Host "OK $n steam-api64-pins $verb on $($sp[0])" }
        $n++
    }
    $saNames = @('SteamAPI_Init', 'SteamAPI_RestartAppIfNecessary', 'SteamAPI_Shutdown', 'SteamAPI_RunCallbacks', 'SteamAPI_RegisterCallResult', 'SteamAPI_UnregisterCallResult', 'SteamAPI_GetHSteamPipe', 'SteamAPI_GetHSteamUser', 'SteamInternal_ContextInit', 'SteamInternal_CreateInterface')
    $saDef = "LIBRARY steam_api64.dll`r`nEXPORTS`r`n" + (($saNames | ForEach-Object { "    $_" }) -join "`r`n") + "`r`n"
    $saImp = "  Section contains the following imports:`n`n    KERNEL32.dll`n             140123456 Import Address Table`n                         0 SteamAPI_NotDelayed`n`n  Section contains the following delay load imports:`n`n    steam_api64.dll`n              00000001 Characteristics`n      00000001421D9F80 Address of HMODULE`n                     0 time date stamp`n`n" + (($saNames | ForEach-Object { "                                    000000014048319F     3 $_" }) -join "`n") + "`n`n    Galaxy64.dll`n              00000001 Characteristics`n`n                                    0000000140483300     0 Galaxy_NotInSteam`n"
    $saExp = "    ordinal hint RVA      name`n`n" + (($saNames | ForEach-Object { "          1    0 00001200 $_" }) -join "`n") + "`n          9    8 00001300 SteamAPI_Other`n"
    $saDir = Join-Path $tmp 'steam-api64-staged'
    $saGoodBytes = [Text.Encoding]::ASCII.GetBytes('good')
    $saSha = [Security.Cryptography.SHA256]::Create()
    $saGoodPin = ([BitConverter]::ToString($saSha.ComputeHash($saGoodBytes)) -replace '-', '').ToLowerInvariant()
    $saSha.Dispose()
    $saPins = $dpPins.Replace($spPin, $saGoodPin)
    $saCases = @(
        @('a staged copy with the pin and every export', 'PASS', 'good', $saExp, $saImp, $saDef, $null),
        @('a staged copy with another hash', 'FAIL', 'bad', $saExp, $saImp, $saDef, 'pin is'),
        @('exports without SteamAPI_RunCallbacks', 'FAIL', 'good', ($saExp -replace '(?m)^.* SteamAPI_RunCallbacks\r?\n', ''), $saImp, $saDef, 'SteamAPI_RunCallbacks imported by the exe is not exported, SteamAPI_RunCallbacks in steam_api64.def'),
        @('a .def name the DLL does not export', 'FAIL', 'good', $saExp, $saImp, ($saDef + "    SteamAPI_Missing`r`n"), 'SteamAPI_Missing in steam_api64.def is not exported'),
        @('an exe without a steam_api64.dll delay import', 'FAIL', 'good', $saExp, $saImp.Replace('    steam_api64.dll', '    other64.dll'), $saDef, 'no steam_api64.dll delay import'),
        @('an output folder without the copy', 'SKIP', $null, $saExp, $saImp, $saDef, 'not staged')
    )
    $saI = 0
    foreach ($sa in $saCases) {
        $saI++
        $d = Join-Path $saDir "c$saI"
        New-Item -ItemType Directory -Force -Path $d | Out-Null
        if ($sa[2]) { [IO.File]::WriteAllBytes((Join-Path $d 'steam_api64.dll'), [Text.Encoding]::ASCII.GetBytes($sa[2])) }
        $c = Get-SteamApi64StagedCheck $dpRoot $d $sa[4] $sa[3] $saPins $sa[5]
        $line = Get-D3dxStagedLine (New-Object PSObject -Property @{ Status = $c.Status; Name = "hardening:$($c.Name)"; Detail = $c.Detail })
        $why = $null
        if ($c.Status -ne $sa[1]) { $why = "status $($c.Status): $line" }
        elseif ($sa[6] -and $line -notlike "*$($sa[6])*") { $why = "line does not name '$($sa[6])': $line" }
        elseif ($sa[1] -eq 'PASS' -and $line -cne '[PASS] hardening:steam-api64-staged') { $why = "unexpected line: $line" }
        elseif ($sa[1] -eq 'SKIP' -and $line -cne '[SKIP] hardening:steam-api64-staged - not staged') { $why = "unexpected line: $line" }
        if ($why) { $failed++; Write-Host "FAILED ${n}: steam-api64-staged is $($sa[1]) on $($sa[0]): $why" }
        else { Write-Host "OK $n steam-api64-staged is $($sa[1]) on $($sa[0])" }
        $n++
    }
    $sdDir = Join-Path $env:TEMP ("openswbf2-steamid-" + [guid]::NewGuid().ToString('N'))
    try {
        foreach ($d in 'src\Steam', 'src\Foo') { New-Item -ItemType Directory -Path (Join-Path $sdDir $d) | Out-Null }
        Set-Content -Encoding ASCII -Path (Join-Path $sdDir 'src\Steam\steamworks.h') -Value @('class CSteamID;', 'class ISteamUser', '{', '    virtual CSteamID GetSteamID() = 0;', '    virtual void EndAuthSession(CSteamID) = 0;', '};')
        Set-Content -Encoding ASCII -Path (Join-Path $sdDir 'src\Foo\a.cpp') -Value @('void A(ISteamUser* pUser) { pUser->GetSteamID(); }', 'void B(ISteamUser& k) { k.EndAuthSession(x); }', 'void C(CSteamID kId);', 'class CSteamID;', 'void D(const CSteamID* pId, CSteamID& kId);', 'void E() { pUser->BLoggedOn(); }', 'int F() { return ISteamUser::AdvertiseGame (a, b, c); }')
        $bad = @(Get-SourceChecks $sdDir -Only 'steamid-abi')
        Remove-Item -LiteralPath (Join-Path $sdDir 'src\Foo\a.cpp')
        $good = @(Get-SourceChecks $sdDir -Only 'steamid-abi')
        $sdNote = '(settle the x64 CSteamID hidden-return and by-value convention first)'
        $sdWant = (@('src\Foo\a.cpp:1', 'src\Foo\a.cpp:2', 'src\Foo\a.cpp:3', 'src\Foo\a.cpp:7') | ForEach-Object { "$_ $sdNote" }) -join ', '
        if ($bad.Count -ne 1 -or $bad[0].Ok -or $bad[0].Detail -ne $sdWant) { $failed++; Write-Host "FAILED ${n}: steamid-abi flags CSteamID calls and by-value uses outside steamworks.h: $($bad.Detail)" }
        elseif ($good.Count -ne 1 -or -not $good[0].Ok) { $failed++; Write-Host "FAILED ${n}: steamid-abi flags CSteamID calls and by-value uses outside steamworks.h: steamworks.h declarations flagged $($good.Detail)" }
        else { Write-Host "OK $n steamid-abi flags CSteamID calls and by-value uses outside steamworks.h" }
    }
    finally {
        Remove-Item -Recurse -Force $sdDir -ErrorAction SilentlyContinue
    }
    $n++
    $bgRoot = Join-Path $env:TEMP ('openswbf2-binkgone-' + [guid]::NewGuid().ToString('N'))
    $bgProj = '<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003"><ItemDefinitionGroup><Link><AdditionalDependencies>kernel32.lib;{0}</AdditionalDependencies></Link></ItemDefinitionGroup><ItemGroup>{1}</ItemGroup><Target Name="StageVendor"><Delete Files="$(OutDir)unicows.dll" />{2}</Target></Project>'
    $bgDelete = '<Delete Files="$(OutDir)binkw32.dll" />'
    $bgWorkers = @('BattlefrontII.exe', 'openswbf2-ffmpeg.dll')
    $bgCases = @(
        @('a clean tree', $true, @{}, $false, $impGood, $bgWorkers, $null),
        @('a FFmpegFL source calling BinkOpen and a game source with PixelBink, IsBinkRevision and bink in lower case', $true, @{ 'src\FFmpegFL\libavcodec\x.c' = 'BinkOpen(p);'; 'src\Game\a.cpp' = 'PixelBink(1); IsBinkRevision(c); av_find_input_format("bink");' }, $false, $impGood, $bgWorkers, $null),
        @('binkw32.dll in the static import section', $false, @{}, $false, ($impGood + "`n    binkw32.dll`n                   0 _BinkOpen@8`n"), $bgWorkers, 'static import binkw32.dll'),
        @('binkw32.dll in the delay import section', $false, @{}, $false, ($impDelay.Replace('    steam_api.dll', '    binkw32.dll')), $bgWorkers, 'delay import binkw32.dll'),
        @('binkw32.dll in the output folder', $false, @{}, $true, $impGood, $bgWorkers, 'staged in '),
        @('lib\Win32\binkw32.lib', $false, @{ 'lib\Win32\binkw32.lib' = 'x' }, $false, $impGood, $bgWorkers, 'lib file lib\Win32\binkw32.lib'),
        @('binkw32.lib in AdditionalDependencies', $false, @{ 'BattlefrontII.vcxproj' = ($bgProj -f 'lib\Win32\binkw32.lib', '', $bgDelete) }, $false, $impGood, $bgWorkers, 'BattlefrontII.vcxproj AdditionalDependencies'),
        @('a GameDataFile staging binkw32.dll', $false, @{ 'BattlefrontII.vcxproj' = ($bgProj -f '', '<GameDataFile Include="$(SWBF2_GAMEDATA)\binkw32.dll" />', $bgDelete) }, $false, $impGood, $bgWorkers, 'BattlefrontII.vcxproj GameDataFile'),
        @('a library project linking binkw32', $false, @{ 'src\Game\Game.vcxproj' = ($bgProj -f 'binkw32.lib', '', '') }, $false, $impGood, $bgWorkers, 'src\Game\Game.vcxproj AdditionalDependencies'),
        @('a StageVendor without the binkw32.dll Delete', $false, @{ 'BattlefrontII.vcxproj' = ($bgProj -f '', '', '') }, $false, $impGood, $bgWorkers, 'StageVendor does not delete a stale binkw32.dll'),
        @('binkw32 named in tests\game\x.cpp', $false, @{ 'tests\game\x.cpp' = 'const char* p = "binkw32.dll";' }, $false, $impGood, $bgWorkers, 'named in tests\game\x.cpp'),
        @('binkw32 named without the extension in tests\harness\h.cpp', $false, @{ 'tests\harness\h.cpp' = 'const char* p = "binkw32";' }, $false, $impGood, $bgWorkers, 'named in tests\harness\h.cpp'),
        @('an allowed section label in tests\game\x.cpp', $true, @{ 'tests\game\x.cpp' = 'Find(sTable, "binkw32 ingame digest");' }, $false, $impGood, $bgWorkers, '', @('tests\game\x.cpp|binkw32 ingame digest|a table label')),
        @('a section label with no allow entry', $false, @{ 'tests\game\x.cpp' = 'Find(sTable, "binkw32 ingame digest");' }, $false, $impGood, $bgWorkers, 'named in tests\game\x.cpp'),
        @('an allowed section label beside a binkw32.dll name', $false, @{ 'tests\game\x.cpp' = 'Find(sTable, "binkw32 ingame digest"); Load("binkw32.dll");' }, $false, $impGood, $bgWorkers, 'named in tests\game\x.cpp', @('tests\game\x.cpp|binkw32 ingame digest|a table label')),
        @('an allow entry for another file', $false, @{ 'tests\game\x.cpp' = 'Find(sTable, "binkw32 ingame digest");' }, $false, $impGood, $bgWorkers, 'named in tests\game\x.cpp', @('tests\game\y.cpp|binkw32 ingame digest|a table label')),
        @('an unused allow entry', $false, @{}, $false, $impGood, $bgWorkers, 'unused allow tests\game\x.cpp|binkw32 ingame digest|a table label', @('tests\game\x.cpp|binkw32 ingame digest|a table label')),
        @('an allow entry without a reason', $false, @{ 'tests\game\x.cpp' = 'Find(sTable, "binkw32 ingame digest");' }, $false, $impGood, $bgWorkers, 'named in tests\game\x.cpp', @('tests\game\x.cpp|binkw32 ingame digest')),
        @('binkw32 named in tests\common.ps1', $false, @{ 'tests\common.ps1' = '$a = @(''BINKW32.DLL'')' }, $false, $impGood, $bgWorkers, 'named in tests\common.ps1'),
        @('binkw32.dll in the worker list', $false, @{}, $false, $impGood, @('BattlefrontII.exe', 'binkw32.dll'), 'worker list'),
        @('a game source calling BinkOpen', $false, @{ 'src\Game\a.cpp' = "int a;`r`nvoid f() { BinkOpen(0, 0); }" }, $false, $impGood, $bgWorkers, 'source src\Game\a.cpp:2'),
        @('a test source using BINK', $false, @{ 'tests\unit\b.cpp' = 'static_assert(sizeof(BINK) == 0xd4, "");' }, $false, $impGood, $bgWorkers, 'source tests\unit\b.cpp:1'),
        @('a game header declaring BINKIO', $false, @{ 'src\Game\a.h' = 'struct BINKIO;' }, $false, $impGood, $bgWorkers, 'source src\Game\a.h:1'),
        @('a game header declaring BINKFILE', $false, @{ 'src\Game\a.h' = 'struct BINKFILE;' }, $false, $impGood, $bgWorkers, 'source src\Game\a.h:1'),
        @('a game source loading binkw32.dll at run time', $false, @{ 'src\Game\a.cpp' = "int a;`r`nvoid f() { LoadLibraryA(`"binkw32.dll`"); }" }, $false, $impGood, $bgWorkers, 'source src\Game\a.cpp:2'),
        @('a game source naming BinkW64 without the extension', $false, @{ 'src\Game\a.cpp' = 'const char* p = "BinkW64";' }, $false, $impGood, $bgWorkers, 'source src\Game\a.cpp:1'),
        @('a test source naming the binkw32 golden section', $true, @{ 'tests\unit\b.cpp' = 'const char* s = "binkw32 frames";' }, $false, $impGood, $bgWorkers, '')
    )
    $why = @()
    try {
        $k = 0
        foreach ($bg in $bgCases) {
            $k++
            $d = Join-Path $bgRoot "t$k"
            $files = @{ 'BattlefrontII.vcxproj' = ($bgProj -f '', '', $bgDelete); 'src\Game\a.cpp' = 'int a;'; 'tests\unit\b.cpp' = 'int b;'; 'tests\game\x.cpp' = 'int x;'; 'tests\common.ps1' = '$b = 1' }
            foreach ($f in $bg[2].Keys) { $files[$f] = $bg[2][$f] }
            foreach ($f in $files.Keys) {
                $p = Join-Path $d $f
                New-Item -ItemType Directory -Force -Path (Split-Path $p) | Out-Null
                [IO.File]::WriteAllText($p, $files[$f])
            }
            New-Item -ItemType Directory -Force -Path (Join-Path $d 'lib\Win32'), (Join-Path $d 'out') | Out-Null
            if ($bg[3]) { [IO.File]::WriteAllText((Join-Path $d 'out\binkw32.dll'), 'x') }
            $c = Get-BinkGoneCheck $d (Join-Path $d 'out') $bg[4] $bg[5] $(if ($bg.Count -gt 7) { $bg[7] } else { @() })
            if ($c.Name -ne 'bink-gone' -or $c.Ok -ne $bg[1]) { $why += "$($bg[0]): ok=$($c.Ok) $($c.Detail)" }
            elseif ($bg[1] -and $c.Detail) { $why += "$($bg[0]): passing case has detail $($c.Detail)" }
            elseif ($bg[6] -and $c.Detail -cne $bg[6] -and -not $c.Detail.StartsWith($bg[6])) { $why += "$($bg[0]): detail '$($c.Detail)' does not name '$($bg[6])'" }
        }
    }
    catch { $why += "threw: $($_.Exception.Message)" }
    finally { Remove-Item -LiteralPath $bgRoot -Recurse -Force -ErrorAction SilentlyContinue }
    if ($why) { $failed++; Write-Host "FAILED ${n}: bink-gone: $($why -join ' | ')" }
    else { Write-Host "OK $n bink-gone passes a clean tree and Bink names inside FFmpegFL or not at a word start; it fails a binkw32 static or delay import, a staged binkw32.dll, lib\Win32\binkw32.lib, a project element naming binkw32, a StageVendor without its Delete, binkw32 in tests\game, tests\harness outside an allowed label, common.ps1 or the worker list, a Bink call or BINK, BINKIO or BINKFILE in game or test source, and a binkw32 or binkw64 name in game source; a test source may name the archived binkw32 golden sections" }
}
finally {
    Remove-Item -Recurse -Force $tmp -ErrorAction SilentlyContinue
}
if ($failed) { Write-Host "$failed of $n hardening fixtures failed"; exit 1 }
Write-Host "All $n hardening fixtures passed"
exit 0
