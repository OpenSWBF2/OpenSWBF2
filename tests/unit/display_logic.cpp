#include "third_party/doctest/doctest.h"

#include <d3d9.h>

#include "testutil.h"
#include "RedEngineFL/Graphics/PC/pcDisplayLogic.h"
#include "Battlefront2/Platform/Pc/pcColorRamp.h"

using namespace DisplayLogic;

namespace
{

DisplayModeRec M(unsigned int w, unsigned int h, unsigned int r, unsigned int a = 0)
{
    DisplayModeRec k = {w, h, r, a};
    return k;
}

}

TEST_CASE("display logic mode list keeps one adapter" * doctest::test_suite("unit"))
{
    DisplayModeRec src[] = {M(1920, 1080, 60, 0), M(1280, 720, 60, 1), M(1920, 1080, 60, 1),
                            M(1920, 1080, 144, 1), M(1920, 1080, 144, 1)};
    DisplayModeRec out[8];
    CHECK(BuildModeList(src, 5, 1, out, 8) == 3);
    CHECK(out[0].w == 1280);
    CHECK(out[1].refresh == 60);
    CHECK(out[2].refresh == 144);
    CHECK(BuildModeList(src, 5, 0, out, 8) == 1);
}

TEST_CASE("display logic mode list collapses duplicates and has no cap" * doctest::test_suite("unit"))
{
    DisplayModeRec src[300];
    for (unsigned int i = 0; i < 200; i++)
    {
        src[i] = M(640 + i, 480 + i, 60);
    }
    DisplayModeRec out[300];
    CHECK(BuildModeList(src, 200, 0, out, 300) == 200);
    CHECK(BuildModeList(src, 200, 0, 0, 0) == 200);
    DisplayModeRec dup[] = {M(800, 600, 60), M(800, 600, 60), M(800, 600, 75)};
    CHECK(BuildModeList(dup, 3, 0, out, 300) == 2);
}

TEST_CASE("display logic mode list respects the output cap" * doctest::test_suite("unit"))
{
    DisplayModeRec src[] = {M(640, 480, 60), M(800, 600, 60), M(1024, 768, 60)};
    DisplayModeRec out[4] = {M(7, 7, 7), M(7, 7, 7), M(7, 7, 7), M(7, 7, 7)};
    CHECK(BuildModeList(src, 3, 0, out, 2) == 3);
    CHECK(out[1].w == 800);
    CHECK(out[2].w == 7);
}

TEST_CASE("display logic unique resolutions" * doctest::test_suite("unit"))
{
    DisplayModeRec src[] = {M(640, 480, 60), M(800, 600, 60), M(800, 600, 75), M(1920, 1080, 60)};
    DisplayModeRec out[4];
    CHECK(UniqueResolutions(src, 4, 800, 600, out, 4) == 2);
    CHECK(out[0].w == 800);
    CHECK(out[1].w == 1920);
}

TEST_CASE("display logic desktop default exact and nearest" * doctest::test_suite("unit"))
{
    unsigned int w = 0, h = 0;
    DisplayModeRec a[] = {M(1280, 1024, 60), M(2560, 1440, 60), M(1920, 1080, 60)};
    DesktopDefault(a, 3, 2560, 1440, w, h);
    CHECK(w == 2560);
    CHECK(h == 1440);
    DisplayModeRec b[] = {M(1280, 1024, 60), M(1920, 1080, 60)};
    DesktopDefault(b, 2, 2560, 1440, w, h);
    CHECK(w == 1920);
    CHECK(h == 1080);
    DisplayModeRec c[] = {M(640, 480, 60)};
    DesktopDefault(c, 1, 3840, 2160, w, h);
    CHECK(w == 640);
    CHECK(h == 480);
}

TEST_CASE("display logic desktop default empty list" * doctest::test_suite("unit"))
{
    unsigned int w = 1, h = 1;
    DesktopDefault(0, 0, 1920, 1080, w, h);
    CHECK(w == 800);
    CHECK(h == 600);
}

TEST_CASE("display logic desktop default zero desktop" * doctest::test_suite("unit"))
{
    unsigned int w = 1, h = 1;
    DisplayModeRec a[] = {M(1920, 1080, 60)};
    DesktopDefault(a, 1, 0, 0, w, h);
    CHECK(w == 800);
    CHECK(h == 600);
}

TEST_CASE("display logic desktop default nothing fits" * doctest::test_suite("unit"))
{
    unsigned int w = 1, h = 1;
    DisplayModeRec a[] = {M(1920, 1080, 60)};
    DesktopDefault(a, 1, 1024, 768, w, h);
    CHECK(w == 800);
    CHECK(h == 600);
}

TEST_CASE("display logic refresh windowed or unrequested" * doctest::test_suite("unit"))
{
    DisplayModeRec a[] = {M(1920, 1080, 60), M(1920, 1080, 144)};
    CHECK(PickRefresh(a, 2, 1920, 1080, 144, true) == 0);
    CHECK(PickRefresh(a, 2, 1920, 1080, 0, false) == 0);
    CHECK(PickRefresh(a, 2, 1920, 1080, 144, false) == 144);
    CHECK(PickRefresh(a, 2, 1280, 720, 60, false) == 0);
}

TEST_CASE("display logic refresh fallback" * doctest::test_suite("unit"))
{
    DisplayModeRec a[] = {M(1920, 1080, 60), M(1920, 1080, 144), M(1280, 720, 240)};
    LogCapture capture;
    CHECK(PickRefresh(a, 3, 1920, 1080, 100, false) == 144);
    CHECK(capture.Contains("display: refresh 100Hz not listed"));
}

