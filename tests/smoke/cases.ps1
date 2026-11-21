$script:SmokeStandardMission = 'geo1c_con'
$script:SoakShortTurns = 9300
$script:SoakLongTurns = 617300
$script:SmokeTurnCap = $script:SoakLongTurns
$script:PerturbSoakTurns = 100000
$script:SmokeCapRx = '(?s)\A(?!.*OPENSWBF2 SOAK turns=)'
$script:SteamIfaceRx = @('OPENSWBF2 STEAM iface=SteamClient ok', 'OPENSWBF2 STEAM iface=SteamUser ok', 'OPENSWBF2 STEAM iface=SteamFriends ok', 'OPENSWBF2 STEAM iface=SteamUtils ok')
$script:SmokeAsanExcluded = @('crash-nullderef', 'crash-divzero', 'crash-stackoverflow', 'crash-debugbreak', 'crash-abort', 'crash-invalidparam', 'crash-fastfail', 'crash-delayload', 'crash-delayload-x64', 'crash-early', 'crash-heapinit', 'crash-check', 'crash-hang', 'crash-stall', 'crash-arenaoom', 'crash-debugheapoom', 'crash-luapanic', 'crash-resetfatal', 'fatal-init', 'crash-midoublefree', 'commit-flat', 'crash-capexhaust', 'vendor-x64-none', 'vendor-x64-steam-wrongbits', 'vendor-x64-galaxy-wrongbits', 'vendor-x64-steam-on')
$script:SmokeAddrSpaceRx = 'OPENSWBF2 ADDRSPACE arena=0x[89A-F][0-9A-F]{7} arena_end=0x[0-9A-F]{8} laa=1 steam=1 steam_api=0x(?!0{8})[0-9A-F]{8} Galaxy=0x(?!0{8})[0-9A-F]{8} GalaxyPeer=0x[0-9A-F]{8} binkw32=0x(?!0{8})[0-9A-F]{8}'
$script:SmokeAddrSpaceRxX64 = 'OPENSWBF2 ADDRSPACE arena=0x[1-9A-F][0-9A-F]{8,15} arena_end=0x[1-9A-F][0-9A-F]{8,15} laa=1 steam=(?:0 steam_api=0x0{8}|1 steam_api=0x[1-9A-F][0-9A-F]{8,15}) Galaxy=0x0{8} GalaxyPeer=0x0{8} binkw32=0x0{8}'
$script:ImportStaticAllow = @{
    Win32 = @(
        'advapi32.dll',
        'api-ms-win-crt-convert-l1-1-0.dll',
        'api-ms-win-crt-environment-l1-1-0.dll',
        'api-ms-win-crt-filesystem-l1-1-0.dll',
        'api-ms-win-crt-heap-l1-1-0.dll',
        'api-ms-win-crt-locale-l1-1-0.dll',
        'api-ms-win-crt-math-l1-1-0.dll',
        'api-ms-win-crt-runtime-l1-1-0.dll',
        'api-ms-win-crt-stdio-l1-1-0.dll',
        'api-ms-win-crt-string-l1-1-0.dll',
        'api-ms-win-crt-time-l1-1-0.dll',
        'api-ms-win-crt-utility-l1-1-0.dll',
        'binkw32.dll',
        'd3d9.dll',
        'd3dx9_43.dll',
        'dinput8.dll',
        'dsound.dll',
        'gdi32.dll',
        'kernel32.dll',
        'msvcp140.dll',
        'ole32.dll',
        'user32.dll',
        'vcruntime140.dll',
        'winmm.dll',
        'ws2_32.dll'
    )
    x64 = @(
        'advapi32.dll',
        'api-ms-win-crt-convert-l1-1-0.dll',
        'api-ms-win-crt-environment-l1-1-0.dll',
        'api-ms-win-crt-filesystem-l1-1-0.dll',
        'api-ms-win-crt-heap-l1-1-0.dll',
        'api-ms-win-crt-locale-l1-1-0.dll',
        'api-ms-win-crt-math-l1-1-0.dll',
        'api-ms-win-crt-runtime-l1-1-0.dll',
        'api-ms-win-crt-stdio-l1-1-0.dll',
        'api-ms-win-crt-string-l1-1-0.dll',
        'api-ms-win-crt-time-l1-1-0.dll',
        'api-ms-win-crt-utility-l1-1-0.dll',
        'd3d9.dll',
        'd3dx9_43.dll',
        'dinput8.dll',
        'dsound.dll',
        'gdi32.dll',
        'kernel32.dll',
        'msvcp140.dll',
        'ole32.dll',
        'user32.dll',
        'vcruntime140.dll',
        'vcruntime140_1.dll',
        'winmm.dll',
        'ws2_32.dll'
    )
}
$script:ImportDelayAllow = @{
    Win32 = @('galaxy.dll', 'steam_api.dll')
    x64 = @('galaxy64.dll', 'steam_api64.dll')
}
$script:MiDefines = @{
    Debug = @('MI_DEBUG=2', 'MI_SECURE=0')
    Release = @('MI_DEBUG=0', 'MI_SECURE=4')
    Test = @('MI_DEBUG=1', 'MI_SECURE=4')
    Asan = @('MI_DEBUG=0', 'MI_SECURE=0', 'MI_TRACK_ASAN=1')
}
$script:SpacerAllow = @(
    'src\Battlefront2\Source\CommandFlyer.h|_tailPad|[0x7]|x64:safe:unread tail pad bytes; members are read by name and the class size is sizeof on each platform',
    'src\Battlefront2\Source\CommandHover.h|_tailPad|[0x8]|x64:safe:unread tail pad bytes; members are read by name and the class size is sizeof on each platform',
    'src\Battlefront2\Source\CommandLine.cpp|szCmdLine|[0x800]|x64:safe:char buffer bounded by sizeof or a fixed char count; no pointer-sized element',
    'src\Battlefront2\Source\CommandLine.cpp|szArgv0|[0x100]|x64:safe:char buffer bounded by sizeof or a fixed char count; no pointer-sized element',
    'src\Battlefront2\Source\CommandLine.cpp|acBuf|[0x80]|x64:safe:char buffer bounded by sizeof or a fixed char count; no pointer-sized element',
    'src\Battlefront2\Source\CollisionMesh.h|buffer|[sizeof(T)*N]|x64:safe:byte buffer sized from sizeof(T)',
    'src\Battlefront2\Source\CommandWalker.h|_tailPad|[0xc]|x64:safe:unread tail pad bytes; members are read by name and the class size is sizeof on each platform',
    'src\Battlefront2\Source\CustomContent.cpp|source|[0x4000]|x64:safe:char buffer bounded by sizeof or a fixed char count; no pointer-sized element',
    'src\Battlefront2\Source\CustomContent.cpp|unusedPath|[64]|x64:safe:char buffer bounded by sizeof or a fixed char count; no pointer-sized element',
    'src\Battlefront2\Source\EntityBuildingAnimatedArmed.h|mTailPadding|[0xc]|x64:safe:unread tail pad bytes; members are read by name and the class size is sizeof on each platform',
    'src\Battlefront2\Source\EntityFlyer.h|_flightFlagPadding|[3]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\Battlefront2\Source\EntityFlyer.h|_turretPadding|[3]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\Battlefront2\Source\EntityFlyer.h|_renderPadding|[3]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\Battlefront2\Source\EntityCarrier.h|_tailPad|[0xc]|x64:safe:unread tail pad bytes; members are read by name and the class size is sizeof on each platform',
    'src\Battlefront2\Source\EntityPortableTurret.h|_tailPad|[6]|x64:safe:unread tail pad bytes; members are read by name and the class size is sizeof on each platform',
    'src\Battlefront2\Source\EntityPropAnimated.cpp|acUnused|[256]|x64:safe:char buffer bounded by sizeof or a fixed char count; no pointer-sized element',
    'src\Battlefront2\Source\FLJoystick.h|mReservedAfterFlip|[3]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\Battlefront2\Source\FLJoystick.h|mReservedBeforeGuiInputs|[2]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\Battlefront2\Source\FLJoystick.h|mReservedBeforeProcessDestination|[3]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\Battlefront2\Source\FLJoystick.h|mReservedBeforeMouse|[2]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\Battlefront2\Source\FLInputManager.h|mReservedBeforeTextTimer|[2]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\Battlefront2\Source\FLInputManager.h|mReservedBeforeJoysticks|[4]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\Battlefront2\Source\FLInputManager.h|mReservedBeforeMissingPort|[3]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\Battlefront2\Source\gamespyStatsManager.cpp|sStatsMatchData|[0x40]|x64:safe:byte buffer used with sizeof or a byte count; it holds bytes and no pointers',
    'src\Battlefront2\Source\gamespyStatsManager.cpp|sStatsMatchData|[0x40]|x64:safe:byte buffer used with sizeof or a byte count; it holds bytes and no pointers',
    'src\Battlefront2\Source\GameVoiceChat.cpp|_gDecoderStorage|[sizeof(Decoder)+alignof(void*)]|x64:fixed:45-09 sizes the storage as sizeof(T) + alignof and aligns the object through RedAlignUp',
    'src\Battlefront2\Source\GameVoiceChat.cpp|_gNodeStorage|[sizeof(Node)+alignof(Node)]|x64:fixed:45-09 sizes the storage as sizeof(T) + alignof and aligns the object through RedAlignUp',
    'src\Battlefront2\Source\GameVoiceChat.cpp|_gEncoderStorage|[sizeof(Encoder)+alignof(Encoder)]|x64:fixed:45-09 sizes the storage as sizeof(T) + alignof and aligns the object through RedAlignUp',
    'src\Battlefront2\Source\GameVoiceChat.cpp|pad|[3]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\Battlefront2\Source\HUDEvent.cpp|acName|[0x400]|x64:safe:char buffer bounded by sizeof or a fixed char count; no pointer-sized element',
    'src\Battlefront2\Source\HUDManager.cpp|acName|[0x400]|x64:safe:char buffer bounded by sizeof or a fixed char count; no pointer-sized element',
    'src\Battlefront2\Source\LuaCallbacks_LoadSave.cpp|aSerialized|[0x10000]|x64:safe:byte buffer used with sizeof or a byte count; it holds bytes and no pointers',
    'src\Battlefront2\Source\LuaCallbacks_Net.cpp|padding|[3]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\Battlefront2\Source\LuaCallbacks_Net.cpp|padding|[3]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\Battlefront2\Source\LuaCallbacks_Options.cpp|kSha512Padding|[128]|x64:safe:SHA-512 padding constant whose size the algorithm defines in bytes',
    'src\Battlefront2\Source\NetAdmin.cpp|remoteResponse|[0x1800]|x64:safe:byte buffer used with sizeof or a byte count; it holds bytes and no pointers',
    'src\Battlefront2\Source\NetAdmin.cpp|remoteResponse|[0x1800]|x64:safe:byte buffer used with sizeof or a byte count; it holds bytes and no pointers',
    'src\Battlefront2\Source\NetAdmin.h|pad|[3]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\GameSpyFL\md5c.c|md5Padding|[64]|x64:safe:MD5 padding constant whose size the algorithm defines in bytes',
    'src\Battlefront2\Source\NetSetup.cpp|sStatsMatchData|[0x40]|x64:safe:byte buffer used with sizeof or a byte count; it holds bytes and no pointers',
    'src\Battlefront2\Source\NetSetup.h|mQueryWaitPadding|[3]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\Battlefront2\Source\NetState.h|mInvisibilityStateStorage|[0x258-0x22c]|x64:safe:unread byte gap in WeaponInvisibilityClass; mInitialSalvoDelay is read by name and nothing offsets into the gap',
    'src\Battlefront2\Source\ParticleSystem.cpp|pool|[301*sizeof(PblHeap<unsignedint,float>::RECORD)]|x64:safe:byte buffer sized from sizeof(T)',
    'src\Battlefront2\Source\PlayerProfile.h|mAlignmentBeforeDifficulty|[2]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\Battlefront2\Source\PlayerProfile.h|mAlignmentBeforeUnlockState|[3]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\Battlefront2\Source\PlayerProfile.h|mPadding|[61]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\Battlefront2\Source\PlayerProfile.h|mTailAlignment|[2]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\Battlefront2\Source\PlayerProfile.h|mPadding|[63]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\Battlefront2\Source\PlayerProfile.h|mAlignmentBeforeMustBeZero|[2]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\Battlefront2\Source\PlayerProfile.h|mPadding|[64]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\Battlefront2\Source\PlayerProfile.h|mPadding|[32]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\Battlefront2\Source\PlayerProfile.h|mPadding|[128]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\Battlefront2\Source\PlayerProfile.h|mAlignmentBeforeGameOptions|[3]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\Battlefront2\Source\PlayerProfile.h|mAlignmentBeforeAccumPoints|[2]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\Battlefront2\Source\PlayerProfile.h|mPadding|[96]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\Battlefront2\Source\PlayerProfile.h|mAlignmentBeforeBonus|[3]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\Battlefront2\Source\PlayerProfile.h|mAlignmentBeforeCRC|[3]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\Battlefront2\Source\VisionManager.cpp|heapMem|[(150+1)*sizeof(VisHeap::RECORD)]|x64:safe:byte buffer sized from sizeof(T)',
    'src\Battlefront2\Source\WeaponGrenade.h|mUnmodeledTail|[0x120-0x118]|x64:safe:unread tail pad bytes; members are read by name and the class size is sizeof on each platform',
    'src\Battlefront2\Source\WeaponLauncher.h|mUnmodeledTail|[8]|x64:safe:unread tail pad bytes; members are read by name and the class size is sizeof on each platform',
    'src\Battlefront2\Source\WeaponRepair.h|mUnmodeledTail|[0x120-0x111]|x64:safe:unread tail pad bytes; members are read by name and the class size is sizeof on each platform',
    'src\lglcd\lgLcd.cpp|Data|[0x104]|x64:safe:char buffer bounded by sizeof or a fixed char count; no pointer-sized element',
    'src\RedEngineFL\Application\pc\pcFLRender.h|unknown04|[20]|x64:safe:unread bytes between float and RedColor fields of TransitionT; the struct has no pointer',
    'src\RedEngineFL\Graphics\PC\pcRedRenderer.h|mReservedAfterDistortion|[2]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\RedEngineFL\Graphics\PC\pcRedRenderer.h|mReservedAfterHDR|[3]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\RedEngineFL\Graphics\PC\pcRedRenderer.h|mReservedAfterBumpMapping|[3]|x64:safe:unread pad bytes in the saved profile; tProfile has no pointer or size_t member and is 4352 bytes on x86 and x64',
    'src\RedEngineFL\Graphics\PC\StateManager_DX.h|pad_40932|[2]|x64:safe:unread pad bytes between named members; the compiler aligns the next member on each platform',
    'src\RedEngineFL\Interface\RedBorderElement.h|mOtherPositionAttributes|[0x0c-0x08]|x64:safe:bytes inside a float vertex record that is 0x1c bytes on both platforms',
    'src\RedEngineFL\Interface\RedBorderElement.h|mOtherTextureAttributes|[0x1c-0x14]|x64:safe:bytes inside a float vertex record that is 0x1c bytes on both platforms',
    'src\RedEngineFL\Interface\RedBorderElement.h|mReservedTail|[0x260-0x254]|x64:safe:unread tail pad bytes; members are read by name and the class size is sizeof on each platform',
    'src\RedEngineFL\movie\pc\pcRedMoviePlayer.h|pad1c|[0xd0-0x1c]|x64:safe:mirror of the binkw32 BINK struct, read only by the Win32 Bink player; x64 links the null movie player and no Bink',
    'src\SoundFL\source\sndenginebase.h|smVoiceStorage|[]|x64:fixed:45-08 sizes the storage as count * sizeof(T) with alignas(T) and a static_assert at the definition',
    'src\SoundFL\source\sndenginebase.h|smStreamStorage|[]|x64:fixed:45-08 sizes the storage as count * sizeof(T) with alignas(T) and a static_assert at the definition',
    'src\SoundFL\source\sndglobals.cpp|acMessage|[0x800]|x64:safe:char buffer bounded by sizeof or a fixed char count; no pointer-sized element',
    'src\SoundFL\source\sndsound.h|smVoiceVirtualStorage|[64*sizeof(FireForget)]|x64:fixed:45-08 sizes the storage as count * sizeof(T) with alignas(T) and a static_assert at the definition',
    'src\SoundFL\source\sndsoundlayered.h|smFireForgetLayerStorage|[]|x64:fixed:45-08 sizes the storage as count * sizeof(T) with alignas(T) and a static_assert at the definition',
    'src\SoundFL\source\win\sndengine.cpp|gSoftOutputStorage|[1*sizeof(SoftOutput)]|x64:fixed:45-08 sizes the storage as count * sizeof(T) with alignas(T) and a static_assert at the definition',
    'src\SoundFL\source\win\sndengine.h|smSampleRAMBitmap|[0x8000]|x64:safe:one bit per 0x80-byte block of the 0x2000000-byte sample RAM so the size is fixed in bytes',
    'src\SoundFL\source\win\sndstreamrenderer.h|mHardwareRendererStorage|[1*sizeof(DSBufferRenderer)]|x64:fixed:45-08 sizes the storage as count * sizeof(T) with alignas(T) and a static_assert at the definition'
)
$script:OffsetAllow = @(
    'src\Battlefront2\Source\CollisionTree.cpp|(unsigned short*)(memblock + iBytes - 2 * iIndices)|x64:safe:index pool at the end of one new char[iBytes] block; 2 is sizeof(unsigned short) and the 12-byte node records hold no pointer since WIDTH-02',
    'src\Battlefront2\Source\GameVoiceChatEncoder.cpp|(VoiceChat::AudioPacket*)(mEncoderPackets + 1)|x64:safe:typed or sizeof-based offset inside one block whose size 45-09 builds from sizeof',
    'src\Battlefront2\Source\HUDManager.cpp|(char*)HUD::gEditHeapMem + 0xFFFFF|x64:safe:last byte of the same 0x100000-byte allocation',
    'src\Battlefront2\Source\NetSetup.cpp|(char*)(fileNames + iCount)|x64:safe:strings follow iCount char* slots in a buffer sized iCount * sizeof(char*) plus the string bytes',
    'src\Battlefront2\Source\pcMain.cpp|(char*)p + 0x300000|x64:safe:3 MB byte budget handed to RedCreateHeap; no pointer-sized element',
    'src\Battlefront2\Source\VoiceChatNode.cpp|(SendMetrics*)(pPerNode + uiNodes * sizeof(ReceiveMetrics))|x64:safe:typed or sizeof-based offset inside one block whose size 45-09 builds from sizeof',
    'src\Battlefront2\Source\VoiceChatNode.cpp|(AudioPacket*)(mReceiveStreams + uiNodes)|x64:safe:typed or sizeof-based offset inside one block whose size 45-09 builds from sizeof',
    'src\Battlefront2\Source\VoiceChatNode.cpp|(char*)mBitmap + 1|x64:safe:typed or sizeof-based offset inside one block whose size 45-09 builds from sizeof',
    'src\RedEngineFL\Graphics\PC\pcRedSegment.cpp|(PblVector3*)(vertex + normalOffset)|x64:safe:byte offset from the vertex declaration inside one float vertex record',
    'src\RedEngineFL\Graphics\PC\pcRedSegment.cpp|(PblVector3*)(vertex + normalOffset)|x64:safe:byte offset from the vertex declaration inside one float vertex record',
    'src\RedEngineFL\Graphics\PC\pcRedSegment.cpp|(PblVector3*)(vertex + tangentOffset)|x64:safe:byte offset from the vertex declaration inside one float vertex record',
    'src\RedEngineFL\Graphics\PC\pcRedSegment.cpp|(PblVector3*)(vertex + binormalOffset)|x64:safe:byte offset from the vertex declaration inside one float vertex record',
    'src\RedEngineFL\Memory\RedMemoryPool.cpp|(char**)((char*)mPool + i * mSize)|x64:fixed:WIDTH-04 rounds mSize up to sizeof(void*) in the constructor and Create so every block holds the char* link',
    'src\RedEngineFL\Memory\RedMemoryPool.cpp|(char**)((char*)mPool + (mCount - 1) * mSize)|x64:fixed:WIDTH-04 rounds mSize up to sizeof(void*) in the constructor and Create so every block holds the char* link',
    'src\RedEngineFL\Memory\RedMemoryPool.cpp|(char**)((char*)mFree + i * mSize)|x64:fixed:WIDTH-04 rounds mSize up to sizeof(void*) in the constructor and Create so every block holds the char* link',
    'src\RedEngineFL\Memory\RedMemoryPool.cpp|(char**)((char*)mFree + (mGrow - 1) * mSize)|x64:fixed:WIDTH-04 rounds mSize up to sizeof(void*) in the constructor and Create so every block holds the char* link',
    'src\RedEngineFL\Util\RedLocalizeDB.cpp|(FLLocalizeEntry*)((char*)pEntry + pEntry->mEntryLen)|x64:safe:byte length stored in the file record',
    'src\RedEngineFL\Util\RedScreenShot.cpp|(const unsigned int*)((unsigned char*)rect.pBits + rect.Pitch * y)|x64:safe:D3D row pitch in bytes inside the locked surface',
    'src\SoundFL\source\sndbufferio.cpp|(void*)(mOffset + (char*)mBuffer)|x64:safe:byte offset or sizeof(T) step inside one allocation sized with sizeof',
    'src\SoundFL\source\sndsamplebankbase.cpp|(Sample*)((char*)pMemory + sizeof(SampleBank))|x64:safe:byte offset or sizeof(T) step inside one allocation sized with sizeof',
    'src\SoundFL\source\sndsamplebankbase.cpp|(void*)(GetBaseAddress() - uiSize + uiRAMSize)|x64:safe:uintptr_t sample RAM address minus a byte count inside the one sample RAM block',
    'src\SoundFL\source\sndsoundlayered.cpp|(FireForgetLayer*)(smFireForgetLayerStorage + i * sizeof(FireForgetLayer))|x64:safe:byte offset or sizeof(T) step inside one allocation sized with sizeof',
    'src\SoundFL\source\sndstreambase.cpp|(Segment*)((char*)pMem + sizeof(Info))|x64:safe:byte offset or sizeof(T) step inside one allocation sized with sizeof',
    'src\SoundFL\source\sndstreammixer.cpp|(int*)((char*)mMixBuffer + mMixBufferOffsets[uiInput])|x64:safe:byte offset or sizeof(T) step inside one allocation sized with sizeof',
    'src\SoundFL\source\sndstreamresampler.cpp|(const short*)((const char*)pInput->GetBuffer() + pInput->GetUsed())|x64:safe:byte offset or sizeof(T) step inside one allocation sized with sizeof',
    'src\SoundFL\source\sndstreamresampler.cpp|(short*)((char*)pDst + mBufferSize)|x64:safe:byte offset or sizeof(T) step inside one allocation sized with sizeof',
    'src\SoundFL\source\win\snddsbufferrenderer.cpp|(short*)((unsigned char*)pData + uiWritten)|x64:safe:byte offset or sizeof(T) step inside one allocation sized with sizeof',
    'src\Zephyr\Source\ZephyrAnimBank.cpp|(const ZephyrAnimD*)(pkBlock + 1)|x64:safe:typed step over the 8-byte on-disk block header which has no pointer member',
    'src\Zephyr\Source\ZephyrAnimBank.cpp|(const ZephyrAnimBlockHeader*)(pkAnimD + u16NumAnims)|x64:safe:typed step over the 16-byte on-disk ZephyrAnimD records which have no pointer member',
    'src\Zephyr\Source\ZephyrAnimBank.cpp|(const ZephyrAnimJointD*)(pkBlock + 1)|x64:safe:typed step over the 8-byte on-disk block header which has no pointer member',
    'src\Zephyr\Source\ZephyrAnimBank.cpp|(const ZephyrAnimBlockHeader*)((const char*)pkJointD + 48 * u32NumTotalJoints)|x64:safe:48 is the on-disk ZephyrAnimJointD record of 4-byte fields and sizeof 48 on both platforms',
    'src\Zephyr\Source\ZephyrAnimBank.cpp|(char*)(pkAnims + u16NumAnims)|x64:safe:typed step over the ZephyrAnim array that GetRequiredMemSize sizes with sizeof(ZephyrAnim)',
    'src\Zephyr\Source\ZephyrAnimBank.cpp|(const ZephyrAnimJointD*)((const char*)pkJointD + 48 * u16NumJoints)|x64:safe:48 is the on-disk ZephyrAnimJointD record of 4-byte fields and sizeof 48 on both platforms',
    'src\Zephyr\Source\ZephyrSkeletonShared.cpp|(ZephyrJointShared*)((char*)this + kOffsets.m_uJoints)|x64:safe:offset inside one allocation from GetRequiredMemSize built from sizeof(ZephyrSkeletonShared) and sizeof(ZephyrJointShared)',
    'src\Zephyr\Source\ZephyrSkinShared.cpp|(ZephyrJointSet*)((char*)this + kOffsets.m_uJointSets)|x64:safe:offset inside one allocation from GetRequiredMemSize built from sizeof(ZephyrSkinShared) and sizeof of the skeleton and ZephyrJointSet',
    'src\Zephyr\Source\ZephyrSkinShared.cpp|(ZephyrSkeletonShared*)((char*)this + kOffsets.m_uSkeleton)|x64:safe:offset inside one allocation from GetRequiredMemSize: sizeof(ZephyrSkinShared) aligned to 16'
)
$script:SizeLiteralAllow = @(
    'src\Battlefront2\Source\pcCrash.cpp|MemoryPool kPool("crashtestpool", 16)|x64:safe:crash-test pool of raw 16-byte blocks; no object type is placed in it',
    'src\Battlefront2\Source\pcCrash.cpp|MemoryPool kHuge("crashtesthuge", 16)|x64:safe:crash-test pool of raw 16-byte blocks; no object type is placed in it',
    'src\Battlefront2\Source\pcCrash.cpp|.Create(0x40000001, 16, 0)|x64:safe:crash-test pool of raw 16-byte blocks; no object type is placed in it',
    'src\Battlefront2\Source\pcCrash.cpp|MemoryPool kSmall("crashtestsmall", 16)|x64:safe:crash-test pool of raw 16-byte blocks; no object type is placed in it',
    'src\Battlefront2\Source\pcCrash.cpp|.Create(4, 16, 0)|x64:safe:crash-test pool of raw 16-byte blocks; no object type is placed in it',
    'src\Battlefront2\Source\pcCrash.cpp|MemoryPool kPool("crashtestpool", 16)|x64:safe:crash-test pool of raw 16-byte blocks; no object type is placed in it',
    'src\Battlefront2\Source\pcCrash.cpp|.Create(2, 16, 0)|x64:safe:crash-test pool of raw 16-byte blocks; no object type is placed in it',
    'src\Battlefront2\Source\LuaMemory.cpp|->Create(pInfo->mCount, pInfo->mBin * 4, 0)|x64:safe:a Lua bin counts 4-byte units of a Lua request size (LocatePool takes (uiSize + 3) >> 2), so mBin * 4 is the byte block size of the bin, not a pointer count'
)
$script:SerialAllow = @(
    'src\Battlefront2\Source\CollisionObject.cpp|static unsigned int sCollisionSerialNext = 0;',
    'src\Battlefront2\Source\CollisionObject.cpp|mCollisionSerial = sCollisionSerialNext < 0x3FFFFFFF ? ++sCollisionSerialNext : sCollisionSerialNext;',
    'src\Battlefront2\Source\CollisionObject.cpp|sCollisionSerialNext = 0;',
    'src\Battlefront2\Source\CollisionObject.h|unsigned int mCollisionSerial : 30;',
    'src\Battlefront2\Source\CollisionObject.h|return PblSerialAfter(pA->mCollisionSerial, pB->mCollisionSerial);',
    'src\Battlefront2\Source\CollisionObject.h|if (pA->mCollisionSerial == pB->mCollisionSerial)',
    'src\Battlefront2\Source\CollisionObject.h|return pObject->mCollisionSerial;',
    'src\Battlefront2\Source\Thread.cpp|static unsigned int sThreadActivationKey = 0;',
    'src\Battlefront2\Source\Thread.cpp|return (int)(sThreadActivationKey < 0x7FFFFFFF ? sThreadActivationKey++ : sThreadActivationKey);',
    'src\Battlefront2\Source\Thread.cpp|sThreadActivationKey = 0;'
)
$script:AddrOrderAllow = @(
    'src\Battlefront2\Source\Combo.cpp|if (m_pCatchState >= pOld && m_pCatchState < pOld + uiCount)',
    'src\Battlefront2\Source\Combo.cpp|if (m_pTakeKneeState >= pOld && m_pTakeKneeState < pOld + uiCount)',
    'src\Battlefront2\Source\LoadUtil.cpp|if ((uintptr_t)pRegion > 1)',
    'src\Battlefront2\Source\SoldierAnimatorLowRes.cpp|fTime = (float)(int)(this - pPool->m_apAnimator) * (float)(iNumFrames - 1) /',
    'src\Battlefront2\Source\LuaSaveItems.cpp|if (pOut >= pEnd || !lua_checkstack(L, 3))',
    'src\Battlefront2\Source\LuaSaveItems.cpp|if (pData >= pEnd || !lua_checkstack(L, 3))',
    'src\Battlefront2\Source\LuaSaveItems.cpp|if (pRead >= pEnd)',
    'src\Battlefront2\Source\LuaSaveItems.cpp|if (pRead >= pEnd)',
    'src\Battlefront2\Source\VOHelper.cpp|if (pB > pA)',
    'src\Battlefront2\Source\VOHelper.cpp|if (pB < pA)',
    'src\PebbleFL\Common\PblHashTable.cpp|if (pSlot <= pTable)',
    'src\PebbleFL\Common\PblHashTable.cpp|if (pSlot <= pTable)',
    'src\PebbleFL\Common\PblHashTable.cpp|if (pSlot <= pTable)',
    'src\PebbleFL\Common\PblHashTable.cpp|if (pSlot <= pTable)',
    'src\PebbleFL\Common\PblHashTable.cpp|if (pSlot <= pTable)'
)
$script:FastcallAllow = @(
    'src\Battlefront2\Source\CameraManager.cpp|typedef const PblVector3& (__fastcall *ObjectVelocityFn)(GameObject* pThis, void*);',
    'src\Battlefront2\Source\CameraManager.cpp|typedef Controllable* (__fastcall *ObjectControllableFn)(GameObject* pThis, void*);',
    'src\Battlefront2\Source\ChaseCamera.cpp|typedef const PblVector3& (__fastcall *GetVelocityFn)(GameObject* pThis, void*);',
    'src\Battlefront2\Source\LuaCallbacks_Net.cpp|bool __fastcall IsFriendBuddy(int ignored, int iPlayer);',
    'src\Battlefront2\Source\SkyDome.cpp|typedef void(__fastcall* SceneObjectFn)(void* pThis, void*);'
)

