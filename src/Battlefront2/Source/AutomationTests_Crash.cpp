#ifdef OPENSWBF2_TEST

#include <stdio.h>
#include <string.h>

#include "AutomationTest.h"
#include "pcCrash.h"

namespace
{

typedef pcCrash::ScenarioEvent Event;

int Count()
{
    return (int)pcCrash::ScenarioEventCount();
}

const Event& At(int i)
{
    return pcCrash::GetScenarioEvent((unsigned int)i);
}

bool Any(const Event&)
{
    return true;
}

template <typename F>
int After(int iFrom, int iKind, F fPred)
{
    if (iFrom < -1)
    {
        return -2;
    }
    for (int i = iFrom + 1; i < Count(); ++i)
    {
        if (At(i).iKind == iKind && fPred(At(i)))
        {
            return i;
        }
    }
    return -2;
}

template <typename F>
int First(int iKind, F fPred)
{
    return After(-1, iKind, fPred);
}

int First(int iKind)
{
    return After(-1, iKind, Any);
}

int After(int iFrom, int iKind)
{
    return After(iFrom, iKind, Any);
}

template <typename F>
int CountOf(int iKind, F fPred)
{
    int iCount = 0;
    for (int i = 0; i < Count(); ++i)
    {
        if (At(i).iKind == iKind && fPred(At(i)))
        {
            ++iCount;
        }
    }
    return iCount;
}

int CountOf(int iKind)
{
    return CountOf(iKind, Any);
}

int Last(int iKind)
{
    for (int i = Count() - 1; i >= 0; --i)
    {
        if (At(i).iKind == iKind)
        {
            return i;
        }
    }
    return -2;
}

bool Found(int i)
{
    return i >= 0;
}

bool TextIs(const Event& e, char const* pcText)
{
    return strcmp(e.acText, pcText) == 0;
}

bool Timeout(const Event& e)
{
    return TextIs(e, "timeout");
}

bool RelayoutAt(const Event& e, unsigned int uW, unsigned int uH)
{
    return e.au[0] == uW && e.au[1] == uH && e.au[2] == uW && e.au[3] == uH && e.au[4] == uW && e.au[5] == uH;
}

bool AspectIsCamera(const Event& e)
{
    char acAspect[32];
    char acCamera[32];
    _snprintf(acAspect, sizeof(acAspect) - 1, "%.4f", e.af[0]);
    _snprintf(acCamera, sizeof(acCamera) - 1, "%.4f", e.af[1]);
    acAspect[sizeof(acAspect) - 1] = 0;
    acCamera[sizeof(acCamera) - 1] = 0;
    return strcmp(acAspect, acCamera) == 0;
}

bool UsedDiffers(const Event& e)
{
    return !(e.au[2] == e.au[0] && e.au[3] == e.au[1]);
}

bool FakeLive(const Event& e)
{
    return e.au[2] == 1 && e.au[3] == e.au[0] && e.au[4] == e.au[1] && e.au[5] >= 1 && e.au[6] == 0;
}

bool FakeRamp(const Event& e)
{
    return e.au[2] == 0 && e.au[5] == 0 && e.au[6] >= 1;
}

bool AppliedThenRelayout(bool bAspect)
{
    for (int i = First(pcCrash::kScenarioApplied); Found(i); i = After(i, pcCrash::kScenarioApplied))
    {
        const unsigned int uW = At(i).au[0];
        const unsigned int uH = At(i).au[1];
        const int iRelayout = After(i, pcCrash::kScenarioRelayout,
                                    [&](const Event& e) { return RelayoutAt(e, uW, uH) && (!bAspect || AspectIsCamera(e)); });
        if (Found(iRelayout))
        {
            return true;
        }
    }
    return false;
}

void CheckApplyMode(FAutomationTestBase& t, bool bSmall)
{
    t.TestTrue("a display mode was applied", Found(First(pcCrash::kScenarioApplied, [&](const Event& e) {
                   return !bSmall || (e.au[0] == 320 && e.au[1] == 240);
               })));
    t.TestTrue("the apply rebuilt the render targets at the new size with none stale",
               Found(First(pcCrash::kScenarioApplyTargets, [&](const Event& e) {
                   return e.au[1] >= (bSmall ? 3u : 2u) && e.au[2] == 0 && e.au[3] == 3 && e.au[4] == (bSmall ? 3u : 1u);
               })));
    t.TestTrue("the screen, view and HUD were laid out at an applied size", AppliedThenRelayout(!bSmall));
}

void CheckApplyReverted(FAutomationTestBase& t, bool bTargetFail)
{
    t.TestTrue("the apply reverted to a mode other than the one asked for",
               Found(First(pcCrash::kScenarioReverted, [&](const Event& e) {
                   return UsedDiffers(e) && (!bTargetFail || e.au[4] == 0x8876017Cu);
               })));
    t.TestEqual("applied display modes", CountOf(pcCrash::kScenarioApplied), 0);
    const int iLast = Last(pcCrash::kScenarioRelayout);
    bool bAtReverted = false;
    for (int i = First(pcCrash::kScenarioReverted); Found(i) && i < iLast && !bAtReverted;
         i = After(i, pcCrash::kScenarioReverted))
    {
        bAtReverted = RelayoutAt(At(iLast), At(i).au[2], At(i).au[3]);
    }
    t.TestTrue("the last relayout is at a reverted size", bAtReverted);
}

void CheckApplyFail(FAutomationTestBase& t)
{
    const int iReverted = First(pcCrash::kScenarioReverted);
    t.TestTrue("the apply reverted to a mode other than the one asked for",
               Found(First(pcCrash::kScenarioReverted, UsedDiffers)));
    t.TestTrue("the device reset recovered after the revert", Found(After(iReverted, pcCrash::kScenarioResetRecovered)));
    t.TestTrue("the profile was restored after the revert", Found(After(iReverted, pcCrash::kScenarioProfileRestored)));
    t.TestEqual("applied display modes", CountOf(pcCrash::kScenarioApplied), 0);
    t.TestEqual("relayouts", CountOf(pcCrash::kScenarioRelayout), 0);
}

void CheckGamma(FAutomationTestBase& t, char const* pcKind)
{
    const bool bPass = strcmp(pcKind, "gammapass") == 0;
    const bool bLost = strcmp(pcKind, "gammalost") == 0;
    const bool bFallback = strcmp(pcKind, "gammafallback") == 0;
    const int iPass = First(pcCrash::kScenarioGammaPath, [](const Event& e) { return e.au[0] == 1; });
    if (bPass || bLost)
    {
        t.TestTrue("the gamma pass was chosen", Found(iPass));
        t.TestEqual("gamma falls back to the ramp", CountOf(pcCrash::kScenarioGammaFallback), 0);
        t.TestEqual("gamma ramps that are not the identity",
                    CountOf(pcCrash::kScenarioGammaFake, [](const Event& e) { return e.au[6] != 0; }), 0);
    }
    if (bPass)
    {
        bool bAppliedAtTarget = false;
        for (int i = First(pcCrash::kScenarioGammaFake, FakeLive); Found(i) && !bAppliedAtTarget;
             i = After(i, pcCrash::kScenarioGammaFake, FakeLive))
        {
            const unsigned int uW = At(i).au[0];
            const unsigned int uH = At(i).au[1];
            bAppliedAtTarget = Found(After(i, pcCrash::kScenarioApplied,
                                           [&](const Event& e) { return e.au[0] == uW && e.au[1] == uH; }));
        }
        t.TestTrue("the gamma pass drew into a live target at the back buffer size before a mode was applied at it",
                   bAppliedAtTarget);
        t.TestTrue("the gamma pass drew into a live target after the device reset recovered",
                   Found(After(First(pcCrash::kScenarioResetRecovered), pcCrash::kScenarioGammaFake, FakeLive)));
    }
    if (bLost)
    {
        t.TestTrue("the gamma pass drew into a live target at the back buffer size",
                   Found(First(pcCrash::kScenarioGammaFake, FakeLive)));
        t.TestEqual("gamma ramp paths",
                    CountOf(pcCrash::kScenarioGammaPath, [](const Event& e) { return e.au[0] == 0; }), 0);
    }
    if (bFallback || strcmp(pcKind, "gammadrawfail") == 0)
    {
        if (bFallback)
        {
            t.TestTrue("gamma took the ramp because the target could not be created",
                       Found(First(pcCrash::kScenarioGammaPath, [](const Event& e) {
                           return e.au[0] == 0 && e.au[1] == 0x8876017Cu && TextIs(e, "createfail");
                       })));
            t.TestEqual("gamma pass paths", CountOf(pcCrash::kScenarioGammaPath, [](const Event& e) { return e.au[0] == 1; }), 0);
        }
        else
        {
            t.TestTrue("gamma fell back to the ramp after a failed draw",
                       Found(After(iPass, pcCrash::kScenarioGammaFallback, [](const Event& e) {
                           return e.au[0] == 0x8876086Cu && TextIs(e, "drawfail");
                       })));
            t.TestTrue("gamma fell back at most once", CountOf(pcCrash::kScenarioGammaFallback) <= 1);
        }
        t.TestTrue("the ramp is set and no gamma target is live", Found(First(pcCrash::kScenarioGammaFake, FakeRamp)));
        t.TestEqual("gamma draws", CountOf(pcCrash::kScenarioGammaFake, [](const Event& e) { return e.au[5] != 0; }), 0);
    }
}

void CheckShellStretch(FAutomationTestBase& t)
{
    t.TestTrue("the shell stretch is linear",
               Found(First(pcCrash::kScenarioShellStretch, [](const Event& e) { return TextIs(e, "linear"); })));
    const auto fLinear = [](const Event& e) { return e.au[0] >= 1 && TextIs(e, "linear"); };
    const int iBefore = First(pcCrash::kScenarioShellStretchFake, fLinear);
    t.TestTrue("the stretch chain presented linearly before and after the device reset recovered",
               Found(After(After(iBefore, pcCrash::kScenarioResetRecovered), pcCrash::kScenarioShellStretchFake, fLinear)));
    t.TestEqual("shell stretch unavailable", CountOf(pcCrash::kScenarioShellStretchUnavailable), 0);
}

void CheckAdapter(FAutomationTestBase& t, char const* pcKind)
{
    if (strcmp(pcKind, "applyadapter") == 0)
    {
        t.TestTrue("adapter 1 was refused", Found(First(pcCrash::kScenarioAdapterInvalid, [](const Event& e) { return e.au[0] == 1; })));
        t.TestTrue("adapter 0 is pending with the running adapter unchanged",
                   Found(First(pcCrash::kScenarioAdapterPending, [](const Event& e) { return e.au[0] == 0 && e.au[1] == 1; })));
        t.TestEqual("saved adapters", CountOf(pcCrash::kScenarioAdapterSaved), 0);
        return;
    }
    if (strcmp(pcKind, "adaptercancel") == 0)
    {
        const int iPending = First(pcCrash::kScenarioAdapterPending, [](const Event& e) { return e.au[0] == 0; });
        t.TestTrue("adapter 0 was pending, the cancel restored the menu and nothing was saved",
                   Found(After(After(iPending, pcCrash::kScenarioMenuCancelRestored), pcCrash::kScenarioAdapterTestSave,
                               [](const Event& e) { return e.au[0] == 0; })));
        t.TestEqual("saved adapters", CountOf(pcCrash::kScenarioAdapterSaved), 0);
        return;
    }
    const int iPending = First(pcCrash::kScenarioAdapterPending, [](const Event& e) { return e.au[0] == 0 && e.au[1] == 1; });
    const int iSaved = After(iPending, pcCrash::kScenarioAdapterSaved, [](const Event& e) { return e.au[0] == 0 && e.au[1] == 1; });
    t.TestTrue("adapter 0 was pending, then saved with the running adapter unchanged, then the menu saved",
               Found(After(iSaved, pcCrash::kScenarioMenuTestSave, [](const Event& e) { return e.au[0] == 1; })));
    t.TestTrue("the confirm timed out", Found(First(pcCrash::kScenarioConfirmRevert, Timeout)));
    t.TestTrue("the menu test ended with result 2",
               Found(First(pcCrash::kScenarioMenuTestDone, [](const Event& e) { return e.au[0] == 2; })));
}

bool SameVsync(int iA, int iB)
{
    return Found(iA) && Found(iB) && strcmp(At(iA).acText, At(iB).acText) == 0;
}

void CheckMenuConfirm(FAutomationTestBase& t)
{
    const int iPending = First(pcCrash::kScenarioConfirmPending);
    t.TestTrue("the confirm was pending, the plain change applied and the confirm timed out",
               Found(After(After(iPending, pcCrash::kScenarioMenuTestPlain), pcCrash::kScenarioConfirmRevert, Timeout)));
    const int iBase = First(pcCrash::kScenarioMenuTestBase);
    t.TestTrue("the base was recorded", Found(iBase));
    if (!Found(iBase))
    {
        return;
    }
    const Event& rBase = At(iBase);
    t.TestTrue("the confirm timed out back to the base size", Found(After(iBase, pcCrash::kScenarioConfirmRevert, [&](const Event& e) {
                   return Timeout(e) && e.au[0] == rBase.au[0] && e.au[1] == rBase.au[1];
               })));
    const int iRevert = First(pcCrash::kScenarioConfirmRevert, Timeout);
    t.TestTrue("the reverted size was applied after the timeout",
               Found(iRevert) && Found(After(iRevert, pcCrash::kScenarioApplied, [&](const Event& e) {
                   return e.au[0] == At(iRevert).au[0] && e.au[1] == At(iRevert).au[1];
               })));
    const int iMenu = After(iBase, pcCrash::kScenarioApplied);
    t.TestTrue("the revert applied the vsync of the first menu apply",
               SameVsync(iMenu, After(After(iMenu, pcCrash::kScenarioConfirmRevert, Timeout), pcCrash::kScenarioApplied)));
    const int iPlain = After(First(pcCrash::kScenarioMenuTestPlain), pcCrash::kScenarioApplied);
    const int iPlainRevert =
        After(After(iPlain, pcCrash::kScenarioConfirmRevert, Timeout), pcCrash::kScenarioApplied);
    t.TestTrue("the revert did not keep the vsync of the plain change",
               Found(iPlain) && Found(iPlainRevert) && !SameVsync(iPlain, iPlainRevert));
    const int iFpsRevert = After(iBase, pcCrash::kScenarioConfirmRevert, [&](const Event& e) {
        return Timeout(e) && e.au[2] == rBase.au[2] && e.au[3] == rBase.au[3];
    });
    t.TestTrue("the revert and the end of the menu test kept the base fps and waitquery",
               Found(After(iFpsRevert, pcCrash::kScenarioMenuTestDone, [&](const Event& e) {
                   return e.au[0] == 2 && e.au[1] == rBase.au[2] && e.au[2] == rBase.au[3];
               })));
    t.TestEqual("kept confirms", CountOf(pcCrash::kScenarioConfirmKept), 0);
    t.TestTrue("the display menu options were built", Found(First(pcCrash::kScenarioMenuOptionsBuilt)));
    t.TestEqual("lua errors", CountOf(pcCrash::kScenarioLuaError), 0);
}

void CheckMenuKeep(FAutomationTestBase& t, bool bSaveFail)
{
    t.TestTrue("the menu test saved with result 1",
               Found(First(pcCrash::kScenarioMenuTestSave, [](const Event& e) { return e.au[0] == 1; })));
    const int iKept = First(pcCrash::kScenarioConfirmKept);
    if (bSaveFail)
    {
        t.TestTrue("the save after the kept confirm failed", Found(After(iKept, pcCrash::kScenarioMenuSaveFailed)));
        t.TestTrue("the later menu test save had result 2",
                   Found(First(pcCrash::kScenarioMenuTestSave, [](const Event& e) { return e.au[0] == 2; })));
        t.TestTrue("the menu test ended with result 1 and keep save 2",
                   Found(First(pcCrash::kScenarioMenuTestDone, [](const Event& e) { return e.au[0] == 1 && e.au[3] == 2; })));
        return;
    }
    t.TestTrue("the confirm was pending", Found(First(pcCrash::kScenarioConfirmPending)));
    t.TestTrue("the menu save waited for the confirm", Found(First(pcCrash::kScenarioMenuSaveDeferred)));
    t.TestTrue("the menu saved after the kept confirm", Found(After(iKept, pcCrash::kScenarioMenuSaved)));
    t.TestTrue("the menu test ended with result 1 and keep save 0",
               Found(First(pcCrash::kScenarioMenuTestDone, [](const Event& e) { return e.au[0] == 1 && e.au[3] == 0; })));
    t.TestEqual("timed out confirms", CountOf(pcCrash::kScenarioConfirmRevert, Timeout), 0);
}

void CheckMenuOrder(FAutomationTestBase& t, bool bLegacyFirst)
{
    const int iOrder = First(pcCrash::kScenarioMenuTestOrder,
                             [&](const Event& e) { return e.au[0] == (bLegacyFirst ? 1u : 0u); });
    const int iApplied = After(iOrder, pcCrash::kScenarioApplied);
    const int iNext = After(iApplied, pcCrash::kScenarioApplied);
    const int iPending = After(iApplied, pcCrash::kScenarioConfirmPending);
    t.TestTrue("one mode was applied between the order and the pending confirm",
               Found(iPending) && (!Found(iNext) || iPending < iNext));
    t.TestTrue("the confirm timed out", Found(First(pcCrash::kScenarioConfirmRevert, Timeout)));
    t.TestTrue("the menu test ended with result 2",
               Found(First(pcCrash::kScenarioMenuTestDone, [](const Event& e) { return e.au[0] == 2; })));
}

void CheckMenuRevertSave(FAutomationTestBase& t)
{
    t.TestTrue("the menu test saved with result 1",
               Found(First(pcCrash::kScenarioMenuTestSave, [](const Event& e) { return e.au[0] == 1; })));
    const int iRevert = After(First(pcCrash::kScenarioConfirmPending), pcCrash::kScenarioConfirmRevert, Timeout);
    const int iSaved = After(iRevert, pcCrash::kScenarioMenuSaved, [](const Event& e) { return e.au[0] == 0x10; });
    t.TestTrue("the confirm timed out, the fps alone was saved and the cancel restored the menu",
               Found(After(iSaved, pcCrash::kScenarioMenuCancelRestored)));
    const int iPrior = First(pcCrash::kScenarioMenuTestPrior);
    t.TestTrue("the restore kept the prior vsync and not the prior fps",
               Found(iPrior) && Found(After(iPrior, pcCrash::kScenarioMenuTestRestored, [&](const Event& e) {
                   return e.au[0] == At(iPrior).au[0] && e.au[1] != At(iPrior).au[1];
               })));
    t.TestTrue("the menu test ended with result 2 and keep save 0",
               Found(First(pcCrash::kScenarioMenuTestDone, [](const Event& e) { return e.au[0] == 2 && e.au[3] == 0; })));
}

void CheckMenuApplyFail(FAutomationTestBase& t)
{
    t.TestTrue("the menu test saved with result 1",
               Found(First(pcCrash::kScenarioMenuTestSave, [](const Event& e) { return e.au[0] == 1; })));
    const int iBase = First(pcCrash::kScenarioMenuTestBase);
    const int iPrior = After(iBase, pcCrash::kScenarioMenuTestPrior);
    bool bChain = false;
    if (Found(iPrior))
    {
        const unsigned int uAccept = At(iPrior).au[2];
        const unsigned int uWait = At(iBase).au[3];
        const int iRestored = After(After(iPrior, pcCrash::kScenarioReverted), pcCrash::kScenarioProfileRestored,
                                    [&](const Event& e) { return e.au[4] == uAccept && e.au[5] == uWait; });
        const int iSaved = After(iRestored, pcCrash::kScenarioMenuSaved, [](const Event& e) { return e.au[0] == 0x10; });
        bChain = Found(After(iSaved, pcCrash::kScenarioMenuTestDone, [&](const Event& e) {
            return e.au[0] == 0 && e.au[1] == uAccept && e.au[2] == uWait && e.au[3] == 0;
        }));
    }
    t.TestTrue("the failed apply reverted, restored the accepted fps and base waitquery, saved the fps and ended with "
               "result 0",
               bChain);
    t.TestEqual("pending confirms", CountOf(pcCrash::kScenarioConfirmPending), 0);
    t.TestEqual("applied display modes", CountOf(pcCrash::kScenarioApplied), 0);
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameCrashScenarioTest, "Game.Crash.Scenario", EAutomationTestFlags::ProductFilter)

bool FGameCrashScenarioTest::RunTest(char const* pcParameters)
{
    TestFalse("scenario events were dropped", pcCrash::ScenarioEventsDropped());
    char const* pcKind = pcCrash::GetCrashTest();
    if (strcmp(pcKind, "resetinitlost") == 0 || strcmp(pcKind, "resetrecover") == 0)
    {
        TestTrue("the device reset recovered after 4 attempts",
                 Found(First(pcCrash::kScenarioResetRecovered, [](const Event& e) { return e.au[0] == 4; })));
    }
    else if (strcmp(pcKind, "resetflap") == 0)
    {
        TestTrue("the device reset recovered after 25 attempts",
                 Found(First(pcCrash::kScenarioResetRecovered, [](const Event& e) { return e.au[0] == 25; })));
        TestTrue("the reset failure was logged at most once", CountOf(pcCrash::kScenarioResetFailed) <= 1);
    }
    else if (strcmp(pcKind, "applymode") == 0 || strcmp(pcKind, "applymodesmall") == 0)
    {
        CheckApplyMode(*this, strcmp(pcKind, "applymodesmall") == 0);
    }
    else if (strcmp(pcKind, "applyfail") == 0)
    {
        CheckApplyFail(*this);
    }
    else if (strcmp(pcKind, "applypresentfail") == 0 || strcmp(pcKind, "applytargetfail") == 0)
    {
        CheckApplyReverted(*this, strcmp(pcKind, "applytargetfail") == 0);
    }
    else if (strncmp(pcKind, "gamma", 5) == 0)
    {
        CheckGamma(*this, pcKind);
    }
    else if (strcmp(pcKind, "shellstretch") == 0)
    {
        CheckShellStretch(*this);
    }
    else if (strcmp(pcKind, "applyadapter") == 0 || strcmp(pcKind, "adaptercancel") == 0 ||
             strcmp(pcKind, "adapteraccept") == 0)
    {
        CheckAdapter(*this, pcKind);
    }
    else if (strcmp(pcKind, "menuconfirm") == 0)
    {
        CheckMenuConfirm(*this);
    }
    else if (strcmp(pcKind, "menukeep") == 0 || strcmp(pcKind, "menusavefail") == 0)
    {
        CheckMenuKeep(*this, strcmp(pcKind, "menusavefail") == 0);
    }
    else if (strcmp(pcKind, "menuorder") == 0 || strcmp(pcKind, "menuorderrev") == 0)
    {
        CheckMenuOrder(*this, strcmp(pcKind, "menuorder") == 0);
    }
    else if (strcmp(pcKind, "menurevertsave") == 0)
    {
        CheckMenuRevertSave(*this);
    }
    else if (strcmp(pcKind, "menuapplyfail") == 0)
    {
        CheckMenuApplyFail(*this);
    }
    else if (strcmp(pcKind, "stickdelete") == 0)
    {
        TestTrue("the mine stuck to a prop that is not a game object, cleared when the prop was deleted, and fell",
                 Found(First(pcCrash::kScenarioStickDelete, [](const Event& e) {
                     return e.au[0] == 1 && e.au[1] == 0 && e.au[2] == 1 && e.au[3] == 1 && e.au[4] == 1;
                 })));
    }
    else if (strcmp(pcKind, "particlesound") == 0)
    {
        TestTrue("a reset emitter holds no ambient voice",
                 Found(First(pcCrash::kScenarioParticleSound, [](const Event& e) { return e.au[0] == 0; })));
    }
    else if (strcmp(pcKind, "particleburst") == 0)
    {
        TestTrue("a burst emitter kept its playing ambient voice on the next particle",
                 Found(First(pcCrash::kScenarioParticleBurst, [](const Event& e) {
                     return e.au[0] == 1 && e.au[1] == 1 && e.au[2] == 1;
                 })));
    }
    else
    {
        AddError("no in-game check for -crashtest \"%s\"", pcKind);
    }
    return TestEqual("parameters", pcParameters, "");
}

#endif