TEST_CASE("display logic adapter name beats ordinal" * doctest::test_suite("unit"))
{
    AdapterRec ads[] = {{0, "\\\\.\\DISPLAY1"}, {1, "\\\\.\\DISPLAY2"}};
    CHECK(ResolveAdapter(0, false, "\\\\.\\display2", ads, 2, false) == 1);
}

TEST_CASE("display logic adapter fallback" * doctest::test_suite("unit"))
{
    AdapterRec ads[] = {{0, "\\\\.\\DISPLAY1"}, {1, "\\\\.\\DISPLAY2"}};
    {
        LogCapture capture;
        CHECK(ResolveAdapter(1, false, "\\\\.\\DISPLAY9", ads, 2, false) == 1);
        CHECK(capture.Contains("display: adapter name"));
    }
    {
        LogCapture capture;
        CHECK(ResolveAdapter(5, false, "\\\\.\\DISPLAY9", ads, 2, false) == 0);
        CHECK(capture.Contains("not found, using adapter 0"));
    }
    {
        LogCapture capture;
        CHECK(ResolveAdapter(1, true, "\\\\.\\DISPLAY1", ads, 2, false) == 1);
        CHECK(!capture.Contains("display:"));
    }
    {
        LogCapture capture;
        CHECK(ResolveAdapter(5, true, "", ads, 2, false) == 0);
        CHECK(capture.Contains("display: adapter 5"));
    }
}

TEST_CASE("display logic adapter norender is silent" * doctest::test_suite("unit"))
{
    AdapterRec ads[] = {{0, "a"}};
    LogCapture capture;
    CHECK(ResolveAdapter(5, true, "x", ads, 1, true) == 0);
    CHECK(!capture.Contains("display:"));
}

TEST_CASE("display logic usable uncached" * doctest::test_suite("unit"))
{
    CHECK(UsableUncached(true, false, true, 0, 0));
    CHECK(UsableUncached(false, true, true, 0, 0));
    CHECK(UsableUncached(true, false, false, 4, 0));
    CHECK(!UsableUncached(true, false, false, 0, 0));
    CHECK(!UsableUncached(false, false, true, 4, 0));
}

TEST_CASE("display logic effective vsync precedence" * doctest::test_suite("unit"))
{
    CHECK(EffectiveVsync(true, 2, false) == 2);
    CHECK(EffectiveVsync(true, 0, true) == 0);
    CHECK(EffectiveVsync(false, 1, false) == 0);
    CHECK(EffectiveVsync(false, 0, true) == 1);
}

TEST_CASE("display logic vsync off and on" * doctest::test_suite("unit"))
{
    const unsigned long caps[] = {0, 0x1, 0x3, 0x80000001ul, 0x80000003ul};
    for (int w = 0; w < 2; ++w)
    {
        for (unsigned long c : caps)
        {
            bool fb = true;
            bool lacks = c != 0 && !(c & kIntervalImmediate);
            CHECK(ResolveVsyncInterval(0, c, w != 0, &fb) == (lacks ? kIntervalDefault : kIntervalImmediate));
            CHECK(fb == lacks);
            fb = true;
            CHECK(ResolveVsyncInterval(1, c, w != 0, &fb) == kIntervalDefault);
            CHECK(!fb);
        }
    }
}

TEST_CASE("display logic vsync half needs fullscreen and caps" * doctest::test_suite("unit"))
{
    const unsigned long caps[] = {0, 0x1, 0x3, 0x80000001ul, 0x80000003ul};
    for (int w = 0; w < 2; ++w)
    {
        for (unsigned long c : caps)
        {
            bool fb = false;
            bool ok = !w && (c & 2);
            CHECK(ResolveVsyncInterval(2, c, w != 0, &fb) == (ok ? kIntervalTwo : kIntervalDefault));
            CHECK(fb == !ok);
        }
    }
}

TEST_CASE("display logic vsync unknown request and null flag" * doctest::test_suite("unit"))
{
    bool fb = true;
    CHECK(ResolveVsyncInterval(7, 0x80000003ul, false, &fb) == kIntervalDefault);
    CHECK(!fb);
    CHECK(ResolveVsyncInterval(7, 0, true, nullptr) == kIntervalDefault);
    CHECK(ResolveVsyncInterval(0, 0x1, true, nullptr) == kIntervalDefault);
    CHECK(ResolveVsyncInterval(2, 0, true, nullptr) == kIntervalDefault);
    CHECK(ResolveVsyncInterval(2, 3, false, nullptr) == kIntervalTwo);
}

TEST_CASE("reset step maps every code" * doctest::test_suite("unit"))
{
    CHECK(ResetStep(S_OK, 0, 0xFFFFFFFFu, true, kRuntimeBounds) == kResetDone);
    CHECK(ResetStep(S_OK, 99, 0xFFFFFFFFu, false, kStartupBounds) == kResetDone);
    CHECK(ResetStep(0x0876017C, 1, 0, false, kRuntimeBounds) == kResetDone);
    CHECK(ResetStep(S_FALSE, 1, 0, false, kStartupBounds) == kResetDone);
    CHECK(ResetStep(D3DERR_DEVICENOTRESET, 1, 0, true, kRuntimeBounds) == kResetWait);
    CHECK(ResetStep(D3DERR_DEVICENOTRESET, 3, 100, false, kStartupBounds) == kResetWait);
    CHECK(ResetStep(D3DERR_DRIVERINTERNALERROR, 1, 0, true, kRuntimeBounds) == kResetFatal);
    CHECK(ResetStep(E_FAIL, 1, 0, true, kRuntimeBounds) == kResetFatal);
    CHECK(ResetStep(D3DERR_DEVICEREMOVED, 1, 0, true, kStartupBounds) == kResetFatal);
    CHECK(ResetStep(D3DERR_INVALIDCALL, 1, 0, true, kRuntimeBounds) == kResetRevert);
}

