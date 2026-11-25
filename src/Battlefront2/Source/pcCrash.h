#pragma once

struct _EXCEPTION_POINTERS;

namespace pcCrash
{
    void Install();
    void Reinstall();
    long Filter(struct _EXCEPTION_POINTERS* pInfo);
    void __cdecl Exit(int iCode);
    void SetLogPath(char const* pcPath);
    void SetCrashTest(char const* pcKind);
    void RunCrashTest();
    void RunMissionCrashTest();
    void FatalInit(char const* pcWhat, unsigned int uiSize);
    void FatalExit(char const* pcReason);
    bool SimulateAllocFailure(char const* pcKind);
    void EarlyCrashTest(char const* pcKind);
    bool FindCommandArg(char const* pcName, char const** ppcVal, int* piLen);
    void ReserveLowAddressSpace();
    bool IsLargeAddressAware();
    void __cdecl Check(char const* pcExpr, char const* pcFile, int iLine);
    void Tick();
    bool SetHangTimeout(int iSeconds);
    bool Supervised();
    void StartWatchdog();
    char const* GetBuildId();
    char const* GetCrashTest();
    void FatalReset(long hr, unsigned int uiFailures, unsigned int uiElapsedMs);
#ifdef OPENSWBF2_TEST
    void InitPerturbFromCommandLine();
    void LogPerturbState();
    int RepeatMissionCount();
    void LogRepeatState();
    bool ShellFirstRequested();
    bool TakeShellFirst();
    bool ShellFirstTaken();
    bool ExitOnMovieCloseRequested();
    bool HandsOffRequested();
    struct SaveTestWrite
    {
        bool bWritten;
        bool bOk;
        long lBytes;
        unsigned int uFields;
        char acName[128];
    };
    struct SaveTestRecord
    {
        SaveTestWrite akWrites[2];
        bool bLoaded;
        bool bLoadOk;
        int iLoadType;
        unsigned int uLoadFields;
        char acLoadName[128];
        unsigned int uProblems;
    };
    const SaveTestRecord& GetSaveTestRecord();
    enum ScenarioEventKind
    {
        kScenarioApplied,
        kScenarioReverted,
        kScenarioRelayout,
        kScenarioApplyTargets,
        kScenarioResetRecovered,
        kScenarioResetFailed,
        kScenarioProfileRestored,
        kScenarioGammaPath,
        kScenarioGammaFallback,
        kScenarioGammaFake,
        kScenarioShellStretch,
        kScenarioShellStretchUnavailable,
        kScenarioShellStretchFake,
        kScenarioAdapterInvalid,
        kScenarioAdapterPending,
        kScenarioAdapterSaved,
        kScenarioConfirmPending,
        kScenarioConfirmKept,
        kScenarioConfirmRevert,
        kScenarioMenuTestBase,
        kScenarioMenuTestPrior,
        kScenarioMenuTestPlain,
        kScenarioMenuTestRestored,
        kScenarioMenuTestSave,
        kScenarioMenuTestDone,
        kScenarioMenuTestOrder,
        kScenarioAdapterTestSave,
        kScenarioMenuSaved,
        kScenarioMenuSaveDeferred,
        kScenarioMenuSaveFailed,
        kScenarioMenuCancelRestored,
        kScenarioMenuOptionsBuilt,
        kScenarioLuaError,
        kScenarioStickDelete,
        kScenarioParticleSound,
        kScenarioParticleBurst
    };
    struct ScenarioEvent
    {
        int iKind;
        unsigned int au[7];
        float af[2];
        char acText[16];
    };
    void NoteScenario(int iKind, unsigned int u0 = 0, unsigned int u1 = 0, unsigned int u2 = 0, unsigned int u3 = 0,
                      unsigned int u4 = 0, unsigned int u5 = 0, unsigned int u6 = 0, char const* pcText = 0);
    void NoteRelayout(unsigned int uScreenW, unsigned int uScreenH, unsigned int uViewW, unsigned int uViewH,
                      int iHudW, int iHudH, float fAspect, float fCamera);
    unsigned int ScenarioEventCount();
    ScenarioEvent const& GetScenarioEvent(unsigned int uIndex);
    bool ScenarioEventsDropped();
#endif
    char const* HogLayoutFault();
}

#ifdef OPENSWBF2_TEST
#define PCCRASH_TEST_ONLY(x) x
#else
#define PCCRASH_TEST_ONLY(x)
#endif
