$nw = 'src\Battlefront2\Source\NetWire.cpp'
$np = 'src\Battlefront2\Source\NetPacket.cpp'
$nc = 'src\Battlefront2\Source\NetComm.cpp'
$ll = 'src\Battlefront2\Source\LocalLimits.h'
$pm = 'src\RedEngineFL\Graphics\PC\pcRedPoolMath.h'
$mf = 'src\Battlefront2\Source\MemFailLog.cpp'
$lm = 'src\Battlefront2\Source\LuaMemory.cpp'
$pmain = 'src\Battlefront2\Source\pcMain.cpp'
$rl = 'src\RedEngineFL\Util\RedLog.cpp'
$pp = 'src\PebbleFL\Common\PblPtrOrder.h'
$th = 'src\Battlefront2\Source\Thread.cpp'
$cm = 'src\Battlefront2\Source\CollisionManager.cpp'
$rm = 'src\RedEngineFL\Memory\RedMemory.cpp'
$ds = 'src\RedEngineFL\Graphics\PC\pcDisplaySettings.cpp'
$dl = 'src\RedEngineFL\Graphics\PC\pcDisplayLogic.cpp'
$fl = 'src\RedEngineFL\Application\pc\pcFrameLimiter.cpp'
$md = 'src\RedEngineFL\Application\pc\pcMessageDrain.cpp'
$rr = 'src\RedEngineFL\Graphics\PC\pcRedRenderer.cpp'