TEST_CASE("reset step lost device waits without limit" * doctest::test_suite("unit"))
{
    CHECK(kRuntimeBounds.uiMaxMs == 0);
    CHECK(ResetStep(D3DERR_DEVICELOST, 1, 0xFFFFFFFFu, false, kRuntimeBounds) == kResetWait);
    CHECK(ResetStep(D3DERR_DEVICELOST, 19, 0xFFFFFFFFu, true, kRuntimeBounds) == kResetWait);
    CHECK(CoopStep(D3DERR_DEVICELOST, 0, kRuntimeBounds) == kResetWait);
    CHECK(CoopStep(D3DERR_DEVICELOST, 30000, kRuntimeBounds) == kResetWait);
    CHECK(CoopStep(D3DERR_DEVICELOST, 0xFFFFFFFFu, kRuntimeBounds) == kResetWait);
}

TEST_CASE("coop step maps every code" * doctest::test_suite("unit"))
{
    CHECK(CoopStep(S_OK, 0, kRuntimeBounds) == kResetNow);
    CHECK(CoopStep(S_OK, 0xFFFFFFFFu, kStartupBounds) == kResetNow);
    CHECK(CoopStep(0x08760868, 0, kRuntimeBounds) == kResetNow);
    CHECK(CoopStep(D3DERR_DEVICENOTRESET, 0, kRuntimeBounds) == kResetNow);
    CHECK(CoopStep(D3DERR_DEVICENOTRESET, 0xFFFFFFFFu, kStartupBounds) == kResetNow);
    CHECK(CoopStep(D3DERR_DRIVERINTERNALERROR, 0, kRuntimeBounds) == kResetFatal);
    CHECK(CoopStep(D3DERR_DEVICEREMOVED, 0, kStartupBounds) == kResetFatal);
    CHECK(CoopStep(E_FAIL, 0, kRuntimeBounds) == kResetFatal);
}

TEST_CASE("coop step startup lost device times out without revert" * doctest::test_suite("unit"))
{
    CHECK(CoopStep(D3DERR_DEVICELOST, 29999, kStartupBounds) == kResetWait);
    CHECK(CoopStep(D3DERR_DEVICELOST, 30000, kStartupBounds) == kResetFatal);
    CHECK(CoopStep(D3DERR_DEVICELOST, 0xFFFFFFFFu, kStartupBounds) == kResetFatal);
}

TEST_CASE("reset step startup timeout is fatal" * doctest::test_suite("unit"))
{
    CHECK(kStartupBounds.uiMaxMs == 30000);
    CHECK(ResetStep(D3DERR_DEVICELOST, 5, 29999, false, kStartupBounds) == kResetWait);
    CHECK(ResetStep(D3DERR_DEVICELOST, 5, 30000, false, kStartupBounds) == kResetFatal);
    CHECK(ResetStep(D3DERR_DEVICELOST, 5, 30000, true, kStartupBounds) == kResetRevert);
    CHECK(ResetStep(D3DERR_DEVICELOST, 500, 29999, false, kStartupBounds) == kResetWait);
}

TEST_CASE("reset step runtime failure bound" * doctest::test_suite("unit"))
{
    CHECK(kRuntimeBounds.uiMaxFailures == 20);
    CHECK(ResetStep(D3DERR_DEVICELOST, 19, 0xFFFFFFFFu, false, kRuntimeBounds) == kResetWait);
    CHECK(ResetStep(D3DERR_DEVICELOST, 20, 0, false, kRuntimeBounds) == kResetFatal);
    CHECK(ResetStep(D3DERR_DEVICELOST, 20, 0, true, kRuntimeBounds) == kResetRevert);
    CHECK(ResetStep(D3DERR_DEVICENOTRESET, 19, 0, false, kRuntimeBounds) == kResetWait);
    CHECK(ResetStep(D3DERR_DEVICENOTRESET, 20, 0, false, kRuntimeBounds) == kResetFatal);
    CHECK(ResetStep(D3DERR_DEVICENOTRESET, 20, 0, true, kRuntimeBounds) == kResetRevert);
    CHECK(ResetStep(D3DERR_DEVICENOTRESET, 1, 30000, false, kStartupBounds) == kResetFatal);
}

TEST_CASE("reset step reverts once then fatal" * doctest::test_suite("unit"))
{
    const long codes[] = {D3DERR_INVALIDCALL, D3DERR_NOTAVAILABLE, E_OUTOFMEMORY,
                          D3DERR_OUTOFVIDEOMEMORY};
    for (long hr : codes)
    {
        CHECK(ResetStep(hr, 1, 0, true, kRuntimeBounds) == kResetRevert);
        CHECK(ResetStep(hr, 1, 0, false, kRuntimeBounds) == kResetFatal);
    }
    const long unknown = (long)0x88760999;
    CHECK(ResetStep(unknown, 1, 0, true, kRuntimeBounds) == kResetFatal);
    CHECK(ResetStep(unknown, 1, 0, false, kRuntimeBounds) == kResetFatal);
}

TEST_CASE("display logic mode resolution" * doctest::test_suite("unit"))
{
    CHECK(ResolveMode(true, false, false) == kModeBorderless);
    CHECK(ResolveMode(true, true, true) == kModeBorderless);
    CHECK(ResolveMode(false, true, false) == kModeWindowed);
    CHECK(ResolveMode(false, false, true) == kModeWindowed);
    CHECK(ResolveMode(false, false, false) == kModeFullscreen);
}