function Get-SmokeAddrSpaceRx {
    if ($script:SmokePlatform -eq 'x64') { return $script:SmokeAddrSpaceRxX64 }
    return $script:SmokeAddrSpaceRx
}

function Get-SmokeCases($which, $missions, $movies, $dedicatedSpec, $hog = $false, $soak = 'Short') {
    $soakTurns = $null
    if ($soak -eq 'Short') { $soakTurns = $script:SoakShortTurns }
    elseif ($soak -eq 'Long') { $soakTurns = $script:SoakLongTurns }
    else { throw "unknown soak '$soak'" }
    $standard = $script:SmokeStandardMission
    $render = '-norender'
    if ($movies) { $render = '' }
    $tail = (("$render -nosound -fixedrate 30 -nowait -logfinalscore -DecompCONMinutes 5 -seed 1 -exitongameover -nofocuspause -log ""{CASEDIR}\run.log"" -hangtimeout 0") -replace '^\s+', '')
    $htail = $tail
    $pass = @{ Class = 'Pass' }
    $hrx = @()
    $noWarpRx = '(?s)\A(?!.*OPENSWBF2 CURSOR warp)'
    if ($hog) {
        $htail = "$tail -hogaddrspace"
        $hrx = @(Get-SmokeAddrSpaceRx)
        $pass = @{ Class = 'Pass'; LogRx = $hrx }
    }
    $cases = @()

    $allCase = {
        param($m, $class)
        $x = $pass
        if ($class -eq 'TimeOut') { $x = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 MISSION RUNNING', 'OPENSWBF2 SIM DIGEST turns=1500 ', 'OPENSWBF2 SOAK turns=') + $hrx) } }
        elseif ($class -eq 'Fatal') {
            $x = @{ Class = 'Fatal'; Code = 3; MinDumps = 1; MaxDumps = 1; LogRx = (@('FATAL: exception 0xC0000005 at 0x[0-9A-F]+ \(d3dx9_43\.dll\+0x1BA553\)') + $hrx) }
        }
        elseif ($class -ne 'Pass') { throw "unknown expected class '$class' for $m in missions file" }
        $mArgs = "$m $htail -logcmds ""LogMemory Verbose"" -digesttrace 1500"
        if ($class -ne 'Fatal') {
            $x = $x.Clone()
            $cp = @(100)
            if ($class -eq 'TimeOut' -or $null -ne (Get-DigestExpected $m)) { $cp += 1500 }
            $x.LogRx = @(@($x.LogRx | Where-Object { $_ }) + @($cp | ForEach-Object { "(?m)OPENSWBF2 DIGEST TRACE turn=$_ state=0x[0-9A-F]{8} rand=\d+/\d+\r?$" }))
        }
        if ($m -match '_(1flag|ctf|ass)$' -and $null -eq (Get-DigestExpected $m) -and $class -eq 'Pass') { throw "$m has no digest row and runs with -novictory, so it cannot Pass" }
        if ($m -match '_(1flag|ctf|ass)$' -and $null -eq (Get-DigestExpected $m) -and $class -eq 'TimeOut') {
            $mArgs += ' -novictory'
            $x = $x.Clone()
            $x.LogRx = @($x.LogRx | Where-Object { $_ }) + '(?m)OPENSWBF2 NOVICTORY\r?$'
        }
        if ($class -eq 'TimeOut') { $mArgs += " -soakturns $soakTurns" }
        New-Object PSObject -Property @{ Name = $m; Args = $mArgs; Expect = $x; Twice = 0 }
    }

    if ($which -eq 'Standard') {
        $memRx = @('OPENSWBF2 MEMREPORT source=exit', 'OPENSWBF2 MEMREPORT heap=Runtime cap=\d+ peak=\d+ used=\d+ allocs=\d+ failed=\d+', 'OPENSWBF2 MEMREPORT heap=Lua ', 'OPENSWBF2 MEMREPORT heap=Application ', 'OPENSWBF2 MEMREPORT heap=Artist ', 'OPENSWBF2 MEMREPORT heap=Gamespy ', 'OPENSWBF2 MEMREPORT heap=Debug ', 'OPENSWBF2 MEMREPORT pool=\S+ count=\d+ size=\d+ peak=\d+ grown=\d+', 'OPENSWBF2 MEMREPORT pool=Lua\d+ ', 'OPENSWBF2 MEMREPORT pool=Weapon ', 'OPENSWBF2 CLOTH free=-?\d+ enabled=[01] low=\d+ high=\d+')
        if ($script:SmokeDigestConfiguration -in 'Test', 'Asan') { $memRx += 'OPENSWBF2 MEMREPORT offthread count=0 sources=0 dropped=0' }
        $memRx += @('OPENSWBF2 PACING fps=80 vsync=off interval=0x80000000 waitquery=0 timer=none', 'OPENSWBF2 DPI awareness=(pmv2|pm|system|none) dpi=\d+', 'OPENSWBF2 GAMMA path=ramp reason=norender', '(?s)\A(?!.*OPENSWBF2 GAMMA FAKE)', '(?s)\A(?!.*OPENSWBF2 GAMMA path=pass)', 'OPENSWBF2 CURSOR mode=hw image=skipped')
        $cases += New-Object PSObject -Property @{ Name = $standard; Args = "$standard $htail"; Expect = @{ Class = 'Pass'; LogRx = ($memRx + $hrx) }; Twice = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'heapcap'; Args = "$standard $htail -heapcap Runtime=1M -heapcap Runtime=96M -heapcap Bogus=1M -heapcap Runtime=12x"; Expect = @{ Class = 'Pass'; SameRunAs = $standard; LogRx = (@('OPENSWBF2 MEMREPORT heap=Runtime cap=100663296 ', '-heapcap needs <Name>=<bytes>\[K\|M\], ignored: Bogus=1M', '-heapcap needs <Name>=<bytes>\[K\|M\], ignored: Runtime=12x') + $hrx) }; Twice = 0 }
    }
    if ($which -eq 'Standard') {
        $t1rx = '(?m)OPENSWBF2 SOAK turns=1\r?$'
        $cases += New-Object PSObject -Property @{ Name = 'display-switches'; Args = "$standard $htail -borderless -adapter 0 -refresh 60 -vsync half -fps 120 -nowaitquery -nodpi -soakturns 1"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 PACING fps=120 vsync=on interval=0x00000000 waitquery=0 timer=none', 'pacing: fps cap 120 is above 80', 'vsync: requested interval not supported here, using on', 'OPENSWBF2 DPI awareness=off', $t1rx) + $hrx) }; Twice = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'cursor-sw'; Args = "$standard $htail -swcursor -soakturns 1"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 CURSOR mode=sw image=skipped', '(?s)\A(?!.*OPENSWBF2 CURSOR mode=hw)', $t1rx) + $hrx) }; Twice = 0 }
        $swIni = Join-Path $PSScriptRoot 'display-swcursor.ini'
        $cases += New-Object PSObject -Property @{ Name = 'cursor-sw-ini'; Args = "$standard $htail -displayini `"$swIni`" -soakturns 1"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 CURSOR mode=sw image=skipped', '(?s)\A(?!.*OPENSWBF2 CURSOR mode=hw)', $t1rx) + $hrx) }; Twice = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'window-borderless-headless'; Args = "$standard $htail -borderless -adapter 5 -soakturns 1"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 DPI awareness=(pmv2|pm|system|none) dpi=\d+', $t1rx) + $hrx) }; Twice = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'fps-0'; Args = "$standard $htail -fps 0 -soakturns 1"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 PACING fps=0 ', 'pacing: fps cap is uncapped', $t1rx) + $hrx) }; Twice = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'fps-30'; Args = "$standard $htail -fps 30 -soakturns 1"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 PACING fps=30 ', $t1rx) + $hrx) }; Twice = 0 }
        $rtTail = $htail -replace ' -fixedrate 30', ''
        $cases += New-Object PSObject -Property @{ Name = 'pacing-realtime'; Args = "nab2g_c $rtTail -fps 30 -soakturns 300"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 MISSION RUNNING', 'OPENSWBF2 PACING fps=30 ', 'OPENSWBF2 SOAK turns=', $noWarpRx) + $hrx) }; Twice = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'pacing-realtime-uncapped'; Args = "nab2g_c $rtTail -fps 0 -soakturns 300"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 MISSION RUNNING', 'OPENSWBF2 PACING fps=0 ', 'OPENSWBF2 SOAK turns=', $noWarpRx) + $hrx) }; Twice = 0 }
        $badIni = Join-Path $PSScriptRoot 'display-bad.ini'
        $cases += New-Object PSObject -Property @{ Name = 'display-badini'; Args = "$standard $htail -adapter abc -refresh 99999 -displayini `"$badIni`" -soakturns 1"; Expect = @{ Class = 'Pass'; LogRx = (@($t1rx) + $hrx) }; Twice = 0 }
        $waitIni = Join-Path $PSScriptRoot 'display-waitquery.ini'
        $cases += New-Object PSObject -Property @{ Name = 'pacing-waitquery-ini'; Args = "$standard $htail -displayini `"$waitIni`" -soakturns 1"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 PACING fps=80 vsync=off interval=0x80000000 waitquery=1 timer=none', $t1rx) + $hrx) }; Twice = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'digest-dag1c_ctf'; Args = "dag1c_ctf $htail"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 MISSION RUNNING', 'OPENSWBF2 SIM DIGEST turns=') + $hrx) }; Twice = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'novictory-con'; Args = "$standard $htail -novictory -digesttrace 9300 -soakturns 18500"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 MISSION RUNNING', '(?m)OPENSWBF2 NOVICTORY\r?$', 'OPENSWBF2 DIGEST TRACE turn=9300 ', 'OPENSWBF2 SOAK turns=') + $hrx) }; Twice = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'digest-kas2c_uber'; Args = "kas2c_uber $htail"; Expect = $pass; Twice = 0 }
        if ($script:SmokeDigestConfiguration -eq 'Asan') {
            foreach ($m in 'hot1g_con', 'spa3c_ass') {
                $cases += New-Object PSObject -Property @{ Name = "asan-$m"; Args = "$m $htail"; Expect = $pass; Twice = 0 }
            }
        }
        if ($script:SmokeDigestConfiguration -in 'Test', 'Asan') {
            $scanRx = @('OPENSWBF2 HANDLESCAN state=shell live=0 dead=0', 'OPENSWBF2 HANDLESCAN state=mission live=0 dead=0', '(?s)\A(?!.*OPENSWBF2 HANDLESCAN (live |dead |state=\w+ (?!live=0 dead=0\b)))')
            $cases += New-Object PSObject -Property @{ Name = 'shell-first'; Args = "$standard $htail -shellfirst"; Expect = @{ Class = 'Pass'; SameDigests = 1; LogRx = (@('OPENSWBF2 SHELLFIRST to_mission mission=' + $standard) + $scanRx + $hrx) }; Twice = 0 }
            $cases += New-Object PSObject -Property @{ Name = 'commit-flat'; Args = "$standard $htail -repeatmission 16"; Expect = @{ Class = 'Pass'; SameDigests = 17; CommitFlat = 0; CommitCycles = 16; CommitSettled = 8; LogRx = (@('OPENSWBF2 REPEAT to_shell', 'OPENSWBF2 REPEAT to_mission') + $hrx) }; Twice = 0 }
            if ($script:SmokeDigestConfiguration -eq 'Test') {
                $mvTail = (($htail -replace '(^| )-norender(?= )', '') -replace '(^| )-nosound(?= )', '') -replace '^\s+', ''
                if ($script:SmokePlatform -eq 'Win32') {
                    $ddPinned = Get-DigestExpected $standard
                    $ddDigestRx = $(if ($ddPinned) { '(?m)' + [regex]::Escape($ddPinned) + '\r?$' } else { '(?!)' })
                    $cases += New-Object PSObject -Property @{ Name = 'digest-detail'; Args = "$standard $htail -digestdetail 4 -soakturns 1501"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 MISSION RUNNING', '(?m)OPENSWBF2 DETAIL turn=4 time=[0-9A-F]{8}\r?$', '(?m)OPENSWBF2 DETAIL turn=5 time=[0-9A-F]{8}\r?$', '(?m)OPENSWBF2 DETAIL turn=6 time=[0-9A-F]{8}\r?$', 'OPENSWBF2 DETAIL turn=4 char=0 team=-?\d+ class=-?\d+ ', 'OPENSWBF2 DETAIL turn=6 obj=0x[0-9A-F]{8} pos=[0-9A-F]{8} [0-9A-F]{8} [0-9A-F]{8} vel=[0-9A-F]{8} [0-9A-F]{8} [0-9A-F]{8} hp=[0-9A-F]{8} objteam=-?\d+', 'OPENSWBF2 DETAIL turn=5 team=\d+ points=', '(?s)\A(?!.*OPENSWBF2 DETAIL turn=(?![456] ))', $ddDigestRx, 'OPENSWBF2 SOAK turns=') + $hrx) }; Twice = 0 }
                }
                $cases += New-Object PSObject -Property @{ Name = 'campaign-movies'; Args = "nab2g_c $mvTail -win -exitonmovieclose"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 MOVIE open', 'OPENSWBF2 MOVIE close', 'OPENSWBF2 MISSION RUNNING nab2g_c', 'OPENSWBF2 MEMREPORT source=exit', 'OPENSWBF2 MEMREPORT offthread count=0 sources=0 dropped=0') + $hrx) }; Twice = 0 }
            }
            $cases += New-Object PSObject -Property @{ Name = 'repeat-water'; Args = "nab2g_c $htail -repeatmission -soakturns 1501"; Expect = @{ Class = 'Pass'; SameDigests = 2; LogRx = (@('OPENSWBF2 MISSION RUNNING', '(?m)OPENSWBF2 REPEAT to_shell mission=nab2g_c\r?$', '(?m)OPENSWBF2 REPEAT to_mission mission=nab2g_c\r?$', 'OPENSWBF2 SIM DIGEST turns=', 'OPENSWBF2 SOAK turns=', $noWarpRx) + $hrx) }; Twice = 0 }
            $cases += New-Object PSObject -Property @{ Name = 'repeat-digest'; Args = "$standard $htail -repeatmission"; Expect = @{ Class = 'Pass'; SameDigests = 2; LogRx = (@('OPENSWBF2 REPEAT to_shell', 'OPENSWBF2 REPEAT to_mission') + $scanRx + $hrx) }; Twice = 0 }
        }
    }
    if ($which -eq 'Vendor' -and $script:SmokePlatform -eq 'Win32') {
        $t1rx = '(?m)OPENSWBF2 SOAK turns=1\r?$'
        $vArgs = "$standard $htail -soakturns 1"
        $vRt = @($GalaxyRuntimes | ForEach-Object { Join-Path (Get-SystemDllDir 'Win32') $_ })
        $vRtSkip = 'VS2013 x86 runtime not installed'
        $vRtOff = 'OPENSWBF2 VENDOR steam=on galaxy=off d3dx9=[^\r\n]* msvcr120=0 msvcp120=0'
        $cases += New-Object PSObject -Property @{ Name = 'vendor-present'; Args = $vArgs; NeedsFile = $vRt; SkipReason = $vRtSkip; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 VENDOR steam=on galaxy=on d3dx9=[^\r\n]* msvcr120=1 msvcp120=1', '(?s)\A(?!.*OPENSWBF2 VENDOR \w+ off)', $t1rx) + $script:SteamIfaceRx + $hrx) }; Twice = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'vendor-steam-missing'; Args = $vArgs; Omit = @('steam_api.dll'); Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 VENDOR steam off: steam_api\.dll missing', 'OPENSWBF2 VENDOR steam=off galaxy=on', '(?s)\A(?!.*VENDOR steam off.*VENDOR steam off)', $t1rx) + $hrx) }; Twice = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'vendor-galaxy-missing'; Args = $vArgs; Omit = @('Galaxy.dll'); Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 VENDOR galaxy off: Galaxy\.dll missing', $vRtOff, '(?s)\A(?!.*VENDOR galaxy off.*VENDOR galaxy off)', $t1rx) + $hrx) }; Twice = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'vendor-vcrt-off'; Args = $vArgs; Omit = @('Galaxy.dll'); NeedsFile = $vRt; SkipReason = $vRtSkip; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 VENDOR galaxy off: Galaxy\.dll missing', $vRtOff, '(?s)\A(?!.*VENDOR galaxy off.*VENDOR galaxy off)', $t1rx) + $hrx) }; Twice = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'vendor-galaxypeer-missing'; Args = $vArgs; Omit = @('GalaxyPeer.dll'); Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 VENDOR galaxy off: GalaxyPeer\.dll missing', 'OPENSWBF2 VENDOR steam=on galaxy=off', '(?s)\A(?!.*VENDOR galaxy off.*VENDOR galaxy off)', $t1rx) + $hrx) }; Twice = 0 }
        $vSteam64 = Join-Path $PSScriptRoot 'SWBF2_VENDOR_X64-not-set\steam_api64.dll'
        $vSkip = 'SWBF2_VENDOR_X64 not set'
        if ($env:SWBF2_VENDOR_X64) {
            $vSteam64 = Join-Path $env:SWBF2_VENDOR_X64 'steam_api64.dll'
            $vSkip = 'steam_api64.dll not in SWBF2_VENDOR_X64'
        }
        $cases += New-Object PSObject -Property @{ Name = 'vendor-steam-wrongbits'; Args = $vArgs; Swap = @{ 'steam_api.dll' = $vSteam64 }; NeedsFile = @($vSteam64); SkipReason = $vSkip; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 VENDOR steam off: steam_api\.dll wrong bitness', 'OPENSWBF2 VENDOR steam=off galaxy=on', '(?s)\A(?!.*VENDOR steam off.*VENDOR steam off)', $t1rx) + $hrx) }; Twice = 0 }
        $cases += New-Object PSObject -Property @{ Name = 'vendor-wan-nogalaxy'; Args = "/autonet dedicated -nopregame -playercount 0 $standard 100 100 $htail"; Omit = @('Galaxy.dll'); Expect = @{ Class = 'Fatal'; Code = 1; MaxDumps = 0; LogRx = @('OPENSWBF2 VENDOR galaxy off: Galaxy\.dll missing', '(?s)\A(?!.*VENDOR galaxy off.*VENDOR galaxy off)', 'OPENSWBF2 GALAXY login skipped: Online lobbies are not available because Galaxy could not be loaded\. LAN and direct IP games work\.', 'FATAL: online dedicated server needs Galaxy') }; Twice = 0 }
    }
    if ($which -eq 'Vendor' -and $script:SmokePlatform -eq 'x64') {
        $vArgs = "$standard $tail -crashtest fastfail"
        $werKey = $script:SmokeWerKey
        if ($null -eq $werKey) { $werKey = Test-WerKey }
        $vGame = Find-GameDataRoot
        $vSkip = 'no retail GameData found'
        if (-not $vGame) { $vGame = Join-Path $PSScriptRoot 'GameData-not-found' }
        $vSteamOnce = '(?s)\A(?!.*VENDOR steam off.*VENDOR steam off)'
        $vGalaxyOnce = '(?s)\A(?!.*VENDOR galaxy off.*VENDOR galaxy off)'
        $vRtOff = 'OPENSWBF2 VENDOR steam=\w+ galaxy=off d3dx9=[^\r\n]* msvcr120=0 msvcp120=0'
        $vSpecs = @(
            @{ Name = 'vendor-x64-none'; Omit = @('steam_api64.dll', 'Galaxy64.dll', 'GalaxyPeer64.dll'); LogRx = @('OPENSWBF2 VENDOR steam off: steam_api64\.dll missing', 'OPENSWBF2 VENDOR galaxy off: Galaxy64\.dll missing', 'OPENSWBF2 VENDOR steam=off galaxy=off', $vSteamOnce, $vGalaxyOnce, $vRtOff) },
            @{ Name = 'vendor-x64-steam-wrongbits'; Swap = @{ 'steam_api64.dll' = (Join-Path $vGame 'steam_api.dll') }; LogRx = @('OPENSWBF2 VENDOR steam off: steam_api64\.dll wrong bitness', $vSteamOnce, $vRtOff) },
            @{ Name = 'vendor-x64-galaxy-wrongbits'; Swap = @{ 'Galaxy64.dll' = (Join-Path $vGame 'Galaxy.dll') }; LogRx = @('OPENSWBF2 VENDOR galaxy off: Galaxy64\.dll wrong bitness', $vGalaxyOnce, $vRtOff) },
            @{ Name = 'vendor-x64-steam-on'; Omit = @('Galaxy64.dll', 'GalaxyPeer64.dll'); NeedsFile = @(Join-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) "bin\x64\$script:SmokeDigestConfiguration\steam_api64.dll"); SkipReason = 'SWBF2_VENDOR_X64 not set'; LogRx = (@('OPENSWBF2 VENDOR steam=on galaxy=off', '(?s)\A(?!.*VENDOR steam off)', 'OPENSWBF2 VENDOR galaxy off: Galaxy64\.dll missing', $vGalaxyOnce, $vRtOff) + $script:SteamIfaceRx) }
        )
        foreach ($s in $vSpecs) {
            $x = @{ Class = 'GsFailure'; Code = 3221226505; LogRx = $s.LogRx }
            if ($werKey) { $x.MinDumps = 1 }
            $c = New-Object PSObject -Property @{ Name = $s.Name; Args = $vArgs; Expect = $x; Twice = 0 }
            if ($s.Omit) { $c | Add-Member -NotePropertyName Omit -NotePropertyValue $s.Omit }
            if ($s.Swap) { $c | Add-Member -NotePropertyName Swap -NotePropertyValue $s.Swap; $c | Add-Member -NotePropertyName SkipReason -NotePropertyValue $vSkip }
            if ($s.NeedsFile) { $c | Add-Member -NotePropertyName NeedsFile -NotePropertyValue $s.NeedsFile; $c | Add-Member -NotePropertyName SkipReason -NotePropertyValue $s.SkipReason }
            $cases += $c
        }
    }
    if ($which -eq 'Vendor') {
        $vD3dx = '(?i)OPENSWBF2 VENDOR steam=\w+ galaxy=\w+ d3dx9=[a-z]:\\[^\r\n]*\\artifacts\\smoke-workers\\' + $script:SmokePlatform + '\\[^\\\r\n]+\\[^\\\r\n]+\\w\d+\\d3dx9_43\.dll msvcr120='
        foreach ($c in @($cases | Where-Object { $_.Name -like 'vendor-*' })) { $c.Expect.LogRx = @($c.Expect.LogRx) + $vD3dx }
    }
    if ($which -eq 'Boot') {
        $shellRx = @('(?m)OPENSWBF2 SOAK shell=300\r?$', 'OPENSWBF2 RUN COMPLETE', '(?s)\A(?!.*OPENSWBF2 MISSION RUNNING)')
        $shellArgs = "$tail -soakturns 300"
        if ($script:SmokePlatform -eq 'x64') {
            $shellRx += $script:SmokeAddrSpaceRxX64
            $shellArgs = "$tail -hogaddrspace -soakturns 300"
        }
        $cases += New-Object PSObject -Property @{ Name = 'boot-shell'; Args = $shellArgs; Expect = @{ Class = 'Pass'; LogRx = $shellRx }; Twice = 0 }
    }
    if ($which -eq 'Boot' -and $script:SmokePlatform -eq 'x64') {
        foreach ($m in 'hot1g_con', 'tat2g_eli', 'nab2g_c') {
            $cases += New-Object PSObject -Property @{ Name = "boot-$m"; Args = "$m $tail -hogaddrspace -soakturns 1501"; Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 MISSION RUNNING', 'OPENSWBF2 SIM DIGEST turns=1500 ', 'OPENSWBF2 SOAK turns=', $script:SmokeAddrSpaceRxX64, '(?s)\A(?!.*refused size)', $noWarpRx); HeapCapsX64 = $true }; Twice = 0 }
        }
    }
    if ($which -eq 'All') {
        foreach ($line in $missions) {
            $tok = @($line.Trim() -split '\s+')
            if ($tok[0]) {
                $class = 'Pass'
                if ($tok.Count -gt 1) { $class = $tok[1] }
                if ($tok.Count -gt 2) { throw "missions file line '$($line.Trim())' has more than a mission and a class" }
                $cases += & $allCase $tok[0] $class
            }
        }
    }
    if ($which -eq 'Dedicated' -or $which -eq 'All') {
        $dm = $standard
        if ($dedicatedSpec) { $dm = $dedicatedSpec }
        $cases += New-Object PSObject -Property @{
            Name = 'dedicated'
            Args = "/autonet dedicated -lan -nopregame -playercount 0 $dm 100 100 $htail"
            Expect = $pass
            Twice = 0
        }
    }
    if ($which -eq 'Crash' -or $which -eq 'All') {
        foreach ($kind in 'nullderef', 'divzero', 'stackoverflow', 'debugbreak', 'abort', 'invalidparam') {
            $cases += New-Object PSObject -Property @{
                Name = "crash-$kind"
                Args = "$standard $tail -crashtest $kind"
                Expect = @{ Class = 'Fatal'; Code = 3; MinDumps = 1; MaxDumps = 1; LogRx = 'FATAL:.*\.dmp' }
                Twice = 0
            }
        }
        if ($script:SmokePlatform -eq 'x64') {
            $nd = @($cases | Where-Object { $_.Name -eq 'crash-nullderef' })[0]
            $nd.Args += ' -hogaddrspace'
            $nd.Expect.LogRx = @('FATAL: exception 0xC0000005 at 0x[0-9A-F]{9,16} \(BattlefrontII\.exe\+0x[0-9A-F]+\) build [0-9A-F]+ dump .*\.dmp', $script:SmokeAddrSpaceRxX64)
            $nd.Expect.DumpMatchesFatal = $true
        }
        $luaOomSize = 0x4000000 + 4 * $(if ($script:SmokePlatform -eq 'x64') { 8 } else { 4 })
        $luaOomRx = @(
            "^\[t=\d+\.\d{3}\] \[f=\d+\] LogMemory Error Battlefront2\\Source\\MemFailLog\.cpp:\d+: alloc failure heap Lua \(\d+\) size $luaOomSize ",
            "^\[t=\d+\.\d{3}\] \[f=\d+\] LogLua Error Battlefront2\\Source\\LuaMemory\.cpp:\d+: Lua out of memory \(\d+ used in \d+ blocks, trying to alloc $luaOomSize\)$"
        )
        $luaOomLogRx = @(('(?m)' + $luaOomRx[0]), ('(?m)' + $luaOomRx[1].TrimEnd('$') + '\r?$'))
        $ff = @{ Class = 'GsFailure'; Code = 3221226505 }
        $werKey = $script:SmokeWerKey
        if ($null -eq $werKey) { $werKey = Test-WerKey }
        if ($werKey) { $ff.MinDumps = 1 }
        $cases += New-Object PSObject -Property @{ Name = 'crash-fastfail'; Args = "$standard $tail -crashtest fastfail"; Expect = $ff; Twice = 0 }
        if ($script:SmokePlatform -eq 'Win32') {
            $cases += New-Object PSObject -Property @{
                Name = 'crash-delayload'
                Args = "$standard $tail -crashtest delayload"
                Omit = @('steam_api.dll')
                Expect = @{ Class = 'Fatal'; Code = 3; MinDumps = 1; MaxDumps = 1; LogRx = @('FATAL:.*\.dmp', 'FATAL: exception 0xC06D007E at 0x[0-9A-F]+ \([^)]+\) dll=steam_api\.dll build ', 'OPENSWBF2 VENDOR steam off: steam_api\.dll missing', '(?s)\A(?!.*VENDOR steam off.*VENDOR steam off)') }
                Twice = 0
            }
        }
        else {
            $cases += New-Object PSObject -Property @{
                Name = 'crash-delayload-x64'
                Args = "$standard $tail -crashtest delayload"
                Omit = @('steam_api64.dll')
                Expect = @{ Class = 'Fatal'; Code = 3; MinDumps = 1; MaxDumps = 1; LogRx = @('FATAL:.*\.dmp', 'FATAL: exception 0xC06D007E at 0x[0-9A-F]+ \([^)]+\) dll=steam_api64\.dll build ', 'OPENSWBF2 VENDOR steam off: steam_api64\.dll missing', '(?s)\A(?!.*VENDOR steam off.*VENDOR steam off)') }
                Twice = 0
            }
        }
        foreach ($kind in 'early', 'heapinit') {
            $cases += New-Object PSObject -Property @{
                Name = "crash-$kind"
                Args = "$standard $tail -crashtest $kind"
                Expect = @{ Class = 'Fatal'; Code = 3; MinDumps = 1; MaxDumps = 1; LogRx = @('FATAL: exception 0xC0000005 at 0x[0-9A-F]+ \(BattlefrontII\.exe\+0x[0-9A-F]+\) build [0-9A-F]+ dump .*\.dmp', '(?s)\A(?!.*(?:Opened logfile|log start).*FATAL:)') }
                Twice = 0
            }
        }
        $cases += New-Object PSObject -Property @{
            Name = 'fatal-init'
            Args = "no_such_mission $tail"
            Expect = @{ Class = 'Fatal'; Code = 1; MaxDumps = 0; LogRx = 'FATAL: initialization failed' }
            Twice = 0
        }
        foreach ($kind in 'arenaoom', 'debugheapoom') {
            $cases += New-Object PSObject -Property @{
                Name = "crash-$kind"
                Args = "$standard $tail -crashtest $kind"
                Expect = @{ Class = 'Fatal'; Code = 1; MaxDumps = 0; LogRx = ('FATAL: out of memory allocating ' + $(if ($kind -eq 'arenaoom') { 'application heap' } else { 'debug heap' }) + ' \(\d+ bytes\) load \d+% phys \d+ MB virt \d+ MB') }
                Twice = 0
            }
        }
        if ($script:SmokeDigestConfiguration -eq 'Test') {
            $cases += New-Object PSObject -Property @{
                Name = 'crash-buildheapfail'
                Args = "$standard $tail -crashtest buildheapfail"
                Expect = @{ Class = 'Fatal'; Code = 1; MaxDumps = 0; LogRx = ('FATAL: out of memory allocating Lua heap \(' + $(if ($script:SmokePlatform -eq 'x64') { 29229056 } else { 16777216 }) + ' bytes\) load \d+% phys \d+ MB virt \d+ MB') }
                Twice = 0
            }
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-heapfail'
            Args = "$standard $tail -crashtest heapfail"
            Expect = @{ Class = 'Pass'; LogRx = 'alloc failure heap \S+ \(\d+\) size \d+ free \d+ load \d+% phys \d+ MB virt \d+ MB used \d+ cap \d+ peak \d+ largestfree \d+' }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-capexhaust'
            Args = "$standard $tail -heapcap Runtime=512M -crashtest capexhaust"
            Expect = @{ Class = 'Fatal'; Code = 3; MinDumps = 1; MaxDumps = 1; LogRx = @('alloc failure heap Runtime \(\d+\) size 1048576 .* cap 536870912 ', 'FATAL: '); ErrorRx = @('alloc failure heap Runtime \(\d+\) size 1048576') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-midoublefree'
            Args = "$standard $tail -crashtest midoublefree"
            Expect = @{ Class = 'Fatal'; Code = 3; MinDumps = 1; MaxDumps = 1; LogRx = @('OPENSWBF2 MIERROR err=11', 'FATAL: '); ErrorRx = @('OPENSWBF2 MIERROR err=11') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-poolcount'
            Args = "$standard $tail -crashtest poolcount"
            Expect = @{ Class = 'Pass'; LogRx = @('LogMemory Warning RedEngineFL\\Memory\\RedScriptLimits\.h:\d+: SetMemoryPoolSize\(crashtestpool\): script value 2147483647 clamped to \d+', 'LogMemory Warning RedEngineFL\\Memory\\RedMemoryPool\.cpp:\d+: Memory pool "crashtesthuge" refused count 1073741825 size 16') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'pool-full'
            Args = "$standard $tail -crashtest poolfull"
            Expect = @{ Class = 'Pass'; LogRx = @('LogMemory Warning RedEngineFL\\Memory\\RedMemoryPool\.cpp:\d+: Memory pool "crashtestpool" is full; raise count to at least 3', '(?s)\A(?!.*crashtestpool" is full.*crashtestpool" is full)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'pool-full-strict'
            Args = "$standard $tail -crashtest poolfull -poolerrors"
            Expect = @{ Class = 'Pass'; LogRx = @('LogMemory Error RedEngineFL\\Memory\\RedMemoryPool\.cpp:\d+: Memory pool "crashtestpool" is full; raise count to at least 3'); ErrorRx = @('^\[t=\d+\.\d{3}\] \[f=\d+\] LogMemory Error RedEngineFL\\Memory\\RedMemoryPool\.cpp:\d+: Memory pool "[^"]*" is full; raise count to at least \d+$') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-stickdelete'
            Args = "spa3c_ass $tail -crashtest stickdelete"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 STICKDELETE prop=1 gameobject=0 stuck=1 cleared=1 fell=1', '(?s)\A(?!.*FATAL:)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-particlesound'
            Args = "$standard $tail -crashtest particlesound"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 PARTICLESOUND class=0x[0-9A-F]{8} deleted=1 reset=1 voice_after_reset=0', '(?s)\A(?!.*FATAL:)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-particleburst'
            Args = "$standard $tail -crashtest particleburst"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 PARTICLEBURST class=0x[0-9A-F]{8} playing=1 released=1 voice_on_next=1', '(?s)\A(?!.*FATAL:)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-luaoom'
            Args = "$standard $tail -crashtest luaoom"
            Expect = @{ Class = 'Pass'; LogRx = $luaOomLogRx; ErrorRx = $luaOomRx }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-luapanic'
            Args = "$standard $tail -crashtest luapanic"
            Expect = @{ Class = 'Fatal'; Code = 4; MaxDumps = 0; LogRx = 'FATAL: lua panic' }
            Twice = 0
        }
    }
    if ($which -eq 'Crash' -or $which -eq 'All') {
        $cases += New-Object PSObject -Property @{
            Name = 'crash-hang'
            Args = "$standard $($tail.Replace(' -hangtimeout 0', '')) -crashtest hang -hangtimeout 5"
            Expect = @{ Class = 'Fatal'; Code = 5; MinDumps = 1; MaxDumps = 1; LogRx = 'FATAL: hang' }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-stall'
            Args = "$standard $tail -crashtest hang"
            Expect = @{ Class = 'Fatal'; Code = 5; MinDumps = 1; MaxDumps = 1; Stall = 1; LogRx = 'FATAL: hang \(supervisor saw no progress' }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-check'
            Args = "$standard $tail -crashtest check"
            Expect = @{ Class = 'Fatal'; Code = 3; MinDumps = 1; MaxDumps = 1; LogRx = @('FATAL: check failed', 'ensure summary: 1 failures at 1 sites') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-resetinitlost'
            Args = "$standard $tail -crashtest resetinitlost"
            Expect = @{ Class = 'Pass'; LogRx = 'OPENSWBF2 RESET recovered after 4 attempts' }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-resetrecover'
            Args = "$standard $tail -crashtest resetrecover"
            Expect = @{ Class = 'Pass'; LogRx = 'OPENSWBF2 RESET recovered after 4 attempts' }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-resetflap'
            Args = "$standard $tail -crashtest resetflap"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 RESET recovered after 25 attempts', '(?s)\A(?!.*OPENSWBF2 RESET failed.*OPENSWBF2 RESET failed)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-resetlaterinit'
            Args = "$standard $tail -crashtest resetlaterinit"
            Expect = @{ Class = 'InitFailure'; Code = 0; MaxDumps = 0; LogRx = '(?s)OPENSWBF2 MISSION RUNNING.*OPENSWBF2 CLOSE deferred until loading finishes.*OPENSWBF2 RESET recovered after 2 attempts' }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-resetfatal'
            Args = "$standard $tail -crashtest resetfatal"
            Expect = @{ Class = 'Fatal'; Code = 1; MinDumps = 1; MaxDumps = 1; LogRx = '(?s)\A(?:(?!FATAL:).)*OPENSWBF2 RESET reverting asked=\d+x\d+ windowed=\d used=\d+x\d+ windowed=\d(?:(?!FATAL:).)*FATAL: device reset failed hr=0x8876086C(?:(?!FATAL:).)*\z' }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-applymode'
            Args = "$standard $tail -crashtest applymode"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY applied \d+x\d+ mode=', 'OPENSWBF2 APPLY TARGETS alive=\d+ fresh=(?:[2-9]|[1-9]\d+) stale=0 buffers=3/3 moved=1 ', '(?s)OPENSWBF2 DISPLAY applied (\d+)x(\d+) .*OPENSWBF2 DISPLAY relayout generation=\d+ screen=\1x\2 view=\1x\2 hud=\1x\2 aspect=(\d+\.\d{4}) camera=\3\b', '(?s)\A(?!.*FATAL:)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-applymodesmall'
            Args = "$standard $tail -crashtest applymodesmall"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY applied 320x240 mode=', 'OPENSWBF2 APPLY TARGETS alive=\d+ fresh=(?:[3-9]|[1-9]\d+) stale=0 buffers=3/3 moved=3 ', '(?s)OPENSWBF2 DISPLAY applied (\d+)x(\d+) .*OPENSWBF2 DISPLAY relayout generation=\d+ screen=\1x\2 view=\1x\2 hud=\1x\2 ', '(?s)\A(?!.*FATAL:)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-applyfail'
            Args = "$standard $tail -crashtest applyfail"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY reverted asked=(\d+)x(\d+) mode=\d+ used=(?!\1x\2 )', '(?s)OPENSWBF2 DISPLAY reverted .*OPENSWBF2 RESET recovered after \d+ attempts', '(?s)OPENSWBF2 DISPLAY reverted .*OPENSWBF2 DISPLAY profile restored ', '(?s)\A(?!.*FATAL:)', '(?s)\A(?!.*OPENSWBF2 DISPLAY applied)', '(?s)\A(?!.*OPENSWBF2 DISPLAY relayout)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-applypresentfail'
            Args = "$standard $tail -crashtest applypresentfail"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY reverted asked=(\d+)x(\d+) mode=\d+ used=(?!\1x\2 )', '(?s)\A(?!.*FATAL:)', '(?s)\A(?!.*OPENSWBF2 DISPLAY applied)', '(?s)OPENSWBF2 DISPLAY reverted asked=\d+x\d+ mode=\d+ used=(\d+)x(\d+) .*OPENSWBF2 DISPLAY relayout generation=\d+ screen=\1x\2 view=\1x\2 hud=\1x\2 (?!.*OPENSWBF2 DISPLAY relayout)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-applytargetfail'
            Args = "$standard $tail -crashtest applytargetfail"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY reverted asked=(\d+)x(\d+) mode=\d+ used=(?!\1x\2 )\d+x\d+ mode=\d+ hr=0x8876017C', '(?s)\A(?!.*FATAL:)', '(?s)\A(?!.*OPENSWBF2 DISPLAY applied)', '(?s)OPENSWBF2 DISPLAY reverted asked=\d+x\d+ mode=\d+ used=(\d+)x(\d+) .*OPENSWBF2 DISPLAY relayout generation=\d+ screen=\1x\2 view=\1x\2 hud=\1x\2 (?!.*OPENSWBF2 DISPLAY relayout)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-gammapass'
            Args = "$standard $tail -crashtest gammapass"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 GAMMA path=pass', '(?s)OPENSWBF2 GAMMA FAKE target=(\d+)x(\d+) alive=1 bb=\1x\2 draws=[1-9]\d* ramp_nonid=0.*OPENSWBF2 DISPLAY applied \1x\2 ', '(?s)OPENSWBF2 RESET recovered after \d+ attempts.*OPENSWBF2 GAMMA FAKE target=(\d+)x(\d+) alive=1 bb=\1x\2 draws=[1-9]\d* ramp_nonid=0', '(?s)\A(?!.*ramp_nonid=[1-9])', '(?s)\A(?!.*OPENSWBF2 GAMMA fallback)', '(?s)\A(?!.*FATAL:)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-gammafallback'
            Args = "$standard $tail -crashtest gammafallback"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 GAMMA path=ramp reason=createfail hr=0x8876017C', 'OPENSWBF2 GAMMA FAKE target=\d+x\d+ alive=0 bb=\d+x\d+ draws=0 ramp_nonid=[1-9]', '(?s)\A(?!.*OPENSWBF2 GAMMA path=pass)', '(?s)\A(?!.*draws=[1-9])', '(?s)\A(?!.*FATAL:)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-gammalost'
            Args = "$standard $tail -crashtest gammalost"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 GAMMA path=pass', 'OPENSWBF2 GAMMA FAKE target=(\d+)x(\d+) alive=1 bb=\1x\2 draws=[1-9]\d* ramp_nonid=0', '(?s)\A(?!.*OPENSWBF2 GAMMA path=ramp)', '(?s)\A(?!.*OPENSWBF2 GAMMA fallback)', '(?s)\A(?!.*ramp_nonid=[1-9])', '(?s)\A(?!.*FATAL:)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-shellstretch'
            Args = "$standard $tail -crashtest shellstretch"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY shell stretch linear (\d+)x(\d+)->(\d+)x(\d+)', '(?s)OPENSWBF2 SHELLSTRETCH FAKE chain=\d+x\d+ presents=[1-9]\d* filter=linear.*OPENSWBF2 RESET recovered after \d+ attempts.*OPENSWBF2 SHELLSTRETCH FAKE chain=\d+x\d+ presents=[1-9]\d* filter=linear', '(?s)\A(?!.*shell stretch unavailable)', '(?s)\A(?!.*FATAL:)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-gammadrawfail'
            Args = "$standard $tail -crashtest gammadrawfail"
            Expect = @{ Class = 'Pass'; LogRx = @('(?s)OPENSWBF2 GAMMA path=pass.*OPENSWBF2 GAMMA fallback to ramp reason=drawfail hr=0x8876086C', 'OPENSWBF2 GAMMA FAKE target=\d+x\d+ alive=0 bb=\d+x\d+ draws=0 ramp_nonid=[1-9]', '(?s)\A(?!.*OPENSWBF2 GAMMA fallback.*OPENSWBF2 GAMMA fallback)', '(?s)\A(?!.*draws=[1-9])', '(?s)\A(?!.*FATAL:)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-applyadapter'
            Args = "$standard $tail -crashtest applyadapter -displayini ""{CASEDIR}\display.ini"""
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY adapter=1 invalid', 'OPENSWBF2 DISPLAY adapter=0 pending, running adapter unchanged', '(?s)\A(?!.*OPENSWBF2 DISPLAY adapter=\d+ saved)', '(?s)\A(?!.*FATAL:)'); NoIni = $true }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-adaptercancel'
            Args = "$standard $tail -crashtest adaptercancel -displayini ""{CASEDIR}\display.ini"""
            Expect = @{ Class = 'Pass'; LogRx = @('(?s)OPENSWBF2 DISPLAY adapter=0 pending.*OPENSWBF2 DISPLAY menu cancel restored.*OPENSWBF2 DISPLAY adaptertest save=0', '(?s)\A(?!.*OPENSWBF2 DISPLAY adapter=\d+ saved)', '(?s)\A(?!.*FATAL:)'); NoIni = $true }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-adapteraccept'
            Args = "$standard $tail -crashtest adapteraccept -displayini ""{CASEDIR}\display.ini"""
            Expect = @{ Class = 'Pass'; LogRx = @('(?s)OPENSWBF2 DISPLAY adapter=0 pending, running adapter unchanged.*OPENSWBF2 DISPLAY adapter=0 saved, running adapter unchanged.*OPENSWBF2 DISPLAY menutest save=1', 'OPENSWBF2 DISPLAY confirm timeout, reverting', 'OPENSWBF2 DISPLAY menutest done result=2', '(?s)\A(?!.*FATAL:)'); IniRx = @('(?m)^Adapter=0\r?$', '(?s)\A(?!.*\bVsync=)(?!.*\bAdapter=1)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-menuconfirm'
            Args = "$standard $tail -crashtest menuconfirm"
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY confirm pending 15s', '(?s)OPENSWBF2 DISPLAY confirm pending 15s.*OPENSWBF2 DISPLAY menutest plain vsync=\d.*OPENSWBF2 DISPLAY confirm timeout, reverting to ', '(?s)OPENSWBF2 DISPLAY menutest base (\d+x\d+).*OPENSWBF2 DISPLAY confirm timeout, reverting to \1 mode=', '(?s)OPENSWBF2 DISPLAY confirm timeout, reverting to (\d+x\d+) .*OPENSWBF2 DISPLAY applied \1 ', '(?s)OPENSWBF2 DISPLAY menutest base \d+x\d+.*?OPENSWBF2 DISPLAY applied \d+x\d+ mode=\w+ refresh=\S+ vsync=(\w+) .*OPENSWBF2 DISPLAY confirm timeout, reverting to .*?OPENSWBF2 DISPLAY applied \d+x\d+ mode=\w+ refresh=\S+ vsync=\1 ', '(?s)OPENSWBF2 DISPLAY menutest plain vsync=\d.*?OPENSWBF2 DISPLAY applied \d+x\d+ mode=\w+ refresh=\S+ vsync=(\w+) .*OPENSWBF2 DISPLAY confirm timeout, reverting to .*?OPENSWBF2 DISPLAY applied \d+x\d+ mode=\w+ refresh=\S+ vsync=(?!\1 )', 'OPENSWBF2 DISPLAY menutest done result=2', '(?s)OPENSWBF2 DISPLAY menutest base \d+x\d+ fps=(\d+) waitquery=(\d)\b.*OPENSWBF2 DISPLAY confirm timeout, reverting to \d+x\d+ mode=\w+ fps=\1 waitquery=\2\b.*OPENSWBF2 DISPLAY menutest done result=2 fps=\1 waitquery=\2\b','(?s)\A(?!.*OPENSWBF2 DISPLAY confirm kept)', '(?s)\A(?!.*FATAL:)', 'OPENSWBF2 DISPLAY menu options built adapters=\d+', '(?s)\A(?!.*OPENSWBF2 LUA error)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-menukeep'
            Args = "$standard $tail -crashtest menukeep -displayini ""{CASEDIR}\display.ini"""
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY confirm pending 15s', 'OPENSWBF2 DISPLAY confirm kept', 'OPENSWBF2 DISPLAY menutest done result=1', 'OPENSWBF2 DISPLAY menutest save=1', 'OPENSWBF2 DISPLAY menu save deferred until the confirm ends', '(?s)OPENSWBF2 DISPLAY confirm kept.*OPENSWBF2 DISPLAY menu saved keys=0x', 'menutest done result=1 fps=\d+ waitquery=\d keepsave=0', '(?s)\A(?!.*OPENSWBF2 DISPLAY confirm timeout)', '(?s)\A(?!.*FATAL:)'); IniRx = @('(?m)^Vsync=2\r?$', '(?s)\A(?!.*\bFps=)(?!.*\bRefresh=)(?!.*\bNoWaitQuery=)(?!.*\bBorderless=)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-menusavefail'
            Args = "$standard $tail -crashtest menusavefail -displayini ""{CASEDIR}\display.ini"""
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY menutest save=1', '(?s)OPENSWBF2 DISPLAY confirm kept.*OPENSWBF2 DISPLAY menu save failed', 'OPENSWBF2 DISPLAY menutest save=2', 'menutest done result=1 fps=\d+ waitquery=\d keepsave=2', '(?s)\A(?!.*FATAL:)'); IniRx = @('(?s)\A(?!.*\bVsync=)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-menuorder'
            Args = "$standard $tail -crashtest menuorder"
            Expect = @{ Class = 'Pass'; LogRx = @('(?s)OPENSWBF2 DISPLAY menutest order=legacy-menu(?:(?!OPENSWBF2 DISPLAY applied ).)*OPENSWBF2 DISPLAY applied (?:(?!OPENSWBF2 DISPLAY applied ).)*OPENSWBF2 DISPLAY confirm pending 15s', 'OPENSWBF2 DISPLAY confirm timeout, reverting to ', 'OPENSWBF2 DISPLAY menutest done result=2', '(?s)\A(?!.*FATAL:)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-menuorderrev'
            Args = "$standard $tail -crashtest menuorderrev"
            Expect = @{ Class = 'Pass'; LogRx = @('(?s)OPENSWBF2 DISPLAY menutest order=menu-legacy(?:(?!OPENSWBF2 DISPLAY applied ).)*OPENSWBF2 DISPLAY applied (?:(?!OPENSWBF2 DISPLAY applied ).)*OPENSWBF2 DISPLAY confirm pending 15s', 'OPENSWBF2 DISPLAY confirm timeout, reverting to ', 'OPENSWBF2 DISPLAY menutest done result=2', '(?s)\A(?!.*FATAL:)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-menurevertsave'
            Args = "$standard $tail -crashtest menurevertsave -displayini ""{CASEDIR}\display.ini"""
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY menutest save=1', '(?s)OPENSWBF2 DISPLAY confirm pending 15s.*OPENSWBF2 DISPLAY confirm timeout, reverting to .*OPENSWBF2 DISPLAY menu saved keys=0x10\b.*OPENSWBF2 DISPLAY menu cancel restored', '(?s)OPENSWBF2 DISPLAY menutest prior vsync=(\d+) fps=(\d+)\b.*OPENSWBF2 DISPLAY menutest restored vsync=\1 fps=(?!\2\b)\d+', 'OPENSWBF2 DISPLAY menutest done result=2 fps=\d+ waitquery=\d keepsave=0', '(?s)\A(?!.*FATAL:)'); IniRx = @('(?m)^Fps=(?:30|60)\r?$', '(?s)\A(?!.*\bVsync=)(?!.*\bBorderless=)(?!.*\bRefresh=)') }
            Twice = 0
        }
        $cases += New-Object PSObject -Property @{
            Name = 'crash-menuapplyfail'
            Args = "$standard $tail -crashtest menuapplyfail -displayini ""{CASEDIR}\display.ini"""
            Expect = @{ Class = 'Pass'; LogRx = @('OPENSWBF2 DISPLAY menutest save=1', '(?s)OPENSWBF2 DISPLAY menutest base \d+x\d+ fps=\d+ waitquery=(\d)\b.*OPENSWBF2 DISPLAY menutest prior vsync=\d+ fps=\d+ accept=(\d+)\b.*OPENSWBF2 DISPLAY reverted asked=.*OPENSWBF2 DISPLAY profile restored \d+x\d+ windowed=\d vsync=\d fps=\2 waitquery=\1\b.*OPENSWBF2 DISPLAY menu saved keys=0x10\b.*OPENSWBF2 DISPLAY menutest done result=0 fps=\2 waitquery=\1 keepsave=0', '(?s)\A(?!.*OPENSWBF2 DISPLAY confirm pending)', '(?s)\A(?!.*OPENSWBF2 DISPLAY applied)', '(?s)\A(?!.*FATAL:)'); IniRx = @('(?m)^Fps=(?:30|60)\r?$', '(?s)\A(?!.*\bVsync=)(?!.*\bBorderless=)(?!.*\bRefresh=)(?!.*\bNoWaitQuery=)') }
            Twice = 0
        }
    }
    if ($which -eq 'Twice') {
        foreach ($i in 1, 2) {
            $cases += New-Object PSObject -Property @{ Name = "twice-$i"; Args = "$standard $htail"; Expect = $pass; Twice = $i }
        }
        $digestExpect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 MISSION RUNNING', 'OPENSWBF2 SIM DIGEST turns=', 'OPENSWBF2 SOAK turns=') + $hrx) }
        foreach ($i in 1, 2) {
            $cases += New-Object PSObject -Property @{ Name = "twice-digest-$i"; Args = "nab2g_c $htail -soakturns 1501"; Expect = $digestExpect; Twice = 2 + $i }
        }
        foreach ($m in 'hot1g_con', 'spa3c_ass') {
            $cases += New-Object PSObject -Property @{ Name = "digest-$m"; Args = "$m $htail"; Expect = $pass; Twice = 0 }
        }
        $traceExpect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 DIGEST TRACE turn=1500 ') + $hrx) }
        foreach ($i in 1, 2) {
            $ttail = $htail.Replace('-seed 1 ', "-seed $i ")
            $cases += New-Object PSObject -Property @{ Name = "trace-seed-$i"; Args = "hot1g_con $ttail -digesttrace 1600"; Expect = $traceExpect; Twice = 0 }
        }
    }
    if ($which -eq 'Soak') {
        $cases += New-Object PSObject -Property @{ Name = 'asan-soak'; Args = "nab2g_c $htail -soakturns 815800"; Expect = @{ Class = 'Pass'; LogRx = (@('OPENSWBF2 MISSION RUNNING', 'OPENSWBF2 SIM DIGEST turns=', 'OPENSWBF2 SOAK turns=') + $hrx) }; Twice = 0 }
    }
    if ($which -eq 'Perturb') {
        $all = @('geo1c_con', 'nab2g_c', 'hot1g_con', 'spa3c_ass', 'dag1c_ctf', 'kas2c_uber', 'tan1g_con')
        $pm = $all
        if ($env:OPENSWBF2_PERTURB_MISSIONS) {
            $pm = @($env:OPENSWBF2_PERTURB_MISSIONS -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ })
            foreach ($m in $pm) { if ($all -notcontains $m) { throw "OPENSWBF2_PERTURB_MISSIONS: '$m' is not a Perturb mission" } }
        }
        foreach ($m in $pm) {
            $s = 0
            for ($i = 0; $i -lt $all.Count; $i++) { if ($all[$i] -eq $m) { $s = 1 + ($i % 2) } }
            $runs = @(@{ Mode = 'base'; Seed = 0; Extra = '' })
            foreach ($mode in 'pad', 'fill', 'pool') { $runs += @{ Mode = $mode; Seed = $s; Extra = " -perturbheap ${s}:$mode" } }
            $runs += @{ Mode = 'hog'; Seed = 0; Extra = ' -hogaddrspace' }
            $runs += @{ Mode = 'repeat'; Seed = 0; Extra = ' -repeatmission' }
            foreach ($r in $runs) {
                $rx = @('(?s)\A(?!.*OPENSWBF2 PERTURB )')
                if ($r.Seed -gt 0) { $rx = @("(?m)OPENSWBF2 PERTURB seed=$($r.Seed) modes=$($r.Mode)\r?$") }
                $soakArg = ''
                if ($r.Mode -eq 'hog') { $rx += Get-SmokeAddrSpaceRx }
                if ($r.Mode -eq 'repeat') {
                    $rx += @("(?m)OPENSWBF2 REPEAT to_shell mission=$m\r?$", "(?m)OPENSWBF2 REPEAT to_mission mission=$m\r?$")
                }
                if ($m -eq 'nab2g_c') {
                    $soakArg = " -soakturns $script:PerturbSoakTurns"
                    $rx += 'OPENSWBF2 SOAK turns='
                }
                $name = "perturb-$m-$($r.Mode)"
                if ($r.Seed -gt 0) { $name = "$name-$($r.Seed)" }
                $x = @{ Class = 'Pass'; LogRx = $rx; Perturb = @{ Mission = $m; Mode = $r.Mode; Seed = $r.Seed } }
                $cases += New-Object PSObject -Property @{ Name = $name; Args = "$m $tail -digesttrace 100000$($r.Extra)$soakArg"; Expect = $x; Twice = 0 }
            }
        }
    }
    foreach ($c in $cases) {
        if ($c.Args -match '(^| )-soakturns ') { continue }
        $c.Args += " -soakturns $script:SmokeTurnCap"
        $x = $c.Expect.Clone()
        $x.LogRx = @(@($x.LogRx) | Where-Object { $_ }) + $script:SmokeCapRx
        $c.Expect = $x
    }
    return $cases
}