$GuardRevertRows = @(
    @{ Id = 'CRASH-02'; File = 'src\RedEngineFL\Util\RedWarning.cpp'; Anchor = 'fputs(pcText, pFile);'; Replacement = 'fprintf(pFile, pcText);'; Tests = @('file log writes percent text literally') },
    @{ Id = 'NETF-01'; File = $nc; Anchor = 'if (uCount == 0 || uFragment >= uCount || uCount > kMaxFragments)'; Replacement = 'if (false)'; Tests = @('recvgroup rejects fragment idx ge count', 'recvgroup rejects count zero', 'recvgroup rejects count over max') },
    @{ Id = 'NETF-01B'; File = $nc; Anchor = 'if (NetComm::Header(NetPacket::Get(pPartial->firstPkt)).count != uCount)'; Replacement = 'if (false)'; Tests = @('recvgroup count mismatch frees partial and packet') },
    @{ Id = 'NETF-01C'; File = $nc; Anchor = 'if (iFromSource >= kMaxPartialPerSource)'; Replacement = 'if (false)'; Tests = @('recvgroup evicts oldest partial at per-source cap') },
    @{ Id = 'NETF-02'; File = $np; Anchor = 'if (freePackets.pktCount <= (bSmall ? kRecvReserveSmall : kRecvReserveBig))'; Replacement = 'if (false)'; Tests = @('alloc recv keeps the big reserve', 'alloc recv keeps the small reserve') },
    @{ Id = 'NETF-02B'; File = $np; Anchor = 'pcCrash::Exit(3);'; Replacement = ''; Tests = @('alloc exhaustion is fatal and does not recycle') },
    @{ Id = 'NETF-03'; File = $np; Anchor = 'SetBad(NET_REJECT_NEGATIVE_LENGTH);'; Replacement = ''; Tests = @('group read bytes negative length keeps cursor and marks bad') },
    @{ Id = 'NETF-04'; File = $np; Anchor = "    if (bad)`n    {`n        if (iLen > 0)"; Replacement = "    if (false)`n    {`n        if (iLen > 0)"; Tests = @('group bad discards later readers') },
    @{ Id = 'NETF-05'; File = $ll; Anchor = "    pcDst[uiIndex] = '\0';"; Replacement = ''; Tests = @('lobby copy helper exact fit is terminated', 'lobby copy helper truncates host name to 64') },
    @{ Id = 'NETR-01'; File = $nw; Anchor = 'const unsigned int uCopy = uSize < uCap ? uSize : uCap;'; Replacement = 'const unsigned int uCopy = uSize;'; Tests = @('chat payload over cap is truncated and the group stays in sync') },
    @{ Id = 'NETR-01B'; File = $nw; Anchor = 'if (iPlayerID < 0 || iPlayerID > iMaxID || iCode < -1 || iCode > 7)'; Replacement = 'if (false)'; Tests = @('chat fields out of range are rejected') },
    @{ Id = 'NETR-02'; File = $nw; Anchor = 'bOk = IsNameChar(pcName[uLen], true);'; Replacement = 'bOk = true;'; Tests = @('upload names outside the allowlist are unsafe') },
    @{ Id = 'NETR-02B'; File = $nw; Anchor = 'bOk = IsNameChar(pcDir[i], false);'; Replacement = 'bOk = true;'; Tests = @('upload dirs outside the allowlist are unsafe') },
    @{ Id = 'NETR-02C'; File = $nw; Anchor = 'const unsigned int uCopy = uLength < uCap ? uLength : uCap;'; Replacement = 'const unsigned int uCopy = uLength;'; Tests = @('upload chunk longer than the buffer is rejected and fully consumed') },
    @{ Id = 'NETR-02D'; File = $nw; Anchor = 'if (uLength > uSizeLeft)'; Replacement = 'if (false)'; Tests = @('upload chunk longer than the remaining size is rejected') },
    @{ Id = 'NETR-03'; File = $ll; Anchor = 'const bool kUploadsDefault = false;'; Replacement = 'const bool kUploadsDefault = true;'; Tests = @('uploads are off by default') },
    @{ Id = 'NETR-03B'; File = $ll; Anchor = 'return bSwitch;'; Replacement = 'return true;'; Tests = @('uploads allowed follows the switch') },
    @{ Id = 'NETR-04'; File = $nw; Anchor = 'if (uCount > kRconLineMax)'; Replacement = 'if (false)'; Tests = @('rcon line is capped at 63 characters') },
    @{ Id = 'NETR-04B'; File = $nw; Anchor = 'if (uCount > uAvail)'; Replacement = 'if (false)'; Tests = @('rcon line is capped to the bytes left in the packet') },
    @{ Id = 'NETR-04C'; File = $nw; Anchor = 'if (iAvail < 8)'; Replacement = 'if (false)'; Tests = @('rcon auth needs eight bytes for J and paren but seven for s') },
    @{ Id = 'NETR-05'; File = $nw; Anchor = 'if (entry.iCount >= 5)'; Replacement = 'if (false)'; Tests = @('login limiter allows five attempts per ten seconds') },
    @{ Id = 'NETR-06'; File = $nw; Anchor = 'int iCount = uWire > kMaxPlayers ? (int)kMaxPlayers : (int)uWire;'; Replacement = 'int iCount = (int)uWire;'; Tests = @('player count is clamped to 1 through 64') },
    @{ Id = 'NETR-07'; File = $nw; Anchor = 'if (iTurns < 1 || iTurns > 7 || iDuration < 1 || iDuration > 255 || iDuration / iTurns < 1)'; Replacement = 'if (false)'; Tests = @('turn rate outside the retail range is rejected') },
    @{ Id = 'NETR-08'; File = $np; Anchor = 'fNow - sRejectLastLog[iReason] >= 1.0f'; Replacement = 'fNow - sRejectLastLog[iReason] >= 0.0f'; Tests = @('reject flood logs once per second with a suppressed count') },
    @{ Id = 'NETR-09'; File = $ll; Anchor = 'return iClientOf == 1 || iClientOf == 2;'; Replacement = 'return true;'; Tests = @('client of index accepts only one and two') },
    @{ Id = 'LOCAL-01'; File = $ll; Anchor = 'return iForced < kMaxPosts ? iForced : -1;'; Replacement = 'return iForced;'; Tests = @('command post index') },
    @{ Id = 'LOCAL-02'; File = $ll; Anchor = 'return uiCount < (unsigned int)kMaxNdObjects;'; Replacement = 'return true;'; Tests = @('net object cap') },
    @{ Id = 'LOCAL-02B'; File = $ll; Anchor = 'return (unsigned int)(iUniqueId - kNdIdBase) < (unsigned int)kMaxNdObjects;'; Replacement = 'return true;'; Tests = @('net object cap') },
    @{ Id = 'LOCAL-03'; File = $ll; Anchor = 'iCount >= 0 && iCount < kMaxTeamMembers'; Replacement = 'iCount >= 0'; Tests = @('team slot bounds') },
    @{ Id = 'LOCAL-04'; File = $pm; Anchor = 'if (uiOffset + uiCount >= uiPoolSize)'; Replacement = 'if (false)'; Tests = @('bone matrix pool') },
    @{ Id = 'LOCAL-04B'; File = $pm; Anchor = 'if (uiCount >= uiPoolSize)'; Replacement = 'if (false)'; Tests = @('bone matrix pool') },
    @{ Id = 'LOCAL-05'; File = $pm; Anchor = 'return uiUsed < uiPoolSize;'; Replacement = 'return true;'; Tests = @('render item pool') },
    @{ Id = 'MEM-02'; File = $mf; Anchor = 'if (siCount < 16)'; Replacement = 'if (true)'; Tests = @('alloc failure handler logs and suppresses') },
    @{ Id = 'MEM-02B'; File = $mf; Anchor = 'bool bValid = iHeap >= 0 && iHeap < maxheaps;'; Replacement = 'bool bValid = true;'; Tests = @('alloc failure handler tolerates bad heap id') },
    @{ Id = 'MEM-02C'; File = $mf; Anchor = '    sbInHandler = true;'; Replacement = ''; Tests = @('alloc failure handler ignores re-entry') },
    @{ Id = 'MEM-03'; File = $lm; Anchor = "        return 0;`n    }`n`n    if (pBlock)"; Replacement = "        Lua_Red_Free(pBlock, uiOldSize);`n        return 0;`n    }`n`n    if (pBlock)"; Tests = @('lua failed shrink keeps the old block') },
    @{ Id = 'MEM-03B'; File = 'src\LuaFL\src\lmem.cpp'; Anchor = 'luaD_throw(L, LUA_ERRMEM);'; Replacement = 'return NULL;'; Tests = @('lua oom is raised as LUA_ERRMEM') },
    @{ Id = 'MEM-04'; File = 'src\LuaFL\src\ldo.cpp'; Anchor = 'if (lj.status) RedSetCurrentHeap(iHeap);'; Replacement = ''; Tests = @('lua error restores the current heap') },
    @{ Id = 'MEM-04B'; File = $lm; Anchor = 'pcCrash::Exit(4);'; Replacement = 'pcCrash::Exit(0);'; Tests = @('lua panic handler exits with code 4') },
    @{ Id = 'STRLCPY-NULL'; File = 'src\PebbleFL\Common\PblUnicode.cpp'; Anchor = 'if (uiSize && pcSource)'; Replacement = 'if (uiSize)'; Tests = @('strlcpy treats a null source as empty') },
    @{ Id = 'DIAG-02'; File = $rl; Anchor = 'if (bLogSite)'; Replacement = 'if (bLogSite || pSite)'; Tests = @('redlog ensure counts once per site') },
    @{ Id = 'DIAG-03'; File = 'tests\main.cpp'; Anchor = "TestStrictnessReport(gCurrentCase) != 0)`n    {`n        return 1;"; Replacement = "TestStrictnessReport(gCurrentCase) != 0)`n    {`n        return iRc;"; Tests = @('strictness probe emits an undeclared error', 'strictness probe unmatched expectation') },
    @{ Id = 'DIAG-05'; File = $mf; Anchor = 'free %u load %u%% phys %u MB virt %u MB"'; Replacement = 'free %u"'; Tests = @('alloc failure handler logs and suppresses') },
    @{ Id = 'MEM-01'; Kind = 'Smoke'; Case = 'crash-arenaoom'; File = $pmain; Anchor = 'if (!s_memoryArena || pcCrash::SimulateAllocFailure("arenaoom"))'; Replacement = 'if (false)' },
    @{ Id = 'MEM-01B'; Kind = 'Smoke'; Case = 'crash-debugheapoom'; File = $pmain; Anchor = 'if (!p || pcCrash::SimulateAllocFailure("debugheapoom"))'; Replacement = 'if (false)' },
    @{ Id = 'MEM-02S'; Kind = 'Smoke'; Case = 'crash-heapfail'; File = 'src\Battlefront2\Source\GameMemory.cpp'; Anchor = 'OnFailure = MemFail::OnAllocFailure;'; Replacement = '' },
    @{ Id = 'MEM-03S'; Kind = 'Smoke'; Case = 'crash-luaoom'; File = 'src\LuaFL\src\lmem.cpp'; Anchor = 'luaD_throw(L, LUA_ERRMEM);'; Replacement = 'return NULL;' },
    @{ Id = 'MEM-04S'; Kind = 'Smoke'; Case = 'crash-luapanic'; File = 'src\Battlefront2\Source\LuaHelper.cpp'; Anchor = 'lua_atpanic(mState, Lua_Red_Panic);'; Replacement = '' },
    @{ Id = 'DIAG-04'; Kind = 'Smoke'; Case = 'crash-hang'; File = 'src\Battlefront2\Source\pcCrash.cpp'; Anchor = 'TerminateNow(5);'; Replacement = 'TerminateNow(3);' },
    @{ Id = 'LAA-02A'; File = $pp; Anchor = ' ^ 0x80000000u'; Replacement = ''; Tests = @('ordered list keeps address order across 2GB', 'ptr order key is monotonic across 2GB') },
    @{ Id = 'LAA-02B'; File = $pp; Anchor = 'return (unsigned int)pA > (unsigned int)pB;'; Replacement = 'return (int)pA > (int)pB;'; Tests = @('collision tie-break is unsigned') },
    @{ Id = 'LAA-02C'; File = $th; Anchor = 'PblPtrOrderKey(this)'; Replacement = '(int)this'; Tests = @('pointer order sites use the unsigned helpers') },
    @{ Id = 'LAA-02D'; File = $cm; Anchor = 'PblPtrAfter(pOther, pObject)'; Replacement = '(int)pOther > (int)pObject'; Tests = @('pointer order sites use the unsigned helpers') },
    @{ Id = 'LAA-02E'; File = $rm; Anchor = 'if (sRecursionDepth <= 0)'; Replacement = 'if ((unsigned int)h.start < 0x70000000 && sRecursionDepth <= 0)'; Tests = @('pointer order sites use the unsigned helpers') },
    @{ Id = 'DSET-03'; File = $ds; Anchor = 'if (!bValid)'; Replacement = 'if (false)'; Tests = @('display settings bad value keeps default and logs once') },
    @{ Id = 'DSET-01'; File = $ds; Anchor = 'if (cli.uiSet & kRefresh)'; Replacement = 'if (false)'; Tests = @('display settings merge precedence') },
    @{ Id = 'DSET-04A'; File = $ds; Anchor = 'if (!bGoodFrame)'; Replacement = 'if (false)'; Tests = @('display settings save gate refuses without a good frame') },
    @{ Id = 'DSET-04B'; File = $ds; Anchor = 'return (s.uiCliMask & uiKey) == 0;'; Replacement = 'return true;'; Tests = @('display settings save skips command line values') },
    @{ Id = 'MODE-02'; File = $dl; Anchor = 'if (pSrc[i].adapter != uiAdapter)'; Replacement = 'if (false)'; Tests = @('display logic mode list keeps one adapter') },
    @{ Id = 'MODE-01'; File = $dl; Anchor = 'if (bDuplicate)'; Replacement = 'if (false)'; Tests = @('display logic mode list collapses duplicates and has no cap') },
    @{ Id = 'MODE-03'; File = $dl; Anchor = 'if (uiDeskW == 0 || uiDeskH == 0 || uiCount == 0)'; Replacement = 'if (false)'; Tests = @('display logic desktop default empty list', 'display logic desktop default zero desktop') },
    @{ Id = 'MODE-05'; File = $dl; Anchor = 'return uiHighest;'; Replacement = 'return uiRequested;'; Tests = @('display logic refresh fallback') },
    @{ Id = 'MODE-04'; File = $dl; Anchor = 'if (FindOrdinal(pAdapters, uiCount, uiReqOrdinal))'; Replacement = 'if (true)'; Tests = @('display logic adapter fallback') },
    @{ Id = 'MODE-03U'; File = $dl; Anchor = 'if (!bInList && !bIsDesktop)'; Replacement = 'if (false)'; Tests = @('display logic usable uncached') },
    @{ Id = 'PACE-01'; File = $fl; Anchor = 'if (iRemain > plan.iMarginTicks)'; Replacement = 'if (false)'; Tests = @('frame limiter waits then spins above the margin') },
    @{ Id = 'PACE-01B'; File = $fl; Anchor = 'const __int64 iNeed = plan.iMinTicks < 1 ? 1 : plan.iMinTicks;'; Replacement = 'const __int64 iNeed = plan.iMinTicks;'; Tests = @('frame limiter uncapped never returns zero ticks') },
    @{ Id = 'PACE-03'; File = $md; Anchor = 'if (r.uiCount >= uiMaxMsgs)'; Replacement = 'if (false)'; Tests = @('message drain stops at the message bound') },
    @{ Id = 'PACE-03T'; File = $md; Anchor = 'if (iNow - iStart >= iMaxTicks)'; Replacement = 'if (false)'; Tests = @('message drain stops at the time bound') },
    @{ Id = 'PACE-03Q'; File = $md; Anchor = 'if (msg.message == WM_QUIT)'; Replacement = 'if (false)'; Tests = @('message drain reports WM_QUIT and stops') },
    @{ Id = 'VSYNC-01'; File = $dl; Anchor = '(ulCaps & kIntervalTwo)'; Replacement = 'true'; Tests = @('display logic vsync half needs fullscreen and caps') },
    @{ Id = 'VSYNC-01B'; File = $dl; Anchor = 'return bKeySet ? uiKeyValue : (bProfile ? 1u : 0u);'; Replacement = 'return uiKeyValue;'; Tests = @('display logic effective vsync precedence') },
    @{ Id = 'PACE-02'; Kind = 'Smoke'; Case = 'display-switches'; File = $pmain; Anchor = 'if (uiPaceFps == 0 || uiPaceFps > 80)'; Replacement = 'if (false)' },
    @{ Id = 'VSYNC-02'; Kind = 'Smoke'; Case = 'display-switches'; File = $rr; Anchor = 'sWaitForFrameQuery = !DisplayCfg::gDisplay.bNoWaitQuery;'; Replacement = 'sWaitForFrameQuery = true;' },
    @{ Id = 'LAA-01'; Kind = 'Hardening'; Check = 'laa'; File = 'BattlefrontII.vcxproj'; Anchor = '<LargeAddressAware>true</LargeAddressAware>'; Replacement = '' }
)