TEST_CASE("display logic mode predicates" * doctest::test_suite("unit"))
{
    CHECK_FALSE(D3DWindowed(kModeFullscreen));
    CHECK(D3DWindowed(kModeWindowed));
    CHECK(D3DWindowed(kModeBorderless));
    CHECK(MinimizeOnDeactivate(kModeFullscreen));
    CHECK_FALSE(MinimizeOnDeactivate(kModeWindowed));
    CHECK_FALSE(MinimizeOnDeactivate(kModeBorderless));
    CHECK(Topmost(kModeFullscreen));
    CHECK_FALSE(Topmost(kModeWindowed));
    CHECK_FALSE(Topmost(kModeBorderless));
}

TEST_CASE("display logic window style" * doctest::test_suite("unit"))
{
    WinStyle f = WindowStyleFor(kModeFullscreen, true);
    CHECK(f.style == (WS_POPUP | WS_VISIBLE));
    CHECK(f.exStyle == WS_EX_TOPMOST);
    CHECK((WindowStyleFor(kModeFullscreen, false).style & WS_VISIBLE) == 0);

    WinStyle w = WindowStyleFor(kModeWindowed, false);
    CHECK(w.style == (WS_CAPTION | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX));
    CHECK((w.style & WS_MAXIMIZEBOX) == 0);
    CHECK((w.style & WS_VISIBLE) == 0);
    CHECK(w.exStyle == 0);
    CHECK((WindowStyleFor(kModeWindowed, true).style & WS_VISIBLE) != 0);

    WinStyle b = WindowStyleFor(kModeBorderless, true);
    CHECK(b.style == (WS_POPUP | WS_VISIBLE));
    CHECK((b.style & (WS_CAPTION | WS_THICKFRAME)) == 0);
    CHECK((b.exStyle & WS_EX_TOPMOST) == 0);
}

TEST_CASE("display logic window placement" * doctest::test_suite("unit"))
{
    const Frame none = {0, 0, 0, 0};
    const Rect monitors[] = {{0, 0, 1920, 1080}, {-1920, 0, 0, 1080}, {1920, -200, 3840, 880}};
    for (const Rect& mon : monitors)
    {
        const Rect shrunk = {mon.l + 10, mon.t + 20, mon.r - 30, mon.b - 40};
        Rect r = PlaceWindow(kModeBorderless, mon, shrunk, 800, 600, none);
        CHECK(r.l == mon.l);
        CHECK(r.t == mon.t);
        CHECK(r.r == mon.r);
        CHECK(r.b == mon.b);
    }

    const Frame frame = {-8, -31, 8, 8};
    const Rect work = {0, 0, 1920, 1040};
    Rect w = PlaceWindow(kModeWindowed, monitors[0], work, 800, 600, frame);
    CHECK(w.r - w.l == 816);
    CHECK(w.b - w.t == 639);
    CHECK(w.l == (0 - 816 + 1920) / 2);
    CHECK(w.t == (0 - 639 + 1040) / 2);

    const Rect tiny = {100, 50, 500, 400};
    Rect c = PlaceWindow(kModeWindowed, tiny, tiny, 800, 600, frame);
    CHECK(c.l == tiny.l);
    CHECK(c.t == tiny.t);

    Rect h = PlaceWindow(kModeWindowed, monitors[0], work, 99999, 99999, frame);
    CHECK(h.r - h.l == 16384 + 16);
    CHECK(h.b - h.t == 16384 + 39);

    Rect fs = PlaceWindow(kModeFullscreen, monitors[2], monitors[2], 800, 600, frame);
    CHECK(fs.r - fs.l == 816);
    CHECK(fs.b - fs.t == 639);
    CHECK(fs.l == (1920 - 816 + 3840) / 2);
    CHECK(fs.t == (-200 - 639 + 880) / 2);
}

TEST_CASE("display logic dpi changed rect" * doctest::test_suite("unit"))
{
    const Rect suggested = {100, 50, 900, 700};
    const Frame frame = {-12, -45, 12, 12};
    Rect r = DpiChangedRect(suggested, 800, 600, frame);
    CHECK(r.l == 100);
    CHECK(r.t == 50);
    CHECK(r.r == 100 + 824);
    CHECK(r.b == 50 + 657);
}

namespace
{

char sOrder[8];
int sOrderLen = 0;
int sPmv2Result = kDpiFail;
int sPmResult = kDpiFail;
int sSystemResult = kDpiFail;

int StepPmv2(void*)
{
    sOrder[sOrderLen++] = 'a';
    return sPmv2Result;
}

int StepPm(void*)
{
    sOrder[sOrderLen++] = 'b';
    return sPmResult;
}

int StepSystem(void*)
{
    sOrder[sOrderLen++] = 'c';
    return sSystemResult;
}

DpiLevel RunCascade(int iPmv2, int iPm, int iSystem, bool bNoDpi, bool bNull)
{
    sPmv2Result = iPmv2;
    sPmResult = iPm;
    sSystemResult = iSystem;
    sOrderLen = 0;
    memset(sOrder, 0, sizeof(sOrder));
    DpiSteps steps = {bNull ? 0 : StepPmv2, bNull ? 0 : StepPm, bNull ? 0 : StepSystem, 0};
    return DpiCascade(steps, bNoDpi);
}

}

TEST_CASE("display logic dpi cascade" * doctest::test_suite("unit"))
{
    CHECK(RunCascade(kDpiOk, kDpiOk, kDpiOk, false, false) == kDpiLevelPmV2);
    CHECK(strcmp(sOrder, "a") == 0);
    CHECK(RunCascade(kDpiFail, kDpiOk, kDpiOk, false, false) == kDpiLevelPm);
    CHECK(strcmp(sOrder, "ab") == 0);
    CHECK(RunCascade(kDpiFail, kDpiFail, kDpiOk, false, false) == kDpiLevelSystem);
    CHECK(strcmp(sOrder, "abc") == 0);
    CHECK(RunCascade(kDpiFail, kDpiFail, kDpiFail, false, false) == kDpiLevelNone);
    CHECK(RunCascade(kDpiOk, kDpiOk, kDpiOk, false, true) == kDpiLevelNone);
    CHECK(RunCascade(kDpiAlready, kDpiOk, kDpiOk, false, false) == kDpiLevelPmV2);
    CHECK(strcmp(sOrder, "a") == 0);
    CHECK(RunCascade(kDpiFail, kDpiAlready, kDpiOk, false, false) == kDpiLevelPm);
    CHECK(strcmp(sOrder, "ab") == 0);
    CHECK(RunCascade(kDpiOk, kDpiOk, kDpiOk, true, false) == kDpiLevelOff);
    CHECK(sOrderLen == 0);
    CHECK(strcmp(DpiLevelName(kDpiLevelNone), "none") == 0);
    CHECK(strcmp(DpiLevelName(kDpiLevelPmV2), "pmv2") == 0);
    CHECK(strcmp(DpiLevelName(kDpiLevelPm), "pm") == 0);
    CHECK(strcmp(DpiLevelName(kDpiLevelSystem), "system") == 0);
    CHECK(strcmp(DpiLevelName(kDpiLevelOff), "off") == 0);
}

TEST_CASE("display logic dpi preset" * doctest::test_suite("unit"))
{
    CHECK(PresetDpiLevel(2, true, kDpiLevelPm) == kDpiLevelPmV2);
    CHECK(PresetDpiLevel(2, false, kDpiLevelPmV2) == kDpiLevelPm);
    CHECK(PresetDpiLevel(1, false, kDpiLevelPmV2) == kDpiLevelSystem);
    CHECK(PresetDpiLevel(1, true, kDpiLevelPmV2) == kDpiLevelSystem);
    CHECK(PresetDpiLevel(0, false, kDpiLevelPmV2) == kDpiLevelNone);
    CHECK(PresetDpiLevel(-1, false, kDpiLevelPmV2) == kDpiLevelPmV2);
    CHECK(PresetDpiLevel(-1, false, kDpiLevelPm) == kDpiLevelPm);
}

TEST_CASE("display logic coordinate mapping" * doctest::test_suite("unit"))
{
    int ox = -1;
    int oy = -1;
    MapBackBufferToClient(5, 7, 800, 600, 800, 600, ox, oy);
    CHECK(ox == 5);
    CHECK(oy == 7);
    MapBackBufferToClient(800, 600, 800, 600, 800, 600, ox, oy);
    CHECK(ox == 799);
    CHECK(oy == 599);

    MapBackBufferToClient(0, 0, 800, 600, 1920, 1080, ox, oy);
    CHECK(ox == 0);
    CHECK(oy == 0);
    MapBackBufferToClient(400, 300, 800, 600, 1920, 1080, ox, oy);
    CHECK(ox == 960);
    CHECK(oy == 540);
    MapBackBufferToClient(800, 600, 800, 600, 1920, 1080, ox, oy);
    CHECK(ox == 1919);
    CHECK(oy == 1079);
    MapBackBufferToClient(-5, -9, 800, 600, 1920, 1080, ox, oy);
    CHECK(ox == 0);
    CHECK(oy == 0);

    MapBackBufferToClient(401, 301, 801, 601, 1280, 720, ox, oy);
    CHECK(ox == (401 * 1280 + 400) / 801);
    CHECK(oy == (301 * 720 + 300) / 601);

    MapBackBufferToClient(33, 44, 0, 600, 1920, 0, ox, oy);
    CHECK(ox == 33);
    CHECK(oy == 44);
    MapBackBufferToClient(-3, 44, 800, 600, 0, 0, ox, oy);
    CHECK(ox == 0);
    CHECK(oy == 44);
    MapBackBufferToClient(2000000000, 2000000000, 800, 600, 1920, 1080, ox, oy);
    CHECK(ox == 1919);
    CHECK(oy == 1079);

    for (int x = 0; x < 800; x += 7)
    {
        int cx = 0;
        int cy = 0;
        int bx = 0;
        int by = 0;
        MapBackBufferToClient(x, x % 600, 800, 600, 1920, 1080, cx, cy);
        MapClientToBackBuffer(cx, cy, 1920, 1080, 800, 600, bx, by);
        CHECK(abs(bx - x) <= 1);
        CHECK(abs(by - (x % 600)) <= 1);
    }
}

TEST_CASE("display logic shell size policy" * doctest::test_suite("unit"))
{
    ShellSize s = ShellSizePolicy(kModeFullscreen, false, 0, 0, 1280, 720, 1920, 1080);
    CHECK(s.uiBackW == 1280);
    CHECK(s.uiBackH == 720);
    CHECK(s.uiClientW == 1280);
    CHECK(s.uiClientH == 720);
    CHECK_FALSE(s.bClearMsaa);

    s = ShellSizePolicy(kModeFullscreen, true, 0, 0, 1280, 720, 1920, 1080);
    CHECK(s.uiBackW == 0);
    CHECK(s.uiBackH == 0);
    CHECK(s.uiClientW == 0);
    CHECK(s.uiClientH == 0);
    CHECK(s.bClearMsaa);

    s = ShellSizePolicy(kModeFullscreen, true, 1024, 768, 1280, 720, 1920, 1080);
    CHECK(s.uiBackW == 1024);
    CHECK(s.uiBackH == 768);
    CHECK(s.uiClientW == 1024);
    CHECK(s.uiClientH == 768);

    s = ShellSizePolicy(kModeWindowed, true, 0, 0, 1280, 720, 1920, 1080);
    CHECK(s.uiBackW == 0);
    CHECK(s.uiBackH == 0);
    CHECK(s.uiClientW == 1280);
    CHECK(s.uiClientH == 720);
    CHECK(s.bClearMsaa);

    s = ShellSizePolicy(kModeWindowed, true, 1024, 768, 1280, 720, 1920, 1080);
    CHECK(s.uiBackW == 1024);
    CHECK(s.uiBackH == 768);
    CHECK(s.uiClientW == 1024);
    CHECK(s.uiClientH == 768);

    s = ShellSizePolicy(kModeWindowed, true, 0, 0, 0, 0, 1920, 1080);
    CHECK(s.uiBackW == 0);
    CHECK(s.uiClientW == 0);
    CHECK(s.uiClientH == 0);

    s = ShellSizePolicy(kModeWindowed, false, 0, 0, 1280, 720, 1920, 1080);
    CHECK(s.uiBackW == 1280);
    CHECK(s.uiBackH == 720);
    CHECK(s.uiClientW == 1280);
    CHECK(s.uiClientH == 720);
    CHECK_FALSE(s.bClearMsaa);

    s = ShellSizePolicy(kModeBorderless, false, 0, 0, 1280, 720, 2560, 1440);
    CHECK(s.uiBackW == 2560);
    CHECK(s.uiBackH == 1440);
    CHECK(s.uiClientW == 2560);
    CHECK(s.uiClientH == 1440);
    CHECK(s.bWarnIgnoredSize);

    s = ShellSizePolicy(kModeBorderless, false, 0, 0, 2560, 1440, 2560, 1440);
    CHECK_FALSE(s.bWarnIgnoredSize);
    s = ShellSizePolicy(kModeBorderless, false, 0, 0, 0, 0, 2560, 1440);
    CHECK_FALSE(s.bWarnIgnoredSize);

    CHECK_FALSE(s.bWarnNoMonitor);

    s = ShellSizePolicy(kModeBorderless, false, 0, 0, 1280, 720, 0, 0);
    CHECK(s.uiBackW == 1280);
    CHECK(s.uiBackH == 720);
    CHECK(s.uiClientW == 1280);
    CHECK(s.uiClientH == 720);
    CHECK_FALSE(s.bWarnIgnoredSize);
    CHECK(s.bWarnNoMonitor);
    CHECK_FALSE(s.bClearMsaa);

    s = ShellSizePolicy(kModeBorderless, false, 1024, 768, 1280, 720, 0, 0);
    CHECK(s.uiBackW == 1024);
    CHECK(s.uiClientH == 768);
    CHECK(s.bWarnNoMonitor);

    s = ShellSizePolicy(kModeBorderless, false, 0, 0, 0, 0, 2560, 0);
    CHECK(s.uiBackW == 0);
    CHECK(s.uiClientW == 0);
    CHECK(s.bWarnNoMonitor);

    s = ShellSizePolicy(kModeBorderless, true, 0, 0, 1280, 720, 0, 0);
    CHECK(s.uiBackW == 0);
    CHECK_FALSE(s.bWarnNoMonitor);

    s = ShellSizePolicy(kModeBorderless, true, 0, 0, 1280, 720, 2560, 1440);
    CHECK(s.uiBackW == 0);
    CHECK(s.uiBackH == 0);
    CHECK(s.uiClientW == 2560);
    CHECK(s.uiClientH == 1440);
    CHECK_FALSE(s.bWarnIgnoredSize);

    s = ShellSizePolicy(kModeBorderless, true, 1024, 768, 1280, 720, 2560, 1440);
    CHECK(s.uiBackW == 1024);
    CHECK(s.uiBackH == 768);
    CHECK(s.uiClientW == 2560);
    CHECK(s.uiClientH == 1440);
}

TEST_CASE("display logic apply same params" * doctest::test_suite("unit"))
{
    DisplayRec a = {800, 600, true, 0, 0, 1, 60, kModeWindowed, 800, 600};
    DisplayRec b = a;
    CHECK(SameDisplayParams(a, b));
    b = a; b.uiW = 801; CHECK_FALSE(SameDisplayParams(a, b));
    b = a; b.uiH = 601; CHECK_FALSE(SameDisplayParams(a, b));
    b = a; b.bWindowed = false; CHECK_FALSE(SameDisplayParams(a, b));
    b = a; b.uiMsType = 2; CHECK_FALSE(SameDisplayParams(a, b));
    b = a; b.uiMsQuality = 1; CHECK_FALSE(SameDisplayParams(a, b));
    b = a; b.uiInterval = 2; CHECK_FALSE(SameDisplayParams(a, b));
    b = a; b.uiRefresh = 75; CHECK_FALSE(SameDisplayParams(a, b));
    b = a; b.eMode = kModeBorderless; CHECK_FALSE(SameDisplayParams(a, b));
    b = a; b.uiClientW = 1; CHECK_FALSE(SameDisplayParams(a, b));
    b = a; b.uiClientH = 1; CHECK_FALSE(SameDisplayParams(a, b));
}

TEST_CASE("display logic apply buffer sizes" * doctest::test_suite("unit"))
{
    BufferSet s = BufferSizes(800, 600, 4096, 4096, false);
    CHECK(s.akBuf[1].uiW == 512);
    CHECK(s.akBuf[1].uiH == 512);
    CHECK(s.akBuf[0].uiW == 512);
    CHECK(s.akBuf[0].uiH == 512);
    CHECK(s.akBuf[2].uiW == 256);
    CHECK(s.akBuf[2].uiH == 256);

    s = BufferSizes(1920, 1080, 4096, 4096, true);
    CHECK(s.akBuf[1].uiW == 1920);
    CHECK(s.akBuf[1].uiH == 1080);
    CHECK(s.akBuf[0].uiW == 512);
    CHECK(s.akBuf[0].uiH == 512);
    CHECK(s.akBuf[2].uiW == 256);
    CHECK(s.akBuf[2].uiH == 256);

    s = BufferSizes(200, 150, 4096, 4096, false);
    CHECK(s.akBuf[1].uiW == 128);
    CHECK(s.akBuf[1].uiH == 128);
    CHECK(s.akBuf[0].uiW == 128);
    CHECK(s.akBuf[0].uiH == 128);
    CHECK(s.akBuf[2].uiW == 128);
    CHECK(s.akBuf[2].uiH == 128);

    s = BufferSizes(1920, 1080, 1024, 512, false);
    CHECK(s.akBuf[1].uiW == 1024);
    CHECK(s.akBuf[1].uiH == 512);

    s = BufferSizes(4000, 3000, 2048, 2048, true);
    CHECK(s.akBuf[1].uiW == 2048);
    CHECK(s.akBuf[1].uiH == 2048);
    s = BufferSizes(1920, 3000, 2048, 2048, true);
    CHECK(s.akBuf[1].uiW == 1920);
    CHECK(s.akBuf[1].uiH == 2048);
    s = BufferSizes(16384, 16384, 8192, 4096, true);
    CHECK(s.akBuf[1].uiW == 8192);
    CHECK(s.akBuf[1].uiH == 4096);
    CHECK(s.akBuf[0].uiW == 512);
    CHECK(s.akBuf[2].uiH == 256);
}

TEST_CASE("display logic live size fits caps" * doctest::test_suite("unit"))
{
    CHECK(SizeFitsCaps(1920, 1080, 2048, 2048));
    CHECK(SizeFitsCaps(2048, 2048, 2048, 2048));
    CHECK_FALSE(SizeFitsCaps(2049, 1080, 2048, 2048));
    CHECK_FALSE(SizeFitsCaps(1920, 2049, 2048, 2048));
    CHECK(SizeFitsCaps(16384, 16384, 0, 0));
    CHECK_FALSE(SizeFitsCaps(8192, 1080, 4096, 0));
}

TEST_CASE("display logic apply buffer sizes zero caps" * doctest::test_suite("unit"))
{
    BufferSet s = BufferSizes(800, 600, 0, 0, false);
    CHECK(s.akBuf[1].uiW == 512);
    CHECK(s.akBuf[1].uiH == 512);
    CHECK(s.akBuf[2].uiW == 256);

    s = BufferSizes(0, 0, 0, 0, false);
    for (int i = 0; i < 3; i++)
    {
        CHECK(s.akBuf[i].uiW == 1);
        CHECK(s.akBuf[i].uiH == 1);
    }

    s = BufferSizes(0, 0, 0, 0, true);
    CHECK(s.akBuf[1].uiW == 1);
    CHECK(s.akBuf[1].uiH == 1);
}

TEST_CASE("display logic shadow quad shares sky" * doctest::test_suite("unit"))
{
    CHECK(ShadowQuadSharesSky(1920, 1080, 0, false, 1920, 1080));
    CHECK_FALSE(ShadowQuadSharesSky(1920, 1080, 4, false, 1920, 1080));
    CHECK_FALSE(ShadowQuadSharesSky(1920, 1080, 0, true, 1920, 1080));
    CHECK_FALSE(ShadowQuadSharesSky(1920, 1080, 0, false, 1024, 1024));
    CHECK_FALSE(ShadowQuadSharesSky(1280, 720, 0, false, 1920, 720));
    CHECK_FALSE(ShadowQuadSharesSky(1280, 720, 0, false, 1280, 1080));

    BufferSet s = BufferSizes(1280, 720, 4096, 4096, false);
    CHECK_FALSE(ShadowQuadSharesSky(1280, 720, 0, false, s.akBuf[1].uiW, s.akBuf[1].uiH));
    s = BufferSizes(1280, 720, 4096, 4096, true);
    CHECK(ShadowQuadSharesSky(1280, 720, 0, false, s.akBuf[1].uiW, s.akBuf[1].uiH));
}

TEST_CASE("display logic revert restores profile" * doctest::test_suite("unit"))
{
    CHECK(ShouldRestoreProfile(true, 2, 2, false));
    CHECK_FALSE(ShouldRestoreProfile(false, 2, 2, false));
    CHECK_FALSE(ShouldRestoreProfile(true, 1, 2, false));
    CHECK_FALSE(ShouldRestoreProfile(true, 2, 2, true));
    CHECK_FALSE(ShouldRestoreProfile(true, 0, 3, true));
}

TEST_CASE("display logic apply phase" * doctest::test_suite("unit"))
{
    ApplyStep s = ApplyPhaseStep(kApplyResetting, kEvResetOk);
    CHECK(s.ePhase == kApplyAwaitPresent);
    CHECK(s.eAction == kActNone);

    s = ApplyPhaseStep(kApplyAwaitPresent, kEvResetOk);
    CHECK(s.ePhase == kApplyAwaitPresent);
    CHECK(s.eAction == kActNone);

    s = ApplyPhaseStep(kApplyAwaitPresent, kEvPresentOk);
    CHECK(s.ePhase == kApplyNone);
    CHECK(s.eAction == kActPromote);

    s = ApplyPhaseStep(kApplyAwaitPresent, kEvPresentFail);
    CHECK(s.ePhase == kApplyNone);
    CHECK(s.eAction == kActRevert);

    s = ApplyPhaseStep(kApplyAwaitPresent, kEvPresentLost);
    CHECK(s.ePhase == kApplyAwaitPresent);
    CHECK(s.eAction == kActNone);

    for (int e = kEvResetOk; e <= kEvPresentFail; e++)
    {
        s = ApplyPhaseStep(kApplyNone, (ApplyEvent)e);
        CHECK(s.ePhase == kApplyNone);
        CHECK(s.eAction == kActNone);
    }

    s = ApplyPhaseStep(kApplyResetting, kEvPresentOk);
    CHECK(s.ePhase == kApplyResetting);
    CHECK(s.eAction == kActNone);
}

TEST_CASE("display logic apply present event" * doctest::test_suite("unit"))
{
    CHECK(PresentEvent(S_OK) == kEvPresentOk);
    CHECK(PresentEvent(D3DERR_DEVICELOST) == kEvPresentLost);
    CHECK(PresentEvent(D3DERR_INVALIDCALL) == kEvPresentFail);
    CHECK(PresentEvent(D3DERR_OUTOFVIDEOMEMORY) == kEvPresentFail);
    CHECK(PresentEvent(E_FAIL) == kEvPresentFail);
}

TEST_CASE("display logic adapter decision" * doctest::test_suite("unit"))
{
    AdapterChoice c = AdapterDecision(1, 0, 1);
    CHECK_FALSE(c.bValid);
    CHECK_FALSE(c.bRestart);

    c = AdapterDecision(0, 0, 1);
    CHECK(c.bValid);
    CHECK_FALSE(c.bRestart);

    c = AdapterDecision(1, 0, 2);
    CHECK(c.bValid);
    CHECK(c.bRestart);

    c = AdapterDecision(0, 1, 2);
    CHECK(c.bValid);
    CHECK(c.bRestart);

    c = AdapterDecision(0, 0, 0);
    CHECK_FALSE(c.bValid);
    CHECK_FALSE(c.bRestart);
}

TEST_CASE("display logic apply size bounds" * doctest::test_suite("unit"))
{
    CHECK(SizeInBounds(320, 240));
    CHECK_FALSE(SizeInBounds(319, 240));
    CHECK_FALSE(SizeInBounds(320, 239));
    CHECK(SizeInBounds(16384, 16384));
    CHECK_FALSE(SizeInBounds(16385, 600));
    CHECK_FALSE(SizeInBounds(0, 0));
}

TEST_CASE("display logic gamma lut" * doctest::test_suite("unit"))
{
    unsigned short aw[256];
    unsigned char ab[256];
    for (int i = 0; i < 256; ++i)
    {
        aw[i] = (unsigned short)(i * 0x101);
    }
    RampToLut(aw, ab);
    for (int i = 0; i < 256; ++i)
    {
        CHECK(ab[i] == i);
    }

    for (int i = 0; i < 256; ++i)
    {
        aw[i] = 0xFFFF;
    }
    RampToLut(aw, ab);
    for (int i = 0; i < 256; ++i)
    {
        CHECK(ab[i] == 0xFF);
    }

    for (int i = 0; i < 256; ++i)
    {
        aw[i] = 0x00FF;
    }
    RampToLut(aw, ab);
    for (int i = 0; i < 256; ++i)
    {
        CHECK(ab[i] == 0);
    }

    const float afLevel[3] = {0.8f, 0.8f, 0.5f};
    ColorRamp::Build(afLevel, aw);
    RampToLut(aw, ab);
    for (int i = 0; i < 256; ++i)
    {
        CHECK(ab[i] == (aw[i] >> 8));
    }
}

TEST_CASE("display logic gamma identity" * doctest::test_suite("unit"))
{
    unsigned short aw[256];
    unsigned char ab[256];
    for (int i = 0; i < 256; ++i)
    {
        aw[i] = (unsigned short)(i * 0x101);
    }
    RampToLut(aw, ab);
    CHECK(LutIsIdentity(ab));

    ab[255] = 254;
    CHECK_FALSE(LutIsIdentity(ab));
    ab[255] = 255;
    ab[0] = 1;
    CHECK_FALSE(LutIsIdentity(ab));

    const float afDefault[3] = {0.5f, 0.5f, 0.5f};
    ColorRamp::Build(afDefault, aw);
    RampToLut(aw, ab);
    CHECK(LutIsIdentity(ab));

    const float afBright[3] = {0.8f, 0.8f, 0.5f};
    ColorRamp::Build(afBright, aw);
    RampToLut(aw, ab);
    CHECK_FALSE(LutIsIdentity(ab));

    const float afDark[3] = {0.2f, 0.2f, 0.5f};
    ColorRamp::Build(afDark, aw);
    RampToLut(aw, ab);
    CHECK_FALSE(LutIsIdentity(ab));
}

TEST_CASE("display logic gamma path" * doctest::test_suite("unit"))
{
    CHECK(GammaPathFor(false, false, true) == kGammaReasonNone);
    CHECK(GammaPathFor(false, false, false) == kGammaReasonNoProg2);
    CHECK(GammaPathFor(true, false, true) == kGammaReasonNoRender);
    CHECK(GammaPathFor(true, true, true) == kGammaReasonNone);
    CHECK(GammaPathFor(true, false, false) == kGammaReasonNoRender);
    CHECK(GammaPathFor(true, true, false) == kGammaReasonNoProg2);
}

TEST_CASE("display logic gamma pass runs" * doctest::test_suite("unit"))
{
    CHECK(GammaPassRuns(kGammaPass, false, false, true));
    CHECK_FALSE(GammaPassRuns(kGammaUndecided, false, false, true));
    CHECK_FALSE(GammaPassRuns(kGammaRamp, false, false, true));
    CHECK_FALSE(GammaPassRuns(kGammaPass, true, false, true));
    CHECK_FALSE(GammaPassRuns(kGammaPass, false, true, true));
    CHECK_FALSE(GammaPassRuns(kGammaPass, false, false, false));
}
